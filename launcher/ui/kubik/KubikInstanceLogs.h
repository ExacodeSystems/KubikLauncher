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

#include "QObjectPtr.h"

class LaunchTask;
class LogModel;
class LogView;
class MinecraftInstance;
class QComboBox;
class QLabel;
class QPlainTextEdit;
class QPushButton;
class QStackedWidget;

/**
 * An instance page's Logs tab: what the game printed, for fixing a problem or
 * asking for help with one.
 *
 *   This session   the live log while Minecraft runs, and after it exits; it is
 *                  the instance's launch task's own model, which lives until the
 *                  next launch (LogPage reads it the same way)
 *   Last game      logs/latest.log, the file Minecraft itself writes
 *   Crash reports  the .txt files in crash-reports, newest first
 *
 * Copy and Open logs folder; nothing is uploaded anywhere.
 */
class KubikInstanceLogs : public QWidget {
    Q_OBJECT

   public:
    explicit KubikInstanceLogs(QWidget* parent = nullptr);

    void setInstance(MinecraftInstance* instance);

   protected:
    void changeEvent(QEvent* event) override;
    void showEvent(QShowEvent* event) override;

   private:
    void followLaunchTask(LaunchTask* task);
    void refreshSources();
    void showSource();
    void retranslate();

    QPointer<MinecraftInstance> m_instance;
    QMetaObject::Connection m_launchWatch;
    shared_qobject_ptr<LogModel> m_live;

    QComboBox* m_source;
    QPushButton* m_copy;
    QPushButton* m_folder;
    QStackedWidget* m_views;
    LogView* m_liveView;
    QPlainTextEdit* m_fileView;
    QLabel* m_empty;
};
