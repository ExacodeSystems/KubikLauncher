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
#include <QString>

#include <functional>
#include <optional>

/**
 * What kubik.gg's `/launcher/v1` answers, parsed (the api side is
 * apps/api/src/launcher/views.ts in the kubik.gg repo).
 *
 * Parsing is strict about what the launcher acts on and lenient about the rest:
 * a pack missing a mod's url or hash is refused whole, because a sync applies a
 * pack and a half-read one would remove mods; a new field we do not know is
 * ignored, because v1 only ever adds fields.
 */
namespace Kubik {

struct User {
    QString id;
    QString name;
    QString email;
};

/** Which launcher instance plays a server (@repo/plan clientProfile). Equal keys share an instance. */
struct Profile {
    QString key;
    QString label;
    QString minecraft;
    /** fabric | quilt | forge | neoforge, or empty: a vanilla client, or a modpack (which brings its own). */
    QString loader;
    /** Set for a server built from a modpack: the Modrinth version its players install. */
    QString modpackVersionId;

    bool isModpack() const { return !modpackVersionId.isEmpty(); }
};

struct Server {
    QString id;
    QString name;
    /** A hostname, no port. Empty until the server has an address. */
    QString address;
    /** Bedrock only: its players need the port too. */
    int port = 0;
    QString edition;
    QString loader;
    QString loaderName;
    QString version;
    /** PROVISIONING | STOPPED | RUNNING | SUSPENDED | ERROR */
    QString status;
    QString plan;
    bool locked = false;
    QString lockReason;
    QString modpackName;
    std::optional<Profile> profile;
    /** Why it cannot be played from the launcher: bedrock | unknown-loader | modpack-unpinned. */
    QString unplayable;
    int modCount = 0;
    QString dashboardUrl;

    bool isRunning() const { return status == "RUNNING"; }
    /** Asleep, or stopped: Play has to wake it on kubik.gg first. */
    bool isAsleep() const { return status == "SUSPENDED" || status == "STOPPED"; }
};

struct Mod {
    QString projectId;
    QString versionId;
    QString filename;
    QString url;
    /** Lowercase hex. */
    QString sha512;
    qint64 size = 0;
    bool required = false;
};

struct Unmanaged {
    QString filename;
    /** unknown-origin | no-provenance */
    QString reason;
};

struct ModpackFile {
    QString versionId;
    QString projectId;
    QString name;
    QString url;
    QString sha512;
    qint64 size = 0;
};

/** Everything a player's game must match to join one server. */
struct Pack {
    QString revision;
    QString serverId;
    QString serverName;
    QString address;
    QString status;
    Profile profile;
    QString minecraft;
    QString loader;
    /** The loader build the server runs; empty when kubik.gg has not read it. */
    QString loaderBuild;
    std::optional<ModpackFile> modpack;
    QList<Mod> mods;
    QList<Unmanaged> unmanaged;
};

/** A refusal from the api: a stable code the launcher turns into a sentence (see apiErrorText). */
struct ApiError {
    /** 0 when the request never got an answer (offline, DNS, TLS). */
    int status = 0;
    QString code;
    /** For `unplayable`: bedrock | unknown-loader | modpack-unpinned. */
    QString reason;
};

std::optional<User> parseUser(const QByteArray& json);
std::optional<QList<Server>> parseServers(const QByteArray& json);
std::optional<Server> parseServer(const QByteArray& json);
std::optional<Pack> parsePack(const QByteArray& json);
/** The `{ error: { code, … } }` body of a refusal; a body that is not one still yields the status. */
ApiError parseApiError(int status, const QByteArray& body);

/** Numeric-aware: 0.16.10 sorts after 0.16.9 (the same rule as @repo/plan compareBuilds). */
int compareBuilds(const QString& a, const QString& b);

/** The uid PackProfile knows a loader by, or empty for a vanilla client. */
QString loaderComponentUid(const QString& loader);

/** RFC 7636: a fresh verifier (43 base64url characters), and its S256 challenge. */
QString makePkceVerifier();
QString pkceChallenge(const QString& verifier);

/**
 * Home's order: servers that are up first (they can be joined now), then the ones played most recently,
 * then by name. `lastPlayedMs` answers 0 for a server never played from this launcher.
 */
void sortForHome(QList<Server>& servers, const std::function<qint64(const QString&)>& lastPlayedMs);

}  // namespace Kubik
