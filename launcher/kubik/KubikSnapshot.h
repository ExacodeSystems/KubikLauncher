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

#include <QString>

/**
 * Debug builds only: walk the first-run screens and save each as a PNG.
 *
 * Set KUBIKLAUNCHER_SNAPSHOT_DIR and start a Debug build. Every few seconds it
 * saves whatever is in front (the setup wizard, then the main window's pages),
 * dismisses it, and quits once the last page is saved. The app renders its
 * own widgets (QWidget::grab), so no screen-recording permission is involved:
 * this is how a design change is reviewed from a terminal or in CI.
 */
namespace KubikSnapshot {

/** Starts the walk if KUBIKLAUNCHER_SNAPSHOT_DIR is set; otherwise does nothing. */
void startIfRequested();

}  // namespace KubikSnapshot
