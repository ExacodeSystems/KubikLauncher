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

#include "KubikInstanceOverview.h"

#include <QEvent>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#include "DesktopServices.h"
#include "FileSystem.h"
#include "kubik/KubikProfiles.h"
#include "minecraft/MinecraftInstance.h"

namespace {

QLabel* label(const char* objectName, QWidget* parent)
{
    auto* l = new QLabel(parent);
    l->setObjectName(objectName);
    l->setWordWrap(true);
    l->setTextFormat(Qt::PlainText);
    return l;
}

/** The game folders players reach for, by the subfolder of the game root each lives in. */
struct Folder {
    const char* icon;
    const char* subfolder;
};
constexpr Folder FOLDERS[] = {
    { "worlds", "saves" },
    { "resourcepacks", "resourcepacks" },
    { "shaderpacks", "shaderpacks" },
    { "screenshots", "screenshots" },
};

}  // namespace

KubikInstanceOverview::KubikInstanceOverview(QWidget* parent) : QWidget(parent)
{
    m_usedTitle = label("kubikSectionTitle", this);
    m_usedList = new QVBoxLayout();
    m_usedList->setSpacing(4);

    m_foldersTitle = label("kubikSectionTitle", this);
    auto* folders = new QGridLayout();
    folders->setHorizontalSpacing(10);
    folders->setVerticalSpacing(10);
    for (int i = 0; i < int(std::size(FOLDERS)); ++i) {
        auto* button = new QPushButton(QIcon::fromTheme(FOLDERS[i].icon), QString(), this);
        button->setIconSize(QSize(20, 20));
        button->setCursor(Qt::PointingHandCursor);
        const QString subfolder = FOLDERS[i].subfolder;
        connect(button, &QPushButton::clicked, this, [this, subfolder] {
            if (m_instance) {
                // Created when missing: a fresh instance has no screenshots folder yet.
                DesktopServices::openPath(FS::PathCombine(m_instance->gameRoot(), subfolder), true);
            }
        });
        m_folders << button;
        folders->addWidget(button, i / 2, i % 2);
    }

    m_packTitle = label("kubikSectionTitle", this);
    m_pack = label("kubikPageBody", this);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);
    layout->addWidget(m_usedTitle);
    layout->addLayout(m_usedList);
    layout->addSpacing(20);
    layout->addWidget(m_foldersTitle);
    layout->addLayout(folders);
    layout->addSpacing(20);
    layout->addWidget(m_packTitle);
    layout->addWidget(m_pack);
    layout->addStretch();
}

void KubikInstanceOverview::setInstance(MinecraftInstance* instance)
{
    m_instance = instance;
    rebuild();
}

void KubikInstanceOverview::showEvent(QShowEvent* event)
{
    // Home may have fetched the servers since: names that were unknown can be named now.
    rebuild();
    QWidget::showEvent(event);
}

void KubikInstanceOverview::rebuild()
{
    m_usedTitle->setText(tr("Used by"));
    m_foldersTitle->setText(tr("Folders"));
    const QStringList names = { tr("Worlds"), tr("Resource packs"), tr("Shaders"), tr("Screenshots") };
    for (int i = 0; i < m_folders.size(); ++i) {
        m_folders[i]->setText(names.value(i));
    }

    while (auto* item = m_usedList->takeAt(0)) {
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }
    if (!m_instance) {
        return;
    }

    // Each server that has played here, by name when Home knows it, opening its page.
    int named = 0;
    for (const auto& id : Kubik::serversOf(m_instance)) {
        const QString name = m_serverName ? m_serverName(id) : QString();
        if (name.isEmpty()) {
            continue;
        }
        auto* server = new QPushButton(QIcon::fromTheme("server"), name, this);
        server->setProperty("kubikRole", "link");
        server->setCursor(Qt::PointingHandCursor);
        connect(server, &QPushButton::clicked, this, [this, id] { emit serverOpened(id); });
        m_usedList->addWidget(server, 0, Qt::AlignLeft);
        ++named;
    }
    if (named == 0) {
        auto* none = label("kubikPageBody", this);
        none->setText(tr("No kubik.gg server has played in it yet. Servers that need the same Minecraft and loader share it."));
        m_usedList->addWidget(none);
    }

    const bool pack = m_instance->isManagedPack();
    m_packTitle->setVisible(pack);
    m_pack->setVisible(pack);
    if (pack) {
        m_packTitle->setText(tr("Modpack"));
        m_pack->setText(QStringLiteral("%1 · %2").arg(m_instance->getManagedPackName(), m_instance->getManagedPackVersionName()));
    }
}

void KubikInstanceOverview::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange) {
        rebuild();
    }
    QWidget::changeEvent(event);
}
