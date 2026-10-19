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

#include <QAbstractListModel>
#include <QHash>
#include <QImage>
#include <QListView>
#include <QPersistentModelIndex>
#include <QStyledItemDelegate>

#include "kubik/KubikModels.h"

/**
 * Home's grid of the player's kubik.gg servers: the model, a list view that
 * stretches its tiles to fill each row (as Prism's InstanceView does for
 * instances), and the tile itself, drawn in the same language as an instance
 * tile so the two pages read as one product.
 */

/** What a status ping of a running server said: who is on, and its icon. Unknown is -1 / null. */
struct KubikServerLive {
    int online = -1;
    int max = -1;
    QImage favicon;
    /** Its message of the day, plain text, up to two lines. */
    QString motd;
};

class KubikServerModel : public QAbstractListModel {
    Q_OBJECT

   public:
    /** What the search box matches: name, address, loader and version. */
    static constexpr int SearchRole = Qt::UserRole + 1;
    static constexpr int IdRole = Qt::UserRole + 2;
    /** A `const KubikServerLive*`, or null when the server has not answered a ping. */
    static constexpr int LiveRole = Qt::UserRole + 3;

    using QAbstractListModel::QAbstractListModel;

    void setServers(QList<Kubik::Server> servers);
    const QList<Kubik::Server>& servers() const { return m_servers; }
    const Kubik::Server* server(const QModelIndex& index) const;
    const Kubik::Server* serverById(const QString& id) const;
    void setLive(const QString& id, const KubikServerLive& live);
    const KubikServerLive* live(const QString& id) const;

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;

   private:
    QList<Kubik::Server> m_servers;
    QHash<QString, KubikServerLive> m_live;
};

class KubikServerGrid : public QListView {
    Q_OBJECT

   public:
    explicit KubikServerGrid(QWidget* parent = nullptr);
    /** The width every tile gets: at least the minimum, stretched so a row is filled. */
    int tileWidth() const { return m_tileWidth; }
    /** True while the keyboard (Tab) put the focus here: only then does the focused tile show a ring. */
    bool keyboardFocus() const { return m_keyboardFocus && hasFocus(); }

   protected:
    void resizeEvent(QResizeEvent* event) override;
    void focusInEvent(QFocusEvent* event) override;

   private:
    void relayout();
    int m_tileWidth = 0;
    bool m_keyboardFocus = false;
};

class KubikServerDelegate : public QStyledItemDelegate {
    Q_OBJECT

   public:
    explicit KubikServerDelegate(QObject* parent = nullptr);

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

    void setHoveredIndex(const QModelIndex& index) { m_hovered = index; }
    static QRect playRect(const QRect& tile);
    static int tileHeight(int width);

    /** "Online", "Sleeping", …: the status as the player reads it, and its colour. */
    static QString statusText(const Kubik::Server& server);
    static QColor statusColour(const Kubik::Server& server);
    /** "Fabric 1.21.1 · 12 mods", or the modpack's name, or why it cannot be played here. */
    static QString describe(const Kubik::Server& server);
    /** The status with what a ping adds: "Online · 3/20". */
    static QString statusLine(const Kubik::Server& server, const KubikServerLive* live);
    /**
     * A server's picture: its own icon when a ping brought one, otherwise its loader's icon on a ground
     * tinted per server, so a row of Fabric servers is not a row of identical tiles.
     */
    static void paintArt(QPainter* painter, const QRect& art, const Kubik::Server& server, const KubikServerLive* live, bool lit);

   private:
    QPersistentModelIndex m_hovered;
};

Q_DECLARE_METATYPE(const Kubik::Server*)
Q_DECLARE_METATYPE(const KubikServerLive*)
