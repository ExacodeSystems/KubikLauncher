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

#include <QDialog>
#include <QList>

class BaseInstance;
class QListWidget;

/**
 * "Which instance should play this server?": the instances made for the
 * server's profile, the one played last first, and a way to make another.
 *
 * Accepted with an instance chosen, or with none (`chosen()` is nullptr) when the
 * player asked for a new one.
 */
class KubikInstancePicker : public QDialog {
    Q_OBJECT

   public:
    KubikInstancePicker(const QString& serverName, const QList<BaseInstance*>& candidates, QWidget* parent);

    BaseInstance* chosen() const { return m_chosen; }

   private:
    QList<BaseInstance*> m_candidates;
    QListWidget* m_list;
    BaseInstance* m_chosen = nullptr;
};
