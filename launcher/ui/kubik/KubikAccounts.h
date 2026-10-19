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

#include <QIcon>
#include <QPixmap>
#include <QString>

#include "minecraft/auth/MinecraftAccount.h"

class QWidget;

/**
 * The Minecraft profiles a player plays with, as every Kubik screen handles them:
 * the setup wizard, the profile card, Settings and the server page all add and
 * draw accounts through here, so the rules below exist once.
 *
 * Offline profiles follow Prism's own rule: they can only be added once a
 * Microsoft account that owns Minecraft is in the list. Somebody without the
 * game still cannot play, which is what keeps the launcher on the right side of
 * Mojang's review of its app id.
 */
namespace KubikAccounts {

/** Microsoft's sign-in. The new account becomes the one that plays. Null if cancelled. */
MinecraftAccountPtr addMicrosoft(QWidget* parent);

/** True once a Microsoft account that owns Minecraft is in the list. */
bool canAddOffline();

/** Asks for a name and adds an offline profile, which becomes the one that plays. Null if cancelled or not allowed. */
MinecraftAccountPtr addOffline(QWidget* parent);

/** The account that plays, if it can: null when there is none. */
MinecraftAccountPtr current();

/** True when the account needs its owner to sign in again before it can play. */
bool needsSignIn(const MinecraftAccountPtr& account);

/** The skin's face, or a letter on a tinted square for offline profiles and accounts with no skin yet. */
QPixmap avatar(const MinecraftAccountPtr& account, int size);

/** "Microsoft account" or "Offline profile". */
QString typeLabel(const MinecraftAccountPtr& account);

/** Microsoft's four squares, for "Sign in with Microsoft". */
QIcon microsoftIcon();

}  // namespace KubikAccounts
