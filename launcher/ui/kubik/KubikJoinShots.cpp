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

#include "KubikJoinShots.h"

#include <QCryptographicHash>
#include <QFile>
#include <QFontMetricsF>
#include <QPainter>
#include <QPainterPath>

#include "kubik/KubikBrand.h"

namespace {

using KubikJoinShots::Shot;
using KubikJoinShots::Subject;

const QString GUI = QStringLiteral(":/kubik/resourcepack/assets/minecraft/textures/gui/");

// Every scene is laid out in Minecraft's GUI pixels on a 320 x 180 screen (a 16:9 window at a GUI scale
// that keeps the buttons readable), then scaled to the picture.
constexpr qreal SCENE_W = 320;
constexpr qreal SCENE_H = 180;

const QColor WHITE(0xFF, 0xFF, 0xFF);
const QColor LABEL_GREY(0xA0, 0xA0, 0xA0);  // field labels and disabled buttons
const QColor MOTD_GREY(0x80, 0x80, 0x80);   // a server's message of the day and player count

/** One of the pack's textures. By value: QImage is shared, and a cached reference could move. */
QImage art(const QString& path)
{
    static QHash<QString, QImage> cache;
    if (!cache.contains(path)) {
        cache.insert(path, QImage(GUI + path).convertToFormat(QImage::Format_ARGB32_Premultiplied));
    }
    return cache.value(path);
}

/** The middle band of one panorama face, blurred as Minecraft blurs it behind its menus. */
QImage blurredPanorama()
{
    static const QImage blurred = [] {
        const QImage face = art("title/background/panorama_0.png");
        if (face.isNull()) {
            return QImage();
        }
        const int band = face.width() * 9 / 16;
        // Down in two steps and back up smoothly when drawn: an even blur for the price of two scales.
        return face.copy(0, (face.height() - band) / 2, face.width(), band)
            .scaled(256, 144, Qt::IgnoreAspectRatio, Qt::SmoothTransformation)
            .scaled(80, 45, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }();
    return blurred;
}

/** A splash from the pack, the same one for the same server. */
QString splashFor(const QString& serverId)
{
    static const QStringList splashes = [] {
        QStringList lines;
        QFile file(":/kubik/resourcepack/assets/minecraft/texts/splashes.txt");
        if (file.open(QIODevice::ReadOnly)) {
            for (const auto& line : QString::fromUtf8(file.readAll()).split('\n')) {
                if (!line.trimmed().isEmpty()) {
                    lines.append(line.trimmed());
                }
            }
        }
        return lines;
    }();
    if (splashes.isEmpty()) {
        return {};
    }
    const auto digest = QCryptographicHash::hash(serverId.toUtf8(), QCryptographicHash::Sha1);
    return splashes.at(static_cast<unsigned char>(digest.at(1)) % splashes.size());
}

class Scene {
   public:
    enum class Lit { Normal, Highlighted, Disabled };

    Scene(QPainter* painter, const QRectF& target) : m_painter(painter)
    {
        m_scale = qMin(target.width() / SCENE_W, target.height() / SCENE_H);
        m_origin = target.center() - QPointF(SCENE_W, SCENE_H) * m_scale / 2;
        m_font = QFont(KubikBrand::displayFamily());
        m_font.setWeight(QFont::DemiBold);
    }

    QPainter* painter() const { return m_painter; }
    QRectF at(qreal x, qreal y, qreal w, qreal h) const
    {
        return { m_origin.x() + x * m_scale, m_origin.y() + y * m_scale, w * m_scale, h * m_scale };
    }

    void image(const QImage& image, qreal x, qreal y, qreal w, qreal h, bool smooth)
    {
        m_painter->setRenderHint(QPainter::SmoothPixmapTransform, smooth);
        m_painter->drawImage(at(x, y, w, h), image);
    }

    /** A texture repeated over an area, one copy every `tileW` x `tileH` GUI pixels, as menus are drawn. */
    void tiled(const QImage& texture, qreal x, qreal y, qreal w, qreal h, qreal tileW, qreal tileH)
    {
        m_painter->save();
        m_painter->setClipRect(at(x, y, w, h), Qt::IntersectClip);
        m_painter->setRenderHint(QPainter::SmoothPixmapTransform, false);
        for (qreal ty = y; ty < y + h; ty += tileH) {
            for (qreal tx = x; tx < x + w; tx += tileW) {
                m_painter->drawImage(at(tx, ty, tileW, tileH), texture);
            }
        }
        m_painter->restore();
    }

    /** A sprite stretched to any size with its `border` pixels kept, Minecraft's nine_slice. */
    void nineSlice(const QImage& sprite, qreal x, qreal y, qreal w, qreal h, int border)
    {
        if (sprite.isNull()) {
            return;
        }
        m_painter->setRenderHint(QPainter::SmoothPixmapTransform, false);
        const qreal xs[] = { x, x + border, x + w - border, x + w };
        const qreal ys[] = { y, y + border, y + h - border, y + h };
        const int sxs[] = { 0, border, sprite.width() - border, sprite.width() };
        const int sys[] = { 0, border, sprite.height() - border, sprite.height() };
        for (int row = 0; row < 3; ++row) {
            for (int col = 0; col < 3; ++col) {
                const QRectF target = at(xs[col], ys[row], xs[col + 1] - xs[col], ys[row + 1] - ys[row]);
                const QRect source(sxs[col], sys[row], sxs[col + 1] - sxs[col], sys[row + 1] - sys[row]);
                if (!target.isEmpty() && !source.isEmpty()) {
                    m_painter->drawImage(target, sprite, source);
                }
            }
        }
    }

    void button(const QString& label, qreal x, qreal y, qreal w, Lit lit = Lit::Normal)
    {
        const QString sprite = lit == Lit::Highlighted ? "button_highlighted" : lit == Lit::Disabled ? "button_disabled" : "button";
        nineSlice(art("sprites/widget/" + sprite + ".png"), x, y, w, 20, 3);
        text(label, x + 2, y, w - 4, 20, lit == Lit::Disabled ? LABEL_GREY : WHITE, Qt::AlignCenter);
    }

    /** A text field holding `value`; a focused one is lit and shows Minecraft's cursor. */
    void field(const QString& value, qreal x, qreal y, qreal w, bool focused)
    {
        nineSlice(art(focused ? "sprites/widget/text_field_highlighted.png" : "sprites/widget/text_field.png"), x, y, w, 20, 1);
        text(focused ? value + "_" : value, x + 5, y, w - 10, 20, QColor(0xE0, 0xE0, 0xE0), Qt::AlignLeft | Qt::AlignVCenter);
    }

    /** Text with Minecraft's shadow: the same text a quarter as bright, one GUI pixel down and right. */
    void text(const QString& value, qreal x, qreal y, qreal w, qreal h, const QColor& colour, Qt::Alignment align, qreal size = 8.5)
    {
        QFont font = m_font;
        font.setPixelSize(qMax(6, qRound(size * m_scale)));
        m_painter->setFont(font);
        const QRectF box = at(x, y, w, h);
        const QString shown = QFontMetricsF(font).elidedText(value, Qt::ElideRight, box.width());
        m_painter->setPen(QColor(colour.red() / 4, colour.green() / 4, colour.blue() / 4));
        m_painter->drawText(box.translated(m_scale, m_scale), align, shown);
        m_painter->setPen(colour);
        m_painter->drawText(box, align, shown);
    }

    /** Minecraft's yellow splash, tilted, centred on (x, y). */
    void splash(const QString& value, qreal x, qreal y)
    {
        if (value.isEmpty()) {
            return;
        }
        // Minecraft's own sizing: a short splash large, a long one smaller (1.8 x 100 / (width + 32)), and
        // here no wider than 120 GUI pixels, so a long one stays clear of the buttons under it.
        QFont font = m_font;
        font.setPixelSize(qMax(1, qRound(8 * m_scale)));
        const qreal width = QFontMetricsF(font).horizontalAdvance(value) / m_scale;
        const qreal grow = qBound(0.6, qMin(1.8 * 100 / (width + 32), 120 / width), 2.0);
        font.setPixelSize(qMax(6, qRound(8 * grow * m_scale)));
        m_painter->save();
        m_painter->translate(at(x, y, 0, 0).topLeft());
        m_painter->rotate(-20);
        m_painter->setFont(font);
        const QRectF box(-SCENE_W * m_scale / 2, -6 * m_scale, SCENE_W * m_scale, 12 * m_scale);
        m_painter->setPen(QColor(0x3F, 0x3F, 0x15));
        m_painter->drawText(box.translated(m_scale, m_scale), Qt::AlignCenter, value);
        m_painter->setPen(QColor(0xFF, 0xFF, 0x55));
        m_painter->drawText(box, Qt::AlignCenter, value);
        m_painter->restore();
    }

    /** The launcher's mark on a picture: a green ring around what to press. */
    void ring(qreal x, qreal y, qreal w, qreal h)
    {
        const QRectF box = at(x, y, w, h).adjusted(-3, -3, 3, 3);
        m_painter->save();
        m_painter->setRenderHint(QPainter::Antialiasing);
        m_painter->setBrush(Qt::NoBrush);
        QColor glow = KubikBrand::color("grass");
        glow.setAlpha(80);
        m_painter->setPen(QPen(glow, 7));
        m_painter->drawRoundedRect(box, 5, 5);
        m_painter->setPen(QPen(KubikBrand::color("grass"), 2.5));
        m_painter->drawRoundedRect(box, 5, 5);
        m_painter->restore();
    }

   private:
    QPainter* m_painter;
    QPointF m_origin;
    qreal m_scale;
    QFont m_font;
};

void panorama(Scene& scene, bool vignette)
{
    scene.image(blurredPanorama(), 0, 0, SCENE_W, SCENE_H, true);
    if (vignette) {
        scene.image(art("title/background/panorama_overlay.png"), 0, 0, SCENE_W, SCENE_H, true);
    }
}

/** A menu outside a world: the blurred panorama under the menu background, as Minecraft draws it since 1.20.5. */
void menuBackground(Scene& scene)
{
    panorama(scene, false);
    scene.tiled(art("menu_background.png"), 0, 0, SCENE_W, SCENE_H, 32, 32);
}

/** A server's row in Multiplayer: icon, name, message of the day, players and the ping bars. */
void serverEntry(Scene& scene, qreal x, qreal y, qreal w, const Subject& subject)
{
    const KubikServerLive* live = subject.live ? &*subject.live : nullptr;
    const QRect icon = scene.at(x + 2, y + 2, 32, 32).toRect();
    if (live && !live->favicon.isNull()) {
        scene.painter()->setRenderHint(QPainter::SmoothPixmapTransform, false);
        scene.painter()->drawImage(icon, live->favicon);
    } else {
        KubikServerDelegate::paintArt(scene.painter(), icon, subject.server, live, false);
    }
    const qreal textX = x + 38;
    scene.text(subject.server.name, textX, y + 2, w - 38 - 60, 10, WHITE, Qt::AlignLeft | Qt::AlignVCenter);
    // "A Minecraft Server" is what a server says when its owner has not set a message.
    const QString motd = live && !live->motd.isEmpty() ? live->motd : QStringLiteral("A Minecraft Server");
    const QStringList lines = motd.split('\n');
    for (int i = 0; i < qMin(2, static_cast<int>(lines.size())); ++i) {
        scene.text(lines.at(i).trimmed(), textX, y + 13 + i * 10, w - 40, 10, MOTD_GREY, Qt::AlignLeft | Qt::AlignVCenter);
    }
    // The ping bars, all five lit, and the player count beside them.
    QPainter* painter = scene.painter();
    for (int bar = 0; bar < 5; ++bar) {
        const qreal height = 2 + bar * 1.5;
        painter->fillRect(scene.at(x + w - 14 + bar * 2, y + 10 - height, 1.2, height), QColor(0x3C, 0xD8, 0x3C));
    }
    if (live && live->online >= 0 && live->max > 0) {
        scene.text(QString("%1/%2").arg(live->online).arg(live->max), x + w - 80, y + 2, 62, 10, MOTD_GREY,
                   Qt::AlignRight | Qt::AlignVCenter);
    }
}

void connecting(Scene& scene, const Subject& subject)
{
    menuBackground(scene);
    scene.text(subject.labels.connecting, 0, 66, SCENE_W, 12, WHITE, Qt::AlignCenter);
    scene.button(subject.labels.cancel, 60, 104, 200);
}

void titleScreen(Scene& scene, const Subject& subject)
{
    const auto& labels = subject.labels;
    panorama(scene, true);
    // The logo's place stays empty: Minecraft's logo is Mojang's, and the picture is about the buttons.
    scene.splash(splashFor(subject.server.id), 244, 24);
    scene.button(labels.singleplayer, 60, 50, 200);
    scene.button(labels.multiplayer, 60, 74, 200, Scene::Lit::Highlighted);
    scene.button(labels.realms, 60, 98, 200);
    scene.button(labels.options, 60, 134, 98);
    scene.button(labels.quit, 162, 134, 98);
    scene.ring(60, 74, 200, 20);
    if (!subject.server.version.isEmpty()) {
        scene.text("Minecraft " + subject.server.version, 3, SCENE_H - 11, 150, 10, WHITE, Qt::AlignLeft | Qt::AlignVCenter, 7);
    }
}

void serverList(Scene& scene, const Subject& subject)
{
    const auto& labels = subject.labels;
    menuBackground(scene);
    // The list sits between its two separators on a darker ground.
    scene.tiled(art("menu_list_background.png"), 0, 20, SCENE_W, 108, 32, 32);
    scene.tiled(art("header_separator.png"), 0, 18, SCENE_W, 2, 32, 2);
    scene.tiled(art("footer_separator.png"), 0, 128, SCENE_W, 2, 32, 2);
    scene.text(labels.multiplayerTitle, 0, 3, SCENE_W, 13, WHITE, Qt::AlignCenter);

    // This server first, where KubikLauncher puts it, and one the player added before, under it.
    serverEntry(scene, 40, 26, 240, subject);
    scene.painter()->save();
    scene.painter()->setOpacity(0.45);
    Subject other = subject;
    other.server.id = "other";
    other.server.name = labels.defaultServerName;
    other.server.loader.clear();
    other.live.reset();
    serverEntry(scene, 40, 66, 240, other);
    scene.painter()->restore();
    scene.ring(40, 26, 240, 36);

    scene.button(labels.joinServer, 6, 134, 100, Scene::Lit::Highlighted);
    scene.button(labels.directConnection, 110, 134, 100);
    scene.button(labels.addServer, 214, 134, 100);
    scene.button(labels.edit, 6, 158, 74);
    scene.button(labels.remove, 84, 158, 74);
    scene.button(labels.refresh, 162, 158, 74);
    scene.button(labels.back, 240, 158, 74);
    scene.ring(6, 134, 100, 20);
}

void addServer(Scene& scene, const Subject& subject)
{
    const auto& labels = subject.labels;
    menuBackground(scene);
    scene.text(labels.editServerInfo, 0, 5, SCENE_W, 13, WHITE, Qt::AlignCenter);
    scene.text(labels.serverName, 60, 25, 200, 10, LABEL_GREY, Qt::AlignLeft | Qt::AlignVCenter);
    scene.field(subject.server.name, 60, 36, 200, false);
    scene.text(labels.serverAddress, 60, 61, 200, 10, LABEL_GREY, Qt::AlignLeft | Qt::AlignVCenter);
    scene.field(subject.server.address, 60, 72, 200, true);
    scene.ring(60, 72, 200, 20);
    scene.button(labels.serverResourcePacks + ": " + labels.prompt, 60, 102, 200);
    scene.button(labels.done, 60, 128, 200, Scene::Lit::Highlighted);
    scene.ring(60, 128, 200, 20);
    scene.button(labels.cancel, 60, 152, 200);
}

}  // namespace

void KubikJoinShots::paint(QPainter* painter, const QRectF& target, Shot shot, const Subject& subject)
{
    painter->save();
    painter->setClipRect(target, Qt::IntersectClip);
    painter->fillRect(target, KubikBrand::color("block.deep"));
    Scene scene(painter, target);
    switch (shot) {
        case Shot::Connecting:
            connecting(scene, subject);
            break;
        case Shot::TitleScreen:
            titleScreen(scene, subject);
            break;
        case Shot::ServerList:
            serverList(scene, subject);
            break;
        case Shot::AddServer:
            addServer(scene, subject);
            break;
    }
    painter->restore();
}

KubikJoinShotView::KubikJoinShotView(KubikJoinShots::Shot shot, QWidget* parent) : QWidget(parent), m_shot(shot)
{
    QSizePolicy policy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    policy.setHeightForWidth(true);
    setSizePolicy(policy);
}

void KubikJoinShotView::setSubject(const KubikJoinShots::Subject& subject)
{
    m_subject = subject;
    m_cache = QPixmap();
    update();
}

void KubikJoinShotView::setShot(KubikJoinShots::Shot shot)
{
    if (shot != m_shot) {
        m_shot = shot;
        m_cache = QPixmap();
        update();
    }
}

void KubikJoinShotView::paintEvent(QPaintEvent*)
{
    if (!m_subject) {
        return;
    }
    const qreal dpr = devicePixelRatioF();
    if (m_cache.isNull() || m_cache.size() != size() * dpr || m_cache.devicePixelRatio() != dpr) {
        m_cache = QPixmap(size() * dpr);
        m_cache.setDevicePixelRatio(dpr);
        m_cache.fill(Qt::transparent);
        QPainter painter(&m_cache);
        painter.setRenderHint(QPainter::Antialiasing);
        QPainterPath corners;
        corners.addRoundedRect(QRectF(rect()), 6, 6);
        painter.setClipPath(corners);
        KubikJoinShots::paint(&painter, QRectF(rect()), m_shot, *m_subject);
    }
    QPainter(this).drawPixmap(0, 0, m_cache);
}
