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

#include <functional>
#include <memory>

class QButtonGroup;
class QCheckBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QSlider;
class QSpinBox;
class QStackedWidget;
class QVBoxLayout;
class QWidget;

namespace JavaCommon {
class TestCheck;
}

/**
 * Settings: four pages a player can read in a minute, instead of Prism's nine.
 *
 *   General   language, updates, the launcher's folder
 *   Game      the game window, and hiding the launcher while playing
 *   Java      automatic Java (or a path), memory, Java arguments
 *   Profiles  Minecraft profiles and the kubik.gg account (KubikProfilesPage)
 *
 * Every control saves as it changes, so there is one button, Done, and no
 * Cancel that would have to undo a language the whole window already switched to.
 * Prism's own pages stay in the tree for upstream merges; Application no longer
 * registers them.
 *
 * Page ids match the ones Prism's callers already pass: "minecraft-settings",
 * "java-settings" and "accounts". Anything else opens General.
 */
class KubikSettingsDialog : public QDialog {
    Q_OBJECT

   public:
    explicit KubikSettingsDialog(const QString& page, QWidget* parent = nullptr);
    ~KubikSettingsDialog() override;

    /** "4 GB", "1.5 GB": memory the way a player reads it. */
    static QString gigabytes(int mib);
    /** The sentence under a memory slider: what it is, what this computer can spare, or why to lower it. */
    static QString memoryNote(int maxMib);

   protected:
    void changeEvent(QEvent* event) override;
    void done(int result) override;

   private:
    QWidget* buildGeneral();
    QWidget* buildGame();
    QWidget* buildJava();
    /** A page's scrolling column, with its title. */
    QWidget* pageFrame(QWidget* body);
    /** Sets a text now and again whenever the language changes. */
    void text(std::function<void()> apply);
    void retranslate();

    void updateJavaMode();
    void updateMemory();

    QButtonGroup* m_nav;
    QStackedWidget* m_pages;
    QList<std::function<void()>> m_texts;

    QSpinBox* m_width = nullptr;
    QSpinBox* m_height = nullptr;
    QCheckBox* m_maximised = nullptr;

    QCheckBox* m_autoJava = nullptr;
    QWidget* m_manualJava = nullptr;
    QLineEdit* m_javaPath = nullptr;
    QSlider* m_memory = nullptr;
    QLabel* m_memoryValue = nullptr;
    QLabel* m_memoryNote = nullptr;
    QPushButton* m_argsToggle = nullptr;
    QPlainTextEdit* m_args = nullptr;
    QString m_argsAtOpen;
    std::unique_ptr<JavaCommon::TestCheck> m_javaTest;
};
