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

#include <QDateTime>
#include <QList>
#include <QString>
#include <QStringList>

class BaseInstance;

/**
 * Which instances can play which kubik.gg servers.
 *
 * Servers that need the same client share one instance: kubik.gg names the
 * client a server needs with a profile key (`fabric-1.21.1`, `vanilla-1.21.1`,
 * `modpack-<version>`; @repo/plan clientProfile), and an instance made for a
 * server records that key in its `instance.cfg` (`KubikProfile`). Every instance
 * carrying a server's key can play it, so a player who copies "Fabric 1.21.1"
 * into "Fabric 1.21.1 + shaders" gets to pick between the two.
 */
namespace Kubik {

QString profileOf(BaseInstance* instance);

/** Every instance made for this profile, the one played last first. */
QList<BaseInstance*> instancesFor(const QString& profileKey);

/** The instance this server played in last, if it still exists and still fits the server. */
BaseInstance* rememberedInstance(const QString& serverId, const QString& profileKey);

/** Remember the choice, and list the server on the instance (for "Used by …"). */
void rememberInstance(const QString& serverId, BaseInstance* instance);

/** The ids of the servers that have played in this instance. */
QStringList serversOf(BaseInstance* instance);

/** When this server was last played from this launcher; invalid if never. */
QDateTime lastPlayed(const QString& serverId);

/** A launch: the server's last-played time, and which server the instance's game is now joined to. */
void recordLaunch(const QString& serverId, BaseInstance* instance);

/** The server the instance's game was launched for, while it runs; empty otherwise (this session only). */
QString playingServer(BaseInstance* instance);

/** The icon a new instance gets for its loader: Prism's own loader icons, or grass for vanilla. */
QString iconForLoader(const QString& loader);

}  // namespace Kubik
