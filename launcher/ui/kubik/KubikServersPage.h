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

#include <QElapsedTimer>
#include <QSet>
#include <QWidget>

class KubikServerDelegate;
class KubikServerGrid;
class KubikServerModel;
class QFrame;
class QLabel;
class QLineEdit;
class QPushButton;
class QSortFilterProxyModel;
class QStackedWidget;
class QTimer;

namespace Kubik {
struct Server;
}

/**
 * Home: the player's kubik.gg servers as large tiles. A tile opens the server's
 * page; Play on a tile goes straight there and starts playing. Above the grid,
 * "Jump back in" holds the server played last, with its own Play.
 *
 * Servers that are up come first, then the ones played most recently. Running
 * servers are pinged after every refresh, which is what puts "3/20" and the
 * server's own icon on its tile.
 *
 * The list is fetched when the page is shown (if it is more than half a minute
 * old) and every minute while it stays on screen, so a server that wakes or
 * falls asleep shows it without a refresh button being pressed.
 */
class KubikServersPage : public QWidget {
    Q_OBJECT

   public:
    explicit KubikServersPage(QWidget* parent = nullptr);

    /** The server as last fetched, or nullptr. */
    const Kubik::Server* server(const QString& id) const;
    void refresh();

   signals:
    void serverOpened(const QString& serverId);
    void playRequested(const QString& serverId);

   protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void changeEvent(QEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

   private:
    void retranslate();
    void updateState();
    void updateHero();
    void pingServers();
    void showMenu(const QPoint& position);

    KubikServerModel* m_model;
    QSortFilterProxyModel* m_proxy;
    KubikServerGrid* m_grid;
    KubikServerDelegate* m_delegate;

    QLabel* m_title;
    QLabel* m_subtitle;
    QLineEdit* m_search;
    QPushButton* m_refresh;

    QStackedWidget* m_body;
    QLabel* m_loading;
    QLabel* m_emptyTitle;
    QLabel* m_emptyBody;
    QPushButton* m_emptyCreate;
    QLabel* m_errorText;
    QPushButton* m_retry;
    QLabel* m_noMatches;

    QLabel* m_stale;
    QPushButton* m_staleRetry;

    QFrame* m_hero;
    QLabel* m_heroArt;
    QLabel* m_heroEyebrow;
    QLabel* m_heroName;
    QLabel* m_heroLine;
    QLabel* m_heroStatus;
    QPushButton* m_heroPlay;
    QPushButton* m_heroOpen;
    QString m_heroId;
    QLabel* m_intro;
    QLabel* m_allTitle;
    QSet<QString> m_pinging;

    bool m_loaded = false;
    bool m_fetching = false;
    QString m_error;
    QElapsedTimer m_fetchedAt;
    QTimer* m_autoRefresh;
    QString m_pressedPlayId;
};
