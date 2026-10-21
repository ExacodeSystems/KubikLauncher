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

#include "KubikTitle.h"

#include <QEasingCurve>
#include <QFontMetricsF>
#include <QPainter>
#include <QPainterPath>
#include <QRandomGenerator>
#include <QVariantAnimation>
#include <QtMath>

#include "kubik/KubikBrand.h"
#include "ui/kubik/KubikPaint.h"

namespace {

// The game leans its splash 20 degrees; the website leans it 14, and so does this.
constexpr qreal SPLASH_ANGLE = -14;
constexpr qreal SPLASH_GROW = 1.08;
constexpr int SPLASH_PERIOD_MS = 1200;
const QColor SPLASH_YELLOW(0xff, 0xff, 0x55);
const QColor SPLASH_SHADOW(0x3f, 0x3f, 0x15);

QFont splashFont(int pixelSize)
{
    QFont font(KubikBrand::displayFamily());
    font.setPixelSize(pixelSize);
    font.setWeight(QFont::Bold);
    return font;
}

}  // namespace

// ── The wordmark ────────────────────────────────────────────────────────────

KubikWordmark::KubikWordmark(int pixelSize, QWidget* parent) : QWidget(parent), m_pixelSize(pixelSize)
{
    setAccessibleName(QStringLiteral("KubikLauncher"));
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}

QSize KubikWordmark::sizeHint() const
{
    return KubikPaint::wordmarkSize(m_pixelSize).toSize() + QSize(1, 1);
}

QSize KubikWordmark::minimumSizeHint() const
{
    return sizeHint();
}

void KubikWordmark::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(this);
    const QSizeF size = KubikPaint::wordmarkSize(m_pixelSize);
    KubikPaint::drawWordmark(&painter, QPointF(0, (height() - size.height()) / 2), m_pixelSize);
}

// ── The splash ──────────────────────────────────────────────────────────────

KubikSplash::KubikSplash(int pixelSize, QWidget* parent) : QWidget(parent), m_pixelSize(pixelSize), m_pulse(new QVariantAnimation(this))
{
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_NoSystemBackground);
    setFocusPolicy(Qt::NoFocus);
    // `motion-safe:animate-splash`: 1 to 1.08 and back, over and over. Qt has no reduced-motion setting
    // to read, and the pulse moves no layout: it scales the line about its own centre.
    m_pulse->setStartValue(1.0);
    m_pulse->setKeyValueAt(0.5, SPLASH_GROW);
    m_pulse->setEndValue(1.0);
    m_pulse->setDuration(SPLASH_PERIOD_MS);
    m_pulse->setEasingCurve(QEasingCurve::InOutSine);
    m_pulse->setLoopCount(-1);
    connect(m_pulse, &QVariantAnimation::valueChanged, this, [this](const QVariant& value) {
        m_scale = value.toReal();
        update();
    });
}

// The pulse runs only while the line can be seen: signed in, the page it sits on is hidden for good.
void KubikSplash::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    m_pulse->start();
}

void KubikSplash::hideEvent(QHideEvent* event)
{
    m_pulse->stop();
    QWidget::hideEvent(event);
}

void KubikSplash::setLines(const QStringList& lines)
{
    if (lines.isEmpty()) {
        m_text.clear();
    } else {
        // Picked once per run, then kept across a language switch: the same line, in the new language.
        if (m_pick < 0 || m_pick >= lines.size()) {
            m_pick = static_cast<int>(QRandomGenerator::global()->bounded(lines.size()));
        }
        m_text = lines.at(m_pick);
    }
    resize(sizeHint());
    update();
}

QSize KubikSplash::sizeHint() const
{
    // The rotated line at its largest, with its drop shadow: the box it can never paint outside.
    const QFontMetricsF metrics(splashFont(m_pixelSize));
    const qreal w = (metrics.horizontalAdvance(m_text) + 0.1 * m_pixelSize) * SPLASH_GROW;
    const qreal h = (metrics.height() + 0.1 * m_pixelSize) * SPLASH_GROW;
    const qreal a = qDegreesToRadians(qAbs(SPLASH_ANGLE));
    return QSize(qCeil(w * qCos(a) + h * qSin(a)) + 2, qCeil(w * qSin(a) + h * qCos(a)) + 2);
}

void KubikSplash::paintEvent(QPaintEvent* /*event*/)
{
    if (m_text.isEmpty()) {
        return;
    }
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);
    const QFont font = splashFont(m_pixelSize);
    const QFontMetricsF metrics(font);
    painter.translate(width() / 2.0, height() / 2.0);
    painter.rotate(SPLASH_ANGLE);
    painter.scale(m_scale, m_scale);
    QPainterPath line;
    line.addText(QPointF(-metrics.horizontalAdvance(m_text) / 2, metrics.ascent() - metrics.height() / 2), font, m_text);
    // `.splash-text`: the game's yellow over its hard drop, 0.1em down and right.
    const qreal drop = 0.1 * m_pixelSize;
    painter.fillPath(line.translated(drop, drop), SPLASH_SHADOW);
    painter.fillPath(line, SPLASH_YELLOW);
}

// ── The eyebrow ─────────────────────────────────────────────────────────────

KubikEyebrow::KubikEyebrow(QWidget* parent) : QLabel(parent)
{
    setObjectName("kubikEyebrow");
    // Room for the block and the gap after it; the words are drawn by QLabel after that.
    setIndent(16);
}

void KubikEyebrow::setMarkerColour(const QColor& colour)
{
    m_marker = colour;
    update();
}

void KubikEyebrow::paintEvent(QPaintEvent* event)
{
    QLabel::paintEvent(event);
    if (text().isEmpty()) {
        return;
    }
    QPainter painter(this);
    const QRect area = contentsRect();
    // Level with the middle of the words' capitals, not of the line box.
    const QFontMetricsF metrics(font());
    const qreal capMiddle = area.top() + (area.height() - metrics.height()) / 2 + metrics.ascent() - metrics.capHeight() / 2;
    const QColor colour = m_marker.isValid() ? m_marker : palette().color(QPalette::WindowText);
    KubikPaint::drawMarker(&painter, QRectF(area.left(), qRound(capMiddle - 4), 8, 8), colour);
}
