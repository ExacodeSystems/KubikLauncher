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

#include <QClipboard>
#include <QDesktopServices>
#include <QEvent>
#include <QFrame>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
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
#include "kubik/KubikProfiles.h"
#include "kubik/KubikWindowChrome.h"
#include "ui/kubik/KubikInstanceInfo.h"
#include "ui/kubik/KubikServerTiles.h"
#include "ui/pages/instance/ServerPingTask.h"

namespace {

constexpr int MARGIN = 32;
constexpr int GRID_SPACING = 16;
// Shown again after this long away, the list is fetched again; on screen it refreshes every minute.
constexpr qint64 STALE_MS = 30 * 1000;
constexpr int AUTO_REFRESH_MS = 60 * 1000;
constexpr int HERO_ART = 104;
// Java servers are reached through mc-router, which picks the server by the hostname in the handshake.
constexpr int JAVA_PORT = 25565;

qint64 lastPlayedMs(const QString& serverId)
{
    const QDateTime when = Kubik::lastPlayed(serverId);
    return when.isValid() ? when.toMSecsSinceEpoch() : 0;
}

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

    // A refresh that failed after the list loaded: said under the header, the tiles stay.
    m_stale = new QLabel(this);
    m_stale->setObjectName("kubikPageNote");
    m_staleRetry = new QPushButton(this);
    m_staleRetry->setProperty("kubikRole", "link");
    m_staleRetry->setCursor(Qt::PointingHandCursor);
    connect(m_staleRetry, &QPushButton::clicked, this, &KubikServersPage::refresh);
    auto* staleRow = new QHBoxLayout();
    staleRow->setContentsMargins(MARGIN, 0, MARGIN, 0);
    staleRow->setSpacing(8);
    staleRow->addWidget(m_stale);
    staleRow->addWidget(m_staleRetry);
    staleRow->addStretch();

    // ── Jump back in: the server played last, with its own Play ────────────
    m_hero = new QFrame(this);
    m_hero->setObjectName("kubikHero");
    m_heroArt = new QLabel(m_hero);
    m_heroArt->setFixedSize(HERO_ART, HERO_ART);
    m_heroEyebrow = new QLabel(m_hero);
    m_heroEyebrow->setObjectName("kubikStepLabel");
    m_heroName = new QLabel(m_hero);
    m_heroName->setObjectName("kubikHeroName");
    m_heroName->setTextFormat(Qt::PlainText);
    m_heroLine = new QLabel(m_hero);
    m_heroLine->setObjectName("kubikPageBody");
    m_heroLine->setTextFormat(Qt::PlainText);
    m_heroStatus = new QLabel(m_hero);
    m_heroStatus->setObjectName("kubikPageSubtitle");
    m_heroPlay = new QPushButton(m_hero);
    m_heroPlay->setObjectName("kubikHeroPlay");
    m_heroPlay->setProperty("kubikRole", "primary");
    m_heroPlay->setCursor(Qt::PointingHandCursor);
    connect(m_heroPlay, &QPushButton::clicked, this, [this] { emit playRequested(m_heroId); });
    m_heroOpen = new QPushButton(m_hero);
    m_heroOpen->setProperty("kubikRole", "quiet");
    m_heroOpen->setCursor(Qt::PointingHandCursor);
    connect(m_heroOpen, &QPushButton::clicked, this, [this] { emit serverOpened(m_heroId); });
    auto* heroButtons = new QHBoxLayout();
    heroButtons->setSpacing(8);
    heroButtons->addWidget(m_heroPlay);
    heroButtons->addWidget(m_heroOpen);
    heroButtons->addStretch();
    auto* heroWords = new QVBoxLayout();
    heroWords->setSpacing(4);
    heroWords->addStretch();
    heroWords->addWidget(m_heroEyebrow);
    heroWords->addWidget(m_heroName);
    heroWords->addWidget(m_heroLine);
    heroWords->addWidget(m_heroStatus);
    heroWords->addSpacing(8);
    heroWords->addLayout(heroButtons);
    heroWords->addStretch();
    auto* heroLayout = new QHBoxLayout(m_hero);
    heroLayout->setContentsMargins(14, 14, 20, 14);
    heroLayout->setSpacing(24);
    heroLayout->addWidget(m_heroArt, 0, Qt::AlignVCenter);
    heroLayout->addLayout(heroWords, 1);

    m_intro = new QLabel(this);
    m_intro->setObjectName("kubikPageBody");
    m_allTitle = new QLabel(this);
    m_allTitle->setObjectName("kubikSectionTitle");

    // ── The grid ────────────────────────────────────────────────────────────
    m_grid->setModel(m_proxy);
    m_grid->setItemDelegate(m_delegate);
    m_grid->viewport()->installEventFilter(this);
    connect(m_grid, &QListView::clicked, this,
            [this](const QModelIndex& index) { emit serverOpened(index.data(KubikServerModel::IdRole).toString()); });
    // Enter opens the focused tile and Space plays it; a right click offers the rest.
    m_grid->installEventFilter(this);
    m_grid->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_grid, &QWidget::customContextMenuRequested, this, &KubikServersPage::showMenu);
    auto* gridPage = new QWidget(this);
    auto* gridLayout = new QVBoxLayout(gridPage);
    gridLayout->setContentsMargins(0, 0, 0, 0);
    gridLayout->setSpacing(8);
    auto* heroRow = new QVBoxLayout();
    heroRow->setContentsMargins(MARGIN, 4, MARGIN, 10);
    heroRow->setSpacing(14);
    heroRow->addWidget(m_hero);
    heroRow->addWidget(m_intro);
    heroRow->addWidget(m_allTitle);
    auto* gridRow = new QVBoxLayout();
    gridRow->setContentsMargins(MARGIN - GRID_SPACING / 2, 0, MARGIN - GRID_SPACING / 2, 0);
    gridRow->addWidget(m_grid);
    gridLayout->addLayout(heroRow);
    gridLayout->addLayout(gridRow, 1);

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
    layout->addLayout(staleRow);
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

const KubikServerLive* KubikServersPage::live(const QString& id) const
{
    return m_model->live(id);
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
                Kubik::sortForHome(*servers, lastPlayedMs);
                m_model->setServers(*servers);
                m_loaded = true;
                m_error.clear();
                pingServers();
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
    m_staleRetry->setText(tr("Try again"));
    m_heroEyebrow->setText(tr("Jump back in"));
    m_heroPlay->setText(tr("Play"));
    m_heroOpen->setText(tr("Open"));
    m_intro->setText(tr("Pick a server: KubikLauncher installs what it needs and joins it for you."));
    m_allTitle->setText(tr("All servers"));
    updateState();
}

void KubikServersPage::updateHero()
{
    // The server played last that is still on the list. Searching hides it: the grid is the answer then.
    const Kubik::Server* last = nullptr;
    qint64 lastMs = 0;
    for (const auto& s : m_model->servers()) {
        const qint64 played = lastPlayedMs(s.id);
        if (played > lastMs) {
            last = &s;
            lastMs = played;
        }
    }
    const bool searching = !m_search->text().trimmed().isEmpty();
    m_hero->setVisible(last && !searching);
    m_intro->setVisible(!last && !searching);
    m_allTitle->setVisible(last && !searching);
    m_heroId = last ? last->id : QString();
    if (!last) {
        return;
    }
    const auto* live = m_model->live(last->id);
    const qreal dpr = devicePixelRatioF();
    QPixmap art(QSize(HERO_ART, HERO_ART) * dpr);
    art.setDevicePixelRatio(dpr);
    art.fill(Qt::transparent);
    {
        QPainter painter(&art);
        painter.setRenderHint(QPainter::Antialiasing);
        QPainterPath shape;
        shape.addRoundedRect(QRectF(0, 0, HERO_ART, HERO_ART), 8, 8);
        painter.setClipPath(shape);
        KubikServerDelegate::paintArt(&painter, QRect(0, 0, HERO_ART, HERO_ART), *last, live, false);
    }
    m_heroArt->setPixmap(art);
    m_heroName->setText(last->name);
    m_heroLine->setText(QStringLiteral("%1 · %2").arg(KubikServerDelegate::describe(*last), KubikInstanceInfo::playedAgo(lastMs)));
    m_heroStatus->setText(
        QString("<span style=\"color:%1\">●</span>&nbsp;%2")
            .arg(KubikServerDelegate::statusColour(*last).name(), KubikServerDelegate::statusLine(*last, live).toHtmlEscaped()));
    m_heroPlay->setVisible(last->profile.has_value() && !last->locked);
}

void KubikServersPage::pingServers()
{
    for (const auto& s : m_model->servers()) {
        if (!s.isRunning() || s.address.isEmpty() || s.edition == "bedrock" || m_pinging.contains(s.id)) {
            continue;
        }
        m_pinging.insert(s.id);
        auto* task = new ServerPingTask(s.address, JAVA_PORT);
        connect(task, &Task::succeeded, this, [this, task, id = s.id] {
            KubikServerLive live;
            live.online = task->m_outputOnlinePlayers;
            live.max = task->m_outputMaxPlayers;
            live.motd = task->m_outputMotd;
            if (!task->m_outputFavicon.isEmpty()) {
                live.favicon.loadFromData(task->m_outputFavicon, "PNG");
            }
            m_model->setLive(id, live);
            if (id == m_heroId) {
                updateHero();
            }
        });
        connect(task, &Task::finished, this, [this, task, id = s.id] {
            m_pinging.remove(id);
            task->deleteLater();
        });
        task->start();
    }
}

void KubikServersPage::showMenu(const QPoint& position)
{
    const QModelIndex index = m_grid->indexAt(position);
    const auto* s = index.data(Qt::UserRole).value<const Kubik::Server*>();
    if (!s) {
        return;
    }
    const QString id = s->id;
    const QString address = s->address;
    const QString dashboard = s->dashboardUrl;
    QMenu menu(this);
    if (s->profile && !s->locked) {
        menu.addAction(tr("Play"), this, [this, id] { emit playRequested(id); });
    }
    menu.addAction(tr("Open"), this, [this, id] { emit serverOpened(id); });
    if (!address.isEmpty()) {
        menu.addAction(tr("Copy address"), this, [address] { QGuiApplication::clipboard()->setText(address); });
    }
    menu.addAction(tr("Open on kubik.gg"), this, [dashboard] { QDesktopServices::openUrl(QUrl(dashboard)); });
    menu.exec(m_grid->viewport()->mapToGlobal(position));
}

void KubikServersPage::updateState()
{
    const int total = m_model->rowCount();
    const int shown = m_proxy->rowCount();
    m_subtitle->setText(tr("%n server(s)", nullptr, total));
    m_subtitle->setVisible(m_loaded && total > 0);
    m_search->setVisible(m_loaded && total > 1);
    m_refresh->setEnabled(!m_fetching);
    const bool stale = m_loaded && !m_error.isEmpty() && !m_fetching;
    m_stale->setText(tr("Couldn't refresh your servers: %1").arg(m_error));
    m_stale->setVisible(stale);
    m_staleRetry->setVisible(stale);
    updateHero();

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
    if (watched == m_grid && event->type() == QEvent::KeyPress) {
        const auto* key = static_cast<QKeyEvent*>(event);
        const QString id = m_grid->currentIndex().data(KubikServerModel::IdRole).toString();
        if (!id.isEmpty() && (key->key() == Qt::Key_Return || key->key() == Qt::Key_Enter)) {
            emit serverOpened(id);
            return true;
        }
        if (!id.isEmpty() && key->key() == Qt::Key_Space) {
            emit playRequested(id);
            return true;
        }
    }
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
    updateHero();  // a server just played has moved to the front
    m_autoRefresh->start();
}

void KubikServersPage::hideEvent(QHideEvent* event)
{
    m_autoRefresh->stop();
    QWidget::hideEvent(event);
}
