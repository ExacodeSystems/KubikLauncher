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

#include <QLayout>
#include <QList>

/**
 * A row of buttons that wraps onto a second line when it runs out of room,
 * left-aligned, each line centred on its tallest item.
 *
 * A QHBoxLayout of buttons at their natural width has a minimum width of all of
 * them together; in a scroll area that never scrolls sideways, a window
 * narrower than that clips the row and everything beside it. This one's minimum
 * is its widest item, and its height follows the width it is given.
 */
class KubikFlowLayout : public QLayout {
   public:
    KubikFlowLayout(int horizontalSpacing, int verticalSpacing);
    ~KubikFlowLayout() override;

    void addItem(QLayoutItem* item) override;
    int count() const override;
    QLayoutItem* itemAt(int index) const override;
    QLayoutItem* takeAt(int index) override;

    Qt::Orientations expandingDirections() const override;
    bool hasHeightForWidth() const override;
    int heightForWidth(int width) const override;
    QSize minimumSize() const override;
    QSize sizeHint() const override;
    void setGeometry(const QRect& rect) override;

   private:
    /** Lays the items out in `rect` (moving them when `apply`), and returns the height that took. */
    int arrange(const QRect& rect, bool apply) const;

    QList<QLayoutItem*> m_items;
    int m_horizontalSpacing;
    int m_verticalSpacing;
};
