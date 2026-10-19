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

#include "KubikSidebar.h"

#include <QButtonGroup>
#include <QEvent>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QVBoxLayout>

#include <QDesktopServices>
#include <QUrl>

#include "Application.h"
#include "kubik/KubikAccount.h"
#include "kubik/KubikApi.h"
#include "kubik/KubikWindowChrome.h"

namespace {

QPushButton* railButton(const QString& icon, QWidget* parent)
{
    auto* button = new QPushButton(QIcon::fromTheme(icon), QString(), parent);
    button->setIconSize(QSize(20, 20));
    button->setCursor(Qt::PointingHandCursor);
    button->setFocusPolicy(Qt::NoFocus);
    return button;
}

}  // namespace

KubikSidebar::KubikSidebar(QWidget* parent) : QFrame(parent), m_pages(new QButtonGroup(this))
{
    // Styled by name in :/kubik/kubik.qss.
    setObjectName("kubikSidebar");
    setFixedWidth(236);

    auto* layout = new QVBoxLayout(this);
    // On macOS the traffic lights sit in the top strip of the rail.
    layout->setContentsMargins(0, 18 + KubikWindowChrome::titleBarInset(), 0, 14);
    layout->setSpacing(2);

    auto* brand = new QHBoxLayout();
    brand->setContentsMargins(18, 0, 18, 22);
    brand->setSpacing(10);
    auto* mark = new QLabel(this);
    mark->setPixmap(APPLICATION->logo().pixmap(30, 30));
    auto* name = new QLabel(QStringLiteral("Kubik"), this);
    name->setObjectName("kubikBrandName");
    brand->addWidget(mark);
    brand->addWidget(name);
    brand->addStretch();
    layout->addLayout(brand);

    m_servers = railButton("server", this);
    m_instances = railButton("minecraft", this);
    for (auto* page : { m_servers, m_instances }) {
        page->setCheckable(true);
        layout->addWidget(page);
    }
    m_pages->addButton(m_servers, Servers);
    m_pages->addButton(m_instances, Instances);
    m_servers->setChecked(true);
    connect(m_pages, &QButtonGroup::idClicked, this, &KubikSidebar::pageRequested);

    layout->addStretch();

    // The kubik.gg account: whose servers these are. Its menu is the rail's own.
    m_kubik = railButton(QString(), this);
    m_kubik->setObjectName("kubikAccountChip");
    m_kubik->setIcon(APPLICATION->logo());
    m_kubik->setIconSize(QSize(24, 24));
    m_kubikMenu = new QMenu(this);
    layout->addWidget(m_kubik);
    connect(KubikAccount::get(), &KubikAccount::changed, this, &KubikSidebar::updateKubikAccount);

    m_account = railButton("noaccount", this);
    m_account->setObjectName("kubikAccountChip");
    m_account->setIconSize(QSize(24, 24));
    m_settings = railButton("settings", this);
    m_more = railButton("help", this);
    layout->addWidget(m_account);
    layout->addWidget(m_settings);
    layout->addWidget(m_more);
    connect(m_settings, &QPushButton::clicked, this, &KubikSidebar::settingsRequested);
    // The two menus open beside the rail, level with the row that opened them.
    const auto popUp = [](QPushButton* button, QMenu* menu) {
        if (menu) {
            menu->popup(button->mapToGlobal(QPoint(button->width() + 6, 0)));
        }
    };
    connect(m_kubik, &QPushButton::clicked, this, [this, popUp] { popUp(m_kubik, m_kubikMenu); });
    connect(m_account, &QPushButton::clicked, this, [this, popUp] { popUp(m_account, m_accountMenu); });
    connect(m_more, &QPushButton::clicked, this, [this, popUp] { popUp(m_more, m_moreMenu); });

    retranslate();
}

void KubikSidebar::setCurrentPage(Page page)
{
    if (auto* button = m_pages->button(page)) {
        button->setChecked(true);
    }
    emit pageRequested(page);
}

void KubikSidebar::setAccount(const QIcon& face, const QString& name)
{
    m_accountName = name;
    m_account->setIcon(face.isNull() ? QIcon::fromTheme("noaccount") : face);
    retranslate();
}

void KubikSidebar::setAccountMenu(QMenu* menu)
{
    m_accountMenu = menu;
}

void KubikSidebar::setMoreMenu(QMenu* menu)
{
    m_moreMenu = menu;
}

void KubikSidebar::updateKubikAccount()
{
    const auto& user = KubikAccount::get()->user();
    m_kubik->setText(user.name.isEmpty() ? user.email : user.name);
    m_kubik->setToolTip(tr("Signed in to kubik.gg as %1").arg(user.email));

    m_kubikMenu->clear();
    auto* who = m_kubikMenu->addAction(user.email);
    who->setEnabled(false);
    m_kubikMenu->addSeparator();
    m_kubikMenu->addAction(tr("Open kubik.gg"), this, [] { QDesktopServices::openUrl(QUrl(Kubik::webOrigin() + "/dashboard")); });
    m_kubikMenu->addAction(tr("Sign out of kubik.gg"), this, [] { KubikAccount::get()->signOut(); });
}

void KubikSidebar::retranslate()
{
    m_servers->setText(tr("Servers"));
    m_instances->setText(tr("Instances"));
    updateKubikAccount();
    m_account->setText(m_accountName.isEmpty() ? tr("Add Microsoft account") : m_accountName);
    m_account->setToolTip(tr("Switch or manage your Microsoft accounts"));
    m_settings->setText(tr("Settings"));
    // "&&": a lone "&" in a button's text marks a keyboard mnemonic and is not shown.
    m_more->setText(tr("Help && more"));
}

void KubikSidebar::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange) {
        retranslate();
    }
    QFrame::changeEvent(event);
}
