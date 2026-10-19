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

#include "KubikServersPage.h"

#include <QDesktopServices>
#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QStackedWidget>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>

#include "Application.h"
#include "kubik/KubikAccount.h"
#include "kubik/KubikApi.h"
#include "kubik/KubikPlay.h"
#include "kubik/KubikWindowChrome.h"
#include "ui/kubik/KubikServerTiles.h"

namespace {

constexpr int MARGIN = 32;
constexpr int GRID_SPACING = 16;
// Shown again after this long away, the list is fetched again; on screen it refreshes every minute.
constexpr qint64 STALE_MS = 30 * 1000;
constexpr int AUTO_REFRESH_MS = 60 * 1000;

enum BodyState { Loading = 0, Grid = 1, Empty = 2, Error = 3, NoMatches = 4 };

/** A column at the page's left edge: a mark, a headline, a sentence, and the buttons under them. */
QWidget* messageColumn(QWidget* parent, QLabel*& title, QLabel*& body, QHBoxLayout*& buttons)
{
    auto* page = new QWidget(parent);
    auto* column = new QWidget(page);
    column->setFixedWidth(480);
    auto* layout = new QVBoxLayout(column);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);
    auto* mark = new QLabel(column);
    mark->setPixmap(APPLICATION->logo().pixmap(88, 88));
    layout->addWidget(mark);
    title = new QLabel(column);
    title->setObjectName("kubikPageTitle");
    layout->addWidget(title);
    body = new QLabel(column);
    body->setObjectName("kubikPageBody");
    body->setWordWrap(true);
    layout->addWidget(body);
    buttons = new QHBoxLayout();
    buttons->setSpacing(10);
    layout->addSpacing(8);
    layout->addLayout(buttons);
    auto* outer = new QVBoxLayout(page);
    outer->setContentsMargins(MARGIN, 0, MARGIN, 0);
    outer->addStretch(2);
    outer->addWidget(column, 0, Qt::AlignLeft);
    outer->addStretch(3);
    return page;
}

}  // namespace

KubikServersPage::KubikServersPage(QWidget* parent)
    : QWidget(parent)
    , m_model(new KubikServerModel(this))
    , m_proxy(new QSortFilterProxyModel(this))
    , m_grid(new KubikServerGrid(this))
    , m_delegate(new KubikServerDelegate(this))
    , m_autoRefresh(new QTimer(this))
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
    m_proxy->setSourceModel(m_model);
    m_proxy->setFilterRole(KubikServerModel::SearchRole);
    m_proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
    connect(m_search, &QLineEdit::textChanged, this, [this](const QString& text) {
        m_proxy->setFilterFixedString(text.trimmed());
        updateState();
    });

    m_refresh = new QPushButton(this);
    m_refresh->setProperty("kubikRole", "quiet");
    m_refresh->setIcon(QIcon::fromTheme("refresh"));
    m_refresh->setCursor(Qt::PointingHandCursor);
    connect(m_refresh, &QPushButton::clicked, this, &KubikServersPage::refresh);

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
    header->addWidget(m_refresh, 0, Qt::AlignVCenter);

    // ── The grid ────────────────────────────────────────────────────────────
    m_grid->setModel(m_proxy);
    m_grid->setItemDelegate(m_delegate);
    m_grid->viewport()->installEventFilter(this);
    connect(m_grid, &QListView::clicked, this,
            [this](const QModelIndex& index) { emit serverOpened(index.data(KubikServerModel::IdRole).toString()); });
    auto* gridPage = new QWidget(this);
    auto* gridLayout = new QVBoxLayout(gridPage);
    gridLayout->setContentsMargins(MARGIN - GRID_SPACING / 2, 0, MARGIN - GRID_SPACING / 2, 0);
    gridLayout->addWidget(m_grid);

    // ── Loading, empty, error, no matches ───────────────────────────────────
    auto* loadingPage = new QWidget(this);
    m_loading = new QLabel(loadingPage);
    m_loading->setObjectName("kubikPageBody");
    auto* loadingLayout = new QVBoxLayout(loadingPage);
    loadingLayout->setContentsMargins(MARGIN, 24, MARGIN, 0);
    loadingLayout->addWidget(m_loading);
    loadingLayout->addStretch();

    QHBoxLayout* emptyButtons = nullptr;
    auto* emptyPage = messageColumn(this, m_emptyTitle, m_emptyBody, emptyButtons);
    m_emptyCreate = new QPushButton(emptyPage);
    m_emptyCreate->setProperty("kubikRole", "primary");
    m_emptyCreate->setCursor(Qt::PointingHandCursor);
    connect(m_emptyCreate, &QPushButton::clicked, this,
            [] { QDesktopServices::openUrl(QUrl(Kubik::webOrigin() + "/dashboard/servers/new")); });
    emptyButtons->addWidget(m_emptyCreate);
    emptyButtons->addStretch();

    QLabel* errorTitle = nullptr;
    QHBoxLayout* errorButtons = nullptr;
    auto* errorPage = messageColumn(this, errorTitle, m_errorText, errorButtons);
    errorTitle->hide();
    m_retry = new QPushButton(errorPage);
    m_retry->setProperty("kubikRole", "primary");
    m_retry->setCursor(Qt::PointingHandCursor);
    connect(m_retry, &QPushButton::clicked, this, &KubikServersPage::refresh);
    errorButtons->addWidget(m_retry);
    errorButtons->addStretch();

    auto* noMatchesPage = new QWidget(this);
    m_noMatches = new QLabel(noMatchesPage);
    m_noMatches->setObjectName("kubikPageBody");
    auto* noMatchesLayout = new QVBoxLayout(noMatchesPage);
    noMatchesLayout->setContentsMargins(MARGIN, 24, MARGIN, 0);
    noMatchesLayout->addWidget(m_noMatches);
    noMatchesLayout->addStretch();

    m_body = new QStackedWidget(this);
    m_body->insertWidget(Loading, loadingPage);
    m_body->insertWidget(Grid, gridPage);
    m_body->insertWidget(Empty, emptyPage);
    m_body->insertWidget(Error, errorPage);
    m_body->insertWidget(NoMatches, noMatchesPage);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);
    layout->addLayout(header);
    layout->addWidget(m_body, 1);

    m_autoRefresh->setInterval(AUTO_REFRESH_MS);
    connect(m_autoRefresh, &QTimer::timeout, this, &KubikServersPage::refresh);
    // A new sign-in is a new list; a sign-out empties it.
    connect(KubikAccount::get(), &KubikAccount::changed, this, [this] {
        m_model->setServers({});
        m_loaded = false;
        m_error.clear();
        m_fetchedAt.invalidate();
        if (KubikAccount::get()->isSignedIn() && isVisible()) {
            refresh();
        }
        updateState();
    });

    retranslate();
    updateState();
}

const Kubik::Server* KubikServersPage::server(const QString& id) const
{
    return m_model->serverById(id);
}

void KubikServersPage::refresh()
{
    auto* account = KubikAccount::get();
    if (!account->isSignedIn() || m_fetching) {
        return;
    }
    m_fetching = true;
    updateState();
    Kubik::apiGet("servers", account->token(), this, [this](const Kubik::Reply& reply) {
        m_fetching = false;
        m_fetchedAt.start();
        if (reply.ok()) {
            if (auto servers = Kubik::parseServers(reply.body)) {
                m_model->setServers(*servers);
                m_loaded = true;
                m_error.clear();
            } else {
                m_error = tr("kubik.gg sent something KubikLauncher does not understand. Check for a launcher update.");
            }
        } else {
            const auto error = Kubik::parseApiError(reply.status, reply.body);
            if (error.code == "unauthorized") {
                KubikAccount::get()->tokenRejected();
                return;
            }
            m_error = KubikPlay::describe(error);
        }
        updateState();
    });
}

void KubikServersPage::retranslate()
{
    m_title->setText(tr("Your servers"));
    m_search->setPlaceholderText(tr("Search servers"));
    m_refresh->setToolTip(tr("Check your servers again"));
    m_loading->setText(tr("Getting your servers from kubik.gg…"));
    m_emptyTitle->setText(tr("No servers yet"));
    m_emptyBody->setText(
        tr("Make a server on kubik.gg and it shows up here. KubikLauncher then installs the matching Minecraft and "
           "mods for you, and joins it."));
    m_emptyCreate->setText(tr("Create a server on kubik.gg"));
    m_retry->setText(tr("Try again"));
    updateState();
}

void KubikServersPage::updateState()
{
    const int total = m_model->rowCount();
    const int shown = m_proxy->rowCount();
    m_subtitle->setText(tr("%n server(s)", nullptr, total));
    m_subtitle->setVisible(m_loaded && total > 0);
    m_search->setVisible(m_loaded && total > 1);
    m_refresh->setEnabled(!m_fetching);

    if (!m_loaded) {
        if (!m_error.isEmpty() && !m_fetching) {
            m_errorText->setText(m_error);
            m_body->setCurrentIndex(Error);
        } else {
            m_body->setCurrentIndex(Loading);
        }
    } else if (total == 0) {
        m_body->setCurrentIndex(Empty);
    } else if (shown == 0) {
        m_noMatches->setText(tr("No server matches \"%1\".").arg(m_search->text().trimmed()));
        m_body->setCurrentIndex(NoMatches);
    } else {
        m_body->setCurrentIndex(Grid);
    }
}

bool KubikServersPage::eventFilter(QObject* watched, QEvent* event)
{
    if (watched != m_grid->viewport()) {
        return QWidget::eventFilter(watched, event);
    }
    switch (event->type()) {
        case QEvent::MouseMove:
            m_delegate->setHoveredIndex(m_grid->indexAt(static_cast<QMouseEvent*>(event)->position().toPoint()));
            m_grid->viewport()->update();
            break;
        case QEvent::Leave:
            m_delegate->setHoveredIndex(QModelIndex());
            m_grid->viewport()->update();
            break;
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease: {
            // Play is drawn on the tile; its clicks are taken here, before the view turns them into "open".
            auto* mouse = static_cast<QMouseEvent*>(event);
            const QPoint pos = mouse->position().toPoint();
            const QModelIndex index = m_grid->indexAt(pos);
            if (mouse->button() != Qt::LeftButton || !index.isValid() ||
                !KubikServerDelegate::playRect(m_grid->visualRect(index)).contains(pos)) {
                m_pressedPlayId.clear();
                break;
            }
            const QString id = index.data(KubikServerModel::IdRole).toString();
            if (event->type() == QEvent::MouseButtonPress) {
                m_pressedPlayId = id;
            } else if (m_pressedPlayId == id) {
                m_pressedPlayId.clear();
                emit playRequested(id);
            }
            return true;
        }
        default:
            break;
    }
    return QWidget::eventFilter(watched, event);
}

void KubikServersPage::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange) {
        retranslate();
    }
    QWidget::changeEvent(event);
}

void KubikServersPage::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    if (!m_fetchedAt.isValid() || m_fetchedAt.elapsed() > STALE_MS) {
        refresh();
    }
    m_autoRefresh->start();
}

void KubikServersPage::hideEvent(QHideEvent* event)
{
    m_autoRefresh->stop();
    QWidget::hideEvent(event);
}
