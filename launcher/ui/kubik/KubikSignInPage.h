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

class KubikSplash;
class KubikWordmark;
class QFrame;
class QLabel;
class QPushButton;

/**
 * The whole window until the player signs in to kubik.gg: the launcher exists
 * to play the servers on that account, so without it there is nothing else to
 * show. Signing in happens in the browser (KubikAccount); this page says so and
 * waits.
 *
 * It is the launcher's title screen, as kubik.gg/launcher opens: the noon sky,
 * the wordmark with a splash line leaning off its corner, the one action, and
 * the ground everything stands on (KubikPaint).
 *
 * It also shows the Minecraft account the setup wizard just connected. The
 * sidebar's profile card, which shows it everywhere else, is hidden until this
 * page is passed, and without it a player cannot tell whether the Microsoft
 * sign-in worked.
 */
class KubikSignInPage : public QWidget {
    Q_OBJECT

   public:
    explicit KubikSignInPage(QWidget* parent = nullptr);

   protected:
    void changeEvent(QEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

   private:
    void retranslate();
    void updateState();
    void updateMinecraft();
    /** Puts the splash over the wordmark's lower right corner, wherever the layout has put the wordmark. */
    void placeSplash();

    QWidget* m_column;
    KubikWordmark* m_wordmark;
    KubikSplash* m_splash;

    QFrame* m_minecraft;
    QLabel* m_minecraftFace;
    QLabel* m_minecraftName;
    QLabel* m_minecraftOk;
    QLabel* m_minecraftState;
    QLabel* m_title;
    QLabel* m_body;
    QLabel* m_error;
    QPushButton* m_signIn;
    QPushButton* m_cancel;
    QPushButton* m_signUp;
    QString m_lastError;
};
