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

#pragma once

#include <QPixmap>
#include <QWidget>

#include <optional>

#include "kubik/KubikGameText.h"
#include "kubik/KubikModels.h"
#include "ui/kubik/KubikServerTiles.h"

class QPainter;

/**
 * Pictures of Minecraft's own screens for the server page's "How to join": drawn, not captured, so each
 * one shows this server (its name, icon, message of the day and address) and the game's labels in the
 * game's language. They are built from the Kubik resource pack's art (buttons, fields, menu backgrounds,
 * the panorama), which is what the game shows with the pack on. Minecraft's logo and font are Mojang's
 * and are not drawn: the text uses the launcher's display face.
 */
namespace KubikJoinShots {

enum class Shot {
    Connecting,   // "Connecting to the server...", after Play
    TitleScreen,  // Multiplayer, ringed
    ServerList,   // this server at the top, and Join Server, ringed
    AddServer,    // Edit Server Info with this server's address typed in, for another launcher
};

/** What the pictures show of the server. */
struct Subject {
    Kubik::Server server;
    std::optional<KubikServerLive> live;
    Kubik::GameLabels labels;
};

/** The pictures' shape: Minecraft's 16:9 window. */
constexpr int ASPECT_W = 16;
constexpr int ASPECT_H = 9;

void paint(QPainter* painter, const QRectF& target, Shot shot, const Subject& subject);

}  // namespace KubikJoinShots

/** One picture, at the width its layout gives it and 16:9 tall, with rounded corners. */
class KubikJoinShotView : public QWidget {
    Q_OBJECT

   public:
    KubikJoinShotView(KubikJoinShots::Shot shot, QWidget* parent = nullptr);

    void setSubject(const KubikJoinShots::Subject& subject);
    void setShot(KubikJoinShots::Shot shot);

    bool hasHeightForWidth() const override { return true; }
    int heightForWidth(int width) const override { return width * KubikJoinShots::ASPECT_H / KubikJoinShots::ASPECT_W; }
    QSize sizeHint() const override { return { 480, heightForWidth(480) }; }
    QSize minimumSizeHint() const override { return { 240, heightForWidth(240) }; }

   protected:
    void paintEvent(QPaintEvent* event) override;

   private:
    KubikJoinShots::Shot m_shot;
    std::optional<KubikJoinShots::Subject> m_subject;
    // Drawn once per size: painting a scene is a few dozen scaled images.
    QPixmap m_cache;
};
