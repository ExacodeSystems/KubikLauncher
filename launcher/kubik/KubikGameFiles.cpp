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

#include "KubikGameFiles.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStringList>

#include <tag_compound.h>
#include <tag_list.h>
#include <tag_string.h>

#include <optional>

#include "Version.h"
#include "minecraft/mod/ResourcePack.h"

namespace Kubik {

namespace {

/** An address as Minecraft compares them: no case, no default port. */
QString normalAddress(QString address)
{
    address = address.trimmed().toLower();
    if (address.endsWith(":25565")) {
        address.chop(6);
    }
    return address;
}

QString addressOf(const nbt::value& entry)
{
    if (entry.get_type() != nbt::tag_type::Compound) {
        return {};
    }
    const auto& server = entry.as<nbt::tag_compound>();
    if (!server.has_key("ip", nbt::tag_type::String)) {
        return {};
    }
    return normalAddress(QString::fromStdString(server.at("ip").as<nbt::tag_string>().get()));
}

/** A `key:["a","b"]` value from options.txt, or nothing when it is not a JSON list of strings. */
std::optional<QStringList> parseList(const QString& value)
{
    const auto doc = QJsonDocument::fromJson(value.trimmed().toUtf8());
    if (!doc.isArray()) {
        return std::nullopt;
    }
    QStringList out;
    for (const auto& v : doc.array()) {
        out.append(v.toString());
    }
    return out;
}

QString writeList(const QStringList& list)
{
    return QString::fromUtf8(QJsonDocument(QJsonArray::fromStringList(list)).toJson(QJsonDocument::Compact));
}

}  // namespace

bool upsertServerEntry(nbt::tag_compound& root, const ServerEntry& entry, const QString& previousIp)
{
    const QString ip = normalAddress(entry.ip);
    const QString previous = normalAddress(previousIp);

    nbt::tag_list old;
    if (root.has_key("servers", nbt::tag_type::List)) {
        old = std::move(root.at("servers").as<nbt::tag_list>());
    }

    // The player's own entry for this address wins: theirs to name, order and keep.
    bool playersOwn = false;
    for (const auto& server : old) {
        if (addressOf(server) == ip && previous != ip) {
            playersOwn = true;
        }
    }

    nbt::tag_list fresh(nbt::tag_type::Compound);
    if (!playersOwn) {
        // Ours from last time, at this address, is reused so tags Minecraft added to it survive.
        std::optional<nbt::value> ours;
        for (auto& server : old) {
            if (!ours && previous == ip && addressOf(server) == ip) {
                ours = std::move(server);
            }
        }
        if (!ours) {
            ours = nbt::value(nbt::tag_compound{});
        }
        auto& compound = ours->as<nbt::tag_compound>();
        compound.put("name", nbt::tag_string(entry.name.toStdString()));
        compound.put("ip", nbt::tag_string(entry.ip.trimmed().toStdString()));
        // Minecraft swaps the icon for the server's own once it pings; that one is kept.
        if (!compound.has_key("icon", nbt::tag_type::String) && !entry.icon.isEmpty()) {
            compound.put("icon", nbt::tag_string(entry.icon.toBase64().toStdString()));
        }
        fresh.push_back(std::move(*ours));
    }
    for (auto& server : old) {
        if (server.get_type() != nbt::tag_type::Compound) {
            continue;  // already moved into `ours` (an emptied value) or not an entry at all
        }
        // What KubikLauncher added last time goes: for this address it was rebuilt above, for another
        // server it would join with this server's mods.
        if (!previous.isEmpty() && addressOf(server) == previous) {
            continue;
        }
        fresh.push_back(std::move(server));
    }
    root.put("servers", std::move(fresh));
    return !playersOwn;
}

QString enableResourcePack(const QString& optionsTxt, const QString& pack)
{
    QStringList lines = optionsTxt.split('\n');
    bool sawPacks = false;
    bool changed = false;
    for (auto& line : lines) {
        const bool crlf = line.endsWith('\r');
        const QString bare = crlf ? line.chopped(1) : line;
        if (bare.startsWith("resourcePacks:")) {
            sawPacks = true;
            auto list = parseList(bare.mid(QStringLiteral("resourcePacks:").size()));
            if (list && !list->contains(pack)) {
                const auto vanilla = list->indexOf("vanilla");
                list->insert(vanilla >= 0 ? vanilla + 1 : 0, pack);
                line = "resourcePacks:" + writeList(*list) + (crlf ? "\r" : "");
                changed = true;
            }
        } else if (bare.startsWith("incompatibleResourcePacks:")) {
            // Never listed here: when a pack on this list turns out compatible, Minecraft takes it off the list
            // and leaves it switched off for that session (an earlier build put it here, and lost a first launch).
            auto list = parseList(bare.mid(QStringLiteral("incompatibleResourcePacks:").size()));
            if (list && list->removeAll(pack) > 0) {
                line = "incompatibleResourcePacks:" + writeList(*list) + (crlf ? "\r" : "");
                changed = true;
            }
        }
    }
    if (sawPacks && !changed) {
        return optionsTxt;
    }
    if (!sawPacks) {
        // The missing line goes at the end, before the file's final newline.
        if (!lines.isEmpty() && lines.last().isEmpty()) {
            lines.removeLast();
        }
        lines.append("resourcePacks:" + writeList({ "vanilla", pack }));
        return lines.join('\n') + '\n';
    }
    return lines.join('\n');
}

QString resourcePackEntry(const QString& minecraftVersion, const QString& folder)
{
    return Version(minecraftVersion) >= Version("1.13") ? "file/" + folder : folder;
}

QByteArray packMcmeta(const QString& minecraftVersion, const QString& description)
{
    const auto exact = ResourcePack::formatFor(minecraftVersion);
    const auto format = exact.value_or(ResourcePack::newestFormat());
    QJsonObject pack{ { "pack_format", format.first }, { "description", description } };
    // Minecraft 1.21.9 (format 65) describes support as a range of formats, each [major, minor].
    if (format.first >= 65 || !exact) {
        pack.insert("min_format", QJsonArray{ format.first, format.second });
        // A version newer than Prism's table reads a format at or above the newest one it knows.
        pack.insert("max_format", exact ? QJsonValue(QJsonArray{ format.first, format.second }) : QJsonValue(1000));
    }
    return QJsonDocument(QJsonObject{ { "pack", pack } }).toJson(QJsonDocument::Indented);
}

}  // namespace Kubik
