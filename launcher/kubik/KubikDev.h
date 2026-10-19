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

#include <QLatin1String>
#include <QString>
#include <QtGlobal>

#include "minecraft/auth/MinecraftAccount.h"

namespace Kubik {

/**
 * Debug builds only: `KUBIKLAUNCHER_DEV_OFFLINE=1` lets an offline profile be added and played without a
 * Microsoft account that owns Minecraft, so the in-game parts can be tested before Mojang approves the
 * launcher's app id. A release build (NDEBUG) always answers false, so what ships keeps Prism's rule.
 */
inline bool devOfflineAllowed()
{
#ifdef NDEBUG
    return false;
#else
    return qEnvironmentVariable("KUBIKLAUNCHER_DEV_OFFLINE") == QLatin1String("1");
#endif
}

/** The development switch is on and this account is an offline profile: let it play. */
inline bool devOfflinePlays(const MinecraftAccountPtr& account)
{
    return devOfflineAllowed() && account && account->accountType() == AccountType::Offline;
}

}  // namespace Kubik
