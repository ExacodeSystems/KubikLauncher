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

class QWidget;

/**
 * The main window's frame: native on every OS, but dark and quiet.
 *
 *   - macOS: the content runs under a transparent, title-less title bar, so the
 *     traffic lights sit on the rail like any modern Mac app. The strip they sit
 *     in is reserved (titleBarInset) and drags the window.
 *   - Windows 11: the native caption stays (snapping, resizing, the system
 *     buttons) but turns dark and takes the rail's colour.
 *   - Elsewhere: nothing; the window manager owns the frame.
 */
namespace KubikWindowChrome {

/** Applies the chrome to a top-level window. Call once, before it is shown. */
void apply(QWidget* window);

/** Height reserved at the top of the window for the title bar's controls (0 off macOS). */
int titleBarInset();

/**
 * Qt 6.9+ adds the title bar's height to the contents margins of every widget that overlaps it
 * (Qt::WA_ContentsMarginsRespectsSafeArea), which would push the whole page back under a blank
 * band. The shell reserves the strip itself (titleBarInset), so `root` and everything in it opt
 * out. Call once the window's widgets exist.
 */
void ignoreSafeArea(QWidget* root);

}  // namespace KubikWindowChrome
