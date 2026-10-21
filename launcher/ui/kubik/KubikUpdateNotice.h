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

#include <QDateTime>
#include <QPointer>
#include <QSettings>
#include <QTimer>
#include <memory>

#include "updater/ExternalUpdater.h"

class QWidget;

/**
 * The update check where the launcher cannot install an update itself: macOS (no Sparkle feed) and
 * a Linux AppImage. It asks kubik.gg's api for the newest published version
 * (`GET /launcher/v1/version`, `latest`) and, when it is newer than this build, shows a notice at
 * the bottom of the rail with a button to the download page.
 *
 * It is an ExternalUpdater on purpose: Application then has an updater, so "Update…" in the menu,
 * the automatic-check setting and its interval all work through the gates they already have, and
 * the menu item no longer sits there doing nothing on a Mac.
 *
 * Its settings live in their own file in the data dir, like PrismExternalUpdater's. Dismissing the
 * notice hides that version only; the next one shows again.
 */
class KubikUpdateNotice : public ExternalUpdater {
    Q_OBJECT

   public:
    KubikUpdateNotice(QWidget* mainWindow, const QString& dataDir);
    ~KubikUpdateNotice() override;

    void checkForUpdates() override;
    bool getAutomaticallyChecksForUpdates() override;
    double getUpdateCheckInterval() override;
    bool getBetaAllowed() override;
    void setAutomaticallyChecksForUpdates(bool check) override;
    void setUpdateCheckInterval(double seconds) override;
    void setBetaAllowed(bool allowed) override;

    /** Where a newer version is downloaded from. */
    static QString downloadPage();

   private:
    void check(bool manual);
    void showNotice(const QString& version);
    void resetTimer();

    QPointer<QWidget> m_mainWindow;
    std::unique_ptr<QSettings> m_settings;
    QTimer m_timer;
    bool m_autoCheck = true;
    double m_interval = 86400;
    QDateTime m_lastCheck;
    bool m_checking = false;
};
