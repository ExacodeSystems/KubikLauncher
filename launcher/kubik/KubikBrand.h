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

#include <QColor>
#include <QString>

/**
 * The kubik.gg brand, as the launcher draws it.
 *
 * Colours come from `:/kubik/tokens.json`, which is GENERATED in the kubik.gg repo
 * (`pnpm --filter mobile art:launcher`) from the dashboard's own Tailwind palette,
 * so the launcher and the website cannot drift apart by hand. Fonts are the
 * website's three faces, bundled under the OFL.
 */
namespace KubikBrand {

/** Registers the bundled fonts with Qt. Safe to call more than once. */
void registerFonts();

/**
 * A palette colour by its Tailwind name: "void", "void.panel", "grass.deep".
 * An unknown key is a programming error and answers magenta, which is
 * impossible to miss on screen.
 */
QColor color(const QString& key);

/** Chakra Petch: headings, buttons, tabs. */
QString displayFamily();
/** Hanken Grotesk: everything else. */
QString bodyFamily();
/** JetBrains Mono: addresses, versions, logs. */
QString monoFamily();

}  // namespace KubikBrand
