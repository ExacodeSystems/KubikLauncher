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

#include <QJsonValue>
#include <QString>

/**
 * What Minecraft itself says, for the server page's "How to join" pictures and
 * captions: they name the buttons the player will press, so they use the game's
 * words in the game's language, which is not always the launcher's.
 */
namespace Kubik {

/** The labels on Minecraft's title screen, server list and Edit Server Info, as Minecraft 26.2 words them. */
struct GameLabels {
    QString languageCode;
    QString singleplayer;
    QString multiplayer;
    QString realms;
    QString options;
    QString quit;
    QString multiplayerTitle;
    QString joinServer;
    QString directConnection;
    QString addServer;
    QString edit;
    QString remove;
    QString refresh;
    QString back;
    QString defaultServerName;
    QString editServerInfo;
    QString serverName;
    QString serverAddress;
    QString serverResourcePacks;
    QString prompt;
    QString done;
    QString cancel;
    QString connecting;
};

/** Minecraft's labels for `languageCode`: Indonesian for id_id, English for anything else. */
GameLabels gameLabels(const QString& languageCode);

/** The language options.txt sets (`lang:`), lower-cased; en_us, Minecraft's own default, when it sets none. */
QString gameLanguage(const QString& optionsTxt);

/**
 * A server's message of the day as plain text, from a status ping's `description`: a string, or a text
 * component whose `text` and `extra` parts are joined. Formatting codes (§ and a character) are removed.
 */
QString plainMotd(const QJsonValue& description);

}  // namespace Kubik
