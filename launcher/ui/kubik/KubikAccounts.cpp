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

#include "KubikAccounts.h"

#include <QApplication>
#include <QCheckBox>
#include <QCoreApplication>
#include <QPainter>
#include <QPainterPath>

#include "Application.h"
#include "kubik/KubikBrand.h"
#include "kubik/KubikDev.h"
#include "minecraft/auth/AccountList.h"
#include "ui/dialogs/ChooseOfflineNameDialog.h"
#include "ui/dialogs/MSALoginDialog.h"

namespace KubikAccounts {

MinecraftAccountPtr addMicrosoft(QWidget* parent)
{
    auto account = MSALoginDialog::newAccount(parent);
    if (!account) {
        return MinecraftAccountPtr();
    }
    auto* accounts = APPLICATION->accounts();
    accounts->addAccount(account);
    accounts->setDefaultAccount(account);
    return account;
}

bool canAddOffline()
{
    return APPLICATION->accounts()->anyAccountIsValid() || Kubik::devOfflineAllowed();
}

MinecraftAccountPtr addOffline(QWidget* parent)
{
    if (!canAddOffline()) {
        return MinecraftAccountPtr();
    }
    ChooseOfflineNameDialog dialog(
        QCoreApplication::translate("KubikAccounts", "Choose a name for this offline profile. It's the name other players see in game."),
        parent);
    dialog.setWindowTitle(QCoreApplication::translate("KubikAccounts", "Add offline profile"));
    // Prism's escape hatch for names Minecraft rejects; a player has no use for it.
    if (auto* allowInvalid = dialog.findChild<QCheckBox*>("allowInvalidUsernames")) {
        allowInvalid->hide();
    }
    if (dialog.exec() != QDialog::Accepted) {
        return MinecraftAccountPtr();
    }
    const auto account = MinecraftAccount::createOffline(dialog.getUsername());
    if (!account) {
        return MinecraftAccountPtr();
    }
    account->login()->start();  // an offline "login" completes on the spot
    auto* accounts = APPLICATION->accounts();
    accounts->addAccount(account);
    accounts->setDefaultAccount(account);
    return account;
}

MinecraftAccountPtr current()
{
    return APPLICATION->accounts()->defaultAccount();
}

bool needsSignIn(const MinecraftAccountPtr& account)
{
    if (!account) {
        return false;
    }
    const auto state = account->accountState();
    return state == AccountState::Errored || state == AccountState::Expired || state == AccountState::Gone ||
           state == AccountState::Disabled;
}

QPixmap avatar(const MinecraftAccountPtr& account, int size)
{
    const qreal dpr = qApp->devicePixelRatio();
    QPixmap canvas(QSize(size, size) * dpr);
    canvas.setDevicePixelRatio(dpr);
    canvas.fill(Qt::transparent);

    QPainter p(&canvas);
    p.setRenderHint(QPainter::Antialiasing);
    const QRectF box(0, 0, size, size);
    QPainterPath shape;
    shape.addRoundedRect(box, size * 0.22, size * 0.22);
    p.setClipPath(shape);

    const QPixmap face = account ? account->getFace(int(size * dpr), int(size * dpr)) : QPixmap();
    if (!face.isNull()) {
        p.setRenderHint(QPainter::SmoothPixmapTransform, false);  // a skin is pixel art
        p.drawPixmap(box, face, QRectF(face.rect()));
        return canvas;
    }

    // No skin: offline profiles, and Microsoft accounts before their first sign-in finishes. A letter
    // on a tone picked from the name, so two profiles side by side are told apart.
    static const char* tones[] = { "grass.deep", "sky.deep", "gold.deep", "ender.deep", "dirt" };
    const QString name = account ? account->profileName() : QString();
    p.fillRect(box, KubikBrand::color(tones[qHash(name) % (sizeof(tones) / sizeof(tones[0]))]));
    QFont font(KubikBrand::displayFamily());
    font.setPixelSize(int(size * 0.5));
    font.setWeight(QFont::Bold);
    p.setFont(font);
    p.setPen(KubikBrand::color("bone"));
    p.drawText(box, Qt::AlignCenter, name.isEmpty() ? QStringLiteral("?") : name.left(1).toUpper());
    return canvas;
}

QString typeLabel(const MinecraftAccountPtr& account)
{
    if (account && account->accountType() == AccountType::Offline) {
        return QCoreApplication::translate("KubikAccounts", "Offline profile");
    }
    return QCoreApplication::translate("KubikAccounts", "Microsoft account");
}

QIcon microsoftIcon()
{
    // Microsoft's identity branding asks for its four squares on a "Sign in with Microsoft" button.
    // Drawn with a transparent strip on the right: the gap between the mark and the button's text.
    QPixmap mark(QSize(52, 42));
    mark.fill(Qt::transparent);
    QPainter p(&mark);
    p.fillRect(QRect(0, 0, 20, 20), QColor("#f25022"));
    p.fillRect(QRect(22, 0, 20, 20), QColor("#7fba00"));
    p.fillRect(QRect(0, 22, 20, 20), QColor("#00a4ef"));
    p.fillRect(QRect(22, 22, 20, 20), QColor("#ffb900"));
    p.end();
    return QIcon(mark);
}

}  // namespace KubikAccounts
