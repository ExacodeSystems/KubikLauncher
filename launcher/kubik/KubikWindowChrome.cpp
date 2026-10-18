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

// The non-Apple half of KubikWindowChrome; macOS lives in KubikWindowChrome.mm.

#include "KubikWindowChrome.h"

#include <QWidget>

#ifdef Q_OS_WIN
#include <windows.h>

#include <dwmapi.h>

#include "kubik/KubikBrand.h"
#endif

namespace KubikWindowChrome {

void apply([[maybe_unused]] QWidget* window)
{
#ifdef Q_OS_WIN
    // Windows 11 lets an app colour its native caption. Older Windows ignores both
    // attributes and keeps its default frame, which is the right fallback.
    const auto hwnd = reinterpret_cast<HWND>(window->winId());
    const BOOL dark = TRUE;
    DwmSetWindowAttribute(hwnd, 20 /* DWMWA_USE_IMMERSIVE_DARK_MODE */, &dark, sizeof(dark));
    const QColor rail = KubikBrand::color("void.raised");
    const COLORREF caption = RGB(rail.red(), rail.green(), rail.blue());
    DwmSetWindowAttribute(hwnd, 35 /* DWMWA_CAPTION_COLOR */, &caption, sizeof(caption));
#endif
}

int titleBarInset()
{
    return 0;
}

void ignoreSafeArea([[maybe_unused]] QWidget* root) {}

}  // namespace KubikWindowChrome
