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

#include "KubikAccount.h"

#include <QDesktopServices>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QOAuthHttpServerReplyHandler>
#include <QSaveFile>
#include <QTimer>
#include <QUrlQuery>

#include "Application.h"
#include "FileSystem.h"
#include "kubik/KubikApi.h"

namespace {

// How long the loopback stays open for a browser that never comes back. The code itself lives a
// minute; this only bounds how long a forgotten sign-in keeps a port open.
constexpr int GIVE_UP_MS = 10 * 60 * 1000;

QString randomState()
{
    // Any unguessable URL-safe value; the PKCE verifier generator already makes exactly that.
    return Kubik::makePkceVerifier();
}

}  // namespace

KubikAccount* KubikAccount::get()
{
    static KubikAccount* account = [] {
        auto* a = new KubikAccount(APPLICATION);
        a->load();
        return a;
    }();
    return account;
}

KubikAccount::KubikAccount(QObject* parent) : QObject(parent) {}

QString KubikAccount::filePath() const
{
    return FS::PathCombine(APPLICATION->dataRoot(), "kubik-auth.json");
}

void KubikAccount::load()
{
    QFile file(filePath());
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }
    const auto root = QJsonDocument::fromJson(file.readAll()).object();
    m_token = root.value("token").toString();
    const auto user = root.value("user").toObject();
    m_user = { user.value("id").toString(), user.value("name").toString(), user.value("email").toString() };
    if (!m_token.isEmpty() && !m_user.id.isEmpty()) {
        m_state = State::SignedIn;
    } else {
        m_token.clear();
        m_user = {};
    }
}

bool KubikAccount::save() const
{
    QSaveFile file(filePath());
    if (!file.open(QIODevice::WriteOnly)) {
        qCritical() << "KubikAccount: cannot save" << filePath() << file.errorString();
        return false;
    }
    const QJsonObject root{
        { "formatVersion", 1 },
        { "token", m_token },
        { "user", QJsonObject{ { "id", m_user.id }, { "name", m_user.name }, { "email", m_user.email } } },
    };
    file.write(QJsonDocument(root).toJson());
    file.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ReadUser | QFile::WriteUser);
    return file.commit();
}

void KubikAccount::forget()
{
    m_token.clear();
    m_user = {};
    m_state = State::SignedOut;
    QFile::remove(filePath());
    emit changed();
}

void KubikAccount::signIn()
{
    if (m_state == State::SignedIn) {
        return;
    }
    if (!m_listener) {
        m_listener = new QOAuthHttpServerReplyHandler(this);
        m_listener->setCallbackPath("/callback");
        connect(m_listener, &QAbstractOAuthReplyHandler::callbackReceived, this, &KubikAccount::onCallback);
    }
    if (!m_listener->isListening() && !m_listener->listen(QHostAddress::LocalHost, 0)) {
        emit signInFailed(tr("KubikLauncher could not open a local port to finish signing in. Check your firewall, then try again."));
        return;
    }
    // What the browser shows once it lands back here: straight on to the dashboard's "Connected" page.
    const QString done = Kubik::webOrigin() + "/dashboard/launcher/connect?done=1";
    m_listener->setCallbackText(QString("<!doctype html><meta charset=\"utf-8\"><meta http-equiv=\"refresh\" content=\"0;url=%1\">"
                                        "<title>KubikLauncher</title><p>%2</p>")
                                    .arg(done.toHtmlEscaped(), tr("Signed in. You can go back to KubikLauncher.").toHtmlEscaped()));

    m_verifier = Kubik::makePkceVerifier();
    m_expectedState = randomState();
    QUrl url(Kubik::webOrigin() + "/dashboard/launcher/connect");
    QUrlQuery query;
    query.addQueryItem("challenge", Kubik::pkceChallenge(m_verifier));
    query.addQueryItem("port", QString::number(m_listener->port()));
    query.addQueryItem("state", m_expectedState);
    url.setQuery(query);

    if (!m_giveUp) {
        m_giveUp = new QTimer(this);
        m_giveUp->setSingleShot(true);
        connect(m_giveUp, &QTimer::timeout, this, &KubikAccount::cancelSignIn);
    }
    m_giveUp->start(GIVE_UP_MS);
    m_state = State::WaitingForBrowser;
    emit changed();
    QDesktopServices::openUrl(url);
}

void KubikAccount::stopListening()
{
    if (m_giveUp) {
        m_giveUp->stop();
    }
    if (m_listener) {
        m_listener->close();
    }
    m_verifier.clear();
    m_expectedState.clear();
}

void KubikAccount::cancelSignIn()
{
    if (m_state != State::WaitingForBrowser) {
        return;
    }
    stopListening();
    m_state = State::SignedOut;
    emit changed();
}

void KubikAccount::onCallback(const QVariantMap& values)
{
    if (m_state != State::WaitingForBrowser) {
        return;
    }
    // A callback that is not the answer to our own request (a stale tab, another program poking the
    // port) is ignored rather than failing the sign-in the player is still doing.
    if (values.value("state").toString() != m_expectedState) {
        return;
    }
    const QString code = values.value("code").toString();
    const QString verifier = m_verifier;
    stopListening();
    if (code.isEmpty()) {
        m_state = State::SignedOut;
        emit changed();
        emit signInFailed(tr("Signing in was cancelled in the browser."));
        return;
    }
    Kubik::apiPost("token", { { "code", code }, { "verifier", verifier } }, {}, this, [this](const Kubik::Reply& reply) {
        const auto user = reply.ok() ? Kubik::parseUser(reply.body) : std::nullopt;
        const QString token = reply.ok() ? QJsonDocument::fromJson(reply.body).object().value("token").toString() : QString();
        if (!user || token.isEmpty()) {
            m_state = State::SignedOut;
            emit changed();
            emit signInFailed(reply.status == 0 ? tr("KubikLauncher could not reach kubik.gg. Check your connection, then try again.")
                                                : tr("kubik.gg did not accept this sign-in. Try again from the start."));
            return;
        }
        m_token = token;
        m_user = *user;
        m_state = State::SignedIn;
        save();
        emit changed();
    });
}

void KubikAccount::signOut()
{
    if (m_state == State::WaitingForBrowser) {
        cancelSignIn();
        return;
    }
    if (!m_token.isEmpty()) {
        // Best effort: revoked on kubik.gg when it answers, forgotten here regardless.
        Kubik::apiPost("logout", {}, m_token, APPLICATION, [](const Kubik::Reply&) {});
    }
    forget();
}

void KubikAccount::tokenRejected()
{
    if (m_state == State::SignedIn) {
        forget();
    }
}
