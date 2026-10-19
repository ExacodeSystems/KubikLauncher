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
#include <QPointer>
#include <QWidget>

class MinecraftInstance;
class QCheckBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QSlider;

/**
 * An instance page's Advanced tab: what the instance is (Minecraft, loader,
 * play time, folder), the two things a player may want different for it
 * (memory and Java), and the instance itself (rename, icon, copy, delete).
 *
 * Memory and Java are Prism's own per-instance overrides (OverrideMemory,
 * OverrideJavaLocation), so the instance window and this tab agree. Anything
 * rarer is one "More settings" link away, in Prism's instance settings.
 */
class KubikInstanceAdvanced : public QWidget {
    Q_OBJECT

   public:
    enum class Request { Rename, ChangeIcon, Copy, Delete, MoreSettings };
    Q_ENUM(Request)

    explicit KubikInstanceAdvanced(QWidget* parent = nullptr);

    void setInstance(MinecraftInstance* instance);

   signals:
    void requested(KubikInstanceAdvanced::Request request);

   protected:
    void changeEvent(QEvent* event) override;
    void showEvent(QShowEvent* event) override;

   private:
    void retranslate();
    void refresh();
    void updateMemory();

    QPointer<MinecraftInstance> m_instance;
    bool m_loading = false;  // set while the controls are filled in, so filling them writes nothing

    QLabel* m_aboutTitle;
    QList<QLabel*> m_keys;
    QLabel* m_minecraft;
    QLabel* m_loader;
    QLabel* m_played;
    QLineEdit* m_folderPath;
    QPushButton* m_openFolder;

    QLabel* m_memoryTitle;
    QCheckBox* m_ownMemory;
    QSlider* m_memory;
    QLabel* m_memoryValue;
    QLabel* m_memoryNote;

    QLabel* m_javaTitle;
    QCheckBox* m_ownJava;
    QWidget* m_javaRow;
    QLineEdit* m_javaPath;
    QPushButton* m_javaBrowse;
    QLabel* m_javaNote;

    QLabel* m_instanceTitle;
    QPushButton* m_rename;
    QPushButton* m_icon;
    QPushButton* m_copy;
    QPushButton* m_delete;
    QPushButton* m_moreSettings;
};
