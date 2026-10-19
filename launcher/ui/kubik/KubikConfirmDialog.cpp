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

#include "KubikConfirmDialog.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

KubikConfirmDialog::Answer KubikConfirmDialog::ask(QWidget* parent,
                                                   const QString& title,
                                                   const QString& body,
                                                   const QString& primary,
                                                   const QString& secondary,
                                                   bool danger)
{
    KubikConfirmDialog dialog(title, body, primary, secondary, danger, parent);
    dialog.exec();
    return dialog.m_answer;
}

KubikConfirmDialog::KubikConfirmDialog(const QString& title,
                                       const QString& body,
                                       const QString& primary,
                                       const QString& secondary,
                                       bool danger,
                                       QWidget* parent)
    : QDialog(parent)
{
    setObjectName("kubikConfirm");
    setWindowTitle(title);
    setMinimumWidth(420);
    setMaximumWidth(520);

    auto* heading = new QLabel(title, this);
    heading->setObjectName("kubikDialogTitle");
    heading->setWordWrap(true);
    auto* text = new QLabel(body, this);
    text->setObjectName("kubikPageBody");
    text->setWordWrap(true);

    auto* no = new QPushButton(secondary, this);
    no->setProperty("kubikRole", "quiet");
    no->setCursor(Qt::PointingHandCursor);
    auto* yes = new QPushButton(primary, this);
    yes->setProperty("kubikRole", danger ? "danger" : "primary");
    yes->setCursor(Qt::PointingHandCursor);
    yes->setDefault(true);

    connect(yes, &QPushButton::clicked, this, [this] {
        m_answer = Answer::Primary;
        accept();
    });
    connect(no, &QPushButton::clicked, this, [this] {
        m_answer = Answer::Secondary;
        accept();
    });

    auto* buttons = new QHBoxLayout();
    buttons->setSpacing(10);
    buttons->addStretch();
    buttons->addWidget(no);
    buttons->addWidget(yes);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(26, 24, 26, 22);
    layout->setSpacing(10);
    layout->addWidget(heading);
    layout->addWidget(text);
    layout->addSpacing(12);
    layout->addLayout(buttons);
}
