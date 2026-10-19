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
#include <QJsonObject>
#include <QString>

#include <functional>

class QObject;

/**
 * Requests to kubik.gg's `/launcher/v1`.
 *
 * Straight on the application's QNetworkAccessManager rather than Prism's
 * Net::Request: that class treats every 4xx as a failure and drops the body, and
 * here the body of a refusal is the point (`{ error: { code } }` is how the
 * launcher knows "frozen" from "not yours" from "still being built").
 */
namespace Kubik {

struct Reply {
    /** The HTTP status, or 0 when nothing answered (offline, DNS, TLS, timeout). */
    int status = 0;
    QByteArray body;

    bool ok() const { return status >= 200 && status < 300; }
};

using ReplyHandler = std::function<void(const Reply&)>;

/** kubik.gg's api and dashboard origins: the settings override, else the env var, else the build's. */
QString apiOrigin();
QString webOrigin();

/**
 * `GET <api>/launcher/v1/<path>`. `done` runs on the GUI thread, and never once
 * `context` has been destroyed, so a page closed mid-request is safe.
 */
void apiGet(const QString& path, const QString& token, QObject* context, ReplyHandler done);
void apiPost(const QString& path, const QJsonObject& body, const QString& token, QObject* context, ReplyHandler done);

}  // namespace Kubik
