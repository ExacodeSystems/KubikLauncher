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

#include "KubikInstanceLogs.h"

#include <QClipboard>
#include <QComboBox>
#include <QDir>
#include <QEvent>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QLocale>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollBar>
#include <QStackedWidget>
#include <QVBoxLayout>

#include "DesktopServices.h"
#include "FileSystem.h"
#include "kubik/KubikBrand.h"
#include "launch/LaunchTask.h"
#include "launch/LogModel.h"
#include "minecraft/MinecraftInstance.h"
#include "ui/widgets/LogView.h"

namespace {

const QString LIVE = QStringLiteral("live");
// A long session's latest.log can run to megabytes; the end is what explains a problem.
constexpr qint64 TAIL_BYTES = 1024 * 1024;

QString readTail(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    if (file.size() > TAIL_BYTES) {
        file.seek(file.size() - TAIL_BYTES);
        file.readLine();  // the cut lands mid-line; start at the next one
    }
    return QString::fromUtf8(file.readAll());
}

}  // namespace

KubikInstanceLogs::KubikInstanceLogs(QWidget* parent) : QWidget(parent)
{
    m_source = new QComboBox(this);
    m_source->setMinimumWidth(240);
    connect(m_source, &QComboBox::currentIndexChanged, this, &KubikInstanceLogs::showSource);

    m_copy = new QPushButton(QIcon::fromTheme("copy"), QString(), this);
    m_copy->setProperty("kubikRole", "quiet");
    m_copy->setCursor(Qt::PointingHandCursor);
    connect(m_copy, &QPushButton::clicked, this, [this] {
        const bool live = m_source->currentData().toString() == LIVE;
        QGuiApplication::clipboard()->setText(live && m_live ? m_live->toPlainText() : m_fileView->toPlainText());
    });
    m_folder = new QPushButton(QIcon::fromTheme("viewfolder"), QString(), this);
    m_folder->setProperty("kubikRole", "quiet");
    m_folder->setCursor(Qt::PointingHandCursor);
    connect(m_folder, &QPushButton::clicked, this, [this] {
        if (m_instance) {
            DesktopServices::openPath(FS::PathCombine(m_instance->gameRoot(), "logs"), true);
        }
    });

    auto* header = new QHBoxLayout();
    header->setSpacing(8);
    header->addWidget(m_source);
    header->addStretch();
    header->addWidget(m_copy);
    header->addWidget(m_folder);

    const QFont mono(KubikBrand::monoFamily(), 11);
    m_liveView = new LogView(this);
    m_liveView->setObjectName("kubikLog");
    m_liveView->setFont(mono);
    m_liveView->setWordWrap(false);
    m_fileView = new QPlainTextEdit(this);
    m_fileView->setObjectName("kubikLog");
    m_fileView->setReadOnly(true);
    m_fileView->setLineWrapMode(QPlainTextEdit::NoWrap);
    m_fileView->setFont(mono);
    m_empty = new QLabel(this);
    m_empty->setObjectName("kubikPageBody");
    m_empty->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_empty->setWordWrap(true);

    m_views = new QStackedWidget(this);
    m_views->addWidget(m_liveView);
    m_views->addWidget(m_fileView);
    m_views->addWidget(m_empty);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);
    layout->addLayout(header);
    layout->addWidget(m_views, 1);

    retranslate();
}

void KubikInstanceLogs::setInstance(MinecraftInstance* instance)
{
    disconnect(m_launchWatch);
    m_instance = instance;
    followLaunchTask(instance ? instance->getLaunchTask() : nullptr);
    if (instance) {
        // A new launch is a new log: follow it and show it.
        m_launchWatch = connect(instance, &BaseInstance::launchTaskChanged, this, [this](LaunchTask* task) {
            followLaunchTask(task);
            refreshSources();
            m_source->setCurrentIndex(0);
        });
    }
    refreshSources();
}

void KubikInstanceLogs::followLaunchTask(LaunchTask* task)
{
    m_live = task ? task->getLogModel() : nullptr;
    m_liveView->setModel(m_live.get());
}

void KubikInstanceLogs::refreshSources()
{
    const QString previous = m_source->currentData().toString();
    const QSignalBlocker block(m_source);
    m_source->clear();
    if (m_live) {
        m_source->addItem(tr("This session"), LIVE);
    }
    if (m_instance) {
        const QString latest = FS::PathCombine(m_instance->gameRoot(), "logs", "latest.log");
        if (QFileInfo::exists(latest)) {
            m_source->addItem(tr("Last game"), latest);
        }
        const QDir crashes(FS::PathCombine(m_instance->gameRoot(), "crash-reports"));
        for (const auto& report : crashes.entryInfoList({ "*.txt" }, QDir::Files, QDir::Time)) {
            m_source->addItem(tr("Crash report · %1").arg(QLocale().toString(report.lastModified(), QLocale::ShortFormat)),
                              report.absoluteFilePath());
        }
    }
    const int keep = m_source->findData(previous);
    m_source->setCurrentIndex(keep >= 0 ? keep : 0);
    showSource();
}

void KubikInstanceLogs::showSource()
{
    const QString source = m_source->currentData().toString();
    m_source->setVisible(m_source->count() > 0);
    m_copy->setVisible(!source.isEmpty());
    if (source.isEmpty()) {
        m_views->setCurrentWidget(m_empty);
    } else if (source == LIVE) {
        m_views->setCurrentWidget(m_liveView);
    } else {
        m_fileView->setPlainText(readTail(source));
        m_fileView->verticalScrollBar()->setValue(m_fileView->verticalScrollBar()->maximum());
        m_views->setCurrentWidget(m_fileView);
    }
}

void KubikInstanceLogs::retranslate()
{
    m_copy->setText(tr("Copy"));
    m_folder->setText(tr("Open logs folder"));
    m_empty->setText(tr("No logs yet. They appear here after you play."));
    refreshSources();
}

void KubikInstanceLogs::showEvent(QShowEvent* event)
{
    // Minecraft may have written a new latest.log or a crash report since this was last shown.
    refreshSources();
    QWidget::showEvent(event);
}

void KubikInstanceLogs::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange) {
        retranslate();
    }
    QWidget::changeEvent(event);
}
