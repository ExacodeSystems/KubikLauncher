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

/**
 * "My servers": the signed-in customer's Java servers on kubik.gg.
 *
 * Until Kubik sign-in lands (plan milestone M3) this page says what it will do
 * and links to the dashboard; it never pretends to list anything.
 */
class KubikServersPage : public QWidget {
    Q_OBJECT

   public:
    explicit KubikServersPage(QWidget* parent = nullptr);

   protected:
    void changeEvent(QEvent* event) override;

   private:
    void retranslate();

    QLabel* m_title;
    QLabel* m_body;
    QPushButton* m_open;
};
