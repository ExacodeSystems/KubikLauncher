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

#include <QEvent>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QScrollArea>
#include <QStyle>
#include <QVBoxLayout>

#include "Application.h"
#include "InstanceList.h"
#include "icons/IconList.h"
#include "kubik/KubikWindowChrome.h"
#include "minecraft/MinecraftInstance.h"
#include "ui/kubik/KubikInstanceInfo.h"

namespace {

constexpr int MARGIN = 32;
constexpr int ART_SIDE = 168;
constexpr int ICON_SIDE = 128;

/** A row of the "Manage" list: an icon, a name and one line of what it is for. */
QPushButton* manageRow(const QString& icon, const QString& title, const QString& description, QWidget* parent)
{
    auto* row = new QPushButton(parent);
    row->setProperty("kubikRole", "row");
    row->setCursor(Qt::PointingHandCursor);

    auto* iconLabel = new QLabel(row);
    iconLabel->setPixmap(QIcon::fromTheme(icon).pixmap(24, 24));
    auto* titleLabel = new QLabel(title, row);
    titleLabel->setObjectName("kubikSectionTitle");
    auto* descriptionLabel = new QLabel(description, row);
    descriptionLabel->setObjectName("kubikPageSubtitle");
    for (auto* label : { iconLabel, titleLabel, descriptionLabel }) {
        // The row is the button; its labels must not swallow the click.
        label->setAttribute(Qt::WA_TransparentForMouseEvents);
    }

    auto* words = new QVBoxLayout();
    words->setSpacing(2);
    words->addWidget(titleLabel);
    words->addWidget(descriptionLabel);
    auto* layout = new QHBoxLayout(row);
    layout->setContentsMargins(16, 12, 16, 12);
    layout->setSpacing(14);
    layout->addWidget(iconLabel, 0, Qt::AlignVCenter);
    layout->addLayout(words, 1);
    return row;
}

}  // namespace

KubikInstancePage::KubikInstancePage(QWidget* parent) : QWidget(parent)
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

    m_name = new QLabel(this);
    m_name->setObjectName("kubikPageTitle");
    m_name->setTextFormat(Qt::PlainText);
    m_about = new QLabel(this);
    m_about->setObjectName("kubikPageBody");
    m_stats = new QLabel(this);
    m_stats->setObjectName("kubikPageSubtitle");

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
    connect(m_folder, &QPushButton::clicked, this, [this] { emit actionRequested(m_instanceId, Action::OpenFolder); });
    m_export = new QPushButton(this);
    m_export->setIcon(QIcon::fromTheme("export"));
    m_export->setMenu(new QMenu(m_export));
    m_more = new QPushButton(this);
    m_more->setMenu(new QMenu(m_more));

    auto* actions = new QHBoxLayout();
    actions->setSpacing(10);
    actions->addWidget(m_play);
    actions->addSpacing(6);
    actions->addWidget(m_folder);
    actions->addWidget(m_export);
    actions->addWidget(m_more);
    actions->addStretch();

    auto* words = new QVBoxLayout();
    words->setSpacing(6);
    words->addStretch();
    words->addWidget(m_name);
    words->addWidget(m_about);
    words->addWidget(m_stats);
    words->addSpacing(14);
    words->addLayout(actions);
    words->addStretch();

    auto* hero = new QHBoxLayout();
    hero->setSpacing(28);
    hero->addWidget(m_art, 0, Qt::AlignTop);
    hero->addLayout(words, 1);

    // ── Manage: everything that can be changed, in plain words ──────────────
    m_manageTitle = new QLabel(this);
    m_manageTitle->setObjectName("kubikSectionTitle");
    m_manageGrid = new QGridLayout();
    m_manageGrid->setHorizontalSpacing(12);
    m_manageGrid->setVerticalSpacing(12);

    auto* content = new QWidget(this);
    // The scroll area's viewport would otherwise show the window colour behind the page.
    content->setObjectName("kubikPage");
    content->setAttribute(Qt::WA_StyledBackground);
    auto* contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(MARGIN, 16 + KubikWindowChrome::titleBarInset(), MARGIN, MARGIN);
    contentLayout->setSpacing(0);
    contentLayout->addWidget(m_back, 0, Qt::AlignLeft);
    contentLayout->addSpacing(18);
    contentLayout->addLayout(hero);
    contentLayout->addSpacing(40);
    contentLayout->addWidget(m_manageTitle);
    contentLayout->addSpacing(12);
    contentLayout->addLayout(m_manageGrid);
    contentLayout->addStretch();

    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setWidget(content);
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(scroll);

    connect(KubikInstanceInfo::instance(), &KubikInstanceInfo::summaryChanged, this, [this](const QString& id) {
        if (id == m_instanceId) {
            refresh();
        }
    });
    // Deleted while open: there is nothing left to show, so go home.
    const auto checkStillThere = [this] {
        if (!m_instanceId.isEmpty() && !APPLICATION->instances()->getInstanceById(m_instanceId)) {
            m_instanceId.clear();
            emit backRequested();
        }
    };
    connect(APPLICATION->instances(), &QAbstractItemModel::rowsRemoved, this, checkStillThere);
    connect(APPLICATION->instances(), &QAbstractItemModel::modelReset, this, checkStillThere);

    retranslate();
}

void KubikInstancePage::setInstance(const QString& instanceId)
{
    for (const auto& connection : m_instanceConnections) {
        disconnect(connection);
    }
    m_instanceConnections.clear();
    m_instanceId = instanceId;

    if (auto* instance = APPLICATION->instances()->getInstanceById(instanceId)) {
        m_instanceConnections << connect(instance, &BaseInstance::runningStatusChanged, this, &KubikInstancePage::refresh);
        m_instanceConnections << connect(instance, &BaseInstance::propertiesChanged, this, &KubikInstancePage::refresh);
    }
    rebuildManageRows();
    refresh();
    // Keyboard focus starts on the one big action, so Enter plays.
    m_play->setFocus();
}

void KubikInstancePage::retranslate()
{
    m_back->setText(tr("← Home"));
    m_folder->setText(tr("Open folder"));
    m_folder->setToolTip(tr("Open this instance's files in your file manager"));
    m_export->setText(tr("Export"));
    m_export->setToolTip(tr("Save this instance as a file to share or back up"));
    auto* exportMenu = m_export->menu();
    exportMenu->clear();
    exportMenu->addAction(tr("As a backup (.zip)"), this, [this] { emit actionRequested(m_instanceId, Action::ExportZip); });
    exportMenu->addAction(tr("As a Modrinth modpack (.mrpack)"), this,
                          [this] { emit actionRequested(m_instanceId, Action::ExportModrinth); });
    m_more->setText(tr("More"));
    m_manageTitle->setText(tr("Manage"));

    auto* menu = m_more->menu();
    menu->clear();
    const QList<QPair<QString, Action>> items = {
        { tr("Rename"), Action::Rename },
        { tr("Change icon"), Action::ChangeIcon },
        { tr("Move to group"), Action::ChangeGroup },
        { tr("Make a copy"), Action::Copy },
        { tr("Create desktop shortcut"), Action::CreateShortcut },
    };
    for (const auto& [label, action] : items) {
        menu->addAction(label, this, [this, action] { emit actionRequested(m_instanceId, action); });
    }
    menu->addSeparator();
    menu->addAction(tr("Delete instance"), this, [this] { emit actionRequested(m_instanceId, Action::Delete); });

    rebuildManageRows();
    refresh();
}

void KubikInstancePage::rebuildManageRows()
{
    while (auto* item = m_manageGrid->takeAt(0)) {
        delete item->widget();
        delete item;
    }
    auto* instance = APPLICATION->instances()->getInstanceById(m_instanceId);

    struct Row {
        QString page, icon, title, description;
    };
    QList<Row> rows;
    if (instance && instance->isManagedPack()) {
        rows << Row{ "managed_pack", "minecraft", tr("Modpack"), tr("Update the modpack or switch its version") };
    }
    rows << Row{ "mods", "loadermods", tr("Mods"), tr("Add, remove or update mods") }
         << Row{ "resourcepacks", "resourcepacks", tr("Resource packs"), tr("Change how blocks and items look") }
         << Row{ "shaderpacks", "shaderpacks", tr("Shaders"), tr("Lighting and shadows (needs Iris or OptiFine)") }
         << Row{ "worlds", "worlds", tr("Worlds"), tr("Back up, copy or delete your saves") }
         << Row{ "servers", "server", tr("Servers"), tr("The servers in this instance's multiplayer list") }
         << Row{ "screenshots", "screenshots", tr("Screenshots"), tr("Everything you captured in game") }
         << Row{ "version", "minecraft", tr("Version and loader"), tr("Change the Minecraft version or mod loader") }
         << Row{ "settings", "instance-settings", tr("Settings"), tr("Memory, Java and window size") }
         << Row{ "logs", "log", tr("Logs"), tr("What the game printed, for fixing problems") };

    for (int i = 0; i < rows.size(); ++i) {
        const auto& row = rows[i];
        auto* button = manageRow(row.icon, row.title, row.description, this);
        const QString page = row.page;
        connect(button, &QPushButton::clicked, this, [this, page] { emit manageRequested(m_instanceId, page); });
        m_manageGrid->addWidget(button, i / 2, i % 2);
    }
}

void KubikInstancePage::refresh()
{
    auto* instance = APPLICATION->instances()->getInstanceById(m_instanceId);
    if (!instance) {
        return;
    }
    m_art->setPixmap(APPLICATION->icons()->getIcon(instance->iconKey()).pixmap(ICON_SIDE, ICON_SIDE));
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
    m_stats->setText(QStringLiteral("%1 · %2").arg(KubikInstanceInfo::totalPlayed(instance), KubikInstanceInfo::lastPlayed(instance)));

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
