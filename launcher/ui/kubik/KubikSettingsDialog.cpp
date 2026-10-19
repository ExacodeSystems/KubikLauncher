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

#include "KubikSettingsDialog.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSlider>
#include <QSpinBox>
#include <QStackedWidget>
#include <QVBoxLayout>

#include "Application.h"
#include "BuildConfig.h"
#include "DesktopServices.h"
#include "FileSystem.h"
#include "HardwareInfo.h"
#include "JavaCommon.h"
#include "SysInfo.h"
#include "java/JavaInstall.h"
#include "java/JavaInstallList.h"
#include "java/JavaUtils.h"
#include "settings/SettingsObject.h"
#include "ui/dialogs/VersionSelectDialog.h"
#include "ui/kubik/KubikLanguagePicker.h"
#include "ui/kubik/KubikProfilesPage.h"
#include "updater/ExternalUpdater.h"

namespace {

constexpr int MEMORY_STEP = 512;  // MiB per slider notch

enum Page { General = 0, Game = 1, Java = 2, Profiles = 3 };

QLabel* label(const char* objectName, QWidget* parent)
{
    auto* l = new QLabel(parent);
    l->setObjectName(objectName);
    l->setWordWrap(true);
    l->setTextFormat(Qt::PlainText);
    return l;
}

}  // namespace

KubikSettingsDialog::KubikSettingsDialog(const QString& page, QWidget* parent)
    : QDialog(parent), m_nav(new QButtonGroup(this)), m_pages(new QStackedWidget(this))
{
    setObjectName("kubikSettings");
    resize(860, 620);
    setMinimumSize(720, 520);
    text([this] { setWindowTitle(tr("Settings")); });

    // ── The page list, styled like the main window's rail ───────────────────
    auto* nav = new QFrame(this);
    nav->setObjectName("kubikSettingsNav");
    nav->setFixedWidth(208);
    auto* navLayout = new QVBoxLayout(nav);
    navLayout->setContentsMargins(0, 24, 0, 16);
    navLayout->setSpacing(2);
    auto* heading = new QLabel(nav);
    heading->setObjectName("kubikBrandName");
    heading->setContentsMargins(18, 0, 18, 16);
    text([heading] { heading->setText(tr("Settings")); });
    navLayout->addWidget(heading);
    struct Entry {
        Page page;
        const char* icon;
        std::function<QString()> name;
    };
    const QList<Entry> entries = {
        { General, "settings", [] { return tr("General"); } },
        { Game, "minecraft", [] { return tr("Game"); } },
        { Java, "java", [] { return tr("Java"); } },
        { Profiles, "accounts", [] { return tr("Profiles"); } },
    };
    for (const auto& entry : entries) {
        auto* button = new QPushButton(QIcon::fromTheme(entry.icon), QString(), nav);
        button->setIconSize(QSize(20, 20));
        button->setCheckable(true);
        button->setCursor(Qt::PointingHandCursor);
        button->setFocusPolicy(Qt::NoFocus);
        text([button, name = entry.name] { button->setText(name()); });
        m_nav->addButton(button, entry.page);
        navLayout->addWidget(button);
    }
    navLayout->addStretch();
    connect(m_nav, &QButtonGroup::idClicked, m_pages, &QStackedWidget::setCurrentIndex);

    m_pages->addWidget(pageFrame(buildGeneral()));
    m_pages->addWidget(pageFrame(buildGame()));
    m_pages->addWidget(pageFrame(buildJava()));
    m_pages->addWidget(pageFrame(new KubikProfilesPage()));

    // ── Done: everything above saved as it changed ──────────────────────────
    auto* done = new QPushButton(this);
    done->setProperty("kubikRole", "primary");
    done->setDefault(true);
    done->setCursor(Qt::PointingHandCursor);
    text([done] { done->setText(tr("Done")); });
    connect(done, &QPushButton::clicked, this, &QDialog::accept);
    auto* footer = new QHBoxLayout();
    footer->setContentsMargins(32, 10, 24, 18);
    footer->addStretch();
    footer->addWidget(done);

    auto* right = new QWidget(this);
    right->setObjectName("kubikPage");
    right->setAttribute(Qt::WA_StyledBackground);
    auto* rightLayout = new QVBoxLayout(right);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(0);
    rightLayout->addWidget(m_pages, 1);
    rightLayout->addLayout(footer);

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(nav);
    layout->addWidget(right, 1);

    int open = General;
    if (page == "minecraft-settings") {
        open = Game;
    } else if (page == "java-settings") {
        open = Java;
    } else if (page == "accounts") {
        open = Profiles;
    }
    m_nav->button(open)->setChecked(true);
    m_pages->setCurrentIndex(open);
}

KubikSettingsDialog::~KubikSettingsDialog() = default;

QString KubikSettingsDialog::gigabytes(int mib)
{
    const double gb = mib / 1024.0;
    return tr("%1 GB").arg(QString::number(gb, 'f', mib % 1024 == 0 ? 0 : 1));
}

QString KubikSettingsDialog::memoryNote(int max)
{
    const auto system = HardwareInfo::totalRamMiB();
    if (static_cast<uint64_t>(max) >= system) {
        return tr("That is all of this computer's memory. Minecraft would leave nothing for everything else, so lower it.");
    }
    if (static_cast<uint64_t>(max) * 10 > system * 9) {
        return tr("That is close to all of this computer's memory. If the computer slows down while you play, lower it.");
    }
    return tr("The most memory Minecraft may use. Recommended for this computer: %1.").arg(gigabytes(SysInfo::defaultMaxJvmMem()));
}

QWidget* KubikSettingsDialog::pageFrame(QWidget* body)
{
    auto* content = new QWidget();
    content->setObjectName("kubikPage");
    auto* column = new QVBoxLayout(content);
    column->setContentsMargins(36, 30, 36, 12);
    body->setParent(content);
    column->addWidget(body);
    column->addStretch();

    auto* scroll = new QScrollArea();
    scroll->setWidget(content);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    return scroll;
}

void KubikSettingsDialog::text(std::function<void()> apply)
{
    apply();
    m_texts.append(std::move(apply));
}

void KubikSettingsDialog::retranslate()
{
    for (const auto& apply : m_texts) {
        apply();
    }
    updateMemory();
}

void KubikSettingsDialog::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange) {
        retranslate();
    }
    QDialog::changeEvent(event);
}

void KubikSettingsDialog::done(int result)
{
    // Prism's Java page warned about arguments that fight the launcher (-Xmx and friends) when it
    // saved; this is the one moment that maps to.
    if (m_args && m_args->toPlainText() != m_argsAtOpen) {
        JavaCommon::checkJVMArgs(APPLICATION->settings()->get("JvmArgs").toString(), this);
    }
    QDialog::done(result);
}

// ── General ─────────────────────────────────────────────────────────────────

QWidget* KubikSettingsDialog::buildGeneral()
{
    auto* page = new QWidget();
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    auto* title = label("kubikPageTitle", page);
    text([title] { title->setText(tr("General")); });
    layout->addWidget(title);
    layout->addSpacing(14);

    auto* languageTitle = label("kubikSectionTitle", page);
    text([languageTitle] { languageTitle->setText(tr("Language")); });
    layout->addWidget(languageTitle);
    layout->addWidget(new KubikLanguagePicker(true, page));

    if (auto* updater = APPLICATION->updater()) {
        layout->addSpacing(20);
        auto* updatesTitle = label("kubikSectionTitle", page);
        text([updatesTitle] { updatesTitle->setText(tr("Updates")); });
        auto* automatic = new QCheckBox(page);
        text([automatic] { automatic->setText(tr("Check for KubikLauncher updates automatically")); });
        automatic->setChecked(updater->getAutomaticallyChecksForUpdates());
        connect(automatic, &QCheckBox::toggled, this, [](bool on) {
            if (auto* updater = APPLICATION->updater()) {
                updater->setAutomaticallyChecksForUpdates(on);
            }
        });
        layout->addWidget(updatesTitle);
        layout->addWidget(automatic);
    }

    layout->addSpacing(20);
    auto* filesTitle = label("kubikSectionTitle", page);
    text([filesTitle] { filesTitle->setText(tr("Files")); });
    auto* filesBody = label("kubikPageSubtitle", page);
    text([filesBody] { filesBody->setText(tr("Your instances, worlds, screenshots and logs live in KubikLauncher's folder.")); });
    auto* openFolder = new QPushButton(page);
    openFolder->setCursor(Qt::PointingHandCursor);
    text([openFolder] { openFolder->setText(tr("Open KubikLauncher's folder")); });
    connect(openFolder, &QPushButton::clicked, this, [] { DesktopServices::openPath("."); });
    layout->addWidget(filesTitle);
    layout->addWidget(filesBody);
    layout->addWidget(openFolder, 0, Qt::AlignLeft);
    return page;
}

// ── Game ────────────────────────────────────────────────────────────────────

QWidget* KubikSettingsDialog::buildGame()
{
    auto settings = APPLICATION->settings();
    auto* page = new QWidget();
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    auto* title = label("kubikPageTitle", page);
    text([title] { title->setText(tr("Game")); });
    layout->addWidget(title);
    layout->addSpacing(14);

    auto* windowTitle = label("kubikSectionTitle", page);
    text([windowTitle] { windowTitle->setText(tr("Game window")); });
    m_maximised = new QCheckBox(page);
    text([this] { m_maximised->setText(tr("Start Minecraft maximised")); });
    m_maximised->setChecked(settings->get("LaunchMaximized").toBool());

    m_width = new QSpinBox(page);
    m_height = new QSpinBox(page);
    for (auto* box : { m_width, m_height }) {
        box->setRange(1, 65536);
        box->setFixedWidth(110);
    }
    m_width->setValue(settings->get("MinecraftWinWidth").toInt());
    m_height->setValue(settings->get("MinecraftWinHeight").toInt());
    auto* sizeLabel = label("kubikPageBody", page);
    text([sizeLabel] { sizeLabel->setText(tr("Window size")); });
    auto* times = new QLabel(QStringLiteral("×"), page);
    times->setObjectName("kubikPageSubtitle");
    auto* sizeRow = new QHBoxLayout();
    sizeRow->setSpacing(8);
    sizeRow->addWidget(sizeLabel);
    sizeRow->addSpacing(8);
    sizeRow->addWidget(m_width);
    sizeRow->addWidget(times);
    sizeRow->addWidget(m_height);
    sizeRow->addStretch();

    const auto syncSize = [this] {
        m_width->setEnabled(!m_maximised->isChecked());
        m_height->setEnabled(!m_maximised->isChecked());
    };
    connect(m_maximised, &QCheckBox::toggled, this, [syncSize](bool on) {
        APPLICATION->settings()->set("LaunchMaximized", on);
        syncSize();
    });
    connect(m_width, &QSpinBox::valueChanged, this, [](int value) { APPLICATION->settings()->set("MinecraftWinWidth", value); });
    connect(m_height, &QSpinBox::valueChanged, this, [](int value) { APPLICATION->settings()->set("MinecraftWinHeight", value); });
    syncSize();

    layout->addWidget(windowTitle);
    layout->addWidget(m_maximised);
    layout->addLayout(sizeRow);

    layout->addSpacing(20);
    auto* playTitle = label("kubikSectionTitle", page);
    text([playTitle] { playTitle->setText(tr("While you play")); });
    auto* hide = new QCheckBox(page);
    text([hide] { hide->setText(tr("Hide KubikLauncher while Minecraft is open")); });
    hide->setChecked(settings->get("CloseAfterLaunch").toBool());
    connect(hide, &QCheckBox::toggled, this, [](bool on) { APPLICATION->settings()->set("CloseAfterLaunch", on); });
    auto* hideNote = label("kubikPageNote", page);
    text([hideNote] { hideNote->setText(tr("It comes back by itself when you close the game.")); });
    layout->addWidget(playTitle);
    layout->addWidget(hide);
    layout->addWidget(hideNote);
    return page;
}

// ── Java ────────────────────────────────────────────────────────────────────

QWidget* KubikSettingsDialog::buildJava()
{
    auto settings = APPLICATION->settings();
    auto* page = new QWidget();
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    auto* title = label("kubikPageTitle", page);
    text([title] { title->setText(tr("Java")); });
    layout->addWidget(title);
    layout->addSpacing(14);

    // Java itself: automatic, or a path the player picks.
    auto* javaTitle = label("kubikSectionTitle", page);
    text([javaTitle] { javaTitle->setText(tr("Java")); });
    m_autoJava = new QCheckBox(page);
    const bool downloads = BuildConfig.JAVA_DOWNLOADER_ENABLED;
    text([this, downloads] {
        m_autoJava->setText(downloads ? tr("Pick and download the right Java by itself (recommended)")
                                      : tr("Pick the right Java by itself (recommended)"));
    });
    m_autoJava->setChecked(settings->get("AutomaticJavaSwitch").toBool());
    auto* autoNote = label("kubikPageNote", page);
    text([autoNote, downloads] {
        autoNote->setText(downloads ? tr("Each Minecraft version needs its own Java. KubikLauncher gets it the first time you play.")
                                    : tr("Each Minecraft version needs its own Java. KubikLauncher picks it from the ones installed."));
    });
    connect(m_autoJava, &QCheckBox::toggled, this, [this, downloads](bool on) {
        APPLICATION->settings()->set("AutomaticJavaSwitch", on);
        if (downloads) {
            APPLICATION->settings()->set("AutomaticJavaDownload", on);
        }
        updateJavaMode();
    });

    m_manualJava = new QWidget(page);
    m_javaPath = new QLineEdit(m_manualJava);
    m_javaPath->setText(settings->get("JavaPath").toString());
    text([this] { m_javaPath->setPlaceholderText(tr("Path to the java executable")); });
    connect(m_javaPath, &QLineEdit::textChanged, this, [](const QString& path) { APPLICATION->settings()->set("JavaPath", path); });
    auto* detect = new QPushButton(m_manualJava);
    auto* browse = new QPushButton(m_manualJava);
    auto* test = new QPushButton(m_manualJava);
    for (auto* b : { detect, browse, test }) {
        b->setCursor(Qt::PointingHandCursor);
    }
    text([detect, browse, test] {
        detect->setText(tr("Find installed"));
        browse->setText(tr("Browse"));
        test->setText(tr("Test"));
    });
    connect(detect, &QPushButton::clicked, this, [this] {
        if (JavaUtils::getJavaCheckPath().isEmpty()) {
            JavaCommon::javaCheckNotFound(this);
            return;
        }
        VersionSelectDialog dialog(APPLICATION->javalist(), tr("Choose a Java installation"), this, true);
        dialog.setResizeOn(2);
        if (dialog.exec() == QDialog::Accepted && dialog.selectedVersion()) {
            const auto java = std::dynamic_pointer_cast<JavaInstall>(dialog.selectedVersion());
            if (java) {
                m_javaPath->setText(java->path);
            }
        }
    });
    connect(browse, &QPushButton::clicked, this, [this] {
        const QString raw = QFileDialog::getOpenFileName(this, tr("Find the java executable"));
        if (raw.isEmpty()) {
            return;
        }
        const QString cooked = FS::NormalizePath(raw);
        const QFileInfo info(cooked);
        if (info.exists() && info.isExecutable()) {
            m_javaPath->setText(cooked);
        }
    });
    connect(test, &QPushButton::clicked, this, [this] {
        if (m_javaTest) {
            return;
        }
        auto settings = APPLICATION->settings();
        m_javaTest = std::make_unique<JavaCommon::TestCheck>(this, m_javaPath->text(), settings->get("JvmArgs").toString(),
                                                             settings->get("MinMemAlloc").toInt(), settings->get("MaxMemAlloc").toInt(),
                                                             settings->get("PermGen").toInt());
        connect(m_javaTest.get(), &JavaCommon::TestCheck::finished, this, [this] { m_javaTest.reset(); });
        m_javaTest->run();
    });
    auto* manualRow = new QHBoxLayout(m_manualJava);
    manualRow->setContentsMargins(0, 4, 0, 0);
    manualRow->setSpacing(8);
    manualRow->addWidget(m_javaPath, 1);
    manualRow->addWidget(detect);
    manualRow->addWidget(browse);
    manualRow->addWidget(test);

    layout->addWidget(javaTitle);
    layout->addWidget(m_autoJava);
    layout->addWidget(autoNote);
    layout->addWidget(m_manualJava);

    // Memory: one slider for the most Minecraft may use. The minimum keeps its default.
    layout->addSpacing(20);
    auto* memoryTitle = label("kubikSectionTitle", page);
    text([memoryTitle] { memoryTitle->setText(tr("Memory")); });
    const int system = static_cast<int>(HardwareInfo::totalRamMiB());
    m_memory = new QSlider(Qt::Horizontal, page);
    m_memory->setObjectName("kubikMemory");
    m_memory->setRange(1, qMax(2, system / MEMORY_STEP));
    m_memory->setPageStep(2);
    m_memory->setValue(qMax(1, settings->get("MaxMemAlloc").toInt() / MEMORY_STEP));
    m_memory->setMaximumWidth(460);
    m_memoryValue = label("kubikSectionTitle", page);
    m_memoryValue->setMinimumWidth(70);
    auto* memoryRow = new QHBoxLayout();
    memoryRow->setSpacing(14);
    memoryRow->addWidget(m_memory, 1);
    memoryRow->addWidget(m_memoryValue);
    memoryRow->addStretch();
    m_memoryNote = label("kubikPageNote", page);
    connect(m_memory, &QSlider::valueChanged, this, [this](int notches) {
        auto settings = APPLICATION->settings();
        const int max = notches * MEMORY_STEP;
        settings->set("MaxMemAlloc", max);
        if (settings->get("MinMemAlloc").toInt() > max) {
            settings->set("MinMemAlloc", max);
        }
        updateMemory();
    });
    layout->addWidget(memoryTitle);
    layout->addLayout(memoryRow);
    layout->addWidget(m_memoryNote);

    // Java arguments: folded away, for the few who know what they want there.
    layout->addSpacing(20);
    m_argsToggle = new QPushButton(page);
    m_argsToggle->setProperty("kubikRole", "link");
    m_argsToggle->setCursor(Qt::PointingHandCursor);
    m_args = new QPlainTextEdit(page);
    m_args->setFixedHeight(84);
    m_args->setPlainText(settings->get("JvmArgs").toString());
    m_argsAtOpen = m_args->toPlainText();
    m_args->setVisible(!m_argsAtOpen.isEmpty());
    text([this] {
        m_argsToggle->setText(!m_args->isHidden() ? tr("Java arguments") : tr("Show Java arguments"));
        m_args->setPlaceholderText(tr("Leave this empty unless you know you need it."));
    });
    connect(m_argsToggle, &QPushButton::clicked, this, [this] {
        m_args->setVisible(m_args->isHidden());
        m_argsToggle->setText(!m_args->isHidden() ? tr("Java arguments") : tr("Show Java arguments"));
    });
    connect(m_args, &QPlainTextEdit::textChanged, this,
            [this] { APPLICATION->settings()->set("JvmArgs", m_args->toPlainText().replace("\n", " ")); });
    layout->addWidget(m_argsToggle, 0, Qt::AlignLeft);
    layout->addWidget(m_args);

    updateJavaMode();
    updateMemory();
    return page;
}

void KubikSettingsDialog::updateJavaMode()
{
    if (m_manualJava) {
        m_manualJava->setVisible(!m_autoJava->isChecked());
    }
}

void KubikSettingsDialog::updateMemory()
{
    if (!m_memory) {
        return;
    }
    const int max = m_memory->value() * MEMORY_STEP;
    m_memoryValue->setText(gigabytes(max));
    m_memoryNote->setText(memoryNote(max));
}
