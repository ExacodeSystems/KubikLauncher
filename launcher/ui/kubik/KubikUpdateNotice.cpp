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

#include "KubikUpdateNotice.h"

#include <QDesktopServices>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QPushButton>
#include <QUrl>
#include <QWidget>

#include <limits>

#include "BuildConfig.h"
#include "Version.h"
#include "kubik/KubikApi.h"
#include "ui/kubik/KubikSidebar.h"

namespace {

// How soon after start the first automatic check runs: after the window is up, not in its way.
constexpr int FIRST_CHECK_DELAY_MS = 8000;

}  // namespace

KubikUpdateNotice::KubikUpdateNotice(QWidget* mainWindow, const QString& dataDir) : m_mainWindow(mainWindow)
{
    m_settings = std::make_unique<QSettings>(QDir(dataDir).absoluteFilePath("kubik_update_notice.cfg"), QSettings::IniFormat);
    m_autoCheck = m_settings->value("auto_check", true).toBool();
    bool intervalOk = false;
    m_interval = m_settings->value("update_interval", 86400).toDouble(&intervalOk);
    if (!intervalOk || m_interval < 0) {
        m_interval = 86400;
    }
    m_lastCheck = QDateTime::fromString(m_settings->value("last_check").toString(), Qt::ISODate);

    m_timer.setSingleShot(true);
    connect(&m_timer, &QTimer::timeout, this, [this] {
        check(false);
        resetTimer();
    });
    resetTimer();

    // The rail's Download and close buttons. The sidebar is found once, here: the main window and
    // its rail exist before Application creates the updater.
    if (auto* sidebar = m_mainWindow ? m_mainWindow->findChild<KubikSidebar*>() : nullptr) {
        connect(sidebar, &KubikSidebar::updateNoticeDismissed, this, [this](const QString& version) {
            m_settings->setValue("dismissed", version);
        });
    }
}

KubikUpdateNotice::~KubikUpdateNotice() = default;

QString KubikUpdateNotice::downloadPage()
{
    return QStringLiteral("https://kubik.gg/launcher");
}

void KubikUpdateNotice::resetTimer()
{
    m_timer.stop();
    if (!m_autoCheck) {
        return;
    }
    // "On launch" (0) and a check that is due both run shortly after start; otherwise at the next due time.
    qint64 wait = FIRST_CHECK_DELAY_MS;
    if (m_interval > 0 && m_lastCheck.isValid()) {
        const qint64 remaining = qint64(m_interval * 1000) - m_lastCheck.msecsTo(QDateTime::currentDateTime());
        wait = qMax<qint64>(FIRST_CHECK_DELAY_MS, remaining);
    }
    m_timer.start(int(qMin<qint64>(wait, std::numeric_limits<int>::max())));
}

void KubikUpdateNotice::checkForUpdates()
{
    check(true);
}

void KubikUpdateNotice::check(bool manual)
{
    if (m_checking) {
        return;
    }
    m_checking = true;
    emit canCheckForUpdatesChanged(false);
    Kubik::apiGet("version", QString(), this, [this, manual](const Kubik::Reply& reply) {
        m_checking = false;
        emit canCheckForUpdatesChanged(true);
        const auto latest = QJsonDocument::fromJson(reply.body).object().value("latest").toString();
        if (!reply.ok() || latest.isEmpty()) {
            if (manual) {
                QMessageBox::warning(m_mainWindow, tr("Update check failed"),
                                     tr("KubikLauncher could not reach kubik.gg to check for a new version. Try again later."));
            }
            return;
        }
        m_lastCheck = QDateTime::currentDateTime();
        m_settings->setValue("last_check", m_lastCheck.toString(Qt::ISODate));

        const bool newer = Version(BuildConfig.versionString()) < Version(latest);
        if (!newer) {
            if (manual) {
                QMessageBox::information(m_mainWindow, tr("No update"),
                                         tr("You have the newest version of KubikLauncher (%1).").arg(BuildConfig.versionString()));
            }
            return;
        }
        // A dismissed version stays dismissed on its own; asking by hand always shows it.
        if (manual || m_settings->value("dismissed").toString() != latest) {
            showNotice(latest);
        }
        if (manual) {
            QMessageBox box(QMessageBox::Information, tr("Update available"),
                            tr("KubikLauncher %1 is out. Download it from kubik.gg and install it over this one: your "
                               "instances and settings stay.")
                                .arg(latest),
                            QMessageBox::NoButton, m_mainWindow);
            auto* download = box.addButton(tr("Download"), QMessageBox::AcceptRole);
            box.addButton(tr("Later"), QMessageBox::RejectRole);
            box.exec();
            if (box.clickedButton() == download) {
                QDesktopServices::openUrl(QUrl(downloadPage()));
            }
        }
    });
}

void KubikUpdateNotice::showNotice(const QString& version)
{
    if (auto* sidebar = m_mainWindow ? m_mainWindow->findChild<KubikSidebar*>() : nullptr) {
        sidebar->setUpdateNotice(version);
    }
}

bool KubikUpdateNotice::getAutomaticallyChecksForUpdates()
{
    return m_autoCheck;
}

double KubikUpdateNotice::getUpdateCheckInterval()
{
    return m_interval;
}

bool KubikUpdateNotice::getBetaAllowed()
{
    // The notice follows stable releases only: `latest` is the newest published stable version.
    return false;
}

void KubikUpdateNotice::setAutomaticallyChecksForUpdates(bool check)
{
    m_autoCheck = check;
    m_settings->setValue("auto_check", check);
    resetTimer();
}

void KubikUpdateNotice::setUpdateCheckInterval(double seconds)
{
    m_interval = seconds;
    m_settings->setValue("update_interval", seconds);
    resetTimer();
}

void KubikUpdateNotice::setBetaAllowed(bool /*allowed*/) {}
