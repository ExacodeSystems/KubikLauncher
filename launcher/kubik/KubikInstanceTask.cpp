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

#include "KubikInstanceTask.h"

#include "Application.h"
#include "FileSystem.h"
#include "kubik/KubikProfiles.h"
#include "meta/Index.h"
#include "meta/VersionList.h"
#include "minecraft/PackProfile.h"
#include "settings/INISettingsObject.h"

KubikInstanceTask::KubikInstanceTask(Kubik::Pack pack) : m_pack(std::move(pack))
{
    setName(m_pack.profile.label);
    setIcon(Kubik::iconForLoader(m_pack.loader));
}

void KubikInstanceTask::executeTask()
{
    const QString uid = Kubik::loaderComponentUid(m_pack.loader);
    if (uid.isEmpty() || !m_pack.loaderBuild.isEmpty()) {
        create(m_pack.loaderBuild);
        return;
    }

    // kubik.gg could not read this server's loader build: take the loader's recommended one.
    setStatus(tr("Finding a %1 version for Minecraft %2").arg(m_pack.loader, m_pack.minecraft));
    auto list = APPLICATION->metadataIndex()->get(uid);
    m_metaTask = list->getLoadTask();
    connect(m_metaTask.get(), &Task::succeeded, this, [this, list] {
        // Forge and NeoForge name the Minecraft version they need; Fabric and Quilt work on any.
        Meta::Version::Ptr version = list->getRecommendedForParent("net.minecraft", m_pack.minecraft);
        if (!version) {
            version = list->getLatestForParent("net.minecraft", m_pack.minecraft);
        }
        if (!version) {
            version = std::dynamic_pointer_cast<Meta::Version>(list->getRecommended());
        }
        if (!version) {
            emitFailed(tr("No %1 version supports Minecraft %2.").arg(m_pack.loader, m_pack.minecraft));
            return;
        }
        create(version->descriptor());
    });
    connect(m_metaTask.get(), &Task::failed, this, [this](const QString& reason) { emitFailed(reason); });
    m_metaTask->start();
}

void KubikInstanceTask::create(const QString& loaderBuild)
{
    setStatus(tr("Creating %1").arg(m_pack.profile.label));
    m_instance = std::make_unique<MinecraftInstance>(
        m_globalSettings, std::make_unique<INISettingsObject>(FS::PathCombine(m_stagingPath, "instance.cfg")), m_stagingPath);
    {
        const SettingsObject::Lock lock(m_instance->settings());
        auto* components = m_instance->getPackProfile();
        components->buildingFromScratch();
        components->setComponentVersion("net.minecraft", m_pack.minecraft, true);
        if (const QString uid = Kubik::loaderComponentUid(m_pack.loader); !uid.isEmpty()) {
            components->setComponentVersion(uid, loaderBuild);
        }
        m_instance->setName(name());
        m_instance->setIconKey(m_instIcon);
        m_instance->settings()->set("KubikProfile", m_pack.profile.key);
        components->saveNow();
    }
    downloadFiles(m_instance.get());
}
