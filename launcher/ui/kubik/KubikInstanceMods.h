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
#include <QSet>
#include <QStyledItemDelegate>
#include <QWidget>

#include <functional>

class MinecraftInstance;
class ModFolderModel;
class QLabel;
class QListView;
class QPushButton;
class QSortFilterProxyModel;

/**
 * One mod as a row: its icon, name and version, and on the right either the
 * "Server mod" badge or the player's own controls (an on/off switch and Remove).
 */
class KubikModDelegate : public QStyledItemDelegate {
    Q_OBJECT

   public:
    explicit KubikModDelegate(ModFolderModel* model, QObject* parent = nullptr);

    /** The files sync placed (enabled-form names): a server's mods, which are not the player's to switch. */
    void setManaged(QSet<QString> filenames, const QString& serverName);
    void setModpack(bool modpack) { m_modpack = modpack; }

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    bool helpEvent(QHelpEvent* event, QAbstractItemView* view, const QStyleOptionViewItem& option, const QModelIndex& index) override;

    bool isManaged(int sourceRow) const;
    static QRect switchRect(const QRect& row);
    static QRect removeRect(const QRect& row);

   signals:
    void toggleRequested(int sourceRow);
    void removeRequested(int sourceRow);

   protected:
    bool editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index) override;

   private:
    int sourceRow(const QModelIndex& index) const;

    ModFolderModel* m_model;
    QSet<QString> m_managed;
    QString m_serverName;
    bool m_modpack = false;
};

/**
 * An instance page's Mods tab: every mod in the instance, which ones a server put
 * there, and the player's own mods to switch off, remove, or add from a file.
 *
 * The rule it states and keeps: server mods switch with the server you join, so
 * they have no controls here (the next Play would put them back); the mods you
 * add are yours and stay.
 */
class KubikInstanceMods : public QWidget {
    Q_OBJECT

   public:
    explicit KubikInstanceMods(QWidget* parent = nullptr);

    void setInstance(MinecraftInstance* instance);
    void setServerNames(std::function<QString(const QString&)> lookup) { m_serverName = std::move(lookup); }

   protected:
    void changeEvent(QEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

   private:
    void refresh();
    void readManaged();
    void addFiles();
    void remove(int sourceRow);

    QPointer<MinecraftInstance> m_instance;
    ModFolderModel* m_model = nullptr;
    std::function<QString(const QString&)> m_serverName;

    QLabel* m_count;
    QPushButton* m_add;
    QPushButton* m_folder;
    QLabel* m_note;
    QListView* m_list;
    QSortFilterProxyModel* m_proxy;
    KubikModDelegate* m_delegate = nullptr;
    QLabel* m_empty;
};
