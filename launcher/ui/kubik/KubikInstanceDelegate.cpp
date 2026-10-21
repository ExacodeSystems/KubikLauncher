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

#include "KubikInstanceDelegate.h"

#include <QFontMetrics>
#include <QIcon>
#include <QPainter>

#include "BaseInstance.h"
#include "InstanceList.h"
#include "kubik/KubikBrand.h"
#include "ui/instanceview/InstanceView.h"
#include "ui/kubik/KubikInstanceInfo.h"
#include "ui/kubik/KubikPaint.h"

using KubikBrand::color;

namespace {

// The tile's measurements. The art field is half as tall as the tile is wide, so tiles keep their
// shape as the grid stretches them; everything under it is a fixed-height text block, and under that
// the lip the block stands on.
constexpr int LIP = 4;
constexpr int PAD = 14;
constexpr int NAME_HEIGHT = 22;
constexpr int LINE_HEIGHT = 18;
constexpr int TEXT_BLOCK = 12 + NAME_HEIGHT + 4 + LINE_HEIGHT + 2 + LINE_HEIGHT + PAD;
constexpr int DEFAULT_WIDTH = 220;
constexpr int PLAY_WIDTH = 76;

int artHeight(int width)
{
    return width / 2;
}

QRect artRect(const QRect& tile)
{
    // Inside the block's 1px border.
    return QRect(tile.left() + 1, tile.top() + 1, tile.width() - 2, artHeight(tile.width()));
}

QFont font(const QString& family, int pixelSize, QFont::Weight weight)
{
    QFont f(family);
    f.setPixelSize(pixelSize);
    f.setWeight(weight);
    return f;
}

/** The instance's icon at `side` px. A small pixel-art icon is enlarged without blurring. */
QPixmap iconPixmap(const QIcon& icon, int side, qreal dpr)
{
    const QSize target(qRound(side * dpr), qRound(side * dpr));
    QPixmap pixmap = icon.pixmap(QSize(side, side), dpr);
    if (!pixmap.isNull() && pixmap.width() < target.width()) {
        pixmap = pixmap.scaled(target, Qt::KeepAspectRatio, Qt::FastTransformation);
        pixmap.setDevicePixelRatio(dpr);
    }
    return pixmap;
}

}  // namespace

KubikInstanceDelegate::KubikInstanceDelegate(QObject* parent) : ListViewDelegate(parent) {}

QRect KubikInstanceDelegate::playRect(const QRect& tile)
{
    // In the text block, right-aligned and level with the name and the line under it, so it never
    // covers the icon however narrow the tile gets.
    const QRect art = artRect(tile);
    const QSize size(PLAY_WIDTH, 34);
    const int top = art.bottom() + 1 + 12 + (NAME_HEIGHT + 4 + LINE_HEIGHT - size.height()) / 2;
    return QRect(tile.right() - PAD - size.width() + 1, top, size.width(), size.height());
}

QSize KubikInstanceDelegate::sizeHint(const QStyleOptionViewItem& option, [[maybe_unused]] const QModelIndex& index) const
{
    const auto* view = qobject_cast<const InstanceView*>(option.widget);
    const int width = view ? view->itemWidth() : DEFAULT_WIDTH;
    return QSize(width, artHeight(width) + TEXT_BLOCK + LIP);
}

void KubikInstanceDelegate::updateEditorGeometry(QWidget* editor,
                                                 const QStyleOptionViewItem& option,
                                                 [[maybe_unused]] const QModelIndex& index) const
{
    // Rename edits the name where it is drawn.
    const QRect art = artRect(option.rect);
    editor->setGeometry(option.rect.left() + PAD - 4, art.bottom() + 8, option.rect.width() - 2 * PAD + 8, NAME_HEIGHT + 12);
}

void KubikInstanceDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    auto* instance = static_cast<BaseInstance*>(index.data(InstanceList::InstancePointerRole).value<void*>());
    const QRect tile = option.rect;
    const QRect art = artRect(tile);
    const bool hovered = index == m_hovered;
    const bool selected = option.state & QStyle::State_Selected;
    const bool lit = hovered || selected;
    const bool running = instance && instance->isRunning();

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    // The block, and its art field one tone below it. Lit (hovered or selected), it stands on grass's lip.
    const QRectF body(tile.left(), tile.top(), tile.width(), tile.height() - LIP);
    const auto style = lit ? KubikPaint::litPanel() : KubikPaint::panel();
    KubikPaint::drawPanel(painter, body, style);
    painter->save();
    painter->setClipPath(KubikPaint::panelShape(body.adjusted(1, 1, -1, -1), style));
    painter->fillRect(art, color(lit ? "void.panel" : "void.raised"));
    painter->restore();
    painter->fillRect(QRect(art.left(), art.bottom() + 1, art.width(), 1), color(lit ? "block.edge" : "block"));

    const int side = qMin(96, art.height() * 62 / 100);
    QRect iconRect(0, 0, side, side);
    iconRect.moveCenter(art.center());
    const QIcon icon = index.data(Qt::DecorationRole).value<QIcon>();
    painter->drawPixmap(iconRect, iconPixmap(icon, side, painter->device()->devicePixelRatioF()));

    // Play under the pointer, on the selected tile, and always while running (as Stop).
    if (lit || running) {
        const QRect play = playRect(tile);
        KubikPaint::drawButton(painter, QRectF(play), running ? color("heart") : KubikPaint::primaryFill(false),
                               running ? QColor(0x8f, 0x24, 0x24) : KubikPaint::primaryLip());
        painter->setPen(color("void"));
        painter->setFont(font(KubikBrand::displayFamily(), 14, QFont::Bold));
        painter->drawText(KubikPaint::buttonFace(QRectF(play)), Qt::AlignCenter, running ? tr("Stop") : tr("Play"));
    }

    // The words under the art. While Play shows, the two lines beside it make room for it.
    const int textLeft = tile.left() + PAD;
    const int fullWidth = tile.width() - 2 * PAD;
    const int textWidth = (lit || running) ? fullWidth - PLAY_WIDTH - 10 : fullWidth;
    int y = art.bottom() + 1 + 12;

    const QFont nameFont = font(KubikBrand::displayFamily(), 16, QFont::DemiBold);
    painter->setFont(nameFont);
    painter->setPen(color("bone"));
    const QString name = QFontMetrics(nameFont).elidedText(index.data(Qt::DisplayRole).toString(), Qt::ElideRight, textWidth);
    painter->drawText(QRect(textLeft, y, textWidth, NAME_HEIGHT), Qt::AlignLeft | Qt::AlignVCenter, name);
    y += NAME_HEIGHT + 4;

    const QFont smallFont = font(KubikBrand::bodyFamily(), 13, QFont::Normal);
    const QFontMetrics small(smallFont);
    painter->setFont(smallFont);
    painter->setPen(color("bone.muted"));
    const QString about = instance ? KubikInstanceInfo::describe(KubikInstanceInfo::instance()->summary(instance)) : QString();
    painter->drawText(QRect(textLeft, y, textWidth, LINE_HEIGHT), Qt::AlignLeft | Qt::AlignVCenter,
                      small.elidedText(about, Qt::ElideRight, textWidth));
    y += LINE_HEIGHT + 2;

    QString status;
    QColor statusColour = color("bone.faint");
    if (running) {
        status = tr("Running");
        statusColour = color("grass");
    } else if (instance && instance->hasVersionBroken()) {
        status = tr("Needs repair: open it to fix the version");
        statusColour = color("heart");
    } else if (instance && instance->hasCrashed()) {
        status = tr("Crashed last time");
        statusColour = color("heart");
    } else if (instance) {
        status = KubikInstanceInfo::lastPlayed(instance);
    }
    painter->setPen(statusColour);
    painter->drawText(QRect(textLeft, y, fullWidth, LINE_HEIGHT), Qt::AlignLeft | Qt::AlignVCenter,
                      small.elidedText(status, Qt::ElideRight, fullWidth));

    painter->restore();
}
