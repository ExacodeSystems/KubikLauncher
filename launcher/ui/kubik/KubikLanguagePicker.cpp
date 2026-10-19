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

#include "KubikLanguagePicker.h"

#include <QButtonGroup>
#include <QFocusEvent>
#include <QHBoxLayout>
#include <QPainter>
#include <QPainterPath>

#include "Application.h"
#include "kubik/KubikBrand.h"
#include "settings/Setting.h"
#include "settings/SettingsObject.h"
#include "translations/TranslationsModel.h"

namespace {

constexpr int RADIUS = 8;

/**
 * The flags are drawn, not shipped as emoji (Windows has no flag emoji) or as
 * SVGs (one less renderer to trust with the Union Jack's clipped diagonals).
 */
void paintFlag(QPainter& p, const QRectF& box, const QString& key)
{
    p.save();
    QPainterPath clip;
    clip.addRoundedRect(box, 4, 4);
    p.setClipPath(clip);

    if (key == "id") {
        // Merah putih: red over white, halves.
        const QRectF top(box.left(), box.top(), box.width(), box.height() / 2);
        p.fillRect(box, QColor("#ffffff"));
        p.fillRect(top, QColor("#ce1126"));
    } else {
        // The Union Jack is 2:1; drawn at the box's height and cropped to its width.
        const qreal h = box.height();
        const qreal w = h * 2;
        const QRectF jack(box.center().x() - w / 2, box.top(), w, h);
        const qreal u = h / 30;  // one unit of the 60x30 construction sheet
        const QColor blue("#012169");
        const QColor red("#c8102e");
        p.fillRect(jack, blue);
        p.setRenderHint(QPainter::Antialiasing);

        const QLineF down(jack.topLeft(), jack.bottomRight());
        const QLineF up(jack.bottomLeft(), jack.topRight());
        p.setPen(QPen(Qt::white, 6 * u, Qt::SolidLine, Qt::FlatCap));
        p.drawLine(down);
        p.drawLine(up);

        // The red diagonals are counterchanged: in each quarter only the half of the
        // stripe that lies clockwise of the white one shows.
        const QPointF c = jack.center();
        QPainterPath quarters;
        quarters.moveTo(c);
        quarters.lineTo(jack.right(), c.y());
        quarters.lineTo(jack.bottomRight());
        quarters.closeSubpath();
        quarters.moveTo(c);
        quarters.lineTo(c.x(), jack.bottom());
        quarters.lineTo(jack.bottomLeft());
        quarters.closeSubpath();
        quarters.moveTo(c);
        quarters.lineTo(jack.left(), c.y());
        quarters.lineTo(jack.topLeft());
        quarters.closeSubpath();
        quarters.moveTo(c);
        quarters.lineTo(c.x(), jack.top());
        quarters.lineTo(jack.topRight());
        quarters.closeSubpath();
        p.save();
        p.setClipPath(quarters, Qt::IntersectClip);
        p.setPen(QPen(red, 4 * u, Qt::SolidLine, Qt::FlatCap));
        p.drawLine(down);
        p.drawLine(up);
        p.restore();

        p.setPen(Qt::NoPen);
        p.setBrush(Qt::white);
        p.drawRect(QRectF(c.x() - 5 * u, jack.top(), 10 * u, h));
        p.drawRect(QRectF(jack.left(), c.y() - 5 * u, w, 10 * u));
        p.setBrush(red);
        p.drawRect(QRectF(c.x() - 3 * u, jack.top(), 6 * u, h));
        p.drawRect(QRectF(jack.left(), c.y() - 3 * u, w, 6 * u));
    }
    p.restore();
}

/** The grass disc with a tick that marks the chosen card. */
void paintCheck(QPainter& p, const QRectF& disc)
{
    p.save();
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);
    p.setBrush(KubikBrand::color("grass"));
    p.drawEllipse(disc);
    QPainterPath tick;
    const qreal s = disc.width();
    tick.moveTo(disc.left() + s * 0.28, disc.top() + s * 0.52);
    tick.lineTo(disc.left() + s * 0.44, disc.top() + s * 0.67);
    tick.lineTo(disc.left() + s * 0.73, disc.top() + s * 0.36);
    p.setPen(QPen(KubikBrand::color("void"), s * 0.12, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.setBrush(Qt::NoBrush);
    p.drawPath(tick);
    p.restore();
}

}  // namespace

KubikLanguageCard::KubikLanguageCard(const QString& key, const QString& name, const QString& line, bool compact, QWidget* parent)
    : QAbstractButton(parent), m_key(key), m_name(name), m_line(line), m_compact(compact)
{
    setCheckable(true);
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::StrongFocus);
    setAccessibleName(name);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
}

QSize KubikLanguageCard::sizeHint() const
{
    return m_compact ? QSize(236, 56) : QSize(236, 168);
}

void KubikLanguageCard::focusInEvent(QFocusEvent* event)
{
    m_keyboardFocus = event->reason() == Qt::TabFocusReason || event->reason() == Qt::BacktabFocusReason;
    update();
    QAbstractButton::focusInEvent(event);
}

void KubikLanguageCard::focusOutEvent(QFocusEvent* event)
{
    m_keyboardFocus = false;
    update();
    QAbstractButton::focusOutEvent(event);
}

void KubikLanguageCard::enterEvent(QEnterEvent* event)
{
    m_hovered = true;
    update();
    QAbstractButton::enterEvent(event);
}

void KubikLanguageCard::leaveEvent(QEvent* event)
{
    m_hovered = false;
    update();
    QAbstractButton::leaveEvent(event);
}

void KubikLanguageCard::paintEvent(QPaintEvent* /*event*/)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    const QRectF card = QRectF(rect());

    QPainterPath shape;
    shape.addRoundedRect(card, RADIUS, RADIUS);
    const bool lit = isChecked() || m_hovered || isDown();
    p.fillPath(shape, KubikBrand::color(lit ? "block" : "void.panel"));
    if (hasFocus() && m_keyboardFocus) {
        p.setPen(QPen(KubikBrand::color("grass.deep"), 2));
        p.drawRoundedRect(card.adjusted(1, 1, -1, -1), RADIUS - 1, RADIUS - 1);
    }

    QFont nameFont(KubikBrand::displayFamily());
    QFont lineFont(KubikBrand::bodyFamily());
    QRectF flag;
    QRectF nameBox;
    QRectF lineBox;
    QRectF disc;
    if (m_compact) {
        const qreal pad = 14;
        flag = QRectF(pad, (card.height() - 24) / 2, 36, 24);
        disc = QRectF(card.width() - pad - 18, (card.height() - 18) / 2, 18, 18);
        const qreal textLeft = flag.right() + 14;
        const qreal textWidth = disc.left() - 10 - textLeft;
        nameFont.setPixelSize(15);
        nameFont.setWeight(QFont::DemiBold);
        lineFont.setPixelSize(12);
        // Compact (Settings) is the flag and the name: the line is for the first run, where it helps
        // a player who reads only one of the two languages.
        nameBox = QRectF(textLeft, 0, textWidth, card.height());
    } else {
        const qreal pad = 18;
        flag = QRectF(pad, pad, 66, 44);
        disc = QRectF(card.width() - pad - 22, pad, 22, 22);
        nameFont.setPixelSize(19);
        nameFont.setWeight(QFont::Bold);
        lineFont.setPixelSize(13);
        nameBox = QRectF(pad, flag.bottom() + 16, card.width() - 2 * pad, 26);
        lineBox = QRectF(pad, nameBox.bottom() + 4, card.width() - 2 * pad, card.height() - nameBox.bottom() - 4 - pad);
    }

    paintFlag(p, flag, m_key);
    if (isChecked()) {
        paintCheck(p, disc);
    }

    p.setFont(nameFont);
    p.setPen(KubikBrand::color("bone"));
    p.drawText(nameBox, Qt::AlignLeft | Qt::AlignVCenter, QFontMetrics(nameFont).elidedText(m_name, Qt::ElideRight, int(nameBox.width())));

    if (!m_compact) {
        p.setFont(lineFont);
        p.setPen(KubikBrand::color("bone.muted"));
        p.drawText(lineBox, Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap, m_line);
    }
}

KubikLanguagePicker::KubikLanguagePicker(bool compact, QWidget* parent) : QWidget(parent), m_group(new QButtonGroup(this))
{
    m_group->setExclusive(true);

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(compact ? 10 : 14);

    // Indonesian first: it is the dashboard's default, and most players read it first. Each card's
    // second line is written in its own language and is deliberately not translated.
    struct Choice {
        const char* key;
        const char* name;
        const char* line;
    };
    static const Choice choices[] = {
        { "id", "Bahasa Indonesia", "Pakai KubikLauncher dalam Bahasa Indonesia" },
        { "en_US", "English", "Use KubikLauncher in English" },
    };
    for (const auto& choice : choices) {
        auto* card = new KubikLanguageCard(choice.key, choice.name, choice.line, compact, this);
        m_group->addButton(card);
        layout->addWidget(card);
        connect(card, &QAbstractButton::clicked, this, [this, key = card->key()] { choose(key); });
    }
    layout->addStretch();

    connect(APPLICATION->settings()->getSetting("Language").get(), &Setting::SettingChanged, this, [this] { showSelected(); });
    showSelected();
}

QString KubikLanguagePicker::selectedLanguage() const
{
    return APPLICATION->translations()->selectedLanguage();
}

void KubikLanguagePicker::choose(const QString& key)
{
    auto* translations = APPLICATION->translations();
    if (key != translations->selectedLanguage()) {
        translations->selectLanguage(key);
        translations->updateLanguage(key);
    }
    APPLICATION->settings()->set("Language", key);
    showSelected();
}

void KubikLanguagePicker::showSelected()
{
    const QString selected = selectedLanguage();
    for (auto* button : m_group->buttons()) {
        if (auto* card = qobject_cast<KubikLanguageCard*>(button)) {
            card->setChecked(card->key() == selected);
        }
    }
}
