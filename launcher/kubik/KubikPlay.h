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

#pragma once

#include <QElapsedTimer>
#include <QObject>
#include <QPointer>
#include <QTime>

#include <optional>

#include "QObjectPtr.h"
#include "kubik/KubikModels.h"
#include "tasks/Task.h"

class BaseInstance;
class MinecraftInstance;
class QTimer;

namespace Kubik {

/**
 * The server page's install log: what Play is doing, one timestamped line per step.
 *
 * A line that differs from the one before it only in its numbers (or hash-like runs) is dropped.
 * Prism's download tasks report progress as status lines ("5 out of 3000 are done"), and a new
 * game's assets are thousands of files: the log keeps the step, not its counter.
 */
class InstallLog {
   public:
    /** The line to show for `text` at `time`, or nothing when it is empty or only the last one's numbers moved. */
    std::optional<QString> add(const QString& text, const QTime& time);
    void clear() { m_last.clear(); }

   private:
    QString m_last;
};

}  // namespace Kubik

/**
 * One press of Play on a kubik.gg server, from "which mods does it need" to the
 * game joining it (docs/plans/phase-10-kubik-launcher.md §4.5).
 *
 *   1. Fetch the server's pack.
 *   2. Pick the instance: make one when the profile has none, use it when there is
 *      one, otherwise the one this server played in last, or ask (choiceNeeded).
 *   3. If the server is asleep, open its page on kubik.gg, where waking it goes
 *      through the dashboard's own checks, and watch for it to come up meanwhile.
 *   4. Sync the instance's mods to this server (KubikSyncTask).
 *   5. Launch, joining the server's address. What the launch itself installs (Java, the
 *      game's own files) shows on the same bar; Play is done once the game is running.
 *
 * Every refusal arrives as `failed` with a sentence for the player; nothing is
 * launched after one, because an instance that does not match cannot join.
 */
class KubikPlay : public QObject {
    Q_OBJECT

   public:
    enum class Stage { Checking, Installing, Syncing, WaitingForServer, Launching };
    Q_ENUM(Stage)

    KubikPlay(Kubik::Server server, QObject* parent);
    ~KubikPlay() override;

    const Kubik::Server& server() const { return m_server; }
    /** The pack, once fetched. */
    const std::optional<Kubik::Pack>& pack() const { return m_pack; }

    /** Make a new instance for this server even when one already fits (the picker's "Make another"). */
    void setForceNewInstance(bool force) { m_forceNew = force; }

    void start();
    void cancel();
    /** Whether cancel() can stop Play now: once Minecraft is starting, only while a download is running. */
    bool canCancel() const;
    /** The answer to choiceNeeded: an existing instance, or nullptr to make a new one. */
    void choose(BaseInstance* instance);

    /** A refusal from kubik.gg, as a sentence for the player. */
    static QString describe(const Kubik::ApiError& error);

   signals:
    void stageChanged(KubikPlay::Stage stage, const QString& status);
    void progressChanged(qint64 done, qint64 total);
    /** Several instances fit this server and it has not played in one yet. */
    void choiceNeeded(const QList<BaseInstance*>& candidates);
    void failed(const QString& message);
    /** One line of the install log (Kubik::InstallLog), for the server page's "Show details". */
    void logged(const QString& line);
    /** Everything is ready but no Minecraft account can play: the page asks the player to add one. */
    void accountNeeded();
    void launched(const QString& instanceId);
    void cancelled();

   private:
    void setStage(Stage stage, const QString& status);
    void log(const QString& text);
    /** A task's own status, details, steps and warnings, into the install log. */
    void logTask(Task* task);
    void fail(const QString& message);
    void onPack(const Kubik::Pack& pack);
    void useInstance(BaseInstance* instance);
    void install();
    void onInstalled();
    void sync();
    void watchServer();
    void pollServer();
    void launchIfReady();
    /** A launch step with work to show (installing Java, fetching the game's files), on the page's bar. */
    void followLaunchStep(Task* step);
    void onGameRunning();

    Kubik::Server m_server;
    std::optional<Kubik::Pack> m_pack;
    QPointer<MinecraftInstance> m_instance;

    unique_qobject_ptr<Task> m_installTask;
    QString m_createdId;
    Task::Ptr m_syncTask;

    bool m_forceNew = false;
    bool m_synced = false;
    bool m_serverUp = false;
    bool m_over = false;
    // Handed to Prism's launch, until the game is running.
    bool m_launching = false;
    QTimer* m_poll = nullptr;
    QElapsedTimer m_waiting;
    Kubik::InstallLog m_log;
};
