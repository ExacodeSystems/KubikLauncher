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

#include <QIcon>
#include <QMetaObject>
#include <QPointer>
#include <QWidget>

#include <functional>
#include <optional>

#include "kubik/KubikModels.h"
#include "ui/kubik/KubikServerTiles.h"

class BaseInstance;
class KubikEyebrow;
class KubikJoinGuide;
class KubikPlay;
class QFrame;
class QLabel;
class QPlainTextEdit;
class QProgressBar;
class QPushButton;
class QVBoxLayout;

/**
 * One kubik.gg server: what it is, the instance it plays in, the mods a player
 * needs for it, and Play.
 *
 * Play runs here, on the page, rather than in a progress dialog: the button
 * becomes Cancel, the line under it says what is happening ("Installing Fabric
 * 1.21.1…", "Downloading 4 mods…", "Waiting for the server to wake up"), and
 * anything that stops it is said in the same place, with the one button that
 * fixes it where there is one (pay the invoice, add a Minecraft account).
 *
 * A sleeping server is woken on kubik.gg, not here: the launcher's token is
 * read-only, and the dashboard holds the checks a wake goes through (the
 * captcha, the free tier's wait). So a sleeping server gets "Wake it up on
 * kubik.gg", which opens its dashboard page, and the status line has Refresh,
 * which asks kubik.gg again; the page otherwise shows the status it opened with.
 *
 * The address has a card of its own, the one thing a player may need to type.
 * Under the hero, "How to join" walks through the join with pictures of the
 * Minecraft screens it happens on, drawn for this server (KubikJoinGuide). It is
 * open until the player says "Got it", and opens again by itself on the first
 * game; after that it is a link next to Play.
 */
class KubikServerPage : public QWidget {
    Q_OBJECT

   public:
    explicit KubikServerPage(QWidget* parent = nullptr);

    /** The server, and what Home's last ping of it said (its icon, players and message of the day), if any. */
    void setServer(const Kubik::Server& server, const KubikServerLive* live = nullptr);
    QString serverId() const { return m_server.id; }
    /** Press Play, as the button does. */
    void play(bool newInstance = false);
    bool isPlaying() const { return !m_play.isNull(); }

   signals:
    void backRequested();
    void instanceOpened(const QString& instanceId);
    /** Refresh fetched the server again; Home's copy of the list is older now. */
    void serverRefreshed(const QString& serverId);

   protected:
    void changeEvent(QEvent* event) override;

   private:
    void retranslate();
    void refreshPack();
    /** Refresh: the server's status, and its pack, from kubik.gg again. */
    void refreshStatus();
    void updateRefresh();
    void updateWake();
    void showPack();
    void updateInstanceLine();
    void updatePlayState();
    void showNotice(const QString& message, const QString& actionLabel = {}, const QString& actionUrl = {});
    void showNotice(const QString& message, const QString& actionLabel, std::function<void()> action, const QIcon& icon = {});
    void askForAccount();
    /** The instance this server plays in: the remembered one, or the only one that fits. */
    BaseInstance* currentInstance() const;
    bool inGame() const;
    /** The language the server's game is set to (its options.txt), which the join pictures follow. */
    QString gameLanguage() const;
    void watchInstance();
    void updateJoinCard();
    void updateAccountNote();
    void updateDetails();
    void chooseInstance();
    void onPlayFinished();

    Kubik::Server m_server;
    std::optional<KubikServerLive> m_live;
    std::optional<Kubik::Pack> m_pack;
    QString m_packError;
    QPointer<KubikPlay> m_play;

    QPushButton* m_back;
    QLabel* m_art;
    QLabel* m_name;
    QLabel* m_about;
    KubikEyebrow* m_status;
    QPushButton* m_refresh;
    bool m_checking = false;
    QFrame* m_addressCard;
    QLabel* m_addressLabel;
    QLabel* m_address;
    QLabel* m_port;
    QPushButton* m_copy;

    QPushButton* m_playButton;
    QPushButton* m_open;
    QPushButton* m_wake;
    QLabel* m_wakeNote;
    QLabel* m_progressText;
    QProgressBar* m_progress;
    // The install log (KubikPlay::logged): what Play did, one line per step, kept after a failure.
    QPushButton* m_detailsToggle;
    QPushButton* m_detailsCopy;
    QPlainTextEdit* m_details;
    bool m_detailsOpen = false;

    QLabel* m_accountNote;

    QLabel* m_notice;
    QPushButton* m_noticeAction;
    std::function<void()> m_noticeCallback;

    QPushButton* m_howToJoin;
    KubikJoinGuide* m_joinGuide;
    bool m_joinOpen = false;
    QMetaObject::Connection m_runningWatch;

    QLabel* m_instanceTitle;
    QLabel* m_instanceLine;
    QPushButton* m_instanceOpen;
    QPushButton* m_instanceChange;

    QLabel* m_modsTitle;
    QLabel* m_modsNote;
    QVBoxLayout* m_modsList;
    QLabel* m_unmanaged;
};
