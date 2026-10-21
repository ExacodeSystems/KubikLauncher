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

#include "KubikProfileCard.h"

#include <QDesktopServices>
#include <QEvent>
#include <QLabel>
#include <QMenu>
#include <QPainter>
#include <QPainterPath>
#include <QUrl>
#include <QWidgetAction>

#include "Application.h"
#include "kubik/KubikAccount.h"
#include "kubik/KubikApi.h"
#include "kubik/KubikBrand.h"
#include "minecraft/auth/AccountList.h"
#include "ui/kubik/KubikAccounts.h"
#include "ui/kubik/KubikPaint.h"

namespace {

constexpr int HEIGHT = 60;
constexpr int PAD = 10;
constexpr int AVATAR = 34;

/** A small heading inside the dropdown: "MINECRAFT", "KUBIK.GG". Not an action. */
void addHeader(QMenu* menu, const QString& text)
{
    auto* label = new QLabel(text, menu);
    label->setObjectName("kubikMenuHeader");
    auto* action = new QWidgetAction(menu);
    action->setDefaultWidget(label);
    action->setEnabled(false);
    menu->addAction(action);
}

}  // namespace

KubikProfileCard::KubikProfileCard(QWidget* parent) : QAbstractButton(parent)
{
    setObjectName("kubikProfileCard");
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::NoFocus);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    auto* accounts = APPLICATION->accounts();
    connect(accounts, &AccountList::defaultAccountChanged, this, &KubikProfileCard::refresh);
    connect(accounts, &AccountList::listChanged, this, &KubikProfileCard::refresh);
    connect(accounts, &AccountList::listActivityChanged, this, &KubikProfileCard::refresh);
    connect(KubikAccount::get(), &KubikAccount::changed, this, &KubikProfileCard::refresh);
    connect(this, &QAbstractButton::clicked, this, &KubikProfileCard::openMenu);
    refresh();
}

QSize KubikProfileCard::sizeHint() const
{
    return QSize(212, HEIGHT);
}

void KubikProfileCard::refresh()
{
    const auto account = KubikAccounts::current();
    const auto& kubik = KubikAccount::get()->user();
    QStringList tip;
    if (account) {
        tip << tr("Playing as %1 (%2).").arg(account->profileName(), KubikAccounts::typeLabel(account));
    }
    if (!kubik.email.isEmpty()) {
        tip << tr("Signed in to kubik.gg as %1.").arg(kubik.email);
    }
    setToolTip(tip.join("\n"));
    setAccessibleName(account ? account->profileName() : tr("Add Minecraft account"));
    update();
}

void KubikProfileCard::enterEvent(QEnterEvent* event)
{
    m_hovered = true;
    update();
    QAbstractButton::enterEvent(event);
}

void KubikProfileCard::leaveEvent(QEvent* event)
{
    m_hovered = false;
    update();
    QAbstractButton::leaveEvent(event);
}

void KubikProfileCard::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange) {
        refresh();
    }
    QAbstractButton::changeEvent(event);
}

void KubikProfileCard::paintEvent(QPaintEvent* /*event*/)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    // A block on its lip, like every card; under the pointer, or with its menu open, it lights up.
    auto style = KubikPaint::panel();
    if (m_hovered || m_menuOpen || isDown()) {
        style.fill = KubikBrand::color("block");
        style.border = KubikBrand::color("block.edge");
    }
    const QRectF card(0, 0, width(), height() - style.lipHeight);
    KubikPaint::drawPanel(&p, card, style);

    const auto account = KubikAccounts::current();
    const QRectF avatarBox(PAD, (card.height() - AVATAR) / 2, AVATAR, AVATAR);
    if (account) {
        p.drawPixmap(avatarBox.topLeft(), KubikAccounts::avatar(account, AVATAR));
    } else {
        // No account yet: a plus on a grass-tinted square, the card's call to add one.
        QPainterPath box;
        box.addRoundedRect(avatarBox, 3, 3);
        p.fillPath(box, KubikBrand::color("block"));
        p.setPen(QPen(KubikBrand::color("grass"), 2.5, Qt::SolidLine, Qt::RoundCap));
        const QPointF c = avatarBox.center();
        p.drawLine(QPointF(c.x() - 7, c.y()), QPointF(c.x() + 7, c.y()));
        p.drawLine(QPointF(c.x(), c.y() - 7), QPointF(c.x(), c.y() + 7));
    }

    // The chevron: this card opens a menu.
    const qreal cx = card.width() - PAD - 5;
    const qreal cy = card.height() / 2;
    p.setPen(QPen(KubikBrand::color("bone.muted"), 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.setBrush(Qt::NoBrush);
    QPainterPath chevron;
    chevron.moveTo(cx - 5, cy - 2.5);
    chevron.lineTo(cx, cy + 2.5);
    chevron.lineTo(cx + 5, cy - 2.5);
    p.drawPath(chevron);

    const qreal textLeft = avatarBox.right() + 10;
    const qreal textWidth = cx - 9 - textLeft;
    QString name;
    QString line;
    QString lineColour = "bone.muted";
    if (!account) {
        name = tr("Not signed in");
        line = tr("Add Minecraft account");
        lineColour = "grass";
    } else {
        name = account->profileName();
        if (KubikAccounts::needsSignIn(account)) {
            line = tr("Sign in again");
            lineColour = "heart";
        } else if (account->accountType() == AccountType::Offline) {
            line = tr("Offline · kubik.gg");
        } else {
            line = tr("Microsoft · kubik.gg");
        }
    }

    QFont nameFont(KubikBrand::displayFamily());
    nameFont.setPixelSize(15);
    nameFont.setWeight(QFont::DemiBold);
    p.setFont(nameFont);
    p.setPen(KubikBrand::color("bone"));
    p.drawText(QRectF(textLeft, cy - 19, textWidth, 20), Qt::AlignLeft | Qt::AlignVCenter,
               QFontMetrics(nameFont).elidedText(name, Qt::ElideRight, int(textWidth)));

    QFont lineFont(KubikBrand::bodyFamily());
    lineFont.setPixelSize(12);
    p.setFont(lineFont);
    p.setPen(KubikBrand::color(lineColour));
    p.drawText(QRectF(textLeft, cy + 2, textWidth, 17), Qt::AlignLeft | Qt::AlignVCenter,
               QFontMetrics(lineFont).elidedText(line, Qt::ElideRight, int(textWidth)));
}

void KubikProfileCard::openMenu()
{
    auto* accounts = APPLICATION->accounts();
    QMenu menu(this);
    menu.setObjectName("kubikProfileMenu");
    menu.setToolTipsVisible(true);
    menu.setMinimumWidth(qMax(width(), 260));

    addHeader(&menu, tr("Minecraft"));
    const auto current = KubikAccounts::current();
    for (int i = 0; i < accounts->count(); ++i) {
        const auto account = accounts->at(i);
        // The text after the tab is drawn in the menu's shortcut column: the type, and which one plays.
        const QString type = KubikAccounts::typeLabel(account);
        const bool inUse = account == current;
        auto* action = menu.addAction(QIcon(KubikAccounts::avatar(account, 24)),
                                      account->profileName() + "\t" + (inUse ? tr("%1 · in use").arg(type) : type));
        if (inUse) {
            QFont font = action->font();
            font.setBold(true);
            action->setFont(font);
        }
        connect(action, &QAction::triggered, this, [accounts, account] { accounts->setDefaultAccount(account); });
    }
    if (accounts->count() > 0) {
        menu.addSeparator();
    }
    menu.addAction(KubikAccounts::microsoftIcon(), tr("Add Microsoft account"), this, [this] { KubikAccounts::addMicrosoft(window()); });
    auto* offline = menu.addAction(tr("Add offline profile"), this, [this] { KubikAccounts::addOffline(window()); });
    offline->setEnabled(KubikAccounts::canAddOffline());
    if (!offline->isEnabled()) {
        offline->setToolTip(tr("Add a Microsoft account that owns Minecraft first."));
    }
    menu.addAction(tr("Manage profiles…"), this, &KubikProfileCard::manageRequested);

    const auto& kubik = KubikAccount::get()->user();
    if (KubikAccount::get()->isSignedIn()) {
        menu.addSeparator();
        addHeader(&menu, tr("kubik.gg"));
        auto* who = menu.addAction(kubik.name.isEmpty() ? kubik.email : tr("%1\t%2").arg(kubik.name, kubik.email));
        who->setEnabled(false);
        menu.addAction(tr("Open kubik.gg"), this, [] { QDesktopServices::openUrl(QUrl(Kubik::webOrigin() + "/dashboard")); });
        menu.addAction(tr("Sign out of kubik.gg"), this, [] { KubikAccount::get()->signOut(); });
    }

    // The card sits at the bottom of the rail, so the menu opens upward, flush with its left edge.
    m_menuOpen = true;
    update();
    const QSize size = menu.sizeHint();
    menu.exec(mapToGlobal(QPoint(0, -size.height() - 6)));
    m_menuOpen = false;
    update();
}
