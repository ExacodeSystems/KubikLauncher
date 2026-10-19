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

#include "KubikServerTiles.h"

#include <QCryptographicHash>
#include <QFocusEvent>
#include <QFontMetrics>
#include <QPainter>
#include <QPainterPath>
#include <QResizeEvent>
#include <QScrollBar>

#include "Application.h"
#include "icons/IconList.h"
#include "kubik/KubikBrand.h"
#include "kubik/KubikProfiles.h"

using KubikBrand::color;

namespace {

// The same measurements as an instance tile (KubikInstanceDelegate.cpp).
constexpr int TILE_MIN_WIDTH = 220;
constexpr int SPACING = 16;
constexpr int RADIUS = 6;
constexpr int PAD = 14;
constexpr int NAME_HEIGHT = 22;
constexpr int LINE_HEIGHT = 18;
constexpr int TEXT_BLOCK = 12 + NAME_HEIGHT + 4 + LINE_HEIGHT + 2 + LINE_HEIGHT + PAD;
constexpr int PLAY_WIDTH = 76;

QRect artRect(const QRect& tile)
{
    return QRect(tile.left(), tile.top(), tile.width(), tile.width() / 2);
}

QFont font(const QString& family, int pixelSize, QFont::Weight weight)
{
    QFont f(family);
    f.setPixelSize(pixelSize);
    f.setWeight(weight);
    return f;
}

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

// ── The model ────────────────────────────────────────────────────────────────

void KubikServerModel::setServers(QList<Kubik::Server> servers)
{
    beginResetModel();
    m_servers = std::move(servers);
    // A server that stopped answering keeps nothing it said before.
    for (auto it = m_live.begin(); it != m_live.end();) {
        const auto* s = serverById(it.key());
        it = s && s->isRunning() ? std::next(it) : m_live.erase(it);
    }
    endResetModel();
}

void KubikServerModel::setLive(const QString& id, const KubikServerLive& live)
{
    m_live.insert(id, live);
    for (int row = 0; row < m_servers.size(); ++row) {
        if (m_servers.at(row).id == id) {
            emit dataChanged(index(row), index(row));
        }
    }
}

const KubikServerLive* KubikServerModel::live(const QString& id) const
{
    const auto it = m_live.constFind(id);
    return it == m_live.cend() ? nullptr : &it.value();
}

const Kubik::Server* KubikServerModel::server(const QModelIndex& index) const
{
    return index.isValid() && index.row() < m_servers.size() ? &m_servers.at(index.row()) : nullptr;
}

const Kubik::Server* KubikServerModel::serverById(const QString& id) const
{
    for (const auto& s : m_servers) {
        if (s.id == id) {
            return &s;
        }
    }
    return nullptr;
}

int KubikServerModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_servers.size());
}

QVariant KubikServerModel::data(const QModelIndex& index, int role) const
{
    const auto* s = server(index);
    if (!s) {
        return {};
    }
    switch (role) {
        case Qt::DisplayRole:
            return s->name;
        case SearchRole:
            return QStringList{ s->name, s->address, s->loaderName, s->version, s->modpackName }.join(' ');
        case IdRole:
            return s->id;
        case Qt::UserRole:
            return QVariant::fromValue(s);
        case LiveRole:
            return QVariant::fromValue(live(s->id));
        default:
            return {};
    }
}

// ── The grid ─────────────────────────────────────────────────────────────────

KubikServerGrid::KubikServerGrid(QWidget* parent) : QListView(parent)
{
    setViewMode(QListView::IconMode);
    setMovement(QListView::Static);
    setResizeMode(QListView::Adjust);
    setUniformItemSizes(true);
    setSelectionMode(QAbstractItemView::NoSelection);
    setFrameShape(QFrame::NoFrame);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    verticalScrollBar()->setSingleStep(24);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setMouseTracking(true);
    viewport()->setMouseTracking(true);
    setAttribute(Qt::WA_MacShowFocusRect, false);
    m_tileWidth = TILE_MIN_WIDTH;
}

void KubikServerGrid::resizeEvent(QResizeEvent* event)
{
    QListView::resizeEvent(event);
    relayout();
}

void KubikServerGrid::focusInEvent(QFocusEvent* event)
{
    m_keyboardFocus = event->reason() == Qt::TabFocusReason || event->reason() == Qt::BacktabFocusReason;
    QListView::focusInEvent(event);
}

void KubikServerGrid::relayout()
{
    const int available = viewport()->width() - SPACING;
    const int columns = std::max(1, available / (TILE_MIN_WIDTH + SPACING));
    const int width = std::max(TILE_MIN_WIDTH, available / columns - SPACING);
    if (width == m_tileWidth && gridSize().isValid()) {
        return;
    }
    m_tileWidth = width;
    setGridSize(QSize(width + SPACING, KubikServerDelegate::tileHeight(width) + SPACING));
    doItemsLayout();
}

// ── The tile ─────────────────────────────────────────────────────────────────

KubikServerDelegate::KubikServerDelegate(QObject* parent) : QStyledItemDelegate(parent) {}

int KubikServerDelegate::tileHeight(int width)
{
    return width / 2 + TEXT_BLOCK;
}

QRect KubikServerDelegate::playRect(const QRect& tile)
{
    const QRect art = artRect(tile);
    const QSize size(PLAY_WIDTH, 34);
    const int top = art.bottom() + 1 + 12 + (NAME_HEIGHT + 4 + LINE_HEIGHT - size.height()) / 2;
    return QRect(tile.right() - PAD - size.width() + 1, top, size.width(), size.height());
}

QSize KubikServerDelegate::sizeHint(const QStyleOptionViewItem& option, [[maybe_unused]] const QModelIndex& index) const
{
    const auto* grid = qobject_cast<const KubikServerGrid*>(option.widget);
    const int width = grid ? grid->tileWidth() : TILE_MIN_WIDTH;
    return QSize(width, tileHeight(width));
}

QString KubikServerDelegate::statusText(const Kubik::Server& s)
{
    if (s.locked) {
        return tr("Frozen");
    }
    if (s.status == "RUNNING") {
        return tr("Online");
    }
    if (s.status == "SUSPENDED" || s.status == "STOPPED") {
        return tr("Sleeping");
    }
    if (s.status == "PROVISIONING") {
        return tr("Setting up");
    }
    return tr("Needs attention");
}

QColor KubikServerDelegate::statusColour(const Kubik::Server& s)
{
    if (s.locked || s.status == "ERROR") {
        return color("heart");
    }
    if (s.status == "RUNNING") {
        return color("grass");
    }
    if (s.status == "PROVISIONING") {
        return color("gold");
    }
    return color("sky.soft");
}

QString KubikServerDelegate::statusLine(const Kubik::Server& s, const KubikServerLive* live)
{
    if (!s.locked && s.isRunning() && live && live->online >= 0 && live->max > 0) {
        return tr("Online · %1/%2").arg(live->online).arg(live->max);
    }
    return statusText(s);
}

void KubikServerDelegate::paintArt(QPainter* painter, const QRect& art, const Kubik::Server& server, const KubikServerLive* live, bool lit)
{
    painter->save();
    painter->fillRect(art, color(lit ? "void.panel" : "void.raised"));
    // One of the brand's deep tones, picked by the server's id, washed over the ground.
    static const char* tones[] = { "grass.deep", "sky.deep", "gold.deep", "ender.deep" };
    const auto digest = QCryptographicHash::hash(server.id.toUtf8(), QCryptographicHash::Sha1);
    QColor tint = color(tones[static_cast<unsigned char>(digest.at(0)) % (sizeof(tones) / sizeof(tones[0]))]);
    tint.setAlphaF(lit ? 0.42 : 0.30);
    painter->fillRect(art, tint);

    const qreal dpr = painter->device()->devicePixelRatioF();
    const int side = qMin(96, art.height() * 62 / 100);
    QRect iconRect(0, 0, side, side);
    iconRect.moveCenter(art.center());
    if (live && !live->favicon.isNull()) {
        // The server's own 64x64 icon: pixel art, so scaled without smoothing.
        painter->setRenderHint(QPainter::SmoothPixmapTransform, false);
        painter->drawImage(iconRect, live->favicon);
    } else {
        const QString iconKey = server.edition == "bedrock" ? QString("stone") : Kubik::iconForLoader(server.loader);
        painter->drawPixmap(iconRect, iconPixmap(APPLICATION->icons()->getIcon(iconKey), side, dpr));
    }
    painter->restore();
}

QString KubikServerDelegate::describe(const Kubik::Server& s)
{
    if (s.unplayable == "bedrock") {
        return tr("Bedrock · join from Minecraft Bedrock");
    }
    if (!s.modpackName.isEmpty()) {
        return tr("Modpack: %1").arg(s.modpackName);
    }
    const QString what = s.profile ? s.profile->label : QString("%1 %2").arg(s.loaderName, s.version);
    // Plugins never reach a player's game, so a Paper server's count would only confuse.
    if (s.profile && !s.profile->loader.isEmpty() && s.modCount > 0) {
        return tr("%1 · %n mod(s)", nullptr, s.modCount).arg(what);
    }
    return what;
}

void KubikServerDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    const auto* server = index.data(Qt::UserRole).value<const Kubik::Server*>();
    if (!server) {
        return;
    }
    const QRect tile = option.rect;
    const QRect art = artRect(tile);
    const bool lit = index == m_hovered;
    const bool playable = server->profile.has_value() && !server->locked;

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    const auto* live = index.data(KubikServerModel::LiveRole).value<const KubikServerLive*>();
    QPainterPath shape;
    shape.addRoundedRect(QRectF(tile), RADIUS, RADIUS);
    painter->fillPath(shape, color(lit ? "block" : "void.panel"));
    painter->save();
    painter->setClipPath(shape);
    paintArt(painter, art, *server, live, lit);
    painter->restore();

    // Status, in the art field's corner: a dot that carries the colour, and words that carry the meaning.
    const QFont smallFont = font(KubikBrand::bodyFamily(), 13, QFont::Normal);
    const QFontMetrics small(smallFont);
    {
        const QString text = statusLine(*server, live);
        const int pillWidth = 10 + 8 + 6 + small.horizontalAdvance(text) + 10;
        const QRect pill(art.left() + 10, art.top() + 10, pillWidth, 24);
        QPainterPath path;
        path.addRoundedRect(QRectF(pill), 12, 12);
        painter->fillPath(path, color("void"));
        painter->setPen(Qt::NoPen);
        painter->setBrush(statusColour(*server));
        painter->drawEllipse(QRectF(pill.left() + 10, pill.center().y() - 3.5, 8, 8));
        painter->setPen(color("bone"));
        painter->setFont(smallFont);
        painter->drawText(pill.adjusted(10 + 8 + 6, 0, 0, 0), Qt::AlignLeft | Qt::AlignVCenter, text);
    }

    if (lit && playable) {
        const QRect play = playRect(tile);
        QPainterPath button;
        button.addRoundedRect(QRectF(play), 4, 4);
        painter->fillPath(button, color("grass"));
        painter->setPen(color("void"));
        painter->setFont(font(KubikBrand::displayFamily(), 14, QFont::Bold));
        painter->drawText(play, Qt::AlignCenter, tr("Play"));
    }

    const int textLeft = tile.left() + PAD;
    const int fullWidth = tile.width() - 2 * PAD;
    const int textWidth = (lit && playable) ? fullWidth - PLAY_WIDTH - 10 : fullWidth;
    int y = art.bottom() + 1 + 12;

    const QFont nameFont = font(KubikBrand::displayFamily(), 16, QFont::DemiBold);
    painter->setFont(nameFont);
    painter->setPen(color("bone"));
    painter->drawText(QRect(textLeft, y, textWidth, NAME_HEIGHT), Qt::AlignLeft | Qt::AlignVCenter,
                      QFontMetrics(nameFont).elidedText(server->name, Qt::ElideRight, textWidth));
    y += NAME_HEIGHT + 4;

    painter->setFont(smallFont);
    painter->setPen(color("bone.muted"));
    painter->drawText(QRect(textLeft, y, textWidth, LINE_HEIGHT), Qt::AlignLeft | Qt::AlignVCenter,
                      small.elidedText(describe(*server), Qt::ElideRight, textWidth));
    y += LINE_HEIGHT + 2;

    QString address = server->address;
    if (server->edition == "bedrock" && server->port > 0) {
        address += ":" + QString::number(server->port);
    }
    // Addresses are typed into Minecraft, so they are set in the mono face.
    const QFont monoFont = font(KubikBrand::monoFamily(), 12, QFont::Normal);
    painter->setFont(address.isEmpty() ? smallFont : monoFont);
    painter->setPen(color("bone.faint"));
    painter->drawText(
        QRect(textLeft, y, fullWidth, LINE_HEIGHT), Qt::AlignLeft | Qt::AlignVCenter,
        QFontMetrics(painter->font()).elidedText(address.isEmpty() ? tr("No address yet") : address, Qt::ElideRight, fullWidth));

    // The keyboard's place in the grid (Enter opens it, Space plays it).
    const auto* grid = qobject_cast<const KubikServerGrid*>(option.widget);
    if ((option.state & QStyle::State_HasFocus) && grid && grid->keyboardFocus()) {
        painter->setPen(QPen(color("grass.deep"), 2));
        painter->setBrush(Qt::NoBrush);
        painter->drawRoundedRect(QRectF(tile).adjusted(1, 1, -1, -1), RADIUS - 1, RADIUS - 1);
    }

    painter->restore();
}
