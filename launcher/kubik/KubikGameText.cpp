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

#include "KubikGameText.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QRegularExpression>
#include <QStringList>

namespace Kubik {

namespace {

// Copied from Minecraft 26.2's en_us.json and id_id.json, key by key (menu.multiplayer, selectServer.select,
// addServer.enterIp, …), so a caption that says "press Multiplayer" names the button the player sees.
GameLabels english()
{
    return { "en_us",
             "Singleplayer",
             "Multiplayer",
             "Minecraft Realms",
             "Options...",
             "Quit Game",
             "Play Multiplayer",
             "Join Server",
             "Direct Connection",
             "Add Server",
             "Edit",
             "Delete",
             "Refresh",
             "Back",
             "Minecraft Server",
             "Edit Server Info",
             "Server Name",
             "Server Address",
             "Server Resource Packs",
             "Prompt",
             "Done",
             "Cancel",
             "Connecting to the server..." };
}

GameLabels indonesian()
{
    return { "id_id",
             "Bermain Sendiri",
             "Bermain Bersama",
             "Minecraft Realms",
             "Opsi...",
             "Keluar Permainan",
             "Bermain Bersama",
             "Gabung",
             "Langsung Sambung",
             "Tambah Server",
             "Sunting",
             "Hapus",
             "Segarkan",
             "Kembali",
             "Server Minecraft",
             "Ubah Info Server",
             "Nama Server",
             "Alamat Server",
             "Paket Sumber Server",
             "Wantian",
             "Selesai",
             "Batal",
             "Menyambungkan ke server..." };
}

void collectText(const QJsonValue& component, QString& out)
{
    if (component.isString()) {
        out += component.toString();
    } else if (component.isArray()) {
        for (const auto& part : component.toArray()) {
            collectText(part, out);
        }
    } else if (component.isObject()) {
        const auto object = component.toObject();
        out += object.value("text").toString();
        collectText(object.value("extra"), out);
    }
}

}  // namespace

GameLabels gameLabels(const QString& languageCode)
{
    return languageCode.toLower() == "id_id" ? indonesian() : english();
}

QString gameLanguage(const QString& optionsTxt)
{
    for (const auto& line : optionsTxt.split('\n')) {
        if (line.startsWith("lang:")) {
            const QString code = line.mid(5).trimmed().toLower();
            return code.isEmpty() ? QStringLiteral("en_us") : code;
        }
    }
    return QStringLiteral("en_us");
}

QString plainMotd(const QJsonValue& description)
{
    QString text;
    collectText(description, text);
    static const QRegularExpression formatting(QStringLiteral("§."));
    text.remove(formatting);
    return text.trimmed();
}

}  // namespace Kubik
