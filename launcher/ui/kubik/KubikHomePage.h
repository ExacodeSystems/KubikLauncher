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
 * Home: every instance as a large tile, and the one obvious way to add another.
 *
 * The grid is Prism's InstanceView, kept so that groups, drag-and-drop, renaming,
 * the context menu and the keyboard all keep working; this page gives it large
 * tiles (KubikInstanceDelegate), a header with search, sort and "New instance",
 * and a real empty state in place of the grid when there is nothing to show.
 */
class KubikHomePage : public QWidget {
    Q_OBJECT

   public:
    KubikHomePage(InstanceView* view, InstanceProxyModel* proxy, QWidget* parent = nullptr);

    KubikInstanceDelegate* delegate() const { return m_delegate; }

   signals:
    /** "New instance", optionally straight to one of its pages ("vanilla", "modrinth", "import"). */
    void newInstanceRequested(const QString& page);
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
    QPushButton* m_newInstance;

    QStackedWidget* m_body;
    QLabel* m_emptyTitle;
    QLabel* m_emptyBody;
    QPushButton* m_emptyFresh;
    QPushButton* m_emptyModpack;
    QLabel* m_noMatches;

    QString m_pressedPlayId;
};
