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

#include "KubikPlay.h"

#include <QDesktopServices>
#include <QTimer>
#include <QUrl>

#include "Application.h"
#include "InstanceImportTask.h"
#include "InstanceList.h"
#include "kubik/KubikAccount.h"
#include "kubik/KubikApi.h"
#include "kubik/KubikInstanceTask.h"
#include "kubik/KubikProfiles.h"
#include "kubik/KubikSyncTask.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/auth/AccountList.h"
#include "minecraft/launch/MinecraftTarget.h"

namespace {

constexpr int POLL_MS = 5000;
// How long Play waits for a server to wake before saying so. Free servers boot in about a minute.
constexpr qint64 WAKE_TIMEOUT_MS = 3 * 60 * 1000;

}  // namespace

KubikPlay::KubikPlay(Kubik::Server server, QObject* parent) : QObject(parent), m_server(std::move(server)) {}

KubikPlay::~KubikPlay()
{
    if (m_syncTask && m_syncTask->isRunning()) {
        m_syncTask->abort();
    }
}

QString KubikPlay::describe(const Kubik::ApiError& error)
{
    const QString& code = error.code;
    if (code == "offline") {
        return tr("KubikLauncher could not reach kubik.gg. Check your connection, then try again.");
    }
    if (code == "unauthorized") {
        return tr("Your kubik.gg sign-in has expired. Sign in again to see your servers.");
    }
    if (code == "not_found") {
        return tr("This server is no longer on your kubik.gg account.");
    }
    if (code == "locked") {
        return tr("This server is frozen for an unpaid invoice. Pay it on kubik.gg to play again.");
    }
    if (code == "provisioning") {
        return tr("This server is still being set up. Try again in a minute.");
    }
    if (code == "unavailable") {
        return tr("kubik.gg could not read this server's mods just now. Try again in a minute.");
    }
    if (code == "unplayable") {
        if (error.reason == "bedrock") {
            return tr("This is a Bedrock server. Join it from Minecraft Bedrock Edition instead.");
        }
        if (error.reason == "modpack-unpinned") {
            return tr(
                "kubik.gg does not know which version of this server's modpack is installed yet, so KubikLauncher cannot "
                "match it. Ask kubik.gg support to look at it.");
        }
        return tr("KubikLauncher does not support this kind of server yet.");
    }
    return tr("Something went wrong talking to kubik.gg (%1). Try again in a minute.").arg(code);
}

void KubikPlay::setStage(Stage stage, const QString& status)
{
    emit stageChanged(stage, status);
}

void KubikPlay::fail(const QString& message)
{
    if (m_over) {
        return;
    }
    m_over = true;
    if (m_poll) {
        m_poll->stop();
    }
    emit failed(message);
}

void KubikPlay::cancel()
{
    if (m_over) {
        return;
    }
    m_over = true;
    if (m_poll) {
        m_poll->stop();
    }
    if (m_syncTask && m_syncTask->isRunning()) {
        m_syncTask->abort();
    }
    if (m_installTask && m_installTask->isRunning()) {
        m_installTask->abort();
    }
    emit cancelled();
}

void KubikPlay::start()
{
    setStage(Stage::Checking, tr("Checking %1…").arg(m_server.name));
    Kubik::apiGet("servers/" + m_server.id + "/pack", KubikAccount::get()->token(), this, [this](const Kubik::Reply& reply) {
        if (m_over) {
            return;
        }
        if (!reply.ok()) {
            const auto error = Kubik::parseApiError(reply.status, reply.body);
            if (error.code == "unauthorized") {
                KubikAccount::get()->tokenRejected();
            }
            fail(describe(error));
            return;
        }
        const auto pack = Kubik::parsePack(reply.body);
        if (!pack) {
            fail(tr("kubik.gg sent something KubikLauncher does not understand. Check for a launcher update."));
            return;
        }
        onPack(*pack);
    });
}

void KubikPlay::onPack(const Kubik::Pack& pack)
{
    m_pack = pack;
    m_serverUp = pack.status == "RUNNING";
    if (pack.address.isEmpty()) {
        fail(tr("This server has no address yet. Try again in a minute."));
        return;
    }
    // Start the server waking while the instance gets ready: the two take about as long as each other.
    if (!m_serverUp) {
        watchServer();
    }

    const auto candidates = Kubik::instancesFor(pack.profile.key);
    if (candidates.isEmpty() || m_forceNew) {
        install();
        return;
    }
    if (candidates.size() == 1) {
        useInstance(candidates.first());
        return;
    }
    if (auto* remembered = Kubik::rememberedInstance(m_server.id, pack.profile.key)) {
        useInstance(remembered);
        return;
    }
    emit choiceNeeded(candidates);
}

void KubikPlay::choose(BaseInstance* instance)
{
    if (m_over || !m_pack) {
        return;
    }
    if (instance) {
        useInstance(instance);
    } else {
        install();
    }
}

void KubikPlay::useInstance(BaseInstance* instance)
{
    m_instance = qobject_cast<MinecraftInstance*>(instance);
    if (!m_instance) {
        fail(tr("That instance cannot play Minecraft: Java Edition servers."));
        return;
    }
    Kubik::rememberInstance(m_server.id, m_instance);
    sync();
}

void KubikPlay::install()
{
    const auto& pack = *m_pack;
    setStage(Stage::Installing, tr("Installing %1…").arg(pack.profile.label));

    InstanceTask* task = nullptr;
    if (pack.modpack) {
        QMap<QString, QString> extra{ { "pack_id", pack.modpack->projectId },
                                      { "pack_version_id", pack.modpack->versionId },
                                      { "kubik_optional_files", "enabled" } };
        auto* import = new InstanceImportTask(QUrl(pack.modpack->url), true, nullptr, extra);
        import->setName(pack.profile.label);
        // "default" lets the import take the pack's own icon.
        import->setIcon("default");
        import->setConfirmUpdate(false);
        task = import;
    } else {
        task = new KubikInstanceTask(pack);
    }

    m_createdId.clear();
    auto* list = APPLICATION->instances();
    // The staging step names the new instance only through this signal.
    connect(list, &InstanceList::instanceSelectRequest, this, [this](const QString& id) {
        if (m_installTask && m_installTask->isRunning()) {
            m_createdId = id;
        }
    });
    m_installTask.reset(list->wrapInstanceTask(task));
    // Prism's own status lines here ("Executing task 5 out of 5") say nothing to a player; the stage
    // line stays "Installing Fabric 26.2…" and the bar shows how far along it is.
    connect(m_installTask.get(), &Task::progress, this, &KubikPlay::progressChanged);
    connect(m_installTask.get(), &Task::succeeded, this, [this] {
        // The list picks the new folder up on its own event; look for the instance once it has.
        QTimer::singleShot(0, this, &KubikPlay::onInstalled);
    });
    connect(m_installTask.get(), &Task::failed, this,
            [this](const QString& reason) { fail(tr("The instance could not be installed: %1").arg(reason)); });
    connect(m_installTask.get(), &Task::aborted, this, [this] { fail(tr("Installing was cancelled.")); });
    m_installTask->start();
}

void KubikPlay::onInstalled()
{
    if (m_over) {
        return;
    }
    auto* instance = m_createdId.isEmpty() ? nullptr : APPLICATION->instances()->getInstanceById(m_createdId);
    if (!instance) {
        fail(tr("The instance was installed but KubikLauncher could not find it. Open Instances to check."));
        return;
    }
    // A modpack's instance comes from the pack's own installer, which knows nothing of profiles.
    instance->settings()->set("KubikProfile", m_pack->profile.key);
    useInstance(instance);
}

void KubikPlay::sync()
{
    if (m_over) {
        return;
    }
    setStage(Stage::Syncing, tr("Getting the mods for %1…").arg(m_server.name));
    m_syncTask = makeShared<KubikSyncTask>(m_instance, *m_pack);
    connect(m_syncTask.get(), &Task::progress, this, &KubikPlay::progressChanged);
    connect(m_syncTask.get(), &Task::status, this, [this](const QString& status) { setStage(Stage::Syncing, status); });
    connect(m_syncTask.get(), &Task::succeeded, this, [this] {
        m_synced = true;
        launchIfReady();
    });
    connect(m_syncTask.get(), &Task::failed, this, [this](const QString& reason) { fail(reason); });
    connect(m_syncTask.get(), &Task::aborted, this, [this] { fail(tr("Getting the mods was cancelled.")); });
    m_syncTask->start();
}

void KubikPlay::watchServer()
{
    // Waking goes through kubik.gg in the browser, where the dashboard's own checks for it live.
    if (!m_server.dashboardUrl.isEmpty()) {
        QDesktopServices::openUrl(QUrl(m_server.dashboardUrl));
    }
    m_waiting.start();
    if (!m_poll) {
        m_poll = new QTimer(this);
        m_poll->setInterval(POLL_MS);
        connect(m_poll, &QTimer::timeout, this, &KubikPlay::pollServer);
    }
    m_poll->start();
}

void KubikPlay::pollServer()
{
    if (m_over) {
        return;
    }
    if (m_waiting.elapsed() > WAKE_TIMEOUT_MS) {
        fail(tr("%1 did not wake up. Start it on kubik.gg, then press Play again.").arg(m_server.name));
        return;
    }
    Kubik::apiGet("servers/" + m_server.id, KubikAccount::get()->token(), this, [this](const Kubik::Reply& reply) {
        if (m_over || !reply.ok()) {
            return;  // a missed poll is retried by the next tick
        }
        const auto server = Kubik::parseServer(reply.body);
        if (!server) {
            return;
        }
        if (server->locked) {
            fail(describe({ 403, "locked", {} }));
            return;
        }
        if (server->isRunning()) {
            m_serverUp = true;
            m_poll->stop();
            launchIfReady();
        }
    });
}

void KubikPlay::launchIfReady()
{
    if (m_over) {
        return;
    }
    if (m_synced && !m_serverUp) {
        setStage(Stage::WaitingForServer, tr("Waiting for %1 to wake up. Start it on kubik.gg if it has not started.").arg(m_server.name));
        return;
    }
    if (!m_synced || !m_serverUp || !m_instance) {
        return;
    }
    // Asked here rather than by LaunchController, whose answer is a Yes/No box: everything is installed and
    // synced by now, so adding an account on the server page and pressing Play again goes straight in.
    if (!APPLICATION->accounts()->anyAccountIsValid()) {
        m_over = true;
        emit accountNeeded();
        return;
    }
    setStage(Stage::Launching, tr("Starting Minecraft…"));
    m_over = true;
    const auto target = std::make_shared<MinecraftTarget>(MinecraftTarget::parse(m_pack->address, false));
    if (APPLICATION->launch(m_instance, LaunchMode::Normal, target)) {
        Kubik::recordLaunch(m_server.id, m_instance);
        emit launched(m_instance->id());
    } else {
        emit failed(tr("Minecraft could not be started from %1.").arg(m_instance->name()));
    }
}
