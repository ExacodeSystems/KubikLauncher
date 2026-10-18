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

class QGridLayout;
class QLabel;
class QPushButton;

/**
 * One instance, the way a game page in a launcher shows a game: a big Play
 * button, what the instance is, and a list of what can be changed, each in
 * plain words with one line saying what it is for.
 *
 * The "Manage" rows open Prism's instance window on the matching page, which
 * keeps Prism's one-window-per-instance lifecycle (saving, file watching,
 * modpack updates) exactly as it is.
 */
class KubikInstancePage : public QWidget {
    Q_OBJECT

   public:
    /** The instance actions this page offers; MainWindow carries them out. */
    enum class Action { OpenFolder, ExportZip, ExportModrinth, Rename, ChangeIcon, ChangeGroup, Copy, CreateShortcut, Delete };
    Q_ENUM(Action)

    explicit KubikInstancePage(QWidget* parent = nullptr);

    void setInstance(const QString& instanceId);
    QString instanceId() const { return m_instanceId; }

   signals:
    void backRequested();
    void playRequested(const QString& instanceId);
    void stopRequested(const QString& instanceId);
    /** Open the instance window on one of its pages (an InstancePageProvider id). */
    void manageRequested(const QString& instanceId, const QString& pageId);
    void actionRequested(const QString& instanceId, KubikInstancePage::Action action);

   protected:
    void changeEvent(QEvent* event) override;

   private:
    void retranslate();
    void refresh();
    void rebuildManageRows();

    QString m_instanceId;
    QList<QMetaObject::Connection> m_instanceConnections;

    QPushButton* m_back;
    QLabel* m_art;
    QLabel* m_name;
    QLabel* m_about;
    QLabel* m_stats;
    QPushButton* m_play;
    QPushButton* m_folder;
    QPushButton* m_export;
    QPushButton* m_more;
    QLabel* m_manageTitle;
    QGridLayout* m_manageGrid;
};
