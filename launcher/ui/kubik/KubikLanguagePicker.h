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

#include <QAbstractButton>
#include <QWidget>

class QButtonGroup;
class Setting;

/**
 * One language, as a large card: its flag, its own name, and a line written in
 * that language (never translated), so a player who reads only one of the two
 * finds their card without reading the other.
 */
class KubikLanguageCard : public QAbstractButton {
    Q_OBJECT

   public:
    KubikLanguageCard(const QString& key, const QString& name, const QString& line, bool compact, QWidget* parent = nullptr);

    QString key() const { return m_key; }
    QSize sizeHint() const override;

   protected:
    void paintEvent(QPaintEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void focusInEvent(QFocusEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;

   private:
    QString m_key;
    QString m_name;
    QString m_line;
    bool m_compact;
    bool m_hovered = false;
    bool m_keyboardFocus = false;  // the ring shows for Tab, not for the focus a window gives its first card
};

/**
 * English and Indonesian, side by side. Choosing a card switches the whole
 * launcher at once and saves the choice: the setup wizard's first page and the
 * Language section of Settings are both this widget.
 */
class KubikLanguagePicker : public QWidget {
    Q_OBJECT

   public:
    /** `compact` is the Settings size: the flag beside the name instead of above it. */
    explicit KubikLanguagePicker(bool compact, QWidget* parent = nullptr);

    QString selectedLanguage() const;

   private:
    void choose(const QString& key);
    void showSelected();

    QButtonGroup* m_group;
};
