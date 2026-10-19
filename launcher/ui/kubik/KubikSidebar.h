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

#include <QFrame>

class KubikProfileCard;
class QButtonGroup;
class QLabel;
class QMenu;
class QPushButton;

/**
 * The main window's left rail: the Kubik mark, the pages, and at the bottom the
 * profile card (who plays, and the kubik.gg account inside its menu), Settings
 * and "Help & more".
 *
 * The rail owns no behaviour beyond navigation and the profile card: MainWindow
 * hands it the help menu it already builds, so opening folders or reading the
 * logs work exactly as they did on Prism's toolbars.
 */
class KubikSidebar : public QFrame {
    Q_OBJECT

   public:
    /** The rail's pages, in MainWindow's stack order. */
    enum Page { Servers = 0, Instances = 1 };

    explicit KubikSidebar(QWidget* parent = nullptr);

    void setCurrentPage(Page page);
    void setMoreMenu(QMenu* menu);

   signals:
    void pageRequested(int page);
    void settingsRequested();
    /** The profile card's "Manage profiles…". */
    void profilesRequested();

   protected:
    void changeEvent(QEvent* event) override;

   private:
    void retranslate();

    QButtonGroup* m_pages;
    QPushButton* m_servers;
    QPushButton* m_instances;
    KubikProfileCard* m_profile;
    QPushButton* m_settings;
    QPushButton* m_more;
    QMenu* m_moreMenu = nullptr;
};
