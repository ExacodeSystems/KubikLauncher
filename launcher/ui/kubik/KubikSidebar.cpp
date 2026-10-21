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
#include <QDesktopServices>
#include <QEvent>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QMenu>
#include <QPainter>
#include <QPushButton>
#include <QUrl>
#include <QVBoxLayout>

#include "Application.h"
#include "kubik/KubikWindowChrome.h"
#include "ui/kubik/KubikPaint.h"
#include "ui/kubik/KubikProfileCard.h"
#include "ui/kubik/KubikUpdateNotice.h"

namespace {

// The rail's head, top to bottom: a strip of sky for the sun and a cloud (on macOS, below the
// traffic lights), the lockup, a gap, then the ground and the steps it hangs into the rail with.
constexpr int SKY_STRIP = 30;
constexpr int LOCKUP = 36;
constexpr int LOCKUP_GAP = 12;
constexpr int DIRT = 14;
constexpr int STEPS = 16;
constexpr int ICON = 30;
constexpr int WORDMARK = 22;

/**
 * The top of the rail is KubikLauncher's title screen in miniature, as kubik.gg/launcher opens: the
 * icon and the wordmark in a noon sky with the square sun and a cloud, standing on the ground, whose
 * stepped underside drops into the rail's dark. The rest of the app is the world under that ground.
 */
class RailHead : public QWidget {
   public:
    explicit RailHead(QWidget* parent) : QWidget(parent)
    {
        setAccessibleName(QStringLiteral("KubikLauncher"));
        setFixedHeight(KubikWindowChrome::titleBarInset() + SKY_STRIP + LOCKUP + LOCKUP_GAP + KubikPaint::TURF_HEIGHT + DIRT + STEPS);
    }

   protected:
    void paintEvent(QPaintEvent* /*event*/) override
    {
        QPainter p(this);
        const qreal groundTop = height() - STEPS - DIRT - KubikPaint::TURF_HEIGHT;
        KubikPaint::drawDaySky(&p, QRectF(0, 0, width(), groundTop));
        const int top = KubikWindowChrome::titleBarInset();
        KubikPaint::drawSprite(&p, KubikPaint::Sprite::Sun, QPointF(width() - 16 - 24, top + 4), 3);
        KubikPaint::drawSprite(&p, KubikPaint::Sprite::Cloud, QPointF(width() - 112, top + 9), 3, 0.9);
        KubikPaint::drawGround(&p, QRectF(0, groundTop, width(), KubikPaint::TURF_HEIGHT + DIRT));

        // The lockup: the icon over its hard shadow, then the name.
        const qreal lockupTop = top + SKY_STRIP;
        const QRectF icon(16, lockupTop + (LOCKUP - ICON) / 2.0, ICON, ICON);
        KubikPaint::drawLockupIcon(&p, APPLICATION->logo(), icon);
        const QSizeF name = KubikPaint::wordmarkSize(WORDMARK);
        KubikPaint::drawWordmark(&p, QPointF(icon.right() + 9, lockupTop + (LOCKUP - name.height()) / 2), WORDMARK);
    }
};

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
    // The head runs to the window's top edge: on macOS the traffic lights sit in its sky.
    layout->setContentsMargins(0, 0, 0, 14);
    layout->setSpacing(2);

    layout->addWidget(new RailHead(this));
    layout->addSpacing(8);

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

    m_settings = railButton("settings", this);
    m_more = railButton("help", this);
    layout->addWidget(m_settings);
    layout->addWidget(m_more);
    connect(m_settings, &QPushButton::clicked, this, &KubikSidebar::settingsRequested);
    // "Help & more" opens beside the rail, level with its row.
    connect(m_more, &QPushButton::clicked, this, [this] {
        if (m_moreMenu) {
            m_moreMenu->popup(m_more->mapToGlobal(QPoint(m_more->width() + 6, 0)));
        }
    });

    // A newer KubikLauncher, where the launcher cannot install it itself (macOS, an AppImage). Filled
    // in by KubikUpdateNotice; hidden until there is one.
    m_update = new QFrame(this);
    m_update->setObjectName("kubikUpdateNotice");
    m_updateText = new QLabel(m_update);
    m_updateText->setObjectName("kubikUpdateText");
    m_updateText->setWordWrap(true);
    m_updateClose = new QPushButton(QStringLiteral("×"), m_update);
    m_updateClose->setObjectName("kubikUpdateClose");
    m_updateClose->setProperty("kubikRole", "quiet");
    m_updateClose->setCursor(Qt::PointingHandCursor);
    m_updateDownload = new QPushButton(m_update);
    m_updateDownload->setProperty("kubikRole", "primary");
    m_updateDownload->setCursor(Qt::PointingHandCursor);
    auto* updateTop = new QHBoxLayout();
    updateTop->setSpacing(4);
    updateTop->addWidget(m_updateText, 1);
    updateTop->addWidget(m_updateClose, 0, Qt::AlignTop);
    auto* updateLayout = new QVBoxLayout(m_update);
    updateLayout->setContentsMargins(12, 10, 8, 14);
    updateLayout->setSpacing(8);
    updateLayout->addLayout(updateTop);
    updateLayout->addWidget(m_updateDownload, 0, Qt::AlignLeft);
    auto* updateRow = new QHBoxLayout();
    updateRow->setContentsMargins(12, 10, 12, 0);
    updateRow->addWidget(m_update);
    layout->addLayout(updateRow);
    m_update->hide();
    connect(m_updateDownload, &QPushButton::clicked, this,
            [] { QDesktopServices::openUrl(QUrl(KubikUpdateNotice::downloadPage())); });
    connect(m_updateClose, &QPushButton::clicked, this, [this] {
        m_update->hide();
        emit updateNoticeDismissed(m_updateVersion);
    });

    // Who plays, on a card of its own under everything else, so it reads as an account and not a page.
    m_profile = new KubikProfileCard(this);
    auto* profileRow = new QHBoxLayout();
    profileRow->setContentsMargins(12, 10, 12, 0);
    profileRow->addWidget(m_profile);
    layout->addLayout(profileRow);
    connect(m_profile, &KubikProfileCard::manageRequested, this, &KubikSidebar::profilesRequested);

    retranslate();
}

void KubikSidebar::setCurrentPage(Page page)
{
    if (auto* button = m_pages->button(page)) {
        button->setChecked(true);
    }
    emit pageRequested(page);
}

void KubikSidebar::setMoreMenu(QMenu* menu)
{
    m_moreMenu = menu;
}

void KubikSidebar::setUpdateNotice(const QString& version)
{
    m_updateVersion = version;
    retranslate();
    m_update->setVisible(!version.isEmpty());
}

void KubikSidebar::retranslate()
{
    m_servers->setText(tr("Servers"));
    m_instances->setText(tr("Instances"));
    m_settings->setText(tr("Settings"));
    // "&&": a lone "&" in a button's text marks a keyboard mnemonic and is not shown.
    m_more->setText(tr("Help && more"));
    m_updateText->setText(tr("KubikLauncher %1 is out.").arg(m_updateVersion));
    m_updateDownload->setText(tr("Download"));
    m_updateClose->setToolTip(tr("Hide until the next version"));
    m_updateClose->setAccessibleName(tr("Hide until the next version"));
}

void KubikSidebar::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange) {
        retranslate();
    }
    QFrame::changeEvent(event);
}
