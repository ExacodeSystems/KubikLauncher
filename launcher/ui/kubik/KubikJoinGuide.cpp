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

#include "KubikJoinGuide.h"

#include <QButtonGroup>
#include <QEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLocale>
#include <QPushButton>
#include <QVBoxLayout>

#include "kubik/KubikBrand.h"
#include "kubik/KubikGameText.h"
#include "ui/kubik/KubikJoinShots.h"

namespace {

// Wide enough for the picture's buttons to read like the game's, and no wider: past this it only grows.
constexpr int MAX_WIDTH = 720;

const KubikJoinShots::Shot SHOTS[] = { KubikJoinShots::Shot::Connecting, KubikJoinShots::Shot::TitleScreen,
                                       KubikJoinShots::Shot::ServerList, KubikJoinShots::Shot::AddServer };

/** A button's name as the game shows it, set apart in a caption. */
QString gameWord(const QString& word)
{
    return QString("<span style=\"color:%1; font-weight:600;\">%2</span>").arg(KubikBrand::color("bone").name(), word.toHtmlEscaped());
}

/** The server's address in a caption: monospaced on a block, the one thing the player types. */
QString addressChip(const QString& address)
{
    return QString("<span style=\"font-family:'%1'; color:%2; background-color:%3;\">&nbsp;%4&nbsp;</span>")
        .arg(KubikBrand::monoFamily(), KubikBrand::color("bone").name(), KubikBrand::color("block").name(), address.toHtmlEscaped());
}

}  // namespace

KubikJoinGuide::KubikJoinGuide(QWidget* parent) : QWidget(parent)
{
    setMaximumWidth(MAX_WIDTH);

    m_title = new QLabel(this);
    m_title->setObjectName("kubikSectionTitle");
    m_title->setTextFormat(Qt::PlainText);
    m_dismiss = new QPushButton(this);
    m_dismiss->setProperty("kubikRole", "quiet");
    m_dismiss->setCursor(Qt::PointingHandCursor);
    connect(m_dismiss, &QPushButton::clicked, this, &KubikJoinGuide::dismissed);
    auto* header = new QHBoxLayout();
    header->addWidget(m_title, 1);
    header->addWidget(m_dismiss);

    m_languageNote = new QLabel(this);
    m_languageNote->setObjectName("kubikPageNote");
    m_languageNote->setWordWrap(true);

    // The steps' tabs, styled as the instance page's: all four named, the current one underlined.
    auto* group = new QButtonGroup(this);
    auto* tabs = new QHBoxLayout();
    tabs->setSpacing(22);
    for (int i = 0; i < STEPS; ++i) {
        auto* tab = new QPushButton(this);
        tab->setObjectName("kubikTab");
        tab->setCheckable(true);
        tab->setCursor(Qt::PointingHandCursor);
        group->addButton(tab, i);
        tabs->addWidget(tab);
        m_tabs.append(tab);
    }
    tabs->addStretch();
    connect(group, &QButtonGroup::idClicked, this, &KubikJoinGuide::showStep);

    auto* card = new QFrame(this);
    card->setObjectName("kubikJoinStep");
    m_shot = new KubikJoinShotView(SHOTS[0], card);
    m_number = new QLabel(card);
    m_number->setObjectName("kubikStepNumber");
    m_number->setAlignment(Qt::AlignCenter);
    m_number->setFixedSize(26, 26);
    m_stepTitle = new QLabel(card);
    m_stepTitle->setObjectName("kubikStepTitle");
    m_stepTitle->setWordWrap(true);
    m_stepTitle->setTextFormat(Qt::RichText);
    m_stepBody = new QLabel(card);
    m_stepBody->setObjectName("kubikPageBody");
    m_stepBody->setWordWrap(true);
    m_stepBody->setTextFormat(Qt::RichText);
    m_stepBody->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_stepBody->setAlignment(Qt::AlignLeft | Qt::AlignTop);

    m_back = new QPushButton(card);
    m_back->setProperty("kubikRole", "quiet");
    m_back->setCursor(Qt::PointingHandCursor);
    connect(m_back, &QPushButton::clicked, this, [this] { showStep(m_step - 1); });
    m_next = new QPushButton(card);
    m_next->setProperty("kubikRole", "primary");
    m_next->setCursor(Qt::PointingHandCursor);
    connect(m_next, &QPushButton::clicked, this, [this] { showStep(m_step + 1); });

    auto* heading = new QHBoxLayout();
    heading->setSpacing(10);
    heading->addWidget(m_number, 0, Qt::AlignTop);
    heading->addWidget(m_stepTitle, 1, Qt::AlignVCenter);
    auto* footer = new QHBoxLayout();
    footer->addWidget(m_back);
    footer->addStretch();
    footer->addWidget(m_next);

    auto* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(12, 12, 16, 14);
    cardLayout->setSpacing(10);
    cardLayout->addWidget(m_shot);
    cardLayout->addSpacing(4);
    cardLayout->addLayout(heading);
    cardLayout->addWidget(m_stepBody, 1);
    cardLayout->addLayout(footer);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);
    layout->addLayout(header);
    layout->addWidget(m_languageNote);
    layout->addLayout(tabs);
    layout->addSpacing(4);
    layout->addWidget(card);

    retranslate();
}

void KubikJoinGuide::setServer(const Kubik::Server& server, const std::optional<KubikServerLive>& live, const QString& gameLanguage)
{
    m_server = server;
    m_live = live;
    m_gameLanguage = gameLanguage;
    m_shot->setSubject({ server, live, Kubik::gameLabels(gameLanguage) });
    retranslate();
}

void KubikJoinGuide::showStep(int index)
{
    m_step = qBound(0, index, STEPS - 1);
    m_tabs.at(m_step)->setChecked(true);
    m_shot->setShot(SHOTS[m_step]);
    m_number->setText(QString::number(m_step + 1));
    if (m_step < m_titles.size()) {
        m_stepTitle->setText(m_titles.at(m_step));
        m_stepBody->setText(m_bodies.at(m_step));
    }
    m_back->setVisible(m_step > 0);
    m_next->setVisible(m_step < STEPS - 1);
}

void KubikJoinGuide::retranslate()
{
    m_dismiss->setText(tr("Got it"));
    m_back->setText(tr("← Back"));
    m_next->setText(tr("Next step →"));

    const auto labels = Kubik::gameLabels(m_gameLanguage);
    // The tabs name what each step presses, in the game's words.
    const QString tabNames[] = { tr("Play"), labels.multiplayer, labels.joinServer, labels.addServer };
    for (int i = 0; i < STEPS; ++i) {
        m_tabs.at(i)->setText(QString("%1  %2").arg(i + 1).arg(tabNames[i]));
    }

    m_titles.clear();
    m_bodies.clear();
    if (!m_server) {
        showStep(m_step);
        return;
    }
    const auto& server = *m_server;
    const QString name = server.name.toHtmlEscaped();
    const QString profile = server.profile ? server.profile->label.toHtmlEscaped() : QString();
    const bool modded = server.profile && (!server.profile->loader.isEmpty() || server.profile->isModpack());

    m_title->setText(tr("How to join %1").arg(server.name));

    // The pictures and the button names follow the game, which can be set to another language than this launcher.
    const bool gameIndonesian = labels.languageCode == "id_id";
    const bool launcherIndonesian = QLocale().language() == QLocale::Indonesian;
    m_languageNote->setVisible(gameIndonesian != launcherIndonesian);
    m_languageNote->setText(
        tr("The pictures show the buttons in %1, as your Minecraft shows them.").arg(gameIndonesian ? tr("Indonesian") : tr("English")));

    m_titles << tr("Press Play").toHtmlEscaped();
    m_bodies << (modded ? tr("KubikLauncher installs %1 and the server's mods. Minecraft then opens and connects to %2 by itself.")
                        : tr("KubikLauncher installs %1. Minecraft then opens and connects to %2 by itself."))
                    .arg(profile, name);

    m_titles << tr("Coming back? Press %1").arg(gameWord(labels.multiplayer));
    m_bodies << tr("Left the server or got disconnected? %1 is on Minecraft's title screen, under %2.")
                    .arg(gameWord(labels.multiplayer), gameWord(labels.singleplayer));

    m_titles << tr("Pick %1, then %2").arg(name, gameWord(labels.joinServer));
    m_bodies << tr("Each Play puts %1 at the top of the list, so it is always the first server you see.").arg(name);

    m_titles << tr("From another launcher").toHtmlEscaped();
    m_bodies << (modded ? tr("Press %1, type %2 as the %3, then %4. Play it with %5 and the mods this page lists.")
                        : tr("Press %1, type %2 as the %3, then %4. Play it with %5."))
                    .arg(gameWord(labels.addServer), addressChip(server.address), gameWord(labels.serverAddress), gameWord(labels.done),
                         profile);
    showStep(m_step);
}

void KubikJoinGuide::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange) {
        retranslate();
    }
    QWidget::changeEvent(event);
}
