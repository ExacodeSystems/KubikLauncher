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

#include <QWidget>

class QLabel;
class QPushButton;
class QVBoxLayout;

/**
 * Settings › Profiles: every Minecraft profile as a row (face, name, type, which
 * one plays) with Use, Skin and Remove; adding a Microsoft account or an offline
 * profile; and the kubik.gg account with its Sign out.
 *
 * It replaces Prism's account page, a toolbar over a table. The rows rebuild
 * whenever the account list changes, so a sign-in finishing in the background
 * shows up without reopening Settings.
 */
class KubikProfilesPage : public QWidget {
    Q_OBJECT

   public:
    explicit KubikProfilesPage(QWidget* parent = nullptr);

   protected:
    void changeEvent(QEvent* event) override;

   private:
    void rebuild();

    QLabel* m_title;
    QLabel* m_profilesTitle;
    QLabel* m_profilesBody;
    QVBoxLayout* m_rows;
    QPushButton* m_addMicrosoft;
    QPushButton* m_addOffline;
    QLabel* m_offlineNote;
    QLabel* m_kubikTitle;
    QLabel* m_kubikWho;
    QPushButton* m_signOut;
};
