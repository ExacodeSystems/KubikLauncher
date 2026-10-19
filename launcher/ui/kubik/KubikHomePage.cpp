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

#include "KubikHomePage.h"

#include <QActionGroup>
#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMouseEvent>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>

#include "Application.h"
#include "BaseInstance.h"
#include "InstanceList.h"
#include "kubik/KubikWindowChrome.h"
#include "settings/SettingsObject.h"
#include "ui/instanceview/InstanceProxyModel.h"
#include "ui/instanceview/InstanceView.h"
#include "ui/kubik/KubikInstanceDelegate.h"
#include "ui/kubik/KubikInstanceInfo.h"

namespace {

// Tiles are at least this wide; the grid stretches them to fill each row.
constexpr int TILE_MIN_WIDTH = 220;
constexpr int GRID_SPACING = 16;
// The page's side margin, shared by the header and the grid so their left edges line up.
constexpr int MARGIN = 32;

enum BodyState { Grid = 0, Empty = 1, NoMatches = 2 };

QString instanceIdAt(InstanceView* view, const QPoint& pos)
{
    return view->indexAt(pos).data(InstanceList::InstanceIDRole).toString();
}

}  // namespace

KubikHomePage::KubikHomePage(InstanceView* view, InstanceProxyModel* proxy, QWidget* parent)
    : QWidget(parent), m_view(view), m_proxy(proxy), m_delegate(new KubikInstanceDelegate(this))
{
    setObjectName("kubikPage");
    setAttribute(Qt::WA_StyledBackground);

    // ── Header ──────────────────────────────────────────────────────────────
    m_title = new QLabel(this);
    m_title->setObjectName("kubikPageTitle");
    m_subtitle = new QLabel(this);
    m_subtitle->setObjectName("kubikPageSubtitle");

    m_search = new QLineEdit(this);
    m_search->setObjectName("kubikSearch");
    m_search->setClearButtonEnabled(true);
    connect(m_search, &QLineEdit::textChanged, this, [this](const QString& text) {
        m_proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
        m_proxy->setFilterFixedString(text.trimmed());
        updateState();
    });

    // Sort: the same setting the Settings page writes, in fewer words.
    m_sort = new QPushButton(this);
    m_sort->setProperty("kubikRole", "quiet");
    auto* sortMenu = new QMenu(m_sort);
    auto* sortGroup = new QActionGroup(sortMenu);
    const QList<QPair<QString, const char*>> sortModes = {
        { "LastLaunch", QT_TR_NOOP("Recently played") },
        { "Name", QT_TR_NOOP("Name") },
        { "Playtime", QT_TR_NOOP("Most played") },
    };
    for (const auto& [mode, label] : sortModes) {
        auto* action = sortMenu->addAction(tr(label));
        action->setCheckable(true);
        action->setData(mode);
        action->setChecked(APPLICATION->settings()->get("InstSortMode").toString() == mode);
        sortGroup->addAction(action);
    }
    connect(sortGroup, &QActionGroup::triggered, this, [this](QAction* action) {
        APPLICATION->settings()->set("InstSortMode", action->data());
        m_proxy->invalidate();
        m_proxy->sort(0);
        retranslate();
    });
    m_sort->setMenu(sortMenu);

    auto* titles = new QVBoxLayout();
    titles->setSpacing(2);
    titles->addWidget(m_title);
    titles->addWidget(m_subtitle);

    auto* header = new QHBoxLayout();
    header->setContentsMargins(MARGIN, 24 + KubikWindowChrome::titleBarInset(), MARGIN, 8);
    header->setSpacing(10);
    header->addLayout(titles);
    header->addStretch();
    header->addWidget(m_search, 0, Qt::AlignVCenter);
    header->addWidget(m_sort, 0, Qt::AlignVCenter);

    // ── The grid ────────────────────────────────────────────────────────────
    m_view->setItemDelegate(m_delegate);
    // The grid's own spacing doubles as its margin; the rest of the page margin is the layout's.
    m_view->setTileMetrics(TILE_MIN_WIDTH, GRID_SPACING);
    m_view->setMouseTracking(true);
    m_view->viewport()->setMouseTracking(true);
    m_view->viewport()->installEventFilter(this);
    connect(m_view, &InstanceView::clicked, this,
            [this](const QModelIndex& index) { emit instanceOpened(index.data(InstanceList::InstanceIDRole).toString()); });
    connect(KubikInstanceInfo::instance(), &KubikInstanceInfo::summaryChanged, this, &KubikHomePage::repaintTiles);

    auto* gridPage = new QWidget(this);
    auto* gridLayout = new QVBoxLayout(gridPage);
    gridLayout->setContentsMargins(MARGIN - GRID_SPACING, 0, MARGIN - GRID_SPACING, 0);
    gridLayout->addWidget(m_view);

    // ── Empty state: what an instance is, and where one comes from ──────────
    auto* emptyPage = new QWidget(this);
    auto* emptyColumn = new QWidget(emptyPage);
    emptyColumn->setFixedWidth(480);
    auto* emptyLayout = new QVBoxLayout(emptyColumn);
    emptyLayout->setContentsMargins(0, 0, 0, 0);
    emptyLayout->setSpacing(12);
    auto* mark = new QLabel(emptyColumn);
    mark->setPixmap(APPLICATION->logo().pixmap(88, 88));
    emptyLayout->addWidget(mark);
    m_emptyTitle = new QLabel(emptyColumn);
    m_emptyTitle->setObjectName("kubikPageTitle");
    emptyLayout->addWidget(m_emptyTitle);
    m_emptyBody = new QLabel(emptyColumn);
    m_emptyBody->setObjectName("kubikPageBody");
    m_emptyBody->setWordWrap(true);
    emptyLayout->addWidget(m_emptyBody);
    m_emptyServers = new QPushButton(emptyColumn);
    m_emptyServers->setProperty("kubikRole", "primary");
    m_emptyServers->setCursor(Qt::PointingHandCursor);
    connect(m_emptyServers, &QPushButton::clicked, this, &KubikHomePage::serversRequested);
    auto* emptyButtons = new QHBoxLayout();
    emptyButtons->setSpacing(10);
    emptyButtons->addWidget(m_emptyServers);
    emptyButtons->addStretch();
    emptyLayout->addSpacing(8);
    emptyLayout->addLayout(emptyButtons);
    auto* emptyOuter = new QVBoxLayout(emptyPage);
    emptyOuter->setContentsMargins(MARGIN, 0, MARGIN, 0);
    emptyOuter->addStretch(2);
    emptyOuter->addWidget(emptyColumn, 0, Qt::AlignLeft);
    emptyOuter->addStretch(3);

    // ── No matches for a search ─────────────────────────────────────────────
    auto* noMatchesPage = new QWidget(this);
    m_noMatches = new QLabel(noMatchesPage);
    m_noMatches->setObjectName("kubikPageBody");
    auto* noMatchesLayout = new QVBoxLayout(noMatchesPage);
    noMatchesLayout->setContentsMargins(MARGIN, 24, MARGIN, 0);
    noMatchesLayout->addWidget(m_noMatches);
    noMatchesLayout->addStretch();

    m_body = new QStackedWidget(this);
    m_body->insertWidget(Grid, gridPage);
    m_body->insertWidget(Empty, emptyPage);
    m_body->insertWidget(NoMatches, noMatchesPage);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);
    layout->addLayout(header);
    layout->addWidget(m_body, 1);

    // Keep the count, the empty state and the running tiles in step with the instance list.
    const auto listChanged = [this] {
        updateState();
        watchRunningInstances();
    };
    connect(m_proxy, &QAbstractItemModel::rowsInserted, this, listChanged);
    connect(m_proxy, &QAbstractItemModel::rowsRemoved, this, listChanged);
    connect(m_proxy, &QAbstractItemModel::modelReset, this, listChanged);

    retranslate();
    updateState();
    watchRunningInstances();
}

void KubikHomePage::retranslate()
{
    m_title->setText(tr("Your instances"));
    m_search->setPlaceholderText(tr("Search instances"));

    QString sortLabel;
    for (auto* action : m_sort->menu()->actions()) {
        if (action->isChecked()) {
            sortLabel = action->text();
        }
    }
    m_sort->setText(sortLabel.isEmpty() ? tr("Sort") : tr("Sort: %1").arg(sortLabel));

    m_emptyTitle->setText(tr("No instances yet"));
    m_emptyBody->setText(
        tr("An instance is one Minecraft setup: its own version, mods, worlds and settings. KubikLauncher makes one the "
           "first time you play a server, and servers that need the same setup share it."));
    m_emptyServers->setText(tr("See your servers"));
    updateState();
}

void KubikHomePage::updateState()
{
    const int total = APPLICATION->instances()->rowCount();
    const int shown = m_proxy->rowCount();
    m_subtitle->setText(tr("%n instance(s)", nullptr, total));
    m_subtitle->setVisible(total > 0);
    m_search->setVisible(total > 0);
    m_sort->setVisible(total > 1);

    if (total == 0) {
        m_body->setCurrentIndex(Empty);
    } else if (shown == 0) {
        m_noMatches->setText(tr("No instance matches \"%1\".").arg(m_search->text().trimmed()));
        m_body->setCurrentIndex(NoMatches);
    } else {
        m_body->setCurrentIndex(Grid);
    }
}

void KubikHomePage::watchRunningInstances()
{
    // The instance list model emits nothing when an instance starts or stops, so each instance is
    // watched directly, once (UniqueConnection), to repaint its Play/Stop button.
    auto* instances = APPLICATION->instances();
    for (int row = 0; row < instances->rowCount(); ++row) {
        auto* instance = static_cast<BaseInstance*>(instances->index(row).data(InstanceList::InstancePointerRole).value<void*>());
        if (instance) {
            connect(instance, &BaseInstance::runningStatusChanged, this, &KubikHomePage::repaintTiles, Qt::UniqueConnection);
        }
    }
}

void KubikHomePage::repaintTiles()
{
    m_view->viewport()->update();
}

bool KubikHomePage::eventFilter(QObject* watched, QEvent* event)
{
    if (watched != m_view->viewport()) {
        return QWidget::eventFilter(watched, event);
    }
    switch (event->type()) {
        case QEvent::MouseMove: {
            const auto pos = static_cast<QMouseEvent*>(event)->position().toPoint();
            m_delegate->setHoveredIndex(m_view->indexAt(pos));
            repaintTiles();
            break;
        }
        case QEvent::Leave:
            m_delegate->setHoveredIndex(QModelIndex());
            repaintTiles();
            break;
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease: {
            // Play is a button drawn on the tile. Its clicks are taken here, before InstanceView
            // would turn them into a selection or the start of a drag.
            auto* mouse = static_cast<QMouseEvent*>(event);
            const QPoint pos = mouse->position().toPoint();
            const QModelIndex index = m_view->indexAt(pos);
            if (mouse->button() != Qt::LeftButton || !index.isValid() ||
                !KubikInstanceDelegate::playRect(m_view->visualRect(index)).contains(pos)) {
                m_pressedPlayId.clear();
                break;
            }
            if (event->type() == QEvent::MouseButtonPress) {
                m_pressedPlayId = instanceIdAt(m_view, pos);
            } else if (m_pressedPlayId == instanceIdAt(m_view, pos)) {
                emit playRequested(m_pressedPlayId);
                m_pressedPlayId.clear();
            }
            return true;
        }
        default:
            break;
    }
    return QWidget::eventFilter(watched, event);
}

void KubikHomePage::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange) {
        for (auto* action : m_sort->menu()->actions()) {
            const QString mode = action->data().toString();
            action->setText(mode == "LastLaunch" ? tr("Recently played") : mode == "Playtime" ? tr("Most played") : tr("Name"));
        }
        retranslate();
    } else if (event->type() == QEvent::ActivationChange && window()->isActiveWindow()) {
        // Coming back from an instance window, where mods may have been added: count again.
        KubikInstanceInfo::instance()->invalidateAll();
        repaintTiles();
    }
    QWidget::changeEvent(event);
}
