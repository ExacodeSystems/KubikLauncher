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

class InstanceView;
class InstanceProxyModel;
class KubikInstanceDelegate;
class QLabel;
class QLineEdit;
class QPushButton;
class QStackedWidget;

/**
 * Instances: every instance as a large tile, to manage its mods, shaders, worlds
 * and settings, or delete it.
 *
 * There is no "New instance" here: an instance is made the first time a server
 * is played, and servers that need the same client share it (KubikProfiles.h).
 * The grid is Prism's InstanceView, kept so that groups, renaming, the context
 * menu and the keyboard all keep working; this page gives it large tiles
 * (KubikInstanceDelegate), a header with search and sort, and a real empty state.
 */
class KubikHomePage : public QWidget {
    Q_OBJECT

   public:
    KubikHomePage(InstanceView* view, InstanceProxyModel* proxy, QWidget* parent = nullptr);

    KubikInstanceDelegate* delegate() const { return m_delegate; }

   signals:
    /** The empty state's way back to the servers, where instances come from. */
    void serversRequested();
    void instanceOpened(const QString& instanceId);
    void playRequested(const QString& instanceId);

   protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void changeEvent(QEvent* event) override;

   private:
    void retranslate();
    void updateState();
    void watchRunningInstances();
    void repaintTiles();

    InstanceView* m_view;
    InstanceProxyModel* m_proxy;
    KubikInstanceDelegate* m_delegate;

    QLabel* m_title;
    QLabel* m_subtitle;
    QLineEdit* m_search;
    QPushButton* m_sort;

    QStackedWidget* m_body;
    QLabel* m_emptyTitle;
    QLabel* m_emptyBody;
    QPushButton* m_emptyServers;
    QLabel* m_noMatches;

    QString m_pressedPlayId;
};
