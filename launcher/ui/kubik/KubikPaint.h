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

#include <QColor>
#include <QIcon>
#include <QPainterPath>
#include <QRectF>
#include <QSize>

class QPainter;

/**
 * kubik.gg's blocks and its daylight, for everything KubikLauncher paints by hand.
 *
 * The website (apps/marketing in the kubik.gg repo) is built from a handful of pieces, and kubik.qss
 * draws them on widgets. Tiles, cards and the title screen are painted, so they draw the same pieces
 * from here:
 *
 *   panel    a block: 3px corners, a 1px border lit along the top and left and shaded along the
 *            bottom and right, standing on a solid 4px lip. Dark at rest; a lip in grass means
 *            "this one is lit" (hovered, chosen), the job a glow does elsewhere.
 *   button   a block you press: lit along its top, sunk 3px into its lip while held.
 *   slot     an inventory slot's recessed frame, laid over a picture.
 *   tag      a chip with each corner pixel knocked out, as the game draws its tooltips.
 *   marker   the small shaded square that leads an eyebrow and carries a status's colour.
 *
 * and the world the title screen stands in: the noon sky in hard bands, the website's pixel sprites,
 * and the ground (turf, speckled dirt, a stepped underside).
 *
 * Nothing blurs. Every shadow is a solid offset, as on the website.
 */
namespace KubikPaint {

// ── Blocks ──────────────────────────────────────────────────────────────────

/** `rect` with each corner's outermost `notch` px cut away. */
QPainterPath pixelCorners(const QRectF& rect, qreal notch = 2);

struct Panel {
    QColor fill;
    QColor border;
    QColor lip;
    int lipHeight = 4;
    qreal radius = 3;
};

/** A block at rest: void.panel on a block border, over a near-black lip. */
Panel panel();
/** The same block lit: its border lifted to block.edge and its lip in grass's deep shade. */
Panel litPanel();

/** The panel's body (the lip is not part of it), for clipping what sits inside. */
QPainterPath panelShape(const QRectF& body, const Panel& style);
/** A panel whose body fills `body`. The lip hangs below it, `style.lipHeight` px under body.bottom(). */
void drawPanel(QPainter* painter, const QRectF& body, const Panel& style);

/** Grass's button: grass.mid, or grass under the pointer, on its own deep lip. */
QColor primaryFill(bool hovered);
QColor primaryLip();

/** A 3D button filling `rect`, lip included. Its words go in buttonFace(). */
void drawButton(QPainter* painter, const QRectF& rect, const QColor& fill, const QColor& lip, bool down = false);
QRectF buttonFace(const QRectF& rect, bool down = false);

/** An inventory slot's 2px recessed frame, laid over whatever fills `rect`. */
void drawSlotFrame(QPainter* painter, const QRectF& rect);
void drawTag(QPainter* painter, const QRectF& rect, const QColor& fill, const QColor& border);
void drawMarker(QPainter* painter, const QRectF& square, const QColor& colour);

// ── The world ───────────────────────────────────────────────────────────────

/** The noon sky, ten hard bands from the zenith's blue to the horizon's. */
void drawDaySky(QPainter* painter, const QRectF& rect);

enum class Sprite { Sun, Cloud, Oak, Pig };
/** A sprite's size in its own pixels. */
QSize spriteSize(Sprite sprite);
/** Draws `sprite` with each of its pixels `pixel` device-independent px square. */
void drawSprite(QPainter* painter, Sprite sprite, const QPointF& topLeft, int pixel, qreal opacity = 1.0);

/** Turf over its shaded side, then speckled dirt; the stepped underside hangs below `rect`. */
void drawGround(QPainter* painter, const QRectF& rect, bool steppedUnderside = true);
/** How tall the turf and its shaded side are: things that stand on the ground stand on rect.top(). */
constexpr int TURF_HEIGHT = 12;

/**
 * A run of terrain steps in `band` (16 px tall at scale 1): the website's `.pixel-edge`. Rising steps
 * stand on band.bottom(); hanging ones (`hang`) drop from band.top(). `offset` shifts the profile so
 * two edges near each other differ.
 */
void drawSteppedEdge(QPainter* painter, const QRectF& band, const QColor& colour, qreal scale = 1, bool hang = false, qreal offset = 0);

// ── The name ────────────────────────────────────────────────────────────────

/**
 * "KubikLauncher" as the website sets it on /launcher: Chakra Petch Bold, "Kubik" in white and
 * "Launcher" in grass, each letter outlined in near-black and extruded four steps down and right.
 */
QSizeF wordmarkSize(qreal pixelSize);
void drawWordmark(QPainter* painter, const QPointF& topLeft, qreal pixelSize);

/** The app's icon over its hard 3px drop shadow, the way the lockup sets it beside the wordmark. */
void drawLockupIcon(QPainter* painter, const QIcon& icon, const QRectF& rect);

}  // namespace KubikPaint
