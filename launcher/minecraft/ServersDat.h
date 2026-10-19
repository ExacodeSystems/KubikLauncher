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

#include <memory>

namespace nbt {
class tag_compound;
}

/**
 * An instance's `servers.dat`: Minecraft's Multiplayer list, an uncompressed NBT
 * file whose root holds a `servers` list of {name, ip, icon, acceptTextures}.
 *
 * Moved out of the instance window's Servers page so KubikLauncher can put the
 * server a player joins at the top of that list (Kubik::upsertServerEntry) with
 * the same reader and writer the page uses.
 */
namespace ServersDat {

/** The root compound, or nullptr when the bytes are not a servers.dat. */
std::unique_ptr<nbt::tag_compound> parse(const QByteArray& bytes);
QByteArray serialize(const nbt::tag_compound& root);

/** The file's root compound, or nullptr when it is missing, unreadable or not a servers.dat. */
std::unique_ptr<nbt::tag_compound> read(const QString& path);
/** Writes the file atomically, creating its folder. */
bool write(const QString& path, const nbt::tag_compound& root);

}  // namespace ServersDat
