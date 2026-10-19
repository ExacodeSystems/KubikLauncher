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

#include <QList>
#include <QWidget>

#include <optional>

#include "kubik/KubikModels.h"
#include "ui/kubik/KubikServerTiles.h"

class KubikJoinShotView;
class QLabel;
class QPushButton;

/**
 * The server page's "How to join": four steps, one at a time, each with a picture of the Minecraft screen
 * it happens on (KubikJoinShots), drawn for this server. Press Play; coming back, Multiplayer on the title
 * screen; this server at the top of the list, then Join Server; and from another launcher, Add Server with
 * its address.
 *
 * One step at a time, so the picture is big enough to read at any window width: the steps' tabs above it
 * name all four, and Back and Next walk through them.
 */
class KubikJoinGuide : public QWidget {
    Q_OBJECT

   public:
    explicit KubikJoinGuide(QWidget* parent = nullptr);

    /** The server, what its last ping said, and the language its game is set to (options.txt's `lang:`). */
    void setServer(const Kubik::Server& server, const std::optional<KubikServerLive>& live, const QString& gameLanguage);
    /** Shows step `index`, from 0. */
    void showStep(int index);
    static constexpr int STEPS = 4;
    /** "Coming back? Press Multiplayer": the step for a player whose game is already running. */
    static constexpr int COMING_BACK = 1;

   signals:
    /** "Got it": the player has read it. */
    void dismissed();

   protected:
    void changeEvent(QEvent* event) override;

   private:
    void retranslate();

    QLabel* m_title;
    QLabel* m_languageNote;
    QPushButton* m_dismiss;
    QList<QPushButton*> m_tabs;
    KubikJoinShotView* m_shot;
    QLabel* m_number;
    QLabel* m_stepTitle;
    QLabel* m_stepBody;
    QPushButton* m_back;
    QPushButton* m_next;
    int m_step = 0;
    QStringList m_titles;
    QStringList m_bodies;

    std::optional<Kubik::Server> m_server;
    std::optional<KubikServerLive> m_live;
    QString m_gameLanguage;
};
