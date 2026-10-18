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

#include "KubikBrand.h"

#include <QDebug>
#include <QFile>
#include <QFontDatabase>
#include <QHash>
#include <QJsonDocument>
#include <QJsonObject>

namespace {

// The family names the bundled files declare. Used as the fallback answer if a
// file failed to register, so a stylesheet still names a sensible face.
const QString DISPLAY_FAMILY = QStringLiteral("Chakra Petch");
const QString BODY_FAMILY = QStringLiteral("Hanken Grotesk");
const QString MONO_FAMILY = QStringLiteral("JetBrains Mono");

const QHash<QString, QColor>& palette()
{
    static const QHash<QString, QColor> colors = [] {
        QHash<QString, QColor> out;
        QFile file(QStringLiteral(":/kubik/tokens.json"));
        if (!file.open(QIODevice::ReadOnly)) {
            qWarning() << "KubikBrand: cannot read the palette:" << file.errorString();
            return out;
        }
        const auto root = QJsonDocument::fromJson(file.readAll()).object().value("colors").toObject();
        // { "void": { "DEFAULT": "#090B0C", "panel": "#12181A" }, ... } becomes
        // "void" and "void.panel", the same names Tailwind's classes use.
        for (auto group = root.begin(); group != root.end(); ++group) {
            const auto shades = group.value().toObject();
            for (auto shade = shades.begin(); shade != shades.end(); ++shade) {
                const QString key = shade.key() == "DEFAULT" ? group.key() : group.key() + "." + shade.key();
                out.insert(key, QColor(shade.value().toString()));
            }
        }
        return out;
    }();
    return colors;
}

}  // namespace

namespace KubikBrand {

void registerFonts()
{
    static bool done = false;
    if (done) {
        return;
    }
    done = true;
    const QStringList files = {
        ":/kubik/fonts/ChakraPetch-Medium.ttf",  ":/kubik/fonts/ChakraPetch-SemiBold.ttf", ":/kubik/fonts/ChakraPetch-Bold.ttf",
        ":/kubik/fonts/HankenGrotesk[wght].ttf", ":/kubik/fonts/JetBrainsMono[wght].ttf",
    };
    for (const auto& file : files) {
        if (QFontDatabase::addApplicationFont(file) == -1) {
            qWarning() << "KubikBrand: failed to register font" << file;
        }
    }
}

QColor color(const QString& key)
{
    const auto it = palette().constFind(key);
    if (it == palette().constEnd()) {
        qWarning() << "KubikBrand: unknown colour" << key;
        return QColor(Qt::magenta);
    }
    return it.value();
}

QString displayFamily()
{
    return DISPLAY_FAMILY;
}

QString bodyFamily()
{
    return BODY_FAMILY;
}

QString monoFamily()
{
    return MONO_FAMILY;
}

}  // namespace KubikBrand
