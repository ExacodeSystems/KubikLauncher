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

#include <QLabel>
#include <QStringList>
#include <QWidget>

class QVariantAnimation;

/**
 * The name, set the way kubik.gg/launcher sets it: "Kubik" in white and "Launcher" in grass, outlined
 * and extruded like the game's own title (KubikPaint::drawWordmark). It is the app's logo, so screen
 * readers read it as one word.
 */
class KubikWordmark : public QWidget {
   public:
    explicit KubikWordmark(int pixelSize, QWidget* parent = nullptr);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

   protected:
    void paintEvent(QPaintEvent* event) override;

   private:
    int m_pixelSize;
};

/**
 * The yellow line that leans off the corner of the title, as on Minecraft's title screen and on
 * kubik.gg/launcher: one of `lines`, picked at random each time the launcher opens, pulsing gently.
 *
 * It floats: the owner places it with move() over the corner of the wordmark, outside any layout, and
 * it takes no clicks. Decoration only, so it is hidden from screen readers.
 */
class KubikSplash : public QWidget {
   public:
    explicit KubikSplash(int pixelSize, QWidget* parent = nullptr);

    void setLines(const QStringList& lines);
    QSize sizeHint() const override;

   protected:
    void paintEvent(QPaintEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

   private:
    int m_pixelSize;
    QString m_text;
    int m_pick = -1;
    qreal m_scale = 1.0;
    QVariantAnimation* m_pulse;
};

/**
 * A section's kicker, as on the website: one small shaded block, then the words, both in the label's
 * colour (grass, as #kubikEyebrow). Given a marker colour, the same block carries a status instead:
 * "Online" after a grass block, "Sleeping" after a sky one.
 */
class KubikEyebrow : public QLabel {
   public:
    explicit KubikEyebrow(QWidget* parent = nullptr);

    void setMarkerColour(const QColor& colour);

   protected:
    void paintEvent(QPaintEvent* event) override;

   private:
    QColor m_marker;
};
