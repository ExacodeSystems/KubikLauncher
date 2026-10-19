// SPDX-License-Identifier: GPL-3.0-only
/*
 *  KubikLauncher - Minecraft Launcher
 *  Copyright (C) 2026 PT Exacode Systems Technologies
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "KubikSyncTask.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>

#include <filesystem>

#include "Application.h"
#include "FileSystem.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"
#include "modplatform/packwiz/Packwiz.h"
#include "net/ChecksumValidator.h"
#include "net/Request.h"

namespace {

const QString DISABLED = ".disabled";

QString sidecarPath(MinecraftInstance* instance)
{
    return FS::PathCombine(instance->instanceRoot(), ".kubik", "managed.json");
}

QString sha512Of(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    QCryptographicHash hash(QCryptographicHash::Sha512);
    hash.addData(&file);
    return QString::fromLatin1(hash.result().toHex());
}

}  // namespace

KubikSyncTask::KubikSyncTask(MinecraftInstance* instance, Kubik::Pack pack) : m_instance(instance), m_pack(std::move(pack)) {}

QString KubikSyncTask::cachePath(const QString& sha512)
{
    // Fanned out by the first two hex digits, so no one folder holds every mod ever downloaded.
    return FS::PathCombine(FS::PathCombine(APPLICATION->dataRoot(), "kubik", "cache"), sha512.left(2), sha512);
}

QList<Kubik::LocalFile> KubikSyncTask::scanModsFolder() const
{
    const QDir mods(m_instance->modsRoot());

    // The player's mods installed through Prism carry their Modrinth project in mods/.index.
    QHash<QString, QString> projectByFile;
    const QDir index(mods.filePath(".index"));
    for (const auto& entry : index.entryList({ "*.pw.toml" }, QDir::Files)) {
        const QString slug = entry.chopped(QStringLiteral(".pw.toml").size());
        const auto meta = Packwiz::V1::getIndexForMod(index, slug);
        if (meta.provider == ModPlatform::ResourceProvider::MODRINTH && !meta.filename.isEmpty()) {
            projectByFile.insert(meta.filename, meta.projectId.toString());
        }
    }

    // One entry per enabled-form name; when both forms exist the enabled one is what the game loads.
    QHash<QString, Kubik::LocalFile> byName;
    for (const auto& entry : mods.entryList({ "*.jar", "*.jar" + DISABLED }, QDir::Files)) {
        const bool enabled = !entry.endsWith(DISABLED);
        const QString name = enabled ? entry : entry.chopped(DISABLED.size());
        if (byName.contains(name) && !enabled) {
            continue;
        }
        byName.insert(name, { name, enabled, projectByFile.value(name), {} });
    }

    // Hash only where it decides something: a player jar named exactly like one of this server's mods.
    QSet<QString> wanted;
    for (const auto& mod : m_pack.mods) {
        wanted.insert(mod.filename);
    }
    QList<Kubik::LocalFile> out;
    for (auto file : std::as_const(byName)) {
        if (wanted.contains(file.filename) && file.enabled) {
            file.sha512 = sha512Of(mods.filePath(file.filename));
        }
        out.append(file);
    }
    return out;
}

void KubikSyncTask::executeTask()
{
    if (!m_instance) {
        emitFailed(tr("The instance was deleted."));
        return;
    }
    if (m_instance->isRunning()) {
        emitFailed(tr("Close Minecraft in %1 first, then press Play again.").arg(m_instance->name()));
        return;
    }
    setStatus(tr("Checking the mods for %1").arg(m_pack.serverName));

    QFile sidecar(sidecarPath(m_instance));
    Kubik::ManagedState current;
    if (sidecar.open(QIODevice::ReadOnly)) {
        current = Kubik::readManagedState(sidecar.readAll()).value_or(Kubik::ManagedState{});
    }

    QSet<QString> cached;
    for (const auto& mod : m_pack.mods) {
        if (QFile::exists(cachePath(mod.sha512))) {
            cached.insert(mod.sha512);
        }
    }
    m_plan = Kubik::planSync(current, m_pack, scanModsFolder(), cached);

    if (m_plan.download.isEmpty()) {
        apply();
        return;
    }

    setStatus(tr("Downloading %n mod(s) for %1", "", static_cast<int>(m_plan.download.size())).arg(m_pack.serverName));
    m_downloads = makeShared<NetJob>(tr("Mods for %1").arg(m_pack.serverName), APPLICATION->network());
    // No retry dialog: a failure here is reported on the server page, next to the Play button.
    m_downloads->setAskRetry(false);
    for (const auto& mod : std::as_const(m_plan.download)) {
        const QString target = cachePath(mod.sha512);
        FS::ensureFilePathExists(target);
        auto request = Net::Request::makeFile(QUrl(mod.url), target);
        // Written through QSaveFile after this check passes, so a wrong file never reaches the cache.
        request->addValidator(new Net::ChecksumValidator(QCryptographicHash::Sha512, mod.sha512));
        m_downloads->addNetAction(request);
    }
    connect(m_downloads.get(), &Task::progress, this, &KubikSyncTask::setProgress);
    connect(m_downloads.get(), &Task::succeeded, this, &KubikSyncTask::apply);
    connect(m_downloads.get(), &Task::failed, this, [this](const QString& reason) {
        // The player gets the mods' names; the hashes and HTTP detail go to the log.
        qWarning() << "KubikSyncTask: download failed for" << m_pack.serverName << reason;
        QStringList missing;
        for (const auto& mod : std::as_const(m_plan.download)) {
            if (!QFile::exists(cachePath(mod.sha512))) {
                missing.append(mod.filename);
            }
        }
        emitFailed(tr("%1 could not be downloaded, or what arrived was not the right file. Nothing was changed. Try again; "
                      "if it keeps happening, tell the server's owner.")
                       .arg(missing.isEmpty() ? tr("A mod") : missing.join(", ")));
    });
    connect(m_downloads.get(), &Task::aborted, this, &KubikSyncTask::emitAborted);
    m_downloads->start();
}

bool KubikSyncTask::abort()
{
    if (m_downloads && m_downloads->isRunning()) {
        return m_downloads->abort();
    }
    return Task::abort();
}

bool KubikSyncTask::placeFromCache(const Kubik::Mod& mod)
{
    const QString source = cachePath(mod.sha512);
    const QString target = FS::PathCombine(m_instance->modsRoot(), mod.filename);
    QFile::remove(target);
    // A hard link costs no space and is one file for every instance that shares the mod; a copy is
    // the fallback where links are not possible (another volume, a filesystem without them).
    std::error_code error;
    std::filesystem::create_hard_link(QFileInfo(source).filesystemFilePath(), QFileInfo(target).filesystemFilePath(), error);
    return !error || QFile::copy(source, target);
}

void KubikSyncTask::apply()
{
    if (!m_instance || m_instance->isRunning()) {
        emitFailed(tr("Close Minecraft first, then press Play again."));
        return;
    }
    setStatus(tr("Getting %1 ready").arg(m_instance->name()));
    const QDir mods(m_instance->modsRoot());
    if (!mods.exists() && !QDir().mkpath(mods.absolutePath())) {
        emitFailed(tr("The mods folder of %1 cannot be created.").arg(m_instance->name()));
        return;
    }

    for (const auto& name : std::as_const(m_plan.remove)) {
        // Sync's own file, in whichever form is there: the player may have switched it off.
        if (!QFile::remove(mods.filePath(name))) {
            QFile::remove(mods.filePath(name + DISABLED));
        }
    }
    for (const auto& name : std::as_const(m_plan.enable)) {
        QFile::rename(mods.filePath(name + DISABLED), mods.filePath(name));
    }
    for (const auto& name : std::as_const(m_plan.disable)) {
        QFile::remove(mods.filePath(name + DISABLED));
        QFile::rename(mods.filePath(name), mods.filePath(name + DISABLED));
    }
    for (const auto& mod : std::as_const(m_plan.place)) {
        if (!placeFromCache(mod)) {
            emitFailed(tr("%1 could not be put in the mods folder.").arg(mod.filename));
            return;
        }
    }

    QSaveFile sidecar(sidecarPath(m_instance));
    FS::ensureFilePathExists(sidecar.fileName());
    if (!sidecar.open(QIODevice::WriteOnly) || sidecar.write(Kubik::writeManagedState(m_plan.next)) < 0 || !sidecar.commit()) {
        emitFailed(tr("KubikLauncher could not record the mods it installed in %1.").arg(m_instance->name()));
        return;
    }

    raiseLoaderBuild();
    emitSucceeded();
}

void KubikSyncTask::raiseLoaderBuild()
{
    // A modpack's loader is the pack's, and the pack is the server's own version of it.
    if (m_pack.profile.isModpack() || m_pack.loaderBuild.isEmpty()) {
        return;
    }
    const QString uid = Kubik::loaderComponentUid(m_pack.loader);
    if (uid.isEmpty()) {
        return;
    }
    auto* components = m_instance->getPackProfile();
    const QString installed = components->getComponentVersion(uid);
    // Newer loader builds run older mods, so the instance keeps the newest any of its servers needs.
    if (installed.isEmpty() || Kubik::compareBuilds(installed, m_pack.loaderBuild) < 0) {
        components->setComponentVersion(uid, m_pack.loaderBuild);
        components->saveNow();
    }
}
