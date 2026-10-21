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
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QUrl>
#include <QVBoxLayout>

#include "Application.h"
#include "kubik/KubikAccount.h"
#include "kubik/KubikApi.h"
#include "kubik/KubikWindowChrome.h"
#include "minecraft/auth/AccountList.h"
#include "ui/kubik/KubikAccounts.h"
#include "ui/kubik/KubikPaint.h"
#include "ui/kubik/KubikTitle.h"

namespace {

constexpr int COLUMN = 600;
constexpr int LOCKUP_ICON = 56;
constexpr int LOCKUP_WORDMARK = 46;
constexpr int SPLASH_SIZE = 21;
// The ground's height on screen: its turf, and the dirt under it down to the window's bottom edge.
constexpr int GROUND = 76;

}  // namespace

KubikSignInPage::KubikSignInPage(QWidget* parent) : QWidget(parent)
{
    // Painted here (the sky, the ground) rather than by the style sheet; the sheet only colours the words
    // that stand on the sky (#kubikTitleScreen in kubik.qss).
    setObjectName("kubikTitleScreen");

    m_column = new QWidget(this);
    m_column->setFixedWidth(COLUMN);
    auto* content = new QVBoxLayout(m_column);
    content->setContentsMargins(0, 0, 0, 0);
    content->setSpacing(12);

    // The lockup, as on kubik.gg/launcher: the icon, then the name as the title.
    auto* mark = new QLabel(m_column);
    mark->setPixmap(APPLICATION->logo().pixmap(LOCKUP_ICON, LOCKUP_ICON));
    m_wordmark = new KubikWordmark(LOCKUP_WORDMARK, m_column);
    auto* lockup = new QHBoxLayout();
    lockup->setSpacing(14);
    lockup->addStretch();
    lockup->addWidget(mark, 0, Qt::AlignVCenter);
    lockup->addWidget(m_wordmark, 0, Qt::AlignVCenter);
    lockup->addStretch();
    content->addLayout(lockup);
    content->addSpacing(34);

    m_title = new QLabel(m_column);
    m_title->setObjectName("kubikPageTitle");
    m_title->setWordWrap(true);
    m_title->setAlignment(Qt::AlignHCenter);
    content->addWidget(m_title);

    m_body = new QLabel(m_column);
    m_body->setObjectName("kubikPageBody");
    m_body->setWordWrap(true);
    m_body->setAlignment(Qt::AlignHCenter);
    content->addWidget(m_body);

    m_error = new QLabel(m_column);
    m_error->setObjectName("kubikPageError");
    m_error->setWordWrap(true);
    m_error->setAlignment(Qt::AlignHCenter);
    content->addWidget(m_error);

    m_signIn = new QPushButton(m_column);
    m_signIn->setObjectName("kubikHeroPlay");
    m_signIn->setProperty("kubikRole", "primary");
    m_signIn->setCursor(Qt::PointingHandCursor);
    connect(m_signIn, &QPushButton::clicked, this, [this] {
        m_lastError.clear();
        KubikAccount::get()->signIn();
        updateState();
    });
    m_cancel = new QPushButton(m_column);
    m_cancel->setProperty("kubikRole", "quiet");
    m_cancel->setCursor(Qt::PointingHandCursor);
    connect(m_cancel, &QPushButton::clicked, this, [] { KubikAccount::get()->cancelSignIn(); });

    auto* buttons = new QHBoxLayout();
    buttons->setSpacing(10);
    buttons->addStretch();
    buttons->addWidget(m_signIn);
    buttons->addWidget(m_cancel);
    buttons->addStretch();
    content->addSpacing(14);
    content->addLayout(buttons);

    m_signUp = new QPushButton(m_column);
    m_signUp->setProperty("kubikRole", "quiet");
    m_signUp->setCursor(Qt::PointingHandCursor);
    connect(m_signUp, &QPushButton::clicked, this, [] { QDesktopServices::openUrl(QUrl(Kubik::webOrigin() + "/signup")); });
    content->addSpacing(2);
    content->addWidget(m_signUp, 0, Qt::AlignHCenter);

    // The step before this one, confirmed: who plays, once kubik.gg is connected too. A dark block in the
    // sky, like the launcher window standing in kubik.gg/launcher's.
    m_minecraft = new QFrame(m_column);
    m_minecraft->setObjectName("kubikProfileRow");
    m_minecraft->setFixedWidth(400);
    m_minecraftFace = new QLabel(m_minecraft);
    m_minecraftName = new QLabel(m_minecraft);
    m_minecraftName->setObjectName("kubikSectionTitle");
    m_minecraftOk = new QLabel(m_minecraft);
    m_minecraftOk->setObjectName("kubikStateOk");
    m_minecraftState = new QLabel(m_minecraft);
    m_minecraftState->setObjectName("kubikPageSubtitle");
    m_minecraftState->setWordWrap(true);
    auto* words = new QVBoxLayout();
    words->setSpacing(0);
    words->addWidget(m_minecraftName);
    words->addWidget(m_minecraftOk);
    words->addWidget(m_minecraftState);
    auto* row = new QHBoxLayout(m_minecraft);
    row->setContentsMargins(12, 10, 14, 10);
    row->setSpacing(12);
    row->addWidget(m_minecraftFace, 0, Qt::AlignVCenter);
    row->addLayout(words, 1);
    content->addSpacing(26);
    content->addWidget(m_minecraft, 0, Qt::AlignHCenter);

    // The splash floats over the lockup's corner, outside every layout.
    m_splash = new KubikSplash(SPLASH_SIZE, this);
    m_splash->raise();
    m_column->installEventFilter(this);
    m_wordmark->installEventFilter(this);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(40, 32 + KubikWindowChrome::titleBarInset(), 40, GROUND + 24);
    layout->addStretch(2);
    // Stretches beside the column, not an alignment flag: an aligned item is given its unwrapped height,
    // which clips the column's wrapped lines.
    auto* across = new QHBoxLayout();
    across->addStretch();
    across->addWidget(m_column);
    across->addStretch();
    layout->addLayout(across);
    layout->addStretch(3);

    connect(KubikAccount::get(), &KubikAccount::changed, this, &KubikSignInPage::updateState);
    connect(KubikAccount::get(), &KubikAccount::signInFailed, this, [this](const QString& message) {
        m_lastError = message;
        updateState();
    });
    auto* accounts = APPLICATION->accounts();
    connect(accounts, &AccountList::defaultAccountChanged, this, &KubikSignInPage::updateMinecraft);
    connect(accounts, &AccountList::listChanged, this, &KubikSignInPage::updateMinecraft);
    connect(accounts, &AccountList::listActivityChanged, this, &KubikSignInPage::updateMinecraft);

    retranslate();
}

void KubikSignInPage::retranslate()
{
    // The same lines as kubik.gg/launcher's title screen; one is picked each time the launcher opens.
    m_splash->setLines(
        { tr("Free!"), tr("Just press Play!"), tr("Bring your friends!"), tr("Modpacks too!"), tr("No mod hunting!"), tr("Open source!") });
    placeSplash();
    m_cancel->setText(tr("Cancel"));
    m_signUp->setText(tr("No kubik.gg account yet? Create one"));
    updateState();
    updateMinecraft();
}

void KubikSignInPage::updateMinecraft()
{
    const auto account = KubikAccounts::current();
    m_minecraftFace->setPixmap(account ? KubikAccounts::avatar(account, 36) : KubikAccounts::microsoftIcon().pixmap(24, 24));
    m_minecraftFace->setFixedSize(36, 36);
    m_minecraftFace->setAlignment(Qt::AlignCenter);
    if (!account) {
        m_minecraftName->setText(tr("No Minecraft account yet"));
        m_minecraftState->setText(tr("You can add it from your profile card once you're in. Joining a server needs it."));
    } else if (KubikAccounts::needsSignIn(account)) {
        m_minecraftName->setText(account->profileName());
        m_minecraftState->setText(tr("Microsoft needs you to sign in again. You can do that from your profile card once you're in."));
    } else {
        m_minecraftName->setText(account->profileName());
        m_minecraftOk->setText(QStringLiteral("✓ ") + tr("%1 connected").arg(KubikAccounts::typeLabel(account)));
    }
    // Two labels rather than one restyled: a wrapped label whose font changes after layout keeps the old
    // height and clips its last line.
    const bool ready = account && !KubikAccounts::needsSignIn(account);
    m_minecraftOk->setVisible(ready);
    m_minecraftState->setVisible(!ready);
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

void KubikSignInPage::placeSplash()
{
    // Over the wordmark's lower right corner, leaning out past it, as the website hangs it.
    const QRect name(m_wordmark->mapTo(this, QPoint(0, 0)), m_wordmark->size());
    const QPoint centre(name.right() - name.height() / 4, name.bottom() - name.height() / 8);
    m_splash->move(centre - QPoint(m_splash->width() / 2, m_splash->height() / 2));
}

bool KubikSignInPage::eventFilter(QObject* watched, QEvent* event)
{
    if ((watched == m_column || watched == m_wordmark) && (event->type() == QEvent::Move || event->type() == QEvent::Resize)) {
        placeSplash();
    }
    return QWidget::eventFilter(watched, event);
}

void KubikSignInPage::paintEvent(QPaintEvent* /*event*/)
{
    using KubikPaint::Sprite;
    QPainter p(this);
    const qreal w = width();
    const qreal groundTop = height() - GROUND;
    KubikPaint::drawDaySky(&p, QRectF(0, 0, w, groundTop));

    // Noon's residents, as on kubik.gg/launcher: the square sun high on the right, clouds drifting at
    // the edges where the words are not, and on the horizon a pair of oaks and a Pig.
    const int top = KubikWindowChrome::titleBarInset();
    KubikPaint::drawSprite(&p, Sprite::Sun, QPointF(w * 0.93 - 48, top + 34), 6);
    KubikPaint::drawSprite(&p, Sprite::Cloud, QPointF(w * 0.04, top + 46), 5, 0.9);
    KubikPaint::drawSprite(&p, Sprite::Cloud, QPointF(w * 0.91 - 80, groundTop * 0.34), 5, 0.8);
    KubikPaint::drawSprite(&p, Sprite::Cloud, QPointF(w * 0.07, groundTop * 0.58), 4, 0.7);
    KubikPaint::drawSprite(&p, Sprite::Cloud, QPointF(w * 0.94 - 64, groundTop * 0.72), 4, 0.75);
    // On the horizon, a few pixels into the turf.
    KubikPaint::drawSprite(&p, Sprite::Oak, QPointF(w * 0.04, groundTop - 65 + 6), 5);
    KubikPaint::drawSprite(&p, Sprite::Oak, QPointF(w * 0.13, groundTop - 52 + 5), 4);
    KubikPaint::drawSprite(&p, Sprite::Pig, QPointF(w * 0.80, groundTop - 44 + 4), 4);
    KubikPaint::drawSprite(&p, Sprite::Oak, QPointF(w * 0.96 - 65, groundTop - 65 + 6), 5);

    KubikPaint::drawGround(&p, QRectF(0, groundTop, w, GROUND), false);
}

void KubikSignInPage::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange) {
        retranslate();
    }
    QWidget::changeEvent(event);
}
