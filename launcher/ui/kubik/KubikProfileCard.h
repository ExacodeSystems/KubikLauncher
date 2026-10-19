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

#include <QAbstractButton>

class QMenu;

/**
 * Who is playing, at the bottom of the rail: the Minecraft profile's face and
 * name on a raised card, so it reads as an account and not as one more page.
 *
 * Its dropdown is the one place accounts change: switch the profile that plays,
 * add a Microsoft account or an offline profile, manage them in Settings, and,
 * in a second section, the kubik.gg account the servers belong to.
 */
class KubikProfileCard : public QAbstractButton {
    Q_OBJECT

   public:
    explicit KubikProfileCard(QWidget* parent = nullptr);

    QSize sizeHint() const override;

   signals:
    /** "Manage profiles…": Settings, on its Profiles page. */
    void manageRequested();

   protected:
    void paintEvent(QPaintEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void changeEvent(QEvent* event) override;

   private:
    void openMenu();
    void refresh();

    bool m_hovered = false;
    bool m_menuOpen = false;
};
