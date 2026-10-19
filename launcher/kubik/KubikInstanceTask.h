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

#include <memory>

#include "InstanceTask.h"
#include "kubik/KubikModels.h"
#include "minecraft/MinecraftInstance.h"

/**
 * A new instance for a plain kubik.gg server (a profile with a loader, or plain
 * Minecraft): `net.minecraft` at the server's version, plus its mod loader at the
 * build the server runs, stamped with the profile key so every server of that
 * profile finds it (KubikProfiles.h).
 *
 * Component versions are set as strings, the way ModrinthCreationTask does, so
 * creating needs no metadata download. The one exception is a server whose loader
 * build kubik.gg never managed to read: then the loader's recommended build for
 * this Minecraft version is taken from the metadata index, the same choice
 * Prism's own "new instance" page would offer.
 *
 * A modpack server's instance is not made here: it comes from the pack itself,
 * through InstanceImportTask (KubikPlay.cpp).
 */
class KubikInstanceTask final : public InstanceTask {
    Q_OBJECT

   public:
    explicit KubikInstanceTask(Kubik::Pack pack);

   protected:
    void executeTask() override;

   private:
    void create(const QString& loaderBuild);

    Kubik::Pack m_pack;
    std::unique_ptr<MinecraftInstance> m_instance;
    Task::Ptr m_metaTask;
};
