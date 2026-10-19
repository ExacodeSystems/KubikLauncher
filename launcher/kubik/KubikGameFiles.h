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

#include <QByteArray>
#include <QString>

namespace nbt {
class tag_compound;
}

/**
 * The two game files KubikLauncher touches besides mods, as pure edits so the
 * tests can hold them to their rules. KubikSyncTask applies them before every
 * join, after the mods.
 *
 *   servers.dat   the server being joined goes first in Multiplayer, so a player
 *                 who leaves or gets disconnected finds it there
 *   options.txt   the Kubik resource pack is switched on, once per instance
 */
namespace Kubik {

/** The Multiplayer entry for a server: its name, its address, and an icon (PNG, may be empty). */
struct ServerEntry {
    QString name;
    QString ip;
    QByteArray icon;
};

/**
 * Puts `entry` first in `root`'s "servers" list.
 *
 * `previousIp` is the address KubikLauncher added last time (from the instance's
 * sidecar). That entry is removed when it names another server: a shared
 * instance carries only the joined server's mods, so the other one would fail
 * from the list. Entries the player added are never touched, and when the player
 * already has this address, it is left exactly where it is.
 *
 * Edits the raw NBT, so tags this code does not know (newer Minecraft adds them)
 * survive. Answers whether KubikLauncher owns the entry at `entry.ip` now, which
 * the caller records as the next `previousIp` (or not).
 */
bool upsertServerEntry(nbt::tag_compound& root, const ServerEntry& entry, const QString& previousIp);

/**
 * options.txt with `pack` switched on: added to `resourcePacks` right after "vanilla", so every other
 * pack (a modpack's included) still wins over it. It is never put on `incompatibleResourcePacks`, and is
 * taken off it if there: a compatible pack found on that list is left switched off for that session.
 * packMcmeta's ranges are what make it load on any version. Every other line is kept as it was, and a
 * missing `resourcePacks` line is added. Answers the text unchanged when the pack is already on.
 */
QString enableResourcePack(const QString& optionsTxt, const QString& pack);

/** How options.txt names a pack folder: "file/<folder>" from Minecraft 1.13, the bare folder before. */
QString resourcePackEntry(const QString& minecraftVersion, const QString& folder);

/**
 * pack.mcmeta for the Kubik pack on this Minecraft version: the exact pack format where Prism's
 * table knows the version, otherwise its newest one with an open range, which a newer version reads
 * as compatible.
 */
QByteArray packMcmeta(const QString& minecraftVersion, const QString& description);

}  // namespace Kubik
