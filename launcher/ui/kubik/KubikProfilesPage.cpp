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

#include "KubikProfilesPage.h"

#include <QEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#include "Application.h"
#include "kubik/KubikAccount.h"
#include "minecraft/auth/AccountList.h"
#include "ui/dialogs/skins/SkinManageDialog.h"
#include "ui/kubik/KubikAccounts.h"
#include "ui/kubik/KubikConfirmDialog.h"

namespace {

QLabel* label(const char* objectName, QWidget* parent)
{
    auto* l = new QLabel(parent);
    l->setObjectName(objectName);
    l->setWordWrap(true);
    l->setTextFormat(Qt::PlainText);
    return l;
}

QPushButton* button(const char* role, QWidget* parent)
{
    auto* b = new QPushButton(parent);
    if (role) {
        b->setProperty("kubikRole", role);
    }
    b->setCursor(Qt::PointingHandCursor);
    return b;
}

}  // namespace

KubikProfilesPage::KubikProfilesPage(QWidget* parent) : QWidget(parent)
{
    m_title = label("kubikPageTitle", this);
    m_profilesTitle = label("kubikSectionTitle", this);
    m_profilesBody = label("kubikPageSubtitle", this);
    m_rows = new QVBoxLayout();
    m_rows->setSpacing(6);

    m_addMicrosoft = button("primary", this);
    m_addMicrosoft->setIcon(KubikAccounts::microsoftIcon());
    m_addMicrosoft->setIconSize(QSize(17, 14));
    connect(m_addMicrosoft, &QPushButton::clicked, this, [this] { KubikAccounts::addMicrosoft(window()); });
    m_addOffline = button(nullptr, this);
    connect(m_addOffline, &QPushButton::clicked, this, [this] { KubikAccounts::addOffline(window()); });
    auto* adds = new QHBoxLayout();
    adds->setSpacing(10);
    adds->addWidget(m_addMicrosoft);
    adds->addWidget(m_addOffline);
    adds->addStretch();
    m_offlineNote = label("kubikPageNote", this);

    m_kubikTitle = label("kubikSectionTitle", this);
    m_kubikWho = label("kubikPageBody", this);
    m_signOut = button("quiet", this);
    connect(m_signOut, &QPushButton::clicked, this, [this] {
        const auto answer =
            KubikConfirmDialog::ask(window(), tr("Sign out of kubik.gg?"),
                                    tr("KubikLauncher stops showing your servers until you sign in again. Your instances and "
                                       "Minecraft profiles stay."),
                                    tr("Sign out"), tr("Cancel"), true);
        if (answer == KubikConfirmDialog::Answer::Primary) {
            KubikAccount::get()->signOut();
        }
    });
    auto* kubikRow = new QHBoxLayout();
    kubikRow->addWidget(m_kubikWho, 1);
    kubikRow->addWidget(m_signOut);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);
    layout->addWidget(m_title);
    layout->addSpacing(14);
    layout->addWidget(m_profilesTitle);
    layout->addWidget(m_profilesBody);
    layout->addSpacing(4);
    layout->addLayout(m_rows);
    layout->addSpacing(6);
    layout->addLayout(adds);
    layout->addWidget(m_offlineNote);
    layout->addSpacing(22);
    layout->addWidget(m_kubikTitle);
    layout->addLayout(kubikRow);
    layout->addStretch();

    auto* accounts = APPLICATION->accounts();
    connect(accounts, &AccountList::listChanged, this, &KubikProfilesPage::rebuild);
    connect(accounts, &AccountList::defaultAccountChanged, this, &KubikProfilesPage::rebuild);
    connect(accounts, &AccountList::listActivityChanged, this, &KubikProfilesPage::rebuild);
    connect(KubikAccount::get(), &KubikAccount::changed, this, &KubikProfilesPage::rebuild);
    rebuild();
}

void KubikProfilesPage::rebuild()
{
    m_title->setText(tr("Profiles"));
    m_profilesTitle->setText(tr("Minecraft profiles"));
    m_profilesBody->setText(
        tr("The profile in use is the one that joins servers. Switch it here or from the card at the bottom of "
           "the sidebar."));
    m_addMicrosoft->setText(tr("Add Microsoft account"));
    m_addOffline->setText(tr("Add offline profile"));
    const bool canOffline = KubikAccounts::canAddOffline();
    m_addOffline->setEnabled(canOffline);
    m_offlineNote->setText(canOffline ? tr("Offline profiles can only join servers whose owner turned on offline access.")
                                      : tr("Offline profiles unlock once you add a Microsoft account that owns Minecraft."));

    const auto& kubik = KubikAccount::get()->user();
    m_kubikTitle->setText(tr("kubik.gg account"));
    m_kubikWho->setText(kubik.name.isEmpty() ? tr("Signed in as %1").arg(kubik.email)
                                             : tr("Signed in as %1 (%2)").arg(kubik.name, kubik.email));
    m_signOut->setText(tr("Sign out"));
    m_kubikTitle->setVisible(KubikAccount::get()->isSignedIn());
    m_kubikWho->setVisible(KubikAccount::get()->isSignedIn());
    m_signOut->setVisible(KubikAccount::get()->isSignedIn());

    while (auto* item = m_rows->takeAt(0)) {
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }

    auto* accounts = APPLICATION->accounts();
    const auto current = KubikAccounts::current();
    if (accounts->count() == 0) {
        auto* empty = label("kubikPageBody", this);
        empty->setText(tr("No Minecraft profile yet. Add the Microsoft account you bought Minecraft with to join servers."));
        m_rows->addWidget(empty);
        return;
    }
    for (int i = 0; i < accounts->count(); ++i) {
        const auto account = accounts->at(i);
        const bool inUse = account == current;

        auto* row = new QFrame(this);
        row->setObjectName("kubikProfileRow");
        auto* face = new QLabel(row);
        face->setPixmap(KubikAccounts::avatar(account, 36));
        auto* name = label("kubikSectionTitle", row);
        name->setText(account->profileName());
        auto* type = label("kubikPageSubtitle", row);
        if (KubikAccounts::needsSignIn(account)) {
            type->setText(tr("%1 · sign in again").arg(KubikAccounts::typeLabel(account)));
        } else {
            type->setText(inUse ? tr("%1 · in use").arg(KubikAccounts::typeLabel(account)) : KubikAccounts::typeLabel(account));
        }
        auto* words = new QVBoxLayout();
        words->setSpacing(0);
        words->addWidget(name);
        words->addWidget(type);

        auto* rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(14, 10, 10, 10);
        rowLayout->setSpacing(12);
        rowLayout->addWidget(face);
        rowLayout->addLayout(words, 1);

        if (KubikAccounts::needsSignIn(account)) {
            auto* again = button("primary", row);
            again->setText(tr("Sign in again"));
            connect(again, &QPushButton::clicked, this, [this] { KubikAccounts::addMicrosoft(window()); });
            rowLayout->addWidget(again);
        } else if (!inUse) {
            auto* use = button(nullptr, row);
            use->setText(tr("Use"));
            connect(use, &QPushButton::clicked, this, [accounts, account] { accounts->setDefaultAccount(account); });
            rowLayout->addWidget(use);
        }
        if (account->accountType() != AccountType::Offline) {
            auto* skin = button("quiet", row);
            skin->setText(tr("Skin…"));
            connect(skin, &QPushButton::clicked, this, [this, account] {
                SkinManageDialog dialog(window(), account);
                dialog.exec();
            });
            rowLayout->addWidget(skin);
        }
        auto* remove = button("quiet", row);
        remove->setText(tr("Remove"));
        connect(remove, &QPushButton::clicked, this, [this, accounts, account] {
            const auto answer = KubikConfirmDialog::ask(window(), tr("Remove %1?").arg(account->profileName()),
                                                        tr("KubikLauncher forgets this profile. Your Minecraft account itself is not "
                                                           "touched, and you can add it again any time."),
                                                        tr("Remove"), tr("Cancel"), true);
            if (answer == KubikConfirmDialog::Answer::Primary) {
                for (int row = 0; row < accounts->count(); ++row) {
                    if (accounts->at(row) == account) {
                        accounts->removeAccount(accounts->index(row));
                        break;
                    }
                }
            }
        });
        rowLayout->addWidget(remove);
        m_rows->addWidget(row);
    }
}

void KubikProfilesPage::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange) {
        rebuild();
    }
    QWidget::changeEvent(event);
}
