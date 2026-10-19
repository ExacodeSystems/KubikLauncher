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

#include <QPointer>

#include "kubik/KubikModels.h"
#include "kubik/KubikSyncPlan.h"
#include "net/NetJob.h"
#include "tasks/Task.h"

class MinecraftInstance;

/**
 * Makes an instance ready to join one kubik.gg server: applies KubikSyncPlan.
 *
 *   1. Read what sync owns (`<instance>/.kubik/managed.json`) and what is in the
 *      mods folder, with the Modrinth project of each of the player's own jars
 *      from Prism's metadata.
 *   2. Download what the launcher's cache (`<data>/kubik/cache/<sha512>`) lacks,
 *      every file checked against its sha512 before it is kept. Nothing in the
 *      mods folder changes until every download has succeeded, so a failed sync
 *      leaves the instance as it was.
 *   3. Remove, enable, disable and place, in that order, then write the sidecar.
 *   4. Raise the loader build when the server runs a newer one (never lower it).
 *
 * Refuses while the instance is running: the game holds its jars open.
 */
class KubikSyncTask : public Task {
    Q_OBJECT

   public:
    KubikSyncTask(MinecraftInstance* instance, Kubik::Pack pack);

    /** Where the cache keeps the file with this hash. */
    static QString cachePath(const QString& sha512);

   protected:
    void executeTask() override;
    bool abort() override;

   private:
    QList<Kubik::LocalFile> scanModsFolder() const;
    void apply();
    bool placeFromCache(const Kubik::Mod& mod);
    void raiseLoaderBuild();

    QPointer<MinecraftInstance> m_instance;
    Kubik::Pack m_pack;
    Kubik::SyncPlan m_plan;
    NetJob::Ptr m_downloads;
};
