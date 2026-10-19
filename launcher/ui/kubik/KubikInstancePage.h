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

#include <QList>
#include <QMetaObject>
#include <QWidget>

#include <functional>

class KubikInstanceAdvanced;
class KubikInstanceLogs;
class KubikInstanceMods;
class KubikInstanceOverview;
class QButtonGroup;
class QLabel;
class QPushButton;
class QStackedWidget;

/**
 * One instance, the way a game page in a launcher shows a game: a big Play
 * button, what the instance is, and under it four tabs.
 *
 *   Overview  the servers that play in it, its game folders, its modpack
 *   Mods      every mod, which ones a server put there, the player's own to manage
 *   Logs      the live game log, the last game's log, crash reports
 *   Advanced  what it is, its own memory and Java, rename, copy, delete
 *
 * Everything stays in KubikLauncher's own look. Prism's instance window is one
 * "More settings" link away on Advanced, for what these tabs leave out.
 */
class KubikInstancePage : public QWidget {
    Q_OBJECT

   public:
    /** The instance actions this page offers; MainWindow carries them out. */
    enum class Action { OpenFolder, ExportZip, ExportModrinth, Rename, ChangeIcon, ChangeGroup, Copy, CreateShortcut, Delete };
    Q_ENUM(Action)
    enum Tab { Overview = 0, Mods = 1, Logs = 2, Advanced = 3 };

    explicit KubikInstancePage(QWidget* parent = nullptr);

    void setInstance(const QString& instanceId);
    QString instanceId() const { return m_instanceId; }
    void showTab(int tab);
    /** A kubik.gg server's name by id (Home's last fetch), for "Used by" and the Mods tab. */
    void setServerNames(std::function<QString(const QString&)> lookup);

   signals:
    void backRequested();
    void playRequested(const QString& instanceId);
    void stopRequested(const QString& instanceId);
    /** Open Prism's instance window on one of its pages (an InstancePageProvider id). */
    void manageRequested(const QString& instanceId, const QString& pageId);
    void actionRequested(const QString& instanceId, KubikInstancePage::Action action);
    void serverOpened(const QString& serverId);

   protected:
    void changeEvent(QEvent* event) override;

   private:
    void retranslate();
    void refresh();

    QString m_instanceId;
    QList<QMetaObject::Connection> m_instanceConnections;

    QPushButton* m_back;
    QLabel* m_art;
    QLabel* m_name;
    QLabel* m_about;
    QLabel* m_stats;
    QPushButton* m_play;
    QPushButton* m_folder;
    QPushButton* m_more;

    QButtonGroup* m_tabs;
    QList<QPushButton*> m_tabButtons;
    QStackedWidget* m_tabPages;
    KubikInstanceOverview* m_overview;
    KubikInstanceMods* m_mods;
    KubikInstanceLogs* m_logs;
    KubikInstanceAdvanced* m_advanced;
};
