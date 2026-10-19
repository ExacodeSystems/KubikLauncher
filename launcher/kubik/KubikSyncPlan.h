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
#include <QList>
#include <QSet>
#include <QString>
#include <QStringList>

#include <optional>

#include "kubik/KubikModels.h"

/**
 * What a sync does to an instance's mods folder before it joins one server, decided
 * without touching a file (docs/plans/phase-10-kubik-launcher.md §1.7, §4.6).
 *
 * One instance serves every server with the same profile, so its "server layer" is
 * rebuilt for the server about to be joined: what sync placed for another server
 * goes, this server's mods arrive (from the launcher's cache when it has them), and
 * an identical jar stays where it is. The player's own jars are never deleted. One
 * that conflicts with a server mod (the same file name, or the same Modrinth project
 * by Prism's own metadata) is disabled while that server is played and enabled again
 * once a server that does not need it is.
 *
 * Pure, so the rules are unit-tested (tests/KubikSyncPlan_test.cpp); KubikSyncTask
 * applies the plan in order: remove, enable, disable, place.
 */
namespace Kubik {

/** One jar sync put in the mods folder (its enabled-form name). */
struct ManagedFile {
    QString filename;
    QString sha512;
};

/** The sidecar, `<instance>/.kubik/managed.json`: what sync owns in this instance. */
struct ManagedState {
    QString serverId;
    QString revision;
    QList<ManagedFile> placed;
    /** Player jars sync disabled because a server mod conflicted with them (enabled-form names). */
    QStringList displaced;
    /** The Multiplayer entry KubikLauncher added to servers.dat (its address); empty when the player's own was used. */
    QString serverEntry;
    /** The Kubik resource pack was switched on in options.txt once; never again, so turning it off sticks. */
    bool lookApplied = false;
};

/** A jar in the mods folder, by its enabled-form name. */
struct LocalFile {
    QString filename;
    bool enabled = true;
    /** The Modrinth project, when Prism's metadata (a `.pw.toml` under mods/.index) names one. */
    QString projectId;
    /** Filled by the caller only where it matters: a player jar named like a server mod. */
    QString sha512;
};

struct SyncPlan {
    /** Not in the cache yet: fetched (and checked) before anything in the folder changes. */
    QList<Mod> download;
    /** Deleted, in whichever form is on disk (`x.jar` or `x.jar.disabled`). */
    QStringList remove;
    /** `x.jar.disabled` → `x.jar`. */
    QStringList enable;
    /** `x.jar` → `x.jar.disabled`. */
    QStringList disable;
    /** Copied from the cache to `x.jar`. */
    QList<Mod> place;
    /** The sidecar after the plan has been applied. */
    ManagedState next;

    bool changesFolder() const { return !remove.isEmpty() || !enable.isEmpty() || !disable.isEmpty() || !place.isEmpty(); }
};

SyncPlan planSync(const ManagedState& current, const Pack& pack, const QList<LocalFile>& local, const QSet<QString>& cachedSha512);

/** The sidecar's JSON, both ways. A sidecar that does not parse reads as "sync owns nothing". */
QByteArray writeManagedState(const ManagedState& state);
std::optional<ManagedState> readManagedState(const QByteArray& json);

/** True when `filename` (enabled, or its `.disabled` form) is a file sync placed: a server's mod, not the player's. */
bool isManagedFile(const ManagedState& state, const QString& filename);

}  // namespace Kubik
