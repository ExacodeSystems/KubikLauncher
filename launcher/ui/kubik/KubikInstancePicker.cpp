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

#include "KubikInstancePicker.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

#include "Application.h"
#include "BaseInstance.h"
#include "icons/IconList.h"
#include "ui/kubik/KubikInstanceInfo.h"

KubikInstancePicker::KubikInstancePicker(const QString& serverName, const QList<BaseInstance*>& candidates, QWidget* parent)
    : QDialog(parent), m_candidates(candidates)
{
    setWindowTitle(tr("Choose an instance"));
    setMinimumWidth(460);

    auto* title = new QLabel(tr("Which instance should play %1?").arg(serverName), this);
    title->setObjectName("kubikSectionTitle");
    title->setWordWrap(true);
    auto* body = new QLabel(tr("Each of these can join it. KubikLauncher puts the server's mods in the one you pick, and "
                               "remembers it for next time."),
                            this);
    body->setObjectName("kubikPageSubtitle");
    body->setWordWrap(true);

    m_list = new QListWidget(this);
    m_list->setIconSize(QSize(32, 32));
    m_list->setSpacing(2);
    for (auto* instance : candidates) {
        auto* item = new QListWidgetItem(APPLICATION->icons()->getIcon(instance->iconKey()),
                                         instance->name() + "\n" + KubikInstanceInfo::lastPlayed(instance), m_list);
        item->setSizeHint(QSize(0, 52));
    }
    m_list->setCurrentRow(0);
    m_list->setVisible(!candidates.isEmpty());

    auto* useIt = new QPushButton(tr("Play with this one"), this);
    useIt->setProperty("kubikRole", "primary");
    useIt->setDefault(true);
    useIt->setVisible(!candidates.isEmpty());
    auto* makeNew = new QPushButton(tr("Make a new one"), this);
    auto* cancel = new QPushButton(tr("Cancel"), this);
    cancel->setProperty("kubikRole", "quiet");

    connect(useIt, &QPushButton::clicked, this, [this] {
        const int row = m_list->currentRow();
        if (row >= 0 && row < m_candidates.size()) {
            m_chosen = m_candidates.at(row);
            accept();
        }
    });
    connect(m_list, &QListWidget::itemDoubleClicked, useIt, &QPushButton::click);
    connect(makeNew, &QPushButton::clicked, this, [this] {
        m_chosen = nullptr;
        accept();
    });
    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);

    auto* buttons = new QHBoxLayout();
    buttons->setSpacing(10);
    buttons->addWidget(cancel);
    buttons->addStretch();
    buttons->addWidget(makeNew);
    buttons->addWidget(useIt);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 22, 24, 20);
    layout->setSpacing(10);
    layout->addWidget(title);
    layout->addWidget(body);
    layout->addWidget(m_list, 1);
    layout->addSpacing(6);
    layout->addLayout(buttons);
}
