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

#include "KubikApi.h"

#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPointer>

#include "Application.h"
#include "BuildConfig.h"
#include "settings/SettingsObject.h"

namespace Kubik {

namespace {

QString origin(const char* setting, const char* env, const QString& built)
{
    QString value = APPLICATION->settings()->get(setting).toString().trimmed();
    if (value.isEmpty()) {
        value = qEnvironmentVariable(env).trimmed();
    }
    if (value.isEmpty()) {
        value = built;
    }
    while (value.endsWith('/')) {
        value.chop(1);
    }
    return value;
}

QNetworkRequest requestFor(const QString& path, const QString& token)
{
    QNetworkRequest request(QUrl(apiOrigin() + "/launcher/v1/" + path));
    request.setHeader(QNetworkRequest::UserAgentHeader, APPLICATION->getUserAgent());
    request.setRawHeader("Accept", "application/json");
    if (!token.isEmpty()) {
        request.setRawHeader("Authorization", "Bearer " + token.toLatin1());
    }
    // A request that hangs would hold Play on "Checking the server…" forever.
    request.setTransferTimeout(20000);
    return request;
}

void finish(QNetworkReply* reply, QObject* context, ReplyHandler done)
{
    // The reply is the connection's context, not `context`: the reply must be deleted even when the
    // page that asked has gone, so the handler always runs and only the callback is guarded.
    QPointer<QObject> guard(context);
    QObject::connect(reply, &QNetworkReply::finished, reply, [reply, guard, done = std::move(done)] {
        Reply result;
        result.status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        result.body = reply->readAll();
        reply->deleteLater();
        if (guard) {
            done(result);
        }
    });
}

}  // namespace

QString apiOrigin()
{
    return origin("KubikApiUrlOverride", "KUBIKLAUNCHER_API_URL", BuildConfig.KUBIK_API_URL);
}

QString webOrigin()
{
    return origin("KubikWebUrlOverride", "KUBIKLAUNCHER_WEB_URL", BuildConfig.KUBIK_WEB_URL);
}

void apiGet(const QString& path, const QString& token, QObject* context, ReplyHandler done)
{
    finish(APPLICATION->network()->get(requestFor(path, token)), context, std::move(done));
}

void apiPost(const QString& path, const QJsonObject& body, const QString& token, QObject* context, ReplyHandler done)
{
    auto request = requestFor(path, token);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    finish(APPLICATION->network()->post(request, QJsonDocument(body).toJson(QJsonDocument::Compact)), context, std::move(done));
}

}  // namespace Kubik
