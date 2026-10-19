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

/**
 * A question with two answers, in the launcher's own look: a title, a sentence
 * or two, a quiet button and a filled one. It replaces QMessageBox, whose
 * Yes/No put the accent on whichever button Qt made default.
 *
 * The filled button is the answer we recommend, which is not always the one
 * that goes ahead: on "Skip signing in?" it is "Sign in now".
 */
class KubikConfirmDialog : public QDialog {
    Q_OBJECT

   public:
    enum class Answer { Primary, Secondary, Dismissed };

    /** `danger` paints the filled button in heart, for Remove and Sign out. */
    static Answer ask(QWidget* parent,
                      const QString& title,
                      const QString& body,
                      const QString& primary,
                      const QString& secondary,
                      bool danger = false);

   private:
    KubikConfirmDialog(const QString& title,
                       const QString& body,
                       const QString& primary,
                       const QString& secondary,
                       bool danger,
                       QWidget* parent);

    Answer m_answer = Answer::Dismissed;
};
