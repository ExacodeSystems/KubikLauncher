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

#include "KubikSyncPlan.h"

#include <algorithm>

#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace Kubik {

SyncPlan planSync(const ManagedState& current, const Pack& pack, const QList<LocalFile>& local, const QSet<QString>& cachedSha512)
{
    SyncPlan plan;
    plan.next.serverId = pack.serverId;
    plan.next.revision = pack.revision;
    // Not about mods: carried over for KubikSyncTask's game files step, which updates them.
    plan.next.serverEntry = current.serverEntry;
    plan.next.lookApplied = current.lookApplied;

    QHash<QString, QString> placedSha;  // filename -> sha512, what sync owns now
    for (const auto& file : current.placed) {
        placedSha.insert(file.filename, file.sha512);
    }
    QHash<QString, Mod> wanted;  // filename -> mod
    QSet<QString> wantedProjects;
    for (const auto& mod : pack.mods) {
        wanted.insert(mod.filename, mod);
        if (!mod.projectId.isEmpty()) {
            wantedProjects.insert(mod.projectId);
        }
    }

    // What is on disk, split into sync's own jars and the player's.
    QHash<QString, LocalFile> ours;
    QHash<QString, LocalFile> theirs;
    for (const auto& file : local) {
        (placedSha.contains(file.filename) ? ours : theirs).insert(file.filename, file);
    }

    // 1. Sync's own jars: an identical one stays, anything else goes.
    QSet<QString> kept;
    for (auto it = placedSha.cbegin(); it != placedSha.cend(); ++it) {
        const auto want = wanted.constFind(it.key());
        const bool onDisk = ours.contains(it.key());
        if (want != wanted.cend() && want->sha512 == it.value() && onDisk) {
            kept.insert(it.key());
            // A required mod the player switched off goes back on: without it the server refuses them.
            if (!ours.value(it.key()).enabled && want->required) {
                plan.enable.append(it.key());
            }
            continue;
        }
        if (onDisk) {
            plan.remove.append(it.key());
        }
    }

    // 2. The player's jars that conflict with this server's mods.
    QSet<QString> conflicting;      // the player's, in the way of a server mod
    QSet<QString> satisfiedByThem;  // the player already has this very file
    for (const auto& file : std::as_const(theirs)) {
        const auto want = wanted.constFind(file.filename);
        if (want != wanted.cend()) {
            if (file.enabled && !file.sha512.isEmpty() && file.sha512 == want->sha512) {
                satisfiedByThem.insert(file.filename);
            } else {
                conflicting.insert(file.filename);
            }
            continue;
        }
        if (!file.projectId.isEmpty() && wantedProjects.contains(file.projectId)) {
            conflicting.insert(file.filename);
        }
    }

    // 3. Player jars sync disabled before: back on unless they are still in the way.
    for (const auto& name : current.displaced) {
        const auto file = theirs.constFind(name);
        if (file == theirs.cend()) {
            continue;  // the player deleted it meanwhile; nothing to restore, and nothing to track
        }
        if (conflicting.contains(name)) {
            plan.next.displaced.append(name);
            // Disabled already, so nothing to do; an enabled copy means the player turned it back on.
            if (file->enabled) {
                plan.disable.append(name);
            }
            continue;
        }
        if (!file->enabled) {
            plan.enable.append(name);
        }
    }
    for (const auto& name : std::as_const(conflicting)) {
        if (plan.next.displaced.contains(name)) {
            continue;
        }
        if (theirs.value(name).enabled) {
            plan.disable.append(name);
            plan.next.displaced.append(name);
        }
        // A player jar already disabled by the player is not in the way, and stays theirs to switch on.
    }

    // 4. This server's mods: kept, already the player's, or placed from the cache.
    for (const auto& mod : pack.mods) {
        if (satisfiedByThem.contains(mod.filename)) {
            continue;
        }
        plan.next.placed.append({ mod.filename, mod.sha512 });
        if (kept.contains(mod.filename)) {
            continue;
        }
        plan.place.append(mod);
        if (!cachedSha512.contains(mod.sha512)) {
            plan.download.append(mod);
        }
    }

    // Deterministic, so a plan reads the same twice and the tests can say what they expect.
    plan.remove.sort();
    plan.enable.sort();
    plan.disable.sort();
    plan.next.displaced.sort();
    const auto byName = [](const Mod& a, const Mod& b) { return a.filename < b.filename; };
    std::sort(plan.place.begin(), plan.place.end(), byName);
    std::sort(plan.download.begin(), plan.download.end(), byName);
    std::sort(plan.next.placed.begin(), plan.next.placed.end(),
              [](const ManagedFile& a, const ManagedFile& b) { return a.filename < b.filename; });
    return plan;
}

QByteArray writeManagedState(const ManagedState& state)
{
    QJsonArray placed;
    for (const auto& file : state.placed) {
        placed.append(QJsonObject{ { "filename", file.filename }, { "sha512", file.sha512 } });
    }
    QJsonObject root{
        { "formatVersion", 1 },
        { "serverId", state.serverId },
        { "revision", state.revision },
        { "placed", placed },
        { "displaced", QJsonArray::fromStringList(state.displaced) },
        { "serverEntry", state.serverEntry },
        { "lookApplied", state.lookApplied },
    };
    return QJsonDocument(root).toJson(QJsonDocument::Indented);
}

std::optional<ManagedState> readManagedState(const QByteArray& json)
{
    QJsonParseError error{};
    const auto doc = QJsonDocument::fromJson(json, &error);
    if (error.error != QJsonParseError::NoError || !doc.isObject() || doc.object().value("formatVersion").toInt() != 1) {
        return std::nullopt;
    }
    const auto root = doc.object();
    ManagedState state;
    state.serverId = root.value("serverId").toString();
    state.revision = root.value("revision").toString();
    state.serverEntry = root.value("serverEntry").toString();
    state.lookApplied = root.value("lookApplied").toBool();
    for (const auto& v : root.value("placed").toArray()) {
        const auto o = v.toObject();
        const auto name = o.value("filename").toString();
        if (!name.isEmpty()) {
            state.placed.append({ name, o.value("sha512").toString() });
        }
    }
    for (const auto& v : root.value("displaced").toArray()) {
        if (const auto name = v.toString(); !name.isEmpty()) {
            state.displaced.append(name);
        }
    }
    return state;
}

bool isManagedFile(const ManagedState& state, const QString& filename)
{
    static const QString disabled = QStringLiteral(".disabled");
    const QString name = filename.endsWith(disabled) ? filename.chopped(disabled.size()) : filename;
    return std::any_of(state.placed.cbegin(), state.placed.cend(), [&name](const ManagedFile& file) { return file.filename == name; });
}

}  // namespace Kubik
