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

#include "KubikInstanceAdvanced.h"

#include <QCheckBox>
#include <QDir>
#include <QEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSlider>
#include <QVBoxLayout>

#include "DesktopServices.h"
#include "FileSystem.h"
#include "HardwareInfo.h"
#include "kubik/KubikBrand.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"
#include "settings/SettingsObject.h"
#include "ui/kubik/KubikInstanceInfo.h"
#include "ui/kubik/KubikSettingsDialog.h"

namespace {

constexpr int MEMORY_STEP = 512;  // MiB per notch, as in Settings › Java

QLabel* label(const char* objectName, QWidget* parent)
{
    auto* l = new QLabel(parent);
    l->setObjectName(objectName);
    l->setWordWrap(true);
    l->setTextFormat(Qt::PlainText);
    return l;
}

QPushButton* button(const char* role, QWidget* parent)
{
    auto* b = new QPushButton(parent);
    if (role) {
        b->setProperty("kubikRole", role);
    }
    b->setCursor(Qt::PointingHandCursor);
    return b;
}

}  // namespace

KubikInstanceAdvanced::KubikInstanceAdvanced(QWidget* parent) : QWidget(parent)
{
    // ── What it is ──────────────────────────────────────────────────────────
    m_aboutTitle = label("kubikSectionTitle", this);
    auto* facts = new QGridLayout();
    facts->setHorizontalSpacing(20);
    facts->setVerticalSpacing(8);
    facts->setColumnStretch(1, 1);
    const auto fact = [this, facts](int row) {
        auto* key = label("kubikPageSubtitle", this);
        key->setWordWrap(false);
        m_keys << key;
        facts->addWidget(key, row, 0, Qt::AlignLeft | Qt::AlignVCenter);
    };
    for (int row = 0; row < 4; ++row) {
        fact(row);
    }
    m_minecraft = label("kubikPageBody", this);
    m_loader = label("kubikPageBody", this);
    m_played = label("kubikPageBody", this);
    m_folderPath = new QLineEdit(this);
    m_folderPath->setReadOnly(true);
    m_folderPath->setFont(QFont(KubikBrand::monoFamily(), 12));
    m_openFolder = button("quiet", this);
    m_openFolder->setIcon(QIcon::fromTheme("viewfolder"));
    connect(m_openFolder, &QPushButton::clicked, this, [this] {
        if (m_instance) {
            DesktopServices::openPath(m_instance->instanceRoot(), true);
        }
    });
    auto* folderRow = new QHBoxLayout();
    folderRow->setSpacing(8);
    folderRow->addWidget(m_folderPath, 1);
    folderRow->addWidget(m_openFolder);
    facts->addWidget(m_minecraft, 0, 1);
    facts->addWidget(m_loader, 1, 1);
    facts->addWidget(m_played, 2, 1);
    facts->addLayout(folderRow, 3, 1);

    // ── Memory: this instance's own, or the one in Settings ────────────────
    m_memoryTitle = label("kubikSectionTitle", this);
    m_ownMemory = new QCheckBox(this);
    m_memory = new QSlider(Qt::Horizontal, this);
    m_memory->setObjectName("kubikMemory");
    m_memory->setRange(1, qMax(2, static_cast<int>(HardwareInfo::totalRamMiB()) / MEMORY_STEP));
    m_memory->setPageStep(2);
    m_memory->setMaximumWidth(460);
    m_memoryValue = label("kubikSectionTitle", this);
    m_memoryValue->setMinimumWidth(70);
    m_memoryNote = label("kubikPageNote", this);
    auto* memoryRow = new QHBoxLayout();
    memoryRow->setSpacing(14);
    memoryRow->addWidget(m_memory, 1);
    memoryRow->addWidget(m_memoryValue);
    memoryRow->addStretch();
    connect(m_ownMemory, &QCheckBox::toggled, this, [this](bool own) {
        if (!m_loading && m_instance) {
            auto settings = m_instance->settings();
            settings->set("OverrideMemory", own);
            if (own) {
                settings->set("MaxMemAlloc", m_memory->value() * MEMORY_STEP);
            }
        }
        updateMemory();
    });
    connect(m_memory, &QSlider::valueChanged, this, [this](int notches) {
        if (!m_loading && m_instance && m_ownMemory->isChecked()) {
            auto settings = m_instance->settings();
            const int max = notches * MEMORY_STEP;
            settings->set("MaxMemAlloc", max);
            if (settings->get("MinMemAlloc").toInt() > max) {
                settings->set("MinMemAlloc", max);
            }
        }
        updateMemory();
    });

    // ── Java: a specific one for this instance, or the automatic choice ────
    m_javaTitle = label("kubikSectionTitle", this);
    m_ownJava = new QCheckBox(this);
    m_javaRow = new QWidget(this);
    m_javaPath = new QLineEdit(m_javaRow);
    m_javaBrowse = button(nullptr, m_javaRow);
    auto* javaLayout = new QHBoxLayout(m_javaRow);
    javaLayout->setContentsMargins(0, 2, 0, 0);
    javaLayout->setSpacing(8);
    javaLayout->addWidget(m_javaPath, 1);
    javaLayout->addWidget(m_javaBrowse);
    m_javaNote = label("kubikPageNote", this);
    const auto saveJava = [this] {
        if (m_loading || !m_instance) {
            return;
        }
        auto settings = m_instance->settings();
        const bool own = m_ownJava->isChecked();
        settings->set("OverrideJavaLocation", own);
        if (own) {
            // With automatic Java on, an instance marked AutomaticJava has its override cleared on load
            // (BaseInstance.cpp): a path the player chose is not automatic.
            settings->set("AutomaticJava", false);
            settings->set("JavaPath", m_javaPath->text().trimmed());
        } else {
            settings->reset("JavaPath");
        }
    };
    connect(m_ownJava, &QCheckBox::toggled, this, [this, saveJava] {
        saveJava();
        m_javaRow->setVisible(m_ownJava->isChecked());
    });
    connect(m_javaPath, &QLineEdit::editingFinished, this, saveJava);
    connect(m_javaBrowse, &QPushButton::clicked, this, [this, saveJava] {
        const QString raw = QFileDialog::getOpenFileName(this, tr("Find the java executable"));
        const QString cooked = raw.isEmpty() ? QString() : FS::NormalizePath(raw);
        if (!cooked.isEmpty() && QFileInfo(cooked).isExecutable()) {
            m_javaPath->setText(cooked);
            saveJava();
        }
    });

    // ── The instance itself ─────────────────────────────────────────────────
    m_instanceTitle = label("kubikSectionTitle", this);
    m_rename = button(nullptr, this);
    m_icon = button(nullptr, this);
    m_copy = button(nullptr, this);
    m_delete = button("danger", this);
    m_moreSettings = button("link", this);
    connect(m_rename, &QPushButton::clicked, this, [this] { emit requested(Request::Rename); });
    connect(m_icon, &QPushButton::clicked, this, [this] { emit requested(Request::ChangeIcon); });
    connect(m_copy, &QPushButton::clicked, this, [this] { emit requested(Request::Copy); });
    connect(m_delete, &QPushButton::clicked, this, [this] { emit requested(Request::Delete); });
    connect(m_moreSettings, &QPushButton::clicked, this, [this] { emit requested(Request::MoreSettings); });
    auto* actions = new QHBoxLayout();
    actions->setSpacing(10);
    actions->addWidget(m_rename);
    actions->addWidget(m_icon);
    actions->addWidget(m_copy);
    actions->addStretch();
    actions->addWidget(m_delete);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);
    layout->addWidget(m_aboutTitle);
    layout->addLayout(facts);
    layout->addSpacing(22);
    layout->addWidget(m_memoryTitle);
    layout->addWidget(m_ownMemory);
    layout->addLayout(memoryRow);
    layout->addWidget(m_memoryNote);
    layout->addSpacing(22);
    layout->addWidget(m_javaTitle);
    layout->addWidget(m_ownJava);
    layout->addWidget(m_javaRow);
    layout->addWidget(m_javaNote);
    layout->addSpacing(22);
    layout->addWidget(m_instanceTitle);
    layout->addLayout(actions);
    layout->addSpacing(6);
    layout->addWidget(m_moreSettings, 0, Qt::AlignLeft);
    layout->addStretch();

    retranslate();
}

void KubikInstanceAdvanced::setInstance(MinecraftInstance* instance)
{
    m_instance = instance;
    refresh();
}

void KubikInstanceAdvanced::refresh()
{
    if (!m_instance) {
        return;
    }
    m_loading = true;
    auto settings = m_instance->settings();

    const auto summary = KubikInstanceInfo::instance()->summary(m_instance);
    const QString minecraft = m_instance->getPackProfile()->getComponentVersion("net.minecraft");
    m_minecraft->setText(minecraft.isEmpty() ? tr("Not read yet") : minecraft);
    m_loader->setText(summary.loaderName.isEmpty() ? tr("None (vanilla)")
                                                   : QStringLiteral("%1 %2").arg(summary.loaderName, summary.loaderVersion));
    m_played->setText(KubikInstanceInfo::playLine(m_instance));
    m_folderPath->setText(QDir::toNativeSeparators(m_instance->instanceRoot()));

    // Off, each override reads the global value, which is what the slider and the path then show.
    m_ownMemory->setChecked(settings->get("OverrideMemory").toBool());
    m_memory->setValue(qMax(1, settings->get("MaxMemAlloc").toInt() / MEMORY_STEP));
    const bool ownJava = settings->get("OverrideJavaLocation").toBool();
    m_ownJava->setChecked(ownJava);
    m_javaPath->setText(ownJava ? settings->get("JavaPath").toString() : QString());
    m_javaRow->setVisible(ownJava);

    m_loading = false;
    updateMemory();
}

void KubikInstanceAdvanced::updateMemory()
{
    const int max = m_memory->value() * MEMORY_STEP;
    const bool own = m_ownMemory->isChecked();
    m_memory->setEnabled(own);
    m_memoryValue->setText(KubikSettingsDialog::gigabytes(max));
    m_memoryNote->setText(own ? KubikSettingsDialog::memoryNote(max)
                              : tr("Uses the memory set in Settings › Java (%1).").arg(KubikSettingsDialog::gigabytes(max)));
}

void KubikInstanceAdvanced::retranslate()
{
    m_aboutTitle->setText(tr("About this instance"));
    const QStringList keys = { tr("Minecraft"), tr("Mod loader"), tr("Played"), tr("Folder") };
    for (int i = 0; i < m_keys.size(); ++i) {
        m_keys[i]->setText(keys.value(i));
    }
    m_openFolder->setText(tr("Open"));
    m_memoryTitle->setText(tr("Memory"));
    m_ownMemory->setText(tr("Give this instance its own memory"));
    m_javaTitle->setText(tr("Java"));
    m_ownJava->setText(tr("Use a specific Java for this instance"));
    m_javaPath->setPlaceholderText(tr("Path to the java executable"));
    m_javaBrowse->setText(tr("Browse"));
    m_javaNote->setText(tr("Leave this off unless a mod needs a particular Java: KubikLauncher otherwise picks the right one."));
    m_instanceTitle->setText(tr("Instance"));
    m_rename->setText(tr("Rename"));
    m_icon->setText(tr("Change icon"));
    m_copy->setText(tr("Make a copy"));
    m_delete->setText(tr("Delete instance"));
    m_moreSettings->setText(tr("More settings…"));
    refresh();
}

void KubikInstanceAdvanced::showEvent(QShowEvent* event)
{
    refresh();  // play time, and overrides changed in the instance window meanwhile
    QWidget::showEvent(event);
}

void KubikInstanceAdvanced::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange) {
        retranslate();
    }
    QWidget::changeEvent(event);
}
