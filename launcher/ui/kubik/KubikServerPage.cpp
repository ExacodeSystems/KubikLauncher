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

#include "KubikServerPage.h"

#include <QClipboard>
#include <QDesktopServices>
#include <QEvent>
#include <QFrame>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollArea>
#include <QUrl>
#include <QVBoxLayout>

#include "Application.h"
#include "BaseInstance.h"
#include "icons/IconList.h"
#include "kubik/KubikAccount.h"
#include "kubik/KubikApi.h"
#include "kubik/KubikBrand.h"
#include "kubik/KubikPlay.h"
#include "kubik/KubikProfiles.h"
#include "kubik/KubikWindowChrome.h"
#include "minecraft/auth/AccountList.h"
#include "settings/SettingsObject.h"
#include "ui/kubik/KubikAccounts.h"
#include "ui/kubik/KubikInstancePicker.h"
#include "ui/kubik/KubikServerTiles.h"

namespace {

constexpr int MARGIN = 32;
constexpr int ART_SIDE = 168;

QLabel* sectionTitle(QWidget* parent)
{
    auto* label = new QLabel(parent);
    label->setObjectName("kubikSectionTitle");
    return label;
}

QLabel* bodyText(QWidget* parent, const char* objectName = "kubikPageBody")
{
    auto* label = new QLabel(parent);
    label->setObjectName(objectName);
    label->setWordWrap(true);
    label->setTextFormat(Qt::PlainText);
    return label;
}

/** A mod's file name, the way a person reads it: no `.jar`. */
QString modLabel(const Kubik::Mod& mod)
{
    QString name = mod.filename;
    if (name.endsWith(".jar")) {
        name.chop(4);
    }
    return name;
}

}  // namespace

KubikServerPage::KubikServerPage(QWidget* parent) : QWidget(parent)
{
    setObjectName("kubikPage");
    setAttribute(Qt::WA_StyledBackground);

    auto* content = new QWidget(this);
    content->setObjectName("kubikPage");

    m_back = new QPushButton(content);
    m_back->setProperty("kubikRole", "quiet");
    m_back->setCursor(Qt::PointingHandCursor);
    connect(m_back, &QPushButton::clicked, this, &KubikServerPage::backRequested);

    // ── Hero: the server and its one big action ─────────────────────────────
    m_art = new QLabel(content);
    m_art->setObjectName("kubikArt");
    m_art->setFixedSize(ART_SIDE, ART_SIDE);
    m_art->setAlignment(Qt::AlignCenter);

    m_name = new QLabel(content);
    m_name->setObjectName("kubikPageTitle");
    m_name->setTextFormat(Qt::PlainText);
    m_about = bodyText(content);
    m_status = new QLabel(content);
    m_status->setObjectName("kubikPageSubtitle");

    m_address = new QLabel(content);
    m_address->setObjectName("kubikPageBody");
    m_address->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_copy = new QPushButton(content);
    m_copy->setProperty("kubikRole", "quiet");
    m_copy->setIcon(QIcon::fromTheme("copy"));
    m_copy->setCursor(Qt::PointingHandCursor);
    connect(m_copy, &QPushButton::clicked, this, [this] {
        QString address = m_server.address;
        if (m_server.edition == "bedrock" && m_server.port > 0) {
            address += ":" + QString::number(m_server.port);
        }
        QGuiApplication::clipboard()->setText(address);
        m_copy->setText(tr("Copied"));
    });
    m_open = new QPushButton(content);
    m_open->setProperty("kubikRole", "quiet");
    m_open->setCursor(Qt::PointingHandCursor);
    connect(m_open, &QPushButton::clicked, this, [this] { QDesktopServices::openUrl(QUrl(m_server.dashboardUrl)); });
    auto* addressRow = new QHBoxLayout();
    addressRow->setSpacing(6);
    addressRow->addWidget(m_address);
    addressRow->addWidget(m_copy);
    addressRow->addWidget(m_open);
    addressRow->addStretch();

    m_playButton = new QPushButton(content);
    m_playButton->setObjectName("kubikHeroPlay");
    m_playButton->setProperty("kubikRole", "primary");
    m_playButton->setCursor(Qt::PointingHandCursor);
    connect(m_playButton, &QPushButton::clicked, this, [this] {
        if (m_play) {
            m_play->cancel();
        } else {
            play();
        }
    });
    // One action in the hero: Play. Its natural width, so it never pushes past a narrow window.
    m_playButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_howToJoin = new QPushButton(content);
    m_howToJoin->setObjectName("kubikHowToJoin");
    m_howToJoin->setProperty("kubikRole", "link");
    m_howToJoin->setCursor(Qt::PointingHandCursor);
    connect(m_howToJoin, &QPushButton::clicked, this, [this] {
        m_joinOpen = true;
        updateJoinCard();
    });
    auto* actions = new QHBoxLayout();
    actions->setSpacing(10);
    actions->addWidget(m_playButton);
    actions->addWidget(m_howToJoin);
    actions->addStretch();

    m_progressText = bodyText(content, "kubikPageSubtitle");
    m_progress = new QProgressBar(content);
    m_progress->setTextVisible(false);
    m_progress->setFixedHeight(6);
    m_progress->setMaximumWidth(420);
    m_accountNote = bodyText(content, "kubikPageNote");

    m_notice = bodyText(content);
    m_noticeAction = new QPushButton(content);
    m_noticeAction->setCursor(Qt::PointingHandCursor);
    m_noticeAction->setIconSize(QSize(20, 16));
    connect(m_noticeAction, &QPushButton::clicked, this, [this] {
        if (m_noticeCallback) {
            m_noticeCallback();
        }
    });

    // ── How joining works: open by itself on the first game, a link after that ──
    m_joinCard = new QFrame(content);
    m_joinCard->setObjectName("kubikJoinCard");
    m_joinTitle = sectionTitle(m_joinCard);
    m_joinSteps = bodyText(m_joinCard);
    m_joinDismiss = new QPushButton(m_joinCard);
    m_joinDismiss->setProperty("kubikRole", "quiet");
    m_joinDismiss->setCursor(Qt::PointingHandCursor);
    connect(m_joinDismiss, &QPushButton::clicked, this, [this] {
        APPLICATION->settings()->set("KubikJoinTipSeen", true);
        m_joinOpen = false;
        updateJoinCard();
    });
    auto* joinLayout = new QVBoxLayout(m_joinCard);
    joinLayout->setContentsMargins(18, 16, 18, 12);
    joinLayout->setSpacing(8);
    joinLayout->addWidget(m_joinTitle);
    joinLayout->addWidget(m_joinSteps);
    joinLayout->addWidget(m_joinDismiss, 0, Qt::AlignRight);

    auto* words = new QVBoxLayout();
    words->setSpacing(6);
    words->addStretch();
    words->addWidget(m_name);
    words->addWidget(m_about);
    words->addLayout(addressRow);
    words->addWidget(m_status);
    words->addSpacing(14);
    words->addLayout(actions);
    words->addWidget(m_progressText);
    words->addWidget(m_progress);
    words->addWidget(m_accountNote);
    words->addStretch();

    auto* hero = new QHBoxLayout();
    hero->setSpacing(28);
    hero->addWidget(m_art, 0, Qt::AlignTop);
    hero->addLayout(words, 1);

    // ── The instance it plays in ────────────────────────────────────────────
    m_instanceTitle = sectionTitle(content);
    m_instanceLine = bodyText(content);
    m_instanceOpen = new QPushButton(content);
    m_instanceOpen->setProperty("kubikRole", "quiet");
    m_instanceOpen->setCursor(Qt::PointingHandCursor);
    connect(m_instanceOpen, &QPushButton::clicked, this, [this] {
        if (!m_pack) {
            return;
        }
        auto* instance = Kubik::rememberedInstance(m_server.id, m_pack->profile.key);
        const auto candidates = Kubik::instancesFor(m_pack->profile.key);
        if (!instance && !candidates.isEmpty()) {
            instance = candidates.first();
        }
        if (instance) {
            emit instanceOpened(instance->id());
        }
    });
    m_instanceChange = new QPushButton(content);
    m_instanceChange->setProperty("kubikRole", "quiet");
    m_instanceChange->setCursor(Qt::PointingHandCursor);
    connect(m_instanceChange, &QPushButton::clicked, this, &KubikServerPage::chooseInstance);
    auto* instanceButtons = new QHBoxLayout();
    instanceButtons->setSpacing(6);
    instanceButtons->addWidget(m_instanceOpen);
    instanceButtons->addWidget(m_instanceChange);
    instanceButtons->addStretch();

    // ── The mods a player needs ─────────────────────────────────────────────
    m_modsTitle = sectionTitle(content);
    m_modsNote = bodyText(content, "kubikPageSubtitle");
    m_modsList = new QVBoxLayout();
    m_modsList->setSpacing(4);
    m_unmanaged = bodyText(content, "kubikPageSubtitle");

    auto* column = new QVBoxLayout(content);
    column->setContentsMargins(MARGIN, 16 + KubikWindowChrome::titleBarInset(), MARGIN, MARGIN);
    column->setSpacing(10);
    column->addWidget(m_back, 0, Qt::AlignLeft);
    column->addSpacing(8);
    column->addLayout(hero);
    column->addSpacing(10);
    column->addWidget(m_notice);
    column->addWidget(m_noticeAction, 0, Qt::AlignLeft);
    column->addWidget(m_joinCard);
    column->addSpacing(18);
    column->addWidget(m_instanceTitle);
    column->addWidget(m_instanceLine);
    column->addLayout(instanceButtons);
    column->addSpacing(18);
    column->addWidget(m_modsTitle);
    column->addWidget(m_modsNote);
    column->addLayout(m_modsList);
    column->addWidget(m_unmanaged);
    column->addStretch();

    auto* scroll = new QScrollArea(this);
    scroll->setWidget(content);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(scroll);

    connect(APPLICATION->accounts(), &AccountList::defaultAccountChanged, this, &KubikServerPage::updateAccountNote);
    connect(APPLICATION->accounts(), &AccountList::listChanged, this, &KubikServerPage::updateAccountNote);

    retranslate();
}

void KubikServerPage::setServer(const Kubik::Server& server)
{
    const bool sameServer = server.id == m_server.id;
    m_server = server;
    if (!sameServer) {
        if (m_play) {
            m_play->cancel();
        }
        m_pack.reset();
        m_packError.clear();
        m_copy->setText(QString());
        m_joinOpen = false;
    }

    // The same picture as the server's tile on Home, so the two read as one server.
    const qreal dpr = devicePixelRatioF();
    QPixmap art(QSize(ART_SIDE, ART_SIDE) * dpr);
    art.setDevicePixelRatio(dpr);
    art.fill(Qt::transparent);
    {
        QPainter painter(&art);
        painter.setRenderHint(QPainter::Antialiasing);
        QPainterPath shape;
        shape.addRoundedRect(QRectF(0, 0, ART_SIDE, ART_SIDE), 6, 6);
        painter.setClipPath(shape);
        KubikServerDelegate::paintArt(&painter, QRect(0, 0, ART_SIDE, ART_SIDE), server, nullptr, false);
    }
    m_art->setPixmap(art);
    m_name->setText(server.name);
    m_about->setText(KubikServerDelegate::describe(server));
    QString address = server.address.isEmpty() ? tr("No address yet") : server.address;
    if (server.edition == "bedrock" && server.port > 0) {
        address += ":" + QString::number(server.port);
    }
    m_address->setText(address);
    m_copy->setVisible(!server.address.isEmpty());
    m_status->setText(QString("<span style=\"color:%1\">●</span>&nbsp;%2")
                          .arg(KubikServerDelegate::statusColour(server).name(), KubikServerDelegate::statusText(server).toHtmlEscaped()));

    showNotice({});
    if (server.unplayable == "bedrock") {
        showNotice(
            tr("Bedrock servers are joined from Minecraft Bedrock Edition: on a phone, a console or Windows. Use the "
               "address above."));
    } else if (server.locked) {
        showNotice(KubikPlay::describe({ 403, "locked", {} }), tr("Pay on kubik.gg"), Kubik::webOrigin() + "/dashboard/billing");
    } else if (!server.unplayable.isEmpty()) {
        showNotice(KubikPlay::describe({ 422, "unplayable", server.unplayable }));
    }
    watchInstance();
    updatePlayState();
    updateInstanceLine();
    updateJoinCard();
    updateAccountNote();
    if (!sameServer || !m_pack) {
        refreshPack();
    }
}

void KubikServerPage::refreshPack()
{
    if (!m_server.profile || m_server.locked) {
        showPack();
        return;
    }
    const QString id = m_server.id;
    Kubik::apiGet("servers/" + id + "/pack", KubikAccount::get()->token(), this, [this, id](const Kubik::Reply& reply) {
        if (id != m_server.id) {
            return;  // the page moved on to another server meanwhile
        }
        if (reply.ok()) {
            m_pack = Kubik::parsePack(reply.body);
            m_packError = m_pack ? QString() : tr("kubik.gg sent something KubikLauncher does not understand.");
        } else {
            const auto error = Kubik::parseApiError(reply.status, reply.body);
            if (error.code == "unauthorized") {
                KubikAccount::get()->tokenRejected();
                return;
            }
            m_packError = KubikPlay::describe(error);
        }
        showPack();
        updateInstanceLine();
    });
}

void KubikServerPage::showPack()
{
    while (auto* item = m_modsList->takeAt(0)) {
        delete item->widget();
        delete item;
    }
    const bool modded = m_server.profile && !m_server.profile->loader.isEmpty();
    const bool modpack = m_server.profile && m_server.profile->isModpack();
    m_modsTitle->setVisible(modded || modpack);
    m_modsNote->setVisible(modded || modpack);
    m_unmanaged->setVisible(false);
    if (!modded && !modpack) {
        return;
    }
    if (!m_pack) {
        m_modsNote->setText(m_packError.isEmpty() ? tr("Checking which mods this server runs…") : m_packError);
        return;
    }
    if (modpack) {
        m_modsNote->setText(m_pack->mods.isEmpty() ? tr("Everything comes from the modpack, installed exactly as the server has it.")
                                                   : tr("The modpack, plus these mods the server added to it:"));
    } else {
        m_modsNote->setText(m_pack->mods.isEmpty() ? tr("This server needs no mods on your side.")
                                                   : tr("KubikLauncher installs these for you before joining:"));
    }
    for (const auto& mod : m_pack->mods) {
        auto* row = new QLabel(this);
        row->setObjectName("kubikPageBody");
        row->setTextFormat(Qt::PlainText);
        row->setText(mod.required ? tr("%1 · needed").arg(modLabel(mod)) : tr("%1 · optional").arg(modLabel(mod)));
        m_modsList->addWidget(row);
    }
    if (!m_pack->unmanaged.isEmpty()) {
        QStringList names;
        for (const auto& u : m_pack->unmanaged) {
            names.append(u.filename);
        }
        m_unmanaged->setText(tr("The server also runs %n mod(s) KubikLauncher cannot install for you, because they were added "
                                "by hand: %1. If you cannot join, ask the server's owner for them.",
                                nullptr, static_cast<int>(names.size()))
                                 .arg(names.join(", ")));
        m_unmanaged->setVisible(true);
    }
}

void KubikServerPage::updateInstanceLine()
{
    const bool playable = m_server.profile.has_value();
    m_instanceTitle->setVisible(playable);
    m_instanceLine->setVisible(playable);
    m_instanceOpen->setVisible(false);
    m_instanceChange->setVisible(false);
    if (!playable) {
        return;
    }
    const auto candidates = Kubik::instancesFor(m_server.profile->key);
    auto* instance = currentInstance();
    if (instance) {
        m_instanceLine->setText(tr("Plays in %1.").arg(instance->name()));
        m_instanceOpen->setVisible(true);
    } else if (candidates.isEmpty()) {
        m_instanceLine->setText(
            tr("Needs a %1 instance. Play installs it the first time, and every other %1 server shares it.").arg(m_server.profile->label));
    } else {
        m_instanceLine->setText(tr("%n instance(s) can play it. Play asks which one.", nullptr, static_cast<int>(candidates.size())));
    }
    m_instanceChange->setVisible(!candidates.isEmpty() && !m_play);
}

BaseInstance* KubikServerPage::currentInstance() const
{
    if (!m_server.profile) {
        return nullptr;
    }
    auto* instance = Kubik::rememberedInstance(m_server.id, m_server.profile->key);
    if (!instance) {
        const auto candidates = Kubik::instancesFor(m_server.profile->key);
        if (candidates.size() == 1) {
            instance = candidates.first();
        }
    }
    return instance;
}

bool KubikServerPage::inGame() const
{
    auto* instance = currentInstance();
    return instance && Kubik::playingServer(instance) == m_server.id;
}

void KubikServerPage::watchInstance()
{
    disconnect(m_runningWatch);
    if (auto* instance = currentInstance()) {
        m_runningWatch = connect(instance, &BaseInstance::runningStatusChanged, this, [this](bool running) {
            if (running && !APPLICATION->settings()->get("KubikJoinTipSeen").toBool() && inGame()) {
                m_joinOpen = true;
            }
            updatePlayState();
            updateJoinCard();
        });
    }
}

void KubikServerPage::updateJoinCard()
{
    const bool playable = m_server.profile.has_value() && !m_server.locked;
    m_howToJoin->setVisible(playable && !m_joinOpen);
    m_joinCard->setVisible(playable && m_joinOpen);
    if (!playable) {
        return;
    }
    const bool modded = !m_server.profile->loader.isEmpty() || m_server.profile->isModpack();
    const QString address = m_server.address;
    m_joinTitle->setText(inGame() ? tr("How you join %1").arg(m_server.name) : tr("How joining works"));
    const QStringList steps = {
        tr("1.  Minecraft opens and connects to %1 by itself.").arg(address),
        tr("2.  If you leave or get disconnected, open Multiplayer. %1 is at the top of the list.").arg(m_server.name),
        (modded ? tr("3.  Joining from another launcher? Use the address %1 with %2 and the mods listed below.")
                : tr("3.  Joining from another launcher? Use the address %1 with %2."))
            .arg(address, m_server.profile->label),
    };
    m_joinSteps->setText(steps.join("\n\n"));
}

void KubikServerPage::updateAccountNote()
{
    const auto account = KubikAccounts::current();
    const bool offline = account && account->accountType() == AccountType::Offline;
    m_accountNote->setVisible(offline && m_server.profile.has_value());
    if (offline) {
        m_accountNote->setText(tr("Playing as %1, an offline profile. It can only join servers whose owner turned on offline access.")
                                   .arg(account->profileName()));
    }
}

void KubikServerPage::askForAccount()
{
    showNotice(
        tr("Add your Minecraft account to play. Joining a server needs the Microsoft account you bought Minecraft with."),
        tr("Sign in with Microsoft"),
        [this] {
            if (KubikAccounts::addMicrosoft(window())) {
                showNotice({});
                play();
            }
        },
        KubikAccounts::microsoftIcon());
}

void KubikServerPage::updatePlayState()
{
    const bool running = !m_play.isNull();
    const bool playing = !running && inGame();
    const bool playable = m_server.profile.has_value() && !m_server.locked;
    m_playButton->setVisible(playable);
    m_playButton->setEnabled(!playing);
    m_playButton->setText(running ? tr("Cancel") : playing ? tr("In game") : tr("Play"));
    m_playButton->setProperty("kubikRole", running ? "quiet" : "primary");
    m_playButton->style()->unpolish(m_playButton);
    m_playButton->style()->polish(m_playButton);
    m_progressText->setVisible(running);
    m_progress->setVisible(running);
    m_instanceChange->setEnabled(!running);
}

void KubikServerPage::showNotice(const QString& message, const QString& actionLabel, const QString& actionUrl)
{
    std::function<void()> open;
    if (!actionUrl.isEmpty()) {
        open = [actionUrl] { QDesktopServices::openUrl(QUrl(actionUrl)); };
    }
    showNotice(message, actionLabel, open);
}

void KubikServerPage::showNotice(const QString& message, const QString& actionLabel, std::function<void()> action, const QIcon& icon)
{
    m_notice->setText(message);
    m_notice->setVisible(!message.isEmpty());
    m_noticeCallback = std::move(action);
    m_noticeAction->setText(actionLabel);
    m_noticeAction->setIcon(icon);
    m_noticeAction->setVisible(!actionLabel.isEmpty() && m_noticeCallback);
}

void KubikServerPage::play(bool newInstance)
{
    if (m_play || !m_server.profile || m_server.locked) {
        return;
    }
    showNotice({});
    m_play = new KubikPlay(m_server, this);
    m_play->setForceNewInstance(newInstance);
    connect(m_play, &KubikPlay::stageChanged, this, [this](KubikPlay::Stage stage, const QString& status) {
        m_progressText->setText(status);
        // Only downloading has a measure; the rest is a moving bar.
        if (stage != KubikPlay::Stage::Installing && stage != KubikPlay::Stage::Syncing) {
            m_progress->setRange(0, 0);
        }
    });
    connect(m_play, &KubikPlay::progressChanged, this, [this](qint64 done, qint64 total) {
        if (total > 0) {
            m_progress->setRange(0, 1000);
            m_progress->setValue(static_cast<int>(done * 1000 / total));
        } else {
            m_progress->setRange(0, 0);
        }
    });
    connect(m_play, &KubikPlay::choiceNeeded, this, [this](const QList<BaseInstance*>& candidates) {
        KubikInstancePicker picker(m_server.name, candidates, window());
        if (picker.exec() != QDialog::Accepted) {
            m_play->cancel();
            return;
        }
        m_play->choose(picker.chosen());
    });
    connect(m_play, &KubikPlay::failed, this, [this](const QString& message) {
        showNotice(message);
        onPlayFinished();
    });
    connect(m_play, &KubikPlay::accountNeeded, this, [this] {
        onPlayFinished();
        askForAccount();
    });
    connect(m_play, &KubikPlay::cancelled, this, &KubikServerPage::onPlayFinished);
    connect(m_play, &KubikPlay::launched, this, [this] {
        onPlayFinished();
        // The instance may be new (first Play), so watch the one this server now plays in.
        watchInstance();
        if (!APPLICATION->settings()->get("KubikJoinTipSeen").toBool()) {
            m_joinOpen = true;
        }
        updatePlayState();
        updateJoinCard();
    });
    m_progress->setRange(0, 0);
    m_progressText->clear();
    updatePlayState();
    m_play->start();
}

void KubikServerPage::onPlayFinished()
{
    if (m_play) {
        m_play->deleteLater();
        m_play.clear();
    }
    updatePlayState();
    updateInstanceLine();
}

void KubikServerPage::chooseInstance()
{
    if (!m_server.profile) {
        return;
    }
    const auto candidates = Kubik::instancesFor(m_server.profile->key);
    KubikInstancePicker picker(m_server.name, candidates, window());
    if (picker.exec() != QDialog::Accepted) {
        return;
    }
    if (auto* chosen = picker.chosen()) {
        Kubik::rememberInstance(m_server.id, chosen);
        updateInstanceLine();
    } else {
        play(true);
    }
}

void KubikServerPage::retranslate()
{
    m_back->setText(tr("← Your servers"));
    m_open->setText(tr("Open on kubik.gg"));
    m_copy->setToolTip(tr("Copy the address"));
    m_instanceTitle->setText(tr("Instance"));
    m_instanceOpen->setText(tr("Open instance"));
    m_instanceChange->setText(tr("Change"));
    m_modsTitle->setText(tr("Mods"));
    m_howToJoin->setText(tr("How to join"));
    m_joinDismiss->setText(tr("Got it"));
    if (!m_server.id.isEmpty()) {
        setServer(m_server);
    }
}

void KubikServerPage::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange) {
        retranslate();
    }
    QWidget::changeEvent(event);
}
