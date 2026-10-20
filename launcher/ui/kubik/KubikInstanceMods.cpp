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

#include "KubikInstanceMods.h"

#include <QEvent>
#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QHelpEvent>
#include <QLabel>
#include <QListView>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QToolTip>
#include <QVBoxLayout>

#include "DesktopServices.h"
#include "FileSystem.h"
#include "kubik/KubikBrand.h"
#include "kubik/KubikSyncPlan.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/mod/Mod.h"
#include "minecraft/mod/ModFolderModel.h"
#include "ui/kubik/KubikConfirmDialog.h"

using KubikBrand::color;

namespace {

constexpr int ROW_HEIGHT = 58;
constexpr int GAP = 3;  // half the space between two rows
constexpr int PAD = 12;
constexpr int ICON = 32;
constexpr int SWITCH_W = 36;
constexpr int SWITCH_H = 20;
constexpr int REMOVE_W = 72;

QFont font(const QString& family, int pixelSize, QFont::Weight weight)
{
    QFont f(family);
    f.setPixelSize(pixelSize);
    f.setWeight(weight);
    return f;
}

QString enabledForm(QString filename)
{
    if (filename.endsWith(QStringLiteral(".disabled"))) {
        filename.chop(9);
    }
    return filename;
}

}  // namespace

// ── The row ──────────────────────────────────────────────────────────────────

KubikModDelegate::KubikModDelegate(ModFolderModel* model, QObject* parent) : QStyledItemDelegate(parent), m_model(model) {}

void KubikModDelegate::setManaged(QSet<QString> filenames, const QString& serverName)
{
    m_managed = std::move(filenames);
    m_serverName = serverName;
}

int KubikModDelegate::sourceRow(const QModelIndex& index) const
{
    if (const auto* proxy = qobject_cast<const QSortFilterProxyModel*>(index.model())) {
        return proxy->mapToSource(index).row();
    }
    return index.row();
}

bool KubikModDelegate::isManaged(int sourceRow) const
{
    return sourceRow >= 0 && sourceRow < int(m_model->size()) &&
           m_managed.contains(enabledForm(m_model->at(sourceRow).fileinfo().fileName()));
}

QRect KubikModDelegate::switchRect(const QRect& row)
{
    return QRect(row.right() - PAD - SWITCH_W + 1, row.center().y() - SWITCH_H / 2 + 1, SWITCH_W, SWITCH_H);
}

QRect KubikModDelegate::removeRect(const QRect& row)
{
    const QRect toggle = switchRect(row);
    return QRect(toggle.left() - 14 - REMOVE_W, row.center().y() - 14, REMOVE_W, 28);
}

QSize KubikModDelegate::sizeHint(const QStyleOptionViewItem&, const QModelIndex&) const
{
    // No width of its own: a top-to-bottom QListView stretches each row to the viewport but never shrinks it,
    // so a row sized to the viewport outgrew it as soon as a long list grew its vertical scrollbar, and the list
    // scrolled sideways with the switches cut off.
    return QSize(0, ROW_HEIGHT);
}

void KubikModDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    const int row = sourceRow(index);
    if (row < 0 || row >= int(m_model->size())) {
        return;
    }
    const Mod& mod = m_model->at(row);
    const bool managed = isManaged(row);
    const bool enabled = mod.enabled();
    const QRect rect = option.rect.adjusted(0, GAP, 0, -GAP);

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    QPainterPath shape;
    shape.addRoundedRect(QRectF(rect), 6, 6);
    painter->fillPath(shape, color(option.state & QStyle::State_MouseOver ? "block" : "void.panel"));

    // The mod's own icon (read from its jar once, then cached), or the loader's.
    const QRect iconRect(rect.left() + PAD, rect.center().y() - ICON / 2 + 1, ICON, ICON);
    QPixmap icon = mod.icon(QSize(ICON, ICON) * painter->device()->devicePixelRatioF(), Qt::KeepAspectRatio);
    if (icon.isNull()) {
        icon = QIcon::fromTheme("loadermods").pixmap(QSize(ICON, ICON), painter->device()->devicePixelRatioF());
    }
    painter->setOpacity(enabled ? 1.0 : 0.45);
    painter->drawPixmap(iconRect, icon);
    painter->setOpacity(1.0);

    // Right side: the server's badge, or the player's switch and Remove.
    int right = rect.right() - PAD;
    const QFont smallFont = font(KubikBrand::bodyFamily(), 12, QFont::Normal);
    const QFontMetrics small(smallFont);
    if (managed) {
        const QString badge = tr("Server mod");
        const QFont badgeFont = font(KubikBrand::displayFamily(), 12, QFont::DemiBold);
        const int width = QFontMetrics(badgeFont).horizontalAdvance(badge) + 20;
        const QRect pill(right - width + 1, rect.center().y() - 11, width, 22);
        QPainterPath path;
        path.addRoundedRect(QRectF(pill), 11, 11);
        painter->fillPath(path, color("void"));
        painter->setPen(color("grass"));
        painter->setFont(badgeFont);
        painter->drawText(pill, Qt::AlignCenter, badge);
        right = pill.left() - 12;
    } else {
        const QRect toggle = switchRect(rect);
        QPainterPath track;
        track.addRoundedRect(QRectF(toggle), SWITCH_H / 2.0, SWITCH_H / 2.0);
        painter->fillPath(track, color(enabled ? "grass" : "block"));
        const qreal knob = SWITCH_H - 6;
        const qreal x = enabled ? toggle.right() - 3 - knob + 1 : toggle.left() + 3;
        painter->setPen(Qt::NoPen);
        painter->setBrush(enabled ? color("void") : color("bone.muted"));
        painter->drawEllipse(QRectF(x, toggle.top() + 3, knob, knob));

        const QRect remove = removeRect(rect);
        painter->setFont(font(KubikBrand::displayFamily(), 13, QFont::DemiBold));
        painter->setPen(color("bone.muted"));
        painter->drawText(remove, Qt::AlignRight | Qt::AlignVCenter, tr("Remove"));
        right = remove.left() - 12;
    }

    // The name, and under it the version and whose it is.
    const int textLeft = iconRect.right() + 14;
    const int textWidth = qMax(40, right - textLeft);
    const QFont nameFont = font(KubikBrand::bodyFamily(), 14, QFont::DemiBold);
    painter->setFont(nameFont);
    painter->setPen(color(enabled ? "bone" : "bone.faint"));
    painter->drawText(QRect(textLeft, rect.center().y() - 19, textWidth, 20), Qt::AlignLeft | Qt::AlignVCenter,
                      QFontMetrics(nameFont).elidedText(mod.name(), Qt::ElideRight, textWidth));

    QStringList second;
    if (!mod.version().isEmpty()) {
        second << mod.version();
    }
    if (managed) {
        second << (m_serverName.isEmpty() ? tr("installed for your server") : tr("installed for %1").arg(m_serverName));
    } else if (!m_modpack) {
        second << tr("added by you");
    }
    if (!enabled) {
        second << tr("off");
    }
    painter->setFont(smallFont);
    painter->setPen(color("bone.muted"));
    painter->drawText(QRect(textLeft, rect.center().y() + 2, textWidth, 18), Qt::AlignLeft | Qt::AlignVCenter,
                      small.elidedText(second.join(QStringLiteral(" · ")), Qt::ElideRight, textWidth));
    painter->restore();
}

bool KubikModDelegate::editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index)
{
    if (event->type() != QEvent::MouseButtonRelease) {
        return QStyledItemDelegate::editorEvent(event, model, option, index);
    }
    const auto* mouse = static_cast<QMouseEvent*>(event);
    const int row = sourceRow(index);
    if (mouse->button() != Qt::LeftButton || isManaged(row)) {
        return false;
    }
    const QRect rect = option.rect.adjusted(0, GAP, 0, -GAP);
    const QPoint pos = mouse->position().toPoint();
    if (switchRect(rect).adjusted(-6, -8, 6, 8).contains(pos)) {
        emit toggleRequested(row);
        return true;
    }
    if (removeRect(rect).contains(pos)) {
        emit removeRequested(row);
        return true;
    }
    return false;
}

bool KubikModDelegate::helpEvent(QHelpEvent* event, QAbstractItemView* view, const QStyleOptionViewItem& option, const QModelIndex& index)
{
    const int row = sourceRow(index);
    if (event->type() != QEvent::ToolTip || row < 0 || row >= int(m_model->size())) {
        return QStyledItemDelegate::helpEvent(event, view, option, index);
    }
    QString tip = m_model->at(row).fileinfo().fileName();
    if (isManaged(row)) {
        tip += "\n" + tr("KubikLauncher puts this in for the server you join and takes it out for a server that does not need it.");
    }
    QToolTip::showText(event->globalPos(), tip, view);
    return true;
}

// ── The tab ──────────────────────────────────────────────────────────────────

KubikInstanceMods::KubikInstanceMods(QWidget* parent) : QWidget(parent), m_proxy(new QSortFilterProxyModel(this))
{
    m_count = new QLabel(this);
    m_count->setObjectName("kubikSectionTitle");
    m_add = new QPushButton(QIcon::fromTheme("new"), QString(), this);
    m_add->setCursor(Qt::PointingHandCursor);
    connect(m_add, &QPushButton::clicked, this, &KubikInstanceMods::addFiles);
    m_folder = new QPushButton(QIcon::fromTheme("viewfolder"), QString(), this);
    m_folder->setProperty("kubikRole", "quiet");
    m_folder->setCursor(Qt::PointingHandCursor);
    connect(m_folder, &QPushButton::clicked, this, [this] {
        if (m_instance) {
            DesktopServices::openPath(m_instance->modsRoot(), true);
        }
    });
    auto* header = new QHBoxLayout();
    header->setSpacing(8);
    header->addWidget(m_count);
    header->addStretch();
    header->addWidget(m_add);
    header->addWidget(m_folder);

    m_note = new QLabel(this);
    m_note->setObjectName("kubikPageNote");
    m_note->setWordWrap(true);

    m_proxy->setSortCaseSensitivity(Qt::CaseInsensitive);
    m_proxy->setSortRole(Qt::DisplayRole);
    m_list = new QListView(this);
    m_list->setObjectName("kubikModList");
    m_list->setModel(m_proxy);
    m_list->setSelectionMode(QAbstractItemView::NoSelection);
    m_list->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_list->setMouseTracking(true);
    m_list->setUniformItemSizes(true);
    m_list->setFrameShape(QFrame::NoFrame);
    m_list->setAttribute(Qt::WA_MacShowFocusRect, false);

    m_empty = new QLabel(this);
    m_empty->setObjectName("kubikPageBody");
    m_empty->setWordWrap(true);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);
    layout->addLayout(header);
    layout->addWidget(m_note);
    layout->addSpacing(4);
    layout->addWidget(m_list, 1);
    layout->addWidget(m_empty);
    layout->addStretch();
}

void KubikInstanceMods::setInstance(MinecraftInstance* instance)
{
    if (m_model && isVisible()) {
        m_model->stopWatching();
    }
    m_instance = instance;
    m_model = instance ? instance->loaderModList() : nullptr;
    m_proxy->setSourceModel(m_model);
    m_proxy->setSortRole(Qt::DisplayRole);
    m_proxy->sort(ModFolderModel::NameColumn);
    m_list->setModelColumn(ModFolderModel::NameColumn);

    delete m_delegate;
    m_delegate = nullptr;
    if (m_model) {
        m_delegate = new KubikModDelegate(m_model, this);
        m_delegate->setModpack(instance->isManagedPack());
        connect(m_delegate, &KubikModDelegate::toggleRequested, this, [this](int row) {
            if (m_model && row < int(m_model->size())) {
                m_model->setResourceEnabled({ m_model->index(row, 0) }, EnableAction::TOGGLE);
            }
        });
        connect(m_delegate, &KubikModDelegate::removeRequested, this, &KubikInstanceMods::remove);
        m_list->setItemDelegate(m_delegate);
        connect(m_model, &ModFolderModel::updateFinished, this, &KubikInstanceMods::refresh, Qt::UniqueConnection);
        if (isVisible()) {
            m_model->startWatching();
        }
    }
    readManaged();
    refresh();
}

void KubikInstanceMods::readManaged()
{
    if (!m_delegate || !m_instance) {
        return;
    }
    QSet<QString> managed;
    QString serverName;
    QFile sidecar(FS::PathCombine(m_instance->instanceRoot(), ".kubik", "managed.json"));
    if (sidecar.open(QIODevice::ReadOnly)) {
        if (const auto state = Kubik::readManagedState(sidecar.readAll())) {
            for (const auto& file : state->placed) {
                managed.insert(file.filename);
            }
            serverName = m_serverName ? m_serverName(state->serverId) : QString();
        }
    }
    m_delegate->setManaged(managed, serverName);
}

void KubikInstanceMods::refresh()
{
    readManaged();
    const int count = m_model ? int(m_model->size()) : 0;
    m_count->setText(tr("%n mod(s)", nullptr, count));
    m_add->setText(tr("Add mod file…"));
    m_folder->setText(tr("Open mods folder"));
    m_note->setText(m_instance && m_instance->isManagedPack()
                        ? tr("Server mods switch with the server you join. The modpack's mods and the ones you add stay.")
                        : tr("Server mods switch with the server you join. The ones you add stay, whichever server you play."));
    m_empty->setText(tr("No mods yet. Mods a server needs appear here after you play it, and you can add your own."));
    m_list->setVisible(count > 0);
    m_empty->setVisible(count == 0);
    m_proxy->sort(ModFolderModel::NameColumn);
    m_list->viewport()->update();
}

void KubikInstanceMods::addFiles()
{
    if (!m_model) {
        return;
    }
    const QStringList files = QFileDialog::getOpenFileNames(this, tr("Add mod files"), QString(), tr("Mods (*.jar *.zip)"));
    for (const auto& file : files) {
        m_model->installResource(file);
    }
}

void KubikInstanceMods::remove(int sourceRow)
{
    if (!m_model || sourceRow >= int(m_model->size())) {
        return;
    }
    const QString name = m_model->at(sourceRow).name();
    const auto answer = KubikConfirmDialog::ask(window(), tr("Remove %1?").arg(name),
                                                tr("The mod's file is deleted from this instance. You can add it again any time."),
                                                tr("Remove"), tr("Cancel"), true);
    if (answer == KubikConfirmDialog::Answer::Primary) {
        m_model->deleteResources({ m_model->index(sourceRow, 0) });
    }
}

void KubikInstanceMods::showEvent(QShowEvent* event)
{
    if (m_model) {
        m_model->startWatching();
    }
    refresh();
    QWidget::showEvent(event);
}

void KubikInstanceMods::hideEvent(QHideEvent* event)
{
    if (m_model) {
        m_model->stopWatching();
    }
    QWidget::hideEvent(event);
}

void KubikInstanceMods::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange) {
        refresh();
    }
    QWidget::changeEvent(event);
}
