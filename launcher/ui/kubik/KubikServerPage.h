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

#include <QPointer>
#include <QWidget>

#include <optional>

#include "kubik/KubikModels.h"

class BaseInstance;
class KubikPlay;
class QLabel;
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
 * fixes it where there is one (pay the invoice, open the dashboard).
 */
class KubikServerPage : public QWidget {
    Q_OBJECT

   public:
    explicit KubikServerPage(QWidget* parent = nullptr);

    void setServer(const Kubik::Server& server);
    QString serverId() const { return m_server.id; }
    /** Press Play, as the button does. */
    void play(bool newInstance = false);
    bool isPlaying() const { return !m_play.isNull(); }

   signals:
    void backRequested();
    void instanceOpened(const QString& instanceId);

   protected:
    void changeEvent(QEvent* event) override;

   private:
    void retranslate();
    void refreshPack();
    void showPack();
    void updateInstanceLine();
    void updatePlayState();
    void showNotice(const QString& message, const QString& actionLabel = {}, const QString& actionUrl = {});
    void chooseInstance();
    void onPlayFinished();

    Kubik::Server m_server;
    std::optional<Kubik::Pack> m_pack;
    QString m_packError;
    QPointer<KubikPlay> m_play;

    QPushButton* m_back;
    QLabel* m_art;
    QLabel* m_name;
    QLabel* m_about;
    QLabel* m_status;
    QLabel* m_address;
    QPushButton* m_copy;

    QPushButton* m_playButton;
    QPushButton* m_open;
    QLabel* m_progressText;
    QProgressBar* m_progress;

    QLabel* m_notice;
    QPushButton* m_noticeAction;
    QString m_noticeUrl;

    QLabel* m_instanceTitle;
    QLabel* m_instanceLine;
    QPushButton* m_instanceOpen;
    QPushButton* m_instanceChange;

    QLabel* m_modsTitle;
    QLabel* m_modsNote;
    QVBoxLayout* m_modsList;
    QLabel* m_unmanaged;
};
