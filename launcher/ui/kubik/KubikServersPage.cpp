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

#include <QEvent>
#include <QLabel>
#include <QPushButton>
#include <QUrl>
#include <QVBoxLayout>

#include "Application.h"
#include "DesktopServices.h"
#include "kubik/KubikWindowChrome.h"

KubikServersPage::KubikServersPage(QWidget* parent) : QWidget(parent)
{
    setObjectName("kubikPage");
    setAttribute(Qt::WA_StyledBackground);

    // A fixed-width column: a word-wrapped QLabel only measures its height correctly when the
    // layout gives it a definite width.
    auto* column = new QWidget(this);
    column->setFixedWidth(480);
    auto* content = new QVBoxLayout(column);
    content->setContentsMargins(0, 0, 0, 0);
    content->setSpacing(12);

    auto* mark = new QLabel(column);
    mark->setPixmap(APPLICATION->logo().pixmap(88, 88));
    content->addWidget(mark);

    m_title = new QLabel(column);
    m_title->setObjectName("kubikPageTitle");
    content->addWidget(m_title);

    m_body = new QLabel(column);
    m_body->setObjectName("kubikPageBody");
    m_body->setWordWrap(true);
    content->addWidget(m_body);

    m_open = new QPushButton(column);
    m_open->setProperty("kubikRole", "primary");
    m_open->setCursor(Qt::PointingHandCursor);
    connect(m_open, &QPushButton::clicked, this, [] { DesktopServices::openUrl(QUrl("https://dashboard.kubik.gg")); });
    content->addSpacing(8);
    content->addWidget(m_open, 0, Qt::AlignLeft);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(32, 32 + KubikWindowChrome::titleBarInset(), 32, 32);
    layout->addStretch(2);
    layout->addWidget(column, 0, Qt::AlignLeft);
    layout->addStretch(3);

    retranslate();
}

void KubikServersPage::retranslate()
{
    m_title->setText(tr("Your Kubik servers"));
    m_body->setText(
        tr("Soon you'll sign in with your kubik.gg account here and see every Java server you run. "
           "Pick one and KubikLauncher sets up the same version, loader and client-side mods, "
           "then joins it."));
    m_open->setText(tr("Open the kubik.gg dashboard"));
}

void KubikServersPage::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange) {
        retranslate();
    }
    QWidget::changeEvent(event);
}
