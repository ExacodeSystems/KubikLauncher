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

#include <QPointer>
#include <QWidget>

#include <functional>

class MinecraftInstance;
class QLabel;
class QPushButton;
class QVBoxLayout;

/**
 * An instance page's Overview tab: which kubik.gg servers play in it, its game
 * folders (worlds, resource packs, shaders, screenshots) one click away, and the
 * modpack it came from when it is one.
 */
class KubikInstanceOverview : public QWidget {
    Q_OBJECT

   public:
    explicit KubikInstanceOverview(QWidget* parent = nullptr);

    void setInstance(MinecraftInstance* instance);
    /** A server's name by id, from Home's last fetch; empty when it is not on the list. */
    void setServerNames(std::function<QString(const QString&)> lookup) { m_serverName = std::move(lookup); }

   signals:
    void serverOpened(const QString& serverId);

   protected:
    void changeEvent(QEvent* event) override;
    void showEvent(QShowEvent* event) override;

   private:
    void rebuild();

    QPointer<MinecraftInstance> m_instance;
    std::function<QString(const QString&)> m_serverName;

    QLabel* m_usedTitle;
    QVBoxLayout* m_usedList;
    QLabel* m_foldersTitle;
    QList<QPushButton*> m_folders;
    QLabel* m_packTitle;
    QLabel* m_pack;
};
