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

#include "KubikFlowLayout.h"

#include <QWidget>

KubikFlowLayout::KubikFlowLayout(int horizontalSpacing, int verticalSpacing)
    : m_horizontalSpacing(horizontalSpacing), m_verticalSpacing(verticalSpacing)
{
    setContentsMargins(0, 0, 0, 0);
}

KubikFlowLayout::~KubikFlowLayout()
{
    while (auto* item = takeAt(0)) {
        delete item;
    }
}

void KubikFlowLayout::addItem(QLayoutItem* item)
{
    m_items.append(item);
}

int KubikFlowLayout::count() const
{
    return m_items.size();
}

QLayoutItem* KubikFlowLayout::itemAt(int index) const
{
    return m_items.value(index);
}

QLayoutItem* KubikFlowLayout::takeAt(int index)
{
    return index >= 0 && index < m_items.size() ? m_items.takeAt(index) : nullptr;
}

Qt::Orientations KubikFlowLayout::expandingDirections() const
{
    return {};
}

bool KubikFlowLayout::hasHeightForWidth() const
{
    return true;
}

int KubikFlowLayout::heightForWidth(int width) const
{
    return arrange(QRect(0, 0, width, 0), false);
}

QSize KubikFlowLayout::minimumSize() const
{
    QSize size;
    for (auto* item : m_items) {
        if (!item->isEmpty()) {
            size = size.expandedTo(item->minimumSize());
        }
    }
    const auto margins = contentsMargins();
    return size + QSize(margins.left() + margins.right(), margins.top() + margins.bottom());
}

QSize KubikFlowLayout::sizeHint() const
{
    // Everything on one line, as a QHBoxLayout would ask for.
    int width = 0;
    int height = 0;
    for (auto* item : m_items) {
        if (item->isEmpty()) {
            continue;
        }
        const QSize hint = item->sizeHint();
        width += (width > 0 ? m_horizontalSpacing : 0) + hint.width();
        height = qMax(height, hint.height());
    }
    const auto margins = contentsMargins();
    return QSize(width + margins.left() + margins.right(), height + margins.top() + margins.bottom());
}

void KubikFlowLayout::setGeometry(const QRect& rect)
{
    QLayout::setGeometry(rect);
    arrange(rect, true);
}

int KubikFlowLayout::arrange(const QRect& rect, bool apply) const
{
    const QRect area = rect.marginsRemoved(contentsMargins());
    int y = area.y();
    int lineHeight = 0;
    QList<QLayoutItem*> line;
    int lineWidth = 0;

    // Each line is placed once it is full, so its items can be centred on its tallest.
    const auto place = [&] {
        int x = area.x();
        for (auto* item : line) {
            const QSize hint = item->sizeHint();
            if (apply) {
                item->setGeometry(QRect(QPoint(x, y + (lineHeight - hint.height()) / 2), hint));
            }
            x += hint.width() + m_horizontalSpacing;
        }
        y += lineHeight;
    };
    for (auto* item : m_items) {
        if (item->isEmpty()) {
            continue;  // hidden
        }
        const QSize hint = item->sizeHint();
        const int needed = lineWidth + (line.isEmpty() ? 0 : m_horizontalSpacing) + hint.width();
        if (!line.isEmpty() && needed > area.width()) {
            place();
            y += m_verticalSpacing;
            line.clear();
            lineWidth = 0;
            lineHeight = 0;
        }
        lineWidth += (line.isEmpty() ? 0 : m_horizontalSpacing) + hint.width();
        lineHeight = qMax(lineHeight, hint.height());
        line.append(item);
    }
    if (!line.isEmpty()) {
        place();
    }
    return y - rect.y() + contentsMargins().bottom();
}
