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

#include "KubikPaint.h"

#include <QFont>
#include <QFontMetricsF>
#include <QHash>
#include <QPainter>
#include <QPainterPathStroker>
#include <QtMath>

#include <cmath>
#include <iterator>

#include "kubik/KubikBrand.h"

using KubikBrand::color;

namespace {

// The lip under a block in grass: the website's `drop-grass` and its primary button.
const QColor GRASS_LIP(0x04, 0x6a, 0x4d);
// Near-black, the wordmark's outline and extrusion.
const QColor INK(0x09, 0x0b, 0x0c);

struct SpriteData {
    QList<const char*> rows;
    QHash<char, QColor> palette;
};

// The website's pixel art (apps/marketing/src/components/pixel/props.tsx), row for row.
const SpriteData& spriteData(KubikPaint::Sprite sprite)
{
    static const SpriteData sun{
        { ".oooooo.", "oooooooo", "ooOOOOoo", "oOOOOOOo", "oOOOOOOo", "ooOOOOoo", "oooooooo", ".oooooo." },
        { { 'o', QColor("#FFC24B") }, { 'O', QColor("#FFE9A6") } },
    };
    static const SpriteData cloud{
        { "....CCCCCC......", "..CCCCCCCCCC....", ".CCCCCCCCCCCCC..", "CCCCCCCCCCCCCCCC", ".CCCCCCCCCCCCCC.", "...CCCCCCCCC...." },
        { { 'C', QColor("#F3F7FB") } },
    };
    static const SpriteData oak{
        { "....GGGGG....", "..GGGGGGGGG..", ".GGGGGGGGGGG.", "GGGGGGGGGGGGG", "GGaGGGGGGGaGG", "GGGGGGGGGGGGG", "GGGGGGGGGGGGG",
          ".GGGGGGGGGGG.", "..GGGGGGGGG..", ".....ttk.....", ".....ttk.....", ".....ttk.....", ".....ttk....." },
        { { 'G', QColor("#61B23E") }, { 'a', QColor("#4A8C30") }, { 't', QColor("#7C5330") }, { 'k', QColor("#573A21") } },
    };
    static const SpriteData pig{
        { ".rr......rr.", ".rr......rr.", "rPPPPPPPPPPr", "rPPPPPPPPPPr", "rPeePPPPeePr", "rPPPPPPPPPPr", "rPPnnnnnnPPr", "rPPnFFFFnPPr",
          "rPPnnnnnnPPr", "rPPPPPPPPPPr", ".rPPPPPPPPr." },
        { { 'P', QColor("#F0A6B1") },
          { 'r', QColor("#D6838F") },
          { 'e', QColor("#2C1E21") },
          { 'n', QColor("#E68E9B") },
          { 'F', QColor("#2C1E21") } },
    };
    switch (sprite) {
        case KubikPaint::Sprite::Sun:
            return sun;
        case KubikPaint::Sprite::Cloud:
            return cloud;
        case KubikPaint::Sprite::Oak:
            return oak;
        case KubikPaint::Sprite::Pig:
            return pig;
    }
    return cloud;
}

// The terrain profile of `.pixel-edge` (a 384 x 16 mask): where along it the solid starts, from the
// top. 0 is a full-height step, 8 the half height every run sits at, 16 a gap down to the band below.
struct Step {
    int from;
    int to;
    int top;
};
constexpr Step EDGE[] = {
    { 0, 48, 8 },    { 48, 80, 0 },    { 80, 96, 8 },   { 96, 128, 16 }, { 128, 192, 8 }, { 192, 208, 0 },
    { 208, 240, 8 }, { 240, 288, 16 }, { 288, 304, 8 }, { 304, 352, 0 }, { 352, 384, 8 },
};
constexpr int EDGE_WIDTH = 384;
constexpr int EDGE_HEIGHT = 16;

// `.dirt`: a 32 px tile of 4 px texels in three earth tones, over three darkening strata.
struct Texel {
    int x;
    int y;
};
constexpr Texel DIRT_LIGHT[] = { { 4, 8 }, { 12, 28 }, { 8, 12 }, { 20, 20 }, { 0, 4 }, { 8, 24 }, { 24, 20 }, { 12, 20 } };
constexpr Texel DIRT_DARK[] = { { 8, 16 }, { 24, 8 }, { 28, 0 }, { 0, 0 }, { 8, 0 }, { 4, 12 }, { 20, 8 } };
constexpr Texel DIRT_MID[] = { { 4, 16 }, { 24, 28 }, { 20, 12 }, { 8, 20 }, { 4, 0 }, { 24, 24 }, { 12, 0 } };

QFont wordmarkFont(qreal pixelSize)
{
    QFont font(KubikBrand::displayFamily());
    font.setPixelSize(qMax(1, qRound(pixelSize)));
    font.setWeight(QFont::Bold);
    // tracking-tight, as on the website.
    font.setLetterSpacing(QFont::AbsoluteSpacing, -0.025 * pixelSize);
    return font;
}

}  // namespace

namespace KubikPaint {

QPainterPath pixelCorners(const QRectF& r, qreal n)
{
    QPolygonF polygon;
    polygon << QPointF(r.left(), r.top() + n) << QPointF(r.left() + n, r.top() + n) << QPointF(r.left() + n, r.top())
            << QPointF(r.right() - n, r.top()) << QPointF(r.right() - n, r.top() + n) << QPointF(r.right(), r.top() + n)
            << QPointF(r.right(), r.bottom() - n) << QPointF(r.right() - n, r.bottom() - n) << QPointF(r.right() - n, r.bottom())
            << QPointF(r.left() + n, r.bottom()) << QPointF(r.left() + n, r.bottom() - n) << QPointF(r.left(), r.bottom() - n);
    QPainterPath path;
    path.addPolygon(polygon);
    path.closeSubpath();
    return path;
}

Panel panel()
{
    return { color("void.panel"), color("block"), color("block.lip") };
}

Panel litPanel()
{
    return { color("void.panel"), color("block.edge"), GRASS_LIP };
}

QPainterPath panelShape(const QRectF& body, const Panel& style)
{
    QPainterPath path;
    path.addRoundedRect(body, style.radius, style.radius);
    return path;
}

void drawPanel(QPainter* painter, const QRectF& body, const Panel& style)
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    QPainterPath lip;
    lip.addRoundedRect(body.translated(0, style.lipHeight), style.radius, style.radius);
    painter->fillPath(lip, style.lip);
    const QPainterPath shape = panelShape(body, style);
    painter->fillPath(shape, style.fill);

    // The bevel, just inside the border: lit from the top left, shaded to the bottom right.
    painter->save();
    painter->setClipPath(shape);
    painter->setPen(QPen(QColor(255, 255, 255, 15), 1));
    painter->drawLine(QPointF(body.left() + 1, body.top() + 1.5), QPointF(body.right() - 1, body.top() + 1.5));
    painter->drawLine(QPointF(body.left() + 1.5, body.top() + 1), QPointF(body.left() + 1.5, body.bottom() - 1));
    painter->setPen(QPen(QColor(0, 0, 0, 140), 1));
    painter->drawLine(QPointF(body.left() + 1, body.bottom() - 1.5), QPointF(body.right() - 1, body.bottom() - 1.5));
    painter->drawLine(QPointF(body.right() - 1.5, body.top() + 1), QPointF(body.right() - 1.5, body.bottom() - 1));
    painter->restore();

    painter->setPen(QPen(style.border, 1));
    painter->setBrush(Qt::NoBrush);
    painter->drawRoundedRect(body.adjusted(0.5, 0.5, -0.5, -0.5), style.radius - 0.5, style.radius - 0.5);
    painter->restore();
}

QColor primaryFill(bool hovered)
{
    return color(hovered ? "grass" : "grass.mid");
}

QColor primaryLip()
{
    return GRASS_LIP;
}

QRectF buttonFace(const QRectF& rect, bool down)
{
    return QRectF(rect.left(), rect.top() + (down ? 3 : 0), rect.width(), rect.height() - 4);
}

void drawButton(QPainter* painter, const QRectF& rect, const QColor& fill, const QColor& lip, bool down)
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    const QRectF face = buttonFace(rect, down);
    QPainterPath under;
    under.addRoundedRect(face.translated(0, down ? 1 : 4), 2, 2);
    painter->fillPath(under, lip);
    QPainterPath shape;
    shape.addRoundedRect(face, 2, 2);
    painter->fillPath(shape, fill);
    // `.btn-3d`: a lit line along the top, a little shade along the bottom.
    painter->setClipPath(shape);
    painter->fillRect(QRectF(face.left(), face.top(), face.width(), 1), QColor(255, 255, 255, 56));
    painter->fillRect(QRectF(face.left(), face.bottom() - 2, face.width(), 2), QColor(0, 0, 0, 31));
    painter->restore();
}

void drawSlotFrame(QPainter* painter, const QRectF& rect)
{
    // `.inv-slot`: a 2px border in block, the inside shaded along its top and left as if pressed in.
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    QPainterPath outer;
    outer.addRoundedRect(rect, 2, 2);
    QPainterPath inner;
    const QRectF in = rect.adjusted(2, 2, -2, -2);
    inner.addRect(in);
    painter->fillPath(outer.subtracted(inner), color("block"));
    painter->fillRect(QRectF(in.left(), in.top(), in.width(), 2), QColor(0, 0, 0, 153));
    painter->fillRect(QRectF(in.left(), in.top() + 2, 2, in.height() - 2), QColor(0, 0, 0, 153));
    painter->fillRect(QRectF(in.left() + 2, in.bottom() - 2, in.width() - 2, 2), QColor(255, 255, 255, 10));
    painter->fillRect(QRectF(in.right() - 2, in.top() + 2, 2, in.height() - 4), QColor(255, 255, 255, 10));
    painter->restore();
}

void drawTag(QPainter* painter, const QRectF& rect, const QColor& fill, const QColor& border)
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, false);
    painter->fillPath(pixelCorners(rect, 2), border);
    painter->fillPath(pixelCorners(rect.adjusted(1, 1, -1, -1), 1), fill);
    painter->restore();
}

void drawMarker(QPainter* painter, const QRectF& square, const QColor& colour)
{
    painter->save();
    painter->fillRect(square, colour);
    // `.eyebrow::before`: shaded 2px along the bottom and right, like the side of a block.
    painter->fillRect(QRectF(square.left(), square.bottom() - 2, square.width(), 2), QColor(0, 0, 0, 82));
    painter->fillRect(QRectF(square.right() - 2, square.top(), 2, square.height() - 2), QColor(0, 0, 0, 82));
    painter->restore();
}

void drawDaySky(QPainter* painter, const QRectF& rect)
{
    // `.sky-day`: where each band starts, top to bottom.
    static const QList<QPair<qreal, QColor>> bands = {
        { 0.00, QColor("#6f9df8") }, { 0.18, QColor("#77a4f9") }, { 0.31, QColor("#80abfa") }, { 0.43, QColor("#89b2fa") },
        { 0.54, QColor("#93b9fb") }, { 0.64, QColor("#9dc0fc") }, { 0.73, QColor("#a8c8fc") }, { 0.81, QColor("#b3d0fd") },
        { 0.88, QColor("#bed7fd") }, { 0.94, QColor("#c9defe") },
    };
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, false);
    for (int i = 0; i < bands.size(); ++i) {
        // Whole pixels, so no band ever shows a seam of the one under it.
        const qreal top = qRound(rect.top() + rect.height() * bands[i].first);
        const qreal bottom = i + 1 < bands.size() ? qRound(rect.top() + rect.height() * bands[i + 1].first) : rect.bottom();
        painter->fillRect(QRectF(rect.left(), top, rect.width(), bottom - top), bands[i].second);
    }
    painter->restore();
}

QSize spriteSize(Sprite sprite)
{
    const auto& data = spriteData(sprite);
    return QSize(static_cast<int>(qstrlen(data.rows.first())), static_cast<int>(data.rows.size()));
}

void drawSprite(QPainter* painter, Sprite sprite, const QPointF& topLeft, int pixel, qreal opacity)
{
    const auto& data = spriteData(sprite);
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, false);
    painter->setOpacity(painter->opacity() * opacity);
    for (int row = 0; row < data.rows.size(); ++row) {
        const char* line = data.rows[row];
        for (int column = 0; line[column]; ++column) {
            const auto it = data.palette.constFind(line[column]);
            if (it != data.palette.constEnd()) {
                painter->fillRect(QRectF(topLeft.x() + column * pixel, topLeft.y() + row * pixel, pixel, pixel), it.value());
            }
        }
    }
    painter->restore();
}

void drawSteppedEdge(QPainter* painter, const QRectF& band, const QColor& colour, qreal scale, bool hang, qreal offset)
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, false);
    painter->setClipRect(band);
    const qreal period = EDGE_WIDTH * scale;
    // The repeat that covers band.left(), with the profile's start shifted right by `offset`.
    qreal start = band.left() + std::fmod(offset, period);
    while (start > band.left()) {
        start -= period;
    }
    for (qreal x = start; x < band.right(); x += period) {
        for (const auto& step : EDGE) {
            const qreal solid = (EDGE_HEIGHT - step.top) * scale;
            if (solid <= 0) {
                continue;
            }
            const qreal left = x + step.from * scale;
            const qreal width = (step.to - step.from) * scale;
            const qreal top = hang ? band.top() : band.top() + step.top * scale;
            painter->fillRect(QRectF(left, top, width, solid), colour);
        }
    }
    painter->restore();
}

void drawGround(QPainter* painter, const QRectF& rect, bool steppedUnderside)
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, false);
    // `<Ground>`: 8px of turf, 4px of its shaded side, then dirt.
    painter->fillRect(QRectF(rect.left(), rect.top(), rect.width(), 8), color("grass.top"));
    painter->fillRect(QRectF(rect.left(), rect.top() + 8, rect.width(), 4), color("grass.side"));
    const QRectF dirt(rect.left(), rect.top() + TURF_HEIGHT, rect.width(), rect.height() - TURF_HEIGHT);
    if (dirt.height() > 0) {
        const qreal third = dirt.height() / 3;
        painter->fillRect(QRectF(dirt.left(), dirt.top(), dirt.width(), qCeil(third)), QColor("#5e3c1e"));
        painter->fillRect(QRectF(dirt.left(), dirt.top() + qFloor(third), dirt.width(), qCeil(third)), QColor("#533419"));
        painter->fillRect(QRectF(dirt.left(), dirt.top() + qFloor(2 * third), dirt.width(), dirt.height() - qFloor(2 * third)),
                          QColor("#472c15"));
        painter->save();
        painter->setClipRect(dirt);
        const QList<QPair<const Texel*, QColor>> texels = {
            { DIRT_LIGHT, QColor("#6E4726") },
            { DIRT_DARK, QColor("#3F2815") },
            { DIRT_MID, QColor("#563820") },
        };
        const int counts[] = { std::size(DIRT_LIGHT), std::size(DIRT_DARK), std::size(DIRT_MID) };
        for (qreal ty = dirt.top(); ty < dirt.bottom(); ty += 32) {
            for (qreal tx = dirt.left(); tx < dirt.right(); tx += 32) {
                for (int set = 0; set < texels.size(); ++set) {
                    for (int i = 0; i < counts[set]; ++i) {
                        const Texel& t = texels[set].first[i];
                        painter->fillRect(QRectF(tx + t.x, ty + t.y, 4, 4), texels[set].second);
                    }
                }
            }
        }
        painter->restore();
        // The turf's shaded side hanging over the dirt's lip, at half scale.
        drawSteppedEdge(painter, QRectF(dirt.left(), dirt.top(), dirt.width(), EDGE_HEIGHT / 2), color("grass.side"), 0.5, true);
    }
    if (steppedUnderside) {
        drawSteppedEdge(painter, QRectF(rect.left(), rect.bottom(), rect.width(), EDGE_HEIGHT), QColor("#472c15"), 1, true, 176);
    }
    painter->restore();
}

QSizeF wordmarkSize(qreal pixelSize)
{
    const QFontMetricsF metrics(wordmarkFont(pixelSize));
    const qreal outline = 0.09 * pixelSize;
    const qreal depth = 0.12 * pixelSize;
    return QSizeF(metrics.horizontalAdvance(QStringLiteral("KubikLauncher")) + 2 * outline + depth,
                  metrics.ascent() + metrics.descent() + 2 * outline + depth);
}

void drawWordmark(QPainter* painter, const QPointF& topLeft, qreal pixelSize)
{
    const QFont font = wordmarkFont(pixelSize);
    const QFontMetricsF metrics(font);
    const qreal outline = 0.09 * pixelSize;
    const QPointF baseline = topLeft + QPointF(outline, outline + metrics.ascent());

    QPainterPath kubik;
    kubik.addText(baseline, font, QStringLiteral("Kubik"));
    QPainterPath launcher;
    launcher.addText(baseline + QPointF(metrics.horizontalAdvance(QStringLiteral("Kubik")), 0), font, QStringLiteral("Launcher"));
    QPainterPath letters = kubik;
    letters.addPath(launcher);

    // `.wordmark-3d`: an outline 0.09em wide around every letter (drawn under the fill, so it only shows
    // outside it), and four copies of the outlined letters stepping 0.03em down and right.
    QPainterPathStroker stroker;
    stroker.setWidth(2 * outline);
    stroker.setJoinStyle(Qt::MiterJoin);
    stroker.setMiterLimit(2);
    const QPainterPath stroke = stroker.createStroke(letters);

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    for (int step = 4; step >= 0; --step) {
        const QPointF shift(0.03 * pixelSize * step, 0.03 * pixelSize * step);
        painter->fillPath(stroke.translated(shift), INK);
        painter->fillPath(letters.translated(shift), INK);
    }
    painter->fillPath(kubik, Qt::white);
    painter->fillPath(launcher, color("grass"));
    painter->restore();
}

void drawLockupIcon(QPainter* painter, const QIcon& icon, const QRectF& rect)
{
    // `drop-shadow-[3px_3px_0_#090b0c]`: the icon's own shape, in ink, 3px down and right.
    const qreal dpr = painter->device()->devicePixelRatioF();
    const QPixmap logo = icon.pixmap(rect.size().toSize(), dpr);
    QPixmap shadow(logo.size());
    shadow.setDevicePixelRatio(logo.devicePixelRatio());
    shadow.fill(Qt::transparent);
    {
        QPainter s(&shadow);
        s.drawPixmap(0, 0, logo);
        s.setCompositionMode(QPainter::CompositionMode_SourceIn);
        s.fillRect(shadow.rect(), INK);
    }
    painter->drawPixmap(rect.topLeft() + QPointF(3, 3), shadow);
    painter->drawPixmap(rect.topLeft(), logo);
}

}  // namespace KubikPaint
