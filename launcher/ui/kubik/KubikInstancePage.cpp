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

#include "KubikInstancePage.h"

#include <QButtonGroup>
#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QPainter>
#include <QPushButton>
#include <QScrollArea>
#include <QStackedWidget>
#include <QStyle>
#include <QTimer>
#include <QVBoxLayout>

#include "Application.h"
#include "InstanceList.h"
#include "icons/IconList.h"
#include "kubik/KubikBrand.h"
#include "kubik/KubikWindowChrome.h"
#include "minecraft/MinecraftInstance.h"
#include "ui/kubik/KubikInstanceAdvanced.h"
#include "ui/kubik/KubikInstanceInfo.h"
#include "ui/kubik/KubikInstanceLogs.h"
#include "ui/kubik/KubikInstanceMods.h"
#include "ui/kubik/KubikInstanceOverview.h"
#include "ui/kubik/KubikPaint.h"

namespace {

constexpr int MARGIN = 32;
constexpr int ART_SIDE = 140;
constexpr int ICON_SIDE = 104;

/** A tab whose content is a column of sections: it scrolls on its own, under the fixed hero and tabs. */
QWidget* scrolling(QWidget* content)
{
    auto* page = new QWidget();
    page->setObjectName("kubikPage");
    auto* column = new QVBoxLayout(page);
    column->setContentsMargins(0, 4, 12, 12);
    content->setParent(page);
    column->addWidget(content);
    column->addStretch();
    auto* scroll = new QScrollArea();
    scroll->setWidget(page);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    return scroll;
}

}  // namespace

KubikInstancePage::KubikInstancePage(QWidget* parent) : QWidget(parent), m_tabs(new QButtonGroup(this))
{
    setObjectName("kubikPage");
    setAttribute(Qt::WA_StyledBackground);

    m_back = new QPushButton(this);
    m_back->setProperty("kubikRole", "quiet");
    m_back->setCursor(Qt::PointingHandCursor);
    connect(m_back, &QPushButton::clicked, this, &KubikInstancePage::backRequested);

    // ── Hero: the instance and its one big action ───────────────────────────
    m_art = new QLabel(this);
    m_art->setObjectName("kubikArt");
    m_art->setFixedSize(ART_SIDE, ART_SIDE);
    m_art->setAlignment(Qt::AlignCenter);

    // Wrapping, so a long name or a long translation never pushes the page wider than the window.
    m_name = new QLabel(this);
    m_name->setObjectName("kubikPageTitle");
    m_name->setTextFormat(Qt::PlainText);
    m_name->setWordWrap(true);
    m_about = new QLabel(this);
    m_about->setObjectName("kubikPageBody");
    m_about->setWordWrap(true);
    m_stats = new QLabel(this);
    m_stats->setObjectName("kubikPageSubtitle");
    m_stats->setWordWrap(true);

    m_play = new QPushButton(this);
    m_play->setObjectName("kubikHeroPlay");
    m_play->setCursor(Qt::PointingHandCursor);
    connect(m_play, &QPushButton::clicked, this, [this] {
        auto* instance = APPLICATION->instances()->getInstanceById(m_instanceId);
        if (instance && instance->isRunning()) {
            emit stopRequested(m_instanceId);
        } else {
            emit playRequested(m_instanceId);
        }
    });

    m_folder = new QPushButton(this);
    m_folder->setIcon(QIcon::fromTheme("viewfolder"));
    m_folder->setCursor(Qt::PointingHandCursor);
    connect(m_folder, &QPushButton::clicked, this, [this] { emit actionRequested(m_instanceId, Action::OpenFolder); });
    m_more = new QPushButton(this);
    m_more->setMenu(new QMenu(m_more));
    m_more->setCursor(Qt::PointingHandCursor);

    auto* actions = new QHBoxLayout();
    actions->setSpacing(10);
    actions->addWidget(m_play);
    actions->addSpacing(6);
    actions->addWidget(m_folder);
    actions->addWidget(m_more);
    actions->addStretch();

    auto* words = new QVBoxLayout();
    words->setSpacing(6);
    words->addStretch();
    words->addWidget(m_name);
    words->addWidget(m_about);
    words->addWidget(m_stats);
    words->addSpacing(12);
    words->addLayout(actions);
    words->addStretch();

    auto* hero = new QHBoxLayout();
    hero->setSpacing(24);
    hero->addWidget(m_art, 0, Qt::AlignTop);
    hero->addLayout(words, 1);

    // ── Tabs ────────────────────────────────────────────────────────────────
    m_overview = new KubikInstanceOverview();
    m_mods = new KubikInstanceMods(this);
    m_logs = new KubikInstanceLogs(this);
    m_advanced = new KubikInstanceAdvanced();
    m_tabPages = new QStackedWidget(this);
    m_tabPages->insertWidget(Overview, scrolling(m_overview));
    m_tabPages->insertWidget(Mods, m_mods);
    m_tabPages->insertWidget(Logs, m_logs);
    m_tabPages->insertWidget(Advanced, scrolling(m_advanced));

    auto* tabRow = new QHBoxLayout();
    tabRow->setSpacing(22);
    for (int tab : { Overview, Mods, Logs, Advanced }) {
        auto* button = new QPushButton(this);
        button->setObjectName("kubikTab");
        button->setCheckable(true);
        button->setCursor(Qt::PointingHandCursor);
        m_tabs->addButton(button, tab);
        m_tabButtons << button;
        tabRow->addWidget(button);
    }
    tabRow->addStretch();
    connect(m_tabs, &QButtonGroup::idClicked, this, &KubikInstancePage::showTab);

    connect(m_overview, &KubikInstanceOverview::serverOpened, this, &KubikInstancePage::serverOpened);
    connect(m_advanced, &KubikInstanceAdvanced::requested, this, [this](KubikInstanceAdvanced::Request request) {
        using Request = KubikInstanceAdvanced::Request;
        switch (request) {
            case Request::Rename:
                emit actionRequested(m_instanceId, Action::Rename);
                break;
            case Request::ChangeIcon:
                emit actionRequested(m_instanceId, Action::ChangeIcon);
                break;
            case Request::Copy:
                emit actionRequested(m_instanceId, Action::Copy);
                break;
            case Request::Delete:
                emit actionRequested(m_instanceId, Action::Delete);
                break;
            case Request::MoreSettings:
                emit manageRequested(m_instanceId, QStringLiteral("settings"));
                break;
        }
    });

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(MARGIN, 16 + KubikWindowChrome::titleBarInset(), MARGIN, 20);
    layout->setSpacing(0);
    layout->addWidget(m_back, 0, Qt::AlignLeft);
    layout->addSpacing(14);
    layout->addLayout(hero);
    layout->addSpacing(22);
    layout->addLayout(tabRow);
    layout->addSpacing(14);
    layout->addWidget(m_tabPages, 1);

    connect(KubikInstanceInfo::instance(), &KubikInstanceInfo::summaryChanged, this, [this](const QString& id) {
        if (id == m_instanceId) {
            refresh();
        }
    });
    // Deleted while open: there is nothing left to show, so go home. Checked a moment later, because a
    // rename that renames the folder too reloads the list (old id gone) before it points this page at
    // the new id.
    const auto checkStillThere = [this] {
        QTimer::singleShot(0, this, [this] {
            if (!m_instanceId.isEmpty() && !APPLICATION->instances()->getInstanceById(m_instanceId)) {
                m_instanceId.clear();
                emit backRequested();
            }
        });
    };
    connect(APPLICATION->instances(), &QAbstractItemModel::rowsRemoved, this, checkStillThere);
    connect(APPLICATION->instances(), &QAbstractItemModel::modelReset, this, checkStillThere);

    retranslate();
    showTab(Overview);
}

void KubikInstancePage::setServerNames(std::function<QString(const QString&)> lookup)
{
    m_overview->setServerNames(lookup);
    m_mods->setServerNames(lookup);
}

void KubikInstancePage::setInstance(const QString& instanceId)
{
    for (const auto& connection : m_instanceConnections) {
        disconnect(connection);
    }
    m_instanceConnections.clear();
    const bool another = instanceId != m_instanceId;
    m_instanceId = instanceId;

    auto* instance = dynamic_cast<MinecraftInstance*>(APPLICATION->instances()->getInstanceById(instanceId));
    if (instance) {
        m_instanceConnections << connect(instance, &BaseInstance::runningStatusChanged, this, &KubikInstancePage::refresh);
        m_instanceConnections << connect(instance, &BaseInstance::propertiesChanged, this, &KubikInstancePage::refresh);
    }
    m_overview->setInstance(instance);
    m_mods->setInstance(instance);
    m_logs->setInstance(instance);
    m_advanced->setInstance(instance);
    if (another) {
        showTab(Overview);
    }
    refresh();
    // Keyboard focus starts on the one big action, so Enter plays.
    m_play->setFocus();
}

void KubikInstancePage::showTab(int tab)
{
    if (auto* button = m_tabs->button(tab)) {
        button->setChecked(true);
    }
    m_tabPages->setCurrentIndex(tab);
}

void KubikInstancePage::retranslate()
{
    m_back->setText(tr("← Home"));
    m_folder->setText(tr("Open folder"));
    m_folder->setToolTip(tr("Open this instance's files in your file manager"));
    m_more->setText(tr("More"));
    const QStringList tabs = { tr("Overview"), tr("Mods"), tr("Logs"), tr("Advanced") };
    for (int i = 0; i < m_tabButtons.size(); ++i) {
        m_tabButtons[i]->setText(tabs.value(i));
    }

    // Rename, icon, copy and delete live on the Advanced tab; More keeps what has no other home.
    auto* menu = m_more->menu();
    menu->clear();
    menu->addAction(tr("Export as a backup (.zip)"), this, [this] { emit actionRequested(m_instanceId, Action::ExportZip); });
    menu->addAction(tr("Export as a Modrinth modpack (.mrpack)"), this,
                    [this] { emit actionRequested(m_instanceId, Action::ExportModrinth); });
    menu->addSeparator();
    menu->addAction(tr("Move to group"), this, [this] { emit actionRequested(m_instanceId, Action::ChangeGroup); });
    menu->addAction(tr("Create desktop shortcut"), this, [this] { emit actionRequested(m_instanceId, Action::CreateShortcut); });

    refresh();
}

void KubikInstancePage::refresh()
{
    auto* instance = APPLICATION->instances()->getInstanceById(m_instanceId);
    if (!instance) {
        return;
    }
    // The icon in an inventory slot, as a server's picture is on its page.
    const qreal dpr = devicePixelRatioF();
    QPixmap art(QSize(ART_SIDE, ART_SIDE) * dpr);
    art.setDevicePixelRatio(dpr);
    art.fill(KubikBrand::color("void.raised"));
    {
        QPainter painter(&art);
        QRect icon(0, 0, ICON_SIDE, ICON_SIDE);
        icon.moveCenter(QRect(0, 0, ART_SIDE, ART_SIDE).center());
        painter.drawPixmap(icon, APPLICATION->icons()->getIcon(instance->iconKey()).pixmap(QSize(ICON_SIDE, ICON_SIDE), dpr));
        KubikPaint::drawSlotFrame(&painter, QRectF(0, 0, ART_SIDE, ART_SIDE));
    }
    m_art->setPixmap(art);
    m_name->setText(instance->name());

    const auto summary = KubikInstanceInfo::instance()->summary(instance);
    QStringList about;
    if (!summary.minecraftVersion.isEmpty()) {
        about << tr("Minecraft %1").arg(summary.minecraftVersion);
    }
    about << (summary.loaderName.isEmpty() ? tr("Vanilla") : QString("%1 %2").arg(summary.loaderName, summary.loaderVersion));
    if (summary.modCount >= 0) {
        about << tr("%n mod(s)", nullptr, summary.modCount);
    }
    m_about->setText(about.join(QStringLiteral(" · ")));
    m_stats->setText(KubikInstanceInfo::playLine(instance));

    const bool running = instance->isRunning();
    m_play->setText(running ? tr("Stop") : tr("Play"));
    m_play->setProperty("kubikRole", running ? "danger" : "primary");
    // A dynamic property only restyles after the style is re-applied.
    m_play->style()->unpolish(m_play);
    m_play->style()->polish(m_play);
    m_play->setEnabled(running || instance->canLaunch());
}

void KubikInstancePage::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange) {
        retranslate();
    }
    QWidget::changeEvent(event);
}
