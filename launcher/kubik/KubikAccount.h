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

#include <QObject>
#include <QString>

#include "kubik/KubikModels.h"

class QOAuthHttpServerReplyHandler;
class QTimer;

/**
 * The player's kubik.gg account: the one the launcher lists servers for.
 *
 * Separate from Prism's AccountList, which holds Minecraft (Microsoft) accounts:
 * a player needs both, one to see their servers and one to join them.
 *
 * Signing in happens in the system browser (apps/api/src/launcher/auth.ts in
 * kubik.gg): this listens on 127.0.0.1, opens the dashboard's "Connect
 * KubikLauncher" page with a PKCE challenge, receives the one-time code on the
 * loopback and redeems it, with the verifier only this process holds, for a
 * launcher token. That token can read the player's servers and nothing else.
 *
 * It is kept in `<data>/kubik-auth.json`, owner-only, the way Prism keeps the
 * Microsoft tokens in accounts.json.
 */
class KubikAccount : public QObject {
    Q_OBJECT

   public:
    enum class State { SignedOut, WaitingForBrowser, SignedIn };

    /** The one account, created on first use and owned by the application. */
    static KubikAccount* get();

    State state() const { return m_state; }
    bool isSignedIn() const { return m_state == State::SignedIn; }
    const Kubik::User& user() const { return m_user; }
    QString token() const { return m_token; }

    /** Open the browser and wait for it. Calling again while waiting reopens the browser. */
    void signIn();
    void cancelSignIn();
    /** Revoke the token on kubik.gg (best effort) and forget it here. */
    void signOut();
    /** An api call answered 401: the token was revoked or expired, so the player signs in again. */
    void tokenRejected();

   signals:
    void changed();
    /** Signing in did not finish; `message` is for the player, in their language. */
    void signInFailed(const QString& message);

   private:
    explicit KubikAccount(QObject* parent);
    void load();
    bool save() const;
    void forget();
    void onCallback(const QVariantMap& values);
    void stopListening();
    QString filePath() const;

    State m_state = State::SignedOut;
    Kubik::User m_user;
    QString m_token;

    QOAuthHttpServerReplyHandler* m_listener = nullptr;
    QTimer* m_giveUp = nullptr;
    QString m_verifier;
    QString m_expectedState;
};
