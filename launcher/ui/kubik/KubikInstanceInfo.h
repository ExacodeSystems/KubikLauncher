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

#include <QHash>
#include <QObject>
#include <QSet>
#include <QString>

class BaseInstance;

/** What a tile and the instance page say about an instance, beyond its name. */
struct KubikInstanceSummary {
    QString minecraftVersion;  // "1.21.1"; empty until read
    QString loaderName;        // "Fabric", "NeoForge"...; empty for vanilla
    QString loaderVersion;
    int modCount = -1;  // -1 until counted
};

/**
 * A cache of instance summaries, kept off the paint path.
 *
 * Reading the Minecraft version needs the instance's pack profile loaded from
 * disk, and counting mods means listing a folder, so neither may happen while a
 * grid of tiles paints. summary() answers from the cache at once and schedules
 * the work the first time it is asked; summaryChanged says when there is more
 * to show.
 */
class KubikInstanceInfo : public QObject {
    Q_OBJECT

   public:
    static KubikInstanceInfo* instance();

    /** The cached summary. Never blocks; asks for a refresh if there is none yet. */
    KubikInstanceSummary summary(BaseInstance* instance);

    /** Forgets every summary, so the next ask re-reads (mods added in an instance window). */
    void invalidateAll();

    /** "Minecraft 1.21.1 · Fabric · 12 mods", as much of it as is known. */
    static QString describe(const KubikInstanceSummary& summary);
    /** "Played 2 days ago" / "Never played". */
    static QString lastPlayed(const BaseInstance* instance);
    /** "3 h 20 min played" / "Not played yet". */
    static QString totalPlayed(const BaseInstance* instance);

   signals:
    void summaryChanged(const QString& instanceId);

   private:
    explicit KubikInstanceInfo(QObject* parent = nullptr);
    void refresh(const QString& instanceId);

    QHash<QString, KubikInstanceSummary> m_cache;
    QSet<QString> m_pending;
};
