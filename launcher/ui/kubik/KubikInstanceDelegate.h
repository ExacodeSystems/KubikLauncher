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

#include <QPersistentModelIndex>

#include "ui/instanceview/InstanceDelegate.h"

/**
 * A home-page tile: a large icon on its own field, the name, what the instance
 * is ("Minecraft 1.21.1 · Fabric · 12 mods") and when it was last played, with a
 * Play button that appears under the pointer (Stop while it runs).
 *
 * It subclasses Prism's ListViewDelegate only for renaming in place: the editor
 * and its textChanged signal are what MainWindow's rename handling listens to.
 * InstanceView does its own mouse handling, so this delegate never sees a click;
 * KubikHomePage watches the pointer and tells it which tile is hovered.
 */
class KubikInstanceDelegate : public ListViewDelegate {
    Q_OBJECT

   public:
    explicit KubikInstanceDelegate(QObject* parent = nullptr);

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

    void setHoveredIndex(const QModelIndex& index) { m_hovered = index; }

    /** The Play (or Stop) button's place inside a tile. */
    static QRect playRect(const QRect& tile);

   private:
    QPersistentModelIndex m_hovered;
};
