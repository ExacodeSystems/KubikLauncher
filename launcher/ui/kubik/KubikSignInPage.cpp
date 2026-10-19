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

#include "KubikSignInPage.h"

#include <QDesktopServices>
#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QUrl>
#include <QVBoxLayout>

#include "Application.h"
#include "kubik/KubikAccount.h"
#include "kubik/KubikApi.h"
#include "kubik/KubikWindowChrome.h"

KubikSignInPage::KubikSignInPage(QWidget* parent) : QWidget(parent)
{
    setObjectName("kubikPage");
    setAttribute(Qt::WA_StyledBackground);

    auto* column = new QWidget(this);
    column->setFixedWidth(460);
    auto* content = new QVBoxLayout(column);
    content->setContentsMargins(0, 0, 0, 0);
    content->setSpacing(12);

    auto* mark = new QLabel(column);
    mark->setPixmap(APPLICATION->logo().pixmap(96, 96));
    content->addWidget(mark);

    m_title = new QLabel(column);
    m_title->setObjectName("kubikPageTitle");
    m_title->setWordWrap(true);
    content->addWidget(m_title);

    m_body = new QLabel(column);
    m_body->setObjectName("kubikPageBody");
    m_body->setWordWrap(true);
    content->addWidget(m_body);

    m_error = new QLabel(column);
    m_error->setObjectName("kubikPageError");
    m_error->setWordWrap(true);
    content->addWidget(m_error);

    m_signIn = new QPushButton(column);
    m_signIn->setObjectName("kubikHeroPlay");
    m_signIn->setProperty("kubikRole", "primary");
    m_signIn->setCursor(Qt::PointingHandCursor);
    connect(m_signIn, &QPushButton::clicked, this, [this] {
        m_lastError.clear();
        KubikAccount::get()->signIn();
        updateState();
    });
    m_cancel = new QPushButton(column);
    m_cancel->setProperty("kubikRole", "quiet");
    m_cancel->setCursor(Qt::PointingHandCursor);
    connect(m_cancel, &QPushButton::clicked, this, [] { KubikAccount::get()->cancelSignIn(); });

    auto* buttons = new QHBoxLayout();
    buttons->setSpacing(10);
    buttons->addWidget(m_signIn);
    buttons->addWidget(m_cancel);
    buttons->addStretch();
    content->addSpacing(8);
    content->addLayout(buttons);

    m_signUp = new QPushButton(column);
    m_signUp->setProperty("kubikRole", "quiet");
    m_signUp->setCursor(Qt::PointingHandCursor);
    connect(m_signUp, &QPushButton::clicked, this, [] { QDesktopServices::openUrl(QUrl(Kubik::webOrigin() + "/signup")); });
    content->addSpacing(18);
    content->addWidget(m_signUp, 0, Qt::AlignLeft);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(56, 32 + KubikWindowChrome::titleBarInset(), 56, 32);
    layout->addStretch(2);
    layout->addWidget(column, 0, Qt::AlignLeft);
    layout->addStretch(3);

    connect(KubikAccount::get(), &KubikAccount::changed, this, &KubikSignInPage::updateState);
    connect(KubikAccount::get(), &KubikAccount::signInFailed, this, [this](const QString& message) {
        m_lastError = message;
        updateState();
    });

    retranslate();
}

void KubikSignInPage::retranslate()
{
    m_cancel->setText(tr("Cancel"));
    m_signUp->setText(tr("No kubik.gg account yet? Create one"));
    updateState();
}

void KubikSignInPage::updateState()
{
    const bool waiting = KubikAccount::get()->state() == KubikAccount::State::WaitingForBrowser;
    if (waiting) {
        m_title->setText(tr("Finish signing in in your browser"));
        m_body->setText(
            tr("kubik.gg is open in your browser. Click Connect there, then come back here. Closed the tab? Open it "
               "again below."));
        m_signIn->setText(tr("Open the page again"));
    } else {
        m_title->setText(tr("Sign in to play your servers"));
        m_body->setText(
            tr("Sign in with your kubik.gg account. KubikLauncher then shows your servers, installs the right "
               "Minecraft and mods for each one, and joins it for you."));
        m_signIn->setText(tr("Sign in with kubik.gg"));
    }
    m_cancel->setVisible(waiting);
    m_error->setText(m_lastError);
    m_error->setVisible(!m_lastError.isEmpty() && !waiting);
}

void KubikSignInPage::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange) {
        retranslate();
    }
    QWidget::changeEvent(event);
}
