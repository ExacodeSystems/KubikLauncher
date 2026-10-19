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

#include "ServersDat.h"

#include <io/stream_reader.h>
#include <io/stream_writer.h>
#include <tag_compound.h>

#include <sstream>

#include "FileSystem.h"

namespace ServersDat {

std::unique_ptr<nbt::tag_compound> parse(const QByteArray& bytes)
{
    try {
        std::istringstream in(std::string(bytes.constData(), bytes.size()));
        auto pair = nbt::io::read_compound(in);
        if (pair.first != "" || pair.second == nullptr) {
            return nullptr;
        }
        return std::move(pair.second);
    } catch (...) {
        return nullptr;
    }
}

QByteArray serialize(const nbt::tag_compound& root)
{
    std::ostringstream out;
    nbt::io::write_tag("", root, out);
    const std::string bytes = out.str();
    return QByteArray(bytes.data(), static_cast<qsizetype>(bytes.size()));
}

std::unique_ptr<nbt::tag_compound> read(const QString& path)
{
    try {
        auto res = FS::read(path);
        if (!res) {
            return nullptr;
        }
        return parse(res.value());
    } catch (...) {
        return nullptr;
    }
}

bool write(const QString& path, const nbt::tag_compound& root)
{
    try {
        if (!FS::ensureFilePathExists(path)) {
            return false;
        }
        return FS::write(path, serialize(root)).has_value();
    } catch (...) {
        return false;
    }
}

}  // namespace ServersDat
