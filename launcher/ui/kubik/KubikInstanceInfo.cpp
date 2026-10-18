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

#include "KubikInstanceInfo.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFuture>
#include <QTimer>
#include <QtConcurrent>

#include "Application.h"
#include "BaseInstance.h"
#include "InstanceList.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"

namespace {

// The loaders a tile names, by the component uid that carries their version.
struct LoaderComponent {
    const char* uid;
    const char* name;
};
constexpr LoaderComponent LOADERS[] = {
    { "net.neoforged", "NeoForge" },         { "net.minecraftforge", "Forge" },          { "net.fabricmc.fabric-loader", "Fabric" },
    { "org.quiltmc.quilt-loader", "Quilt" }, { "com.mumfrey.liteloader", "LiteLoader" },
};

int countMods(const QString& modsRoot)
{
    // A jar or zip in mods/ is a mod; "foo.jar.disabled" is not counted, which is what the
    // instance's own mods page shows as enabled.
    return QDir(modsRoot).entryList({ "*.jar", "*.zip", "*.litemod" }, QDir::Files).size();
}

}  // namespace

KubikInstanceInfo* KubikInstanceInfo::instance()
{
    static auto* info = new KubikInstanceInfo(QCoreApplication::instance());
    return info;
}

KubikInstanceInfo::KubikInstanceInfo(QObject* parent) : QObject(parent) {}

KubikInstanceSummary KubikInstanceInfo::summary(BaseInstance* instance)
{
    if (!instance) {
        return {};
    }
    const QString id = instance->id();
    const auto it = m_cache.constFind(id);
    if (it == m_cache.constEnd() && !m_pending.contains(id)) {
        m_pending.insert(id);
        // After this paint, never during it.
        QTimer::singleShot(0, this, [this, id] { refresh(id); });
    }
    return it == m_cache.constEnd() ? KubikInstanceSummary{} : it.value();
}

void KubikInstanceInfo::invalidateAll()
{
    m_cache.clear();
}

void KubikInstanceInfo::refresh(const QString& instanceId)
{
    // Look the instance up again: it may have been deleted since the request was queued.
    auto* instance = APPLICATION->instances()->getInstanceById(instanceId);
    if (!instance) {
        m_pending.remove(instanceId);
        return;
    }

    KubikInstanceSummary summary;
    auto* profile = instance->getPackProfile();
    if (profile->getComponentVersion("net.minecraft").isEmpty()) {
        // Offline: read the instance's own files, never the network.
        (void)profile->reload(Net::Mode::Offline);
    }
    summary.minecraftVersion = profile->getComponentVersion("net.minecraft");
    for (const auto& loader : LOADERS) {
        const QString version = profile->getComponentVersion(loader.uid);
        if (!version.isEmpty()) {
            summary.loaderName = QString::fromLatin1(loader.name);
            summary.loaderVersion = version;
            break;
        }
    }
    m_cache.insert(instanceId, summary);
    m_pending.remove(instanceId);
    emit summaryChanged(instanceId);

    // Listing a folder can be slow on a network drive, so it runs off the UI thread.
    QtConcurrent::run(countMods, instance->modsRoot()).then(this, [this, instanceId](int count) {
        auto it = m_cache.find(instanceId);
        if (it != m_cache.end()) {
            it->modCount = count;
            emit summaryChanged(instanceId);
        }
    });
}

QString KubikInstanceInfo::describe(const KubikInstanceSummary& summary)
{
    QStringList parts;
    if (!summary.minecraftVersion.isEmpty()) {
        parts << tr("Minecraft %1").arg(summary.minecraftVersion);
    }
    parts << (summary.loaderName.isEmpty() ? tr("Vanilla") : summary.loaderName);
    if (summary.modCount > 0) {
        parts << tr("%n mod(s)", nullptr, summary.modCount);
    }
    return parts.join(QStringLiteral(" · "));
}

QString KubikInstanceInfo::lastPlayed(const BaseInstance* instance)
{
    const qint64 last = instance->lastLaunch();
    if (last <= 0) {
        return tr("Never played");
    }
    const qint64 seconds = (QDateTime::currentMSecsSinceEpoch() - last) / 1000;
    if (seconds < 60) {
        return tr("Played just now");
    }
    if (seconds < 3600) {
        return tr("Played %n minute(s) ago", nullptr, int(seconds / 60));
    }
    if (seconds < 86400) {
        return tr("Played %n hour(s) ago", nullptr, int(seconds / 3600));
    }
    const qint64 days = seconds / 86400;
    if (days < 30) {
        return tr("Played %n day(s) ago", nullptr, int(days));
    }
    return tr("Played on %1").arg(QLocale().toString(QDateTime::fromMSecsSinceEpoch(last).date(), QLocale::ShortFormat));
}

QString KubikInstanceInfo::totalPlayed(const BaseInstance* instance)
{
    const qint64 seconds = instance->totalTimePlayed();
    if (seconds < 60) {
        return tr("Not played yet");
    }
    const qint64 hours = seconds / 3600;
    const qint64 minutes = (seconds % 3600) / 60;
    if (hours == 0) {
        return tr("%1 min played").arg(minutes);
    }
    return tr("%1 h %2 min played").arg(hours).arg(minutes);
}
