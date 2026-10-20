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

#include "KubikSnapshot.h"

#include <QApplication>
#include <QDebug>
#include <QDialog>
#include <QDir>
#include <QLineEdit>
#include <QMainWindow>
#include <QMenu>
#include <QPushButton>
#include <QScrollArea>
#include <QTimer>
#include <QWizard>

#include <functional>

#include "Application.h"
#include "InstanceList.h"
#include "kubik/KubikAccount.h"
#include "launch/LaunchTask.h"
#include "translations/TranslationsModel.h"
#include "ui/InstanceWindow.h"
#include "ui/instanceview/InstanceView.h"
#include "ui/kubik/KubikAccounts.h"
#include "ui/kubik/KubikHomePage.h"
#include "ui/kubik/KubikInstanceDelegate.h"
#include "ui/kubik/KubikInstancePage.h"
#include "ui/kubik/KubikJoinGuide.h"
#include "ui/kubik/KubikProfileCard.h"
#include "ui/kubik/KubikServerPage.h"
#include "ui/kubik/KubikServerTiles.h"
#include "ui/kubik/KubikServersPage.h"
#include "ui/kubik/KubikSidebar.h"

#ifndef NDEBUG
namespace {

void save(QWidget* widget, const QString& dir, int index, const QString& name)
{
    const QString path = QDir(dir).filePath(QString("%1-%2.png").arg(index, 2, 10, QChar('0')).arg(name));
    if (widget->grab().save(path)) {
        qInfo() << "KubikSnapshot: saved" << path;
    } else {
        qWarning() << "KubikSnapshot: failed to save" << path;
    }
}

QMainWindow* mainWindow()
{
    for (auto* widget : QApplication::topLevelWidgets()) {
        if (auto* window = qobject_cast<QMainWindow*>(widget); window && window->isVisible() && window->findChild<KubikSidebar*>()) {
            return window;
        }
    }
    return nullptr;
}

QString firstInstanceId(QMainWindow* window)
{
    if (auto* view = window->findChild<InstanceView*>(); view && view->model() && view->model()->rowCount() > 0) {
        return view->model()->index(0, 0).data(InstanceList::InstanceIDRole).toString();
    }
    return {};
}

/** One screen of the walk: get the window into a state, then (next tick) save it. */
struct Step {
    QString name;
    std::function<void(QMainWindow*)> prepare;  // may open a modal dialog, which is saved instead
    bool opensDialog = false;
    // Hold the save until this is true (checked every tick), for at most maxTicks ticks.
    std::function<bool(QMainWindow*)> until = {};
    int maxTicks = 0;
    // Save this window instead of the main one (a window the step opened), then close it.
    std::function<QWidget*(QMainWindow*)> target = {};
};

/** Scroll the page holding `widget` until the widget's top is in view. */
void scrollTo(QWidget* widget)
{
    for (QWidget* parent = widget ? widget->parentWidget() : nullptr; parent; parent = parent->parentWidget()) {
        if (auto* scroll = qobject_cast<QScrollArea*>(parent)) {
            scroll->ensureWidgetVisible(widget, 0, 40);
            return;
        }
    }
}

/** The server page's "How to join" on step `index`, scrolled into view. */
void showGuideStep(QMainWindow* window, int index)
{
    if (auto* guide = window->findChild<KubikJoinGuide*>()) {
        guide->showStep(index);
        scrollTo(guide);
    }
}

/**
 * The play walk's wait for Minecraft itself: true once the game Play started has logged that it is connecting
 * to the server, and a few ticks more so its screen can be captured; or once it has exited, or never started.
 * A loader's install is only proven by the game loading it, so the walk holds here rather than quitting the
 * launcher (which stops the game) the moment Play hands over.
 */
bool gameGotGoing()
{
    static int ticks = 0;
    static int connectingAt = -1;
    ++ticks;
    auto* instances = APPLICATION->instances();
    for (int i = 0; i < instances->count(); ++i) {
        auto* task = instances->at(i)->getLaunchTask();
        if (!task) {
            continue;
        }
        if (connectingAt < 0 && task->getLogModel()->toPlainText().contains("Connecting to ")) {
            connectingAt = ticks;
        }
        if (connectingAt >= 0) {
            return ticks - connectingAt >= 4;
        }
        // Exited before connecting (a crash), after the launch had time to get going.
        return !instances->at(i)->isRunning() && ticks > 4;
    }
    // Nothing launched (no account that can play): there is nothing to wait for.
    return ticks > 12;
}

/** The instance Play launched, else the first in the list (a walk on a data dir with several instances). */
QString playedInstanceId(QMainWindow* window)
{
    auto* instances = APPLICATION->instances();
    for (int i = 0; i < instances->count(); ++i) {
        if (instances->at(i)->getLaunchTask()) {
            return instances->at(i)->id();
        }
    }
    return firstInstanceId(window);
}

/** Open the server called `name` on its page (the play walk). */
void openServer(QMainWindow* window, const QString& name)
{
    auto* page = window->findChild<KubikServersPage*>();
    auto* grid = window->findChild<KubikServerGrid*>();
    if (!page || !grid) {
        return;
    }
    for (int row = 0; row < grid->model()->rowCount(); ++row) {
        const auto index = grid->model()->index(row, 0);
        if (index.data(Qt::DisplayRole).toString() == name) {
            emit page->serverOpened(index.data(KubikServerModel::IdRole).toString());
        }
    }
}

}  // namespace
#endif

namespace KubikSnapshot {

void startIfRequested()
{
#ifdef NDEBUG
    return;
#else
    const QString dir = qEnvironmentVariable("KUBIKLAUNCHER_SNAPSHOT_DIR");
    if (dir.isEmpty()) {
        return;
    }
    QDir().mkpath(dir);

    // Signed out, the window is the sign-in page and nothing else; signed in (a kubik-auth.json in the
    // data dir, pointed at an api with KUBIKLAUNCHER_API_URL), the walk goes through every page.
    static const QList<Step> signedOut = {
        { "SignIn", {} },
    };
    static const QList<Step> signedIn = {
        { "Servers", {} },
        { "Servers-hover",
          [](QMainWindow* window) {
              auto* grid = window->findChild<KubikServerGrid*>();
              auto* delegate = grid ? qobject_cast<KubikServerDelegate*>(grid->itemDelegate()) : nullptr;
              if (delegate && grid->model()->rowCount() > 0) {
                  delegate->setHoveredIndex(grid->model()->index(0, 0));
                  grid->viewport()->update();
              }
          } },
        { "Server",
          [](QMainWindow* window) {
              auto* page = window->findChild<KubikServersPage*>();
              auto* grid = window->findChild<KubikServerGrid*>();
              if (page && grid && grid->model()->rowCount() > 0) {
                  emit page->serverOpened(grid->model()->index(0, 0).data(KubikServerModel::IdRole).toString());
              }
          } },
        // The pack arrives a moment after the page opens; this shows it filled in.
        { "Server-pack", {} },
        { "Server-how-to-join",
          [](QMainWindow* window) {
              if (auto* link = window->findChild<QPushButton*>("kubikHowToJoin")) {
                  link->click();
              }
          } },
        { "Instances",
          [](QMainWindow* window) {
              if (auto* sidebar = window->findChild<KubikSidebar*>()) {
                  sidebar->setCurrentPage(KubikSidebar::Instances);
              }
          } },
        { "Instance",
          [](QMainWindow* window) {
              const QString id = firstInstanceId(window);
              if (auto* home = window->findChild<KubikHomePage*>(); home && !id.isEmpty()) {
                  emit home->instanceOpened(id);
              }
          } },
        // The profile card's dropdown is a popup, saved on its own.
        { "Profile-menu",
          [](QMainWindow* window) {
              if (auto* card = window->findChild<KubikProfileCard*>()) {
                  card->click();
              }
          },
          true },
        { "Settings-General", [](QMainWindow* window) { APPLICATION->ShowGlobalSettings(window, "general"); }, true },
        { "Settings-Game", [](QMainWindow* window) { APPLICATION->ShowGlobalSettings(window, "minecraft-settings"); }, true },
        { "Settings-Java", [](QMainWindow* window) { APPLICATION->ShowGlobalSettings(window, "java-settings"); }, true },
        { "Settings-Profiles", [](QMainWindow* window) { APPLICATION->ShowGlobalSettings(window, "accounts"); }, true },
        // Switching language at runtime: everything on screen must follow without a restart.
        { "Servers-language-switched",
          [](QMainWindow* window) {
              auto* translations = APPLICATION->translations();
              translations->selectLanguage(translations->selectedLanguage() == "id" ? "en_US" : "id");
              if (auto* sidebar = window->findChild<KubikSidebar*>()) {
                  sidebar->setCurrentPage(KubikSidebar::Servers);
              }
          } },
    };
    // KUBIKLAUNCHER_SNAPSHOT_PLAY=<server name>: press Play on that server for real (install the instance,
    // sync its mods, launch), then open the next server of the same profile to show it shares the instance.
    static const QString playServer = qEnvironmentVariable("KUBIKLAUNCHER_SNAPSHOT_PLAY");
    static const QString shareServer = qEnvironmentVariable("KUBIKLAUNCHER_SNAPSHOT_SHARE");
    static const QList<Step> playWalk = {
        { "Servers", {} },
        { "Play-server", [](QMainWindow* window) { openServer(window, playServer); } },
        // Down to "How to join", and each of its steps.
        { "Play-guide", [](QMainWindow* window) { scrollTo(window->findChild<KubikJoinGuide*>()); } },
        { "Play-guide-2", [](QMainWindow* window) { showGuideStep(window, 1); } },
        { "Play-guide-3", [](QMainWindow* window) { showGuideStep(window, 2); } },
        { "Play-guide-4", [](QMainWindow* window) { showGuideStep(window, 3); } },
        { "Play-started",
          [](QMainWindow* window) {
              if (auto* page = window->findChild<KubikServerPage*>()) {
                  page->play();
              }
          } },
        { "Play-progress", {}, false, [](QMainWindow*) { return false; }, 8 },
        // Installing Minecraft and the loader is a real download: give it ten minutes.
        { "Play-finished",
          {},
          false,
          [](QMainWindow* window) {
              auto* page = window->findChild<KubikServerPage*>();
              return page && !page->isPlaying();
          },
          240 },
        // Minecraft starting, up to joining the server: a first start of Forge runs its installer, so allow ten
        // minutes. The Logs tab further on shows what the game logged.
        { "Game-connecting", {}, false, [](QMainWindow*) { return gameGotGoing(); }, 240 },
        // The install log Play kept, opened.
        { "Play-details",
          [](QMainWindow* window) {
              if (auto* toggle = window->findChild<QPushButton*>("kubikDetailsToggle")) {
                  toggle->click();
              }
          } },
        { "Instances-after-play",
          [](QMainWindow* window) {
              if (auto* sidebar = window->findChild<KubikSidebar*>()) {
                  sidebar->setCurrentPage(KubikSidebar::Instances);
              }
          } },
        // The instance Play made, on each of its tabs.
        { "Instance-overview",
          [](QMainWindow* window) {
              const QString id = playedInstanceId(window);
              if (auto* home = window->findChild<KubikHomePage*>(); home && !id.isEmpty()) {
                  emit home->instanceOpened(id);
              }
          } },
        { "Instance-mods",
          [](QMainWindow* window) {
              if (auto* page = window->findChild<KubikInstancePage*>()) {
                  page->showTab(KubikInstancePage::Mods);
              }
          } },
        { "Instance-logs",
          [](QMainWindow* window) {
              if (auto* page = window->findChild<KubikInstancePage*>()) {
                  page->showTab(KubikInstancePage::Logs);
              }
          } },
        { "Instance-advanced",
          [](QMainWindow* window) {
              if (auto* page = window->findChild<KubikInstancePage*>()) {
                  page->showTab(KubikInstancePage::Advanced);
              }
          } },
        { "Shared-server", [](QMainWindow* window) { openServer(window, shareServer); } },
    };
    // KUBIKLAUNCHER_SNAPSHOT_DIALOGS=1 (on a data dir that already has an instance): every window a player can
    // open from an instance and from the profile card, each saved and then dismissed without doing anything.
    const auto action = [](KubikInstancePage::Action act) {
        return [act](QMainWindow* window) {
            if (auto* page = window->findChild<KubikInstancePage*>()) {
                emit page->actionRequested(page->instanceId(), act);
            }
        };
    };
    static const QList<Step> dialogsWalk = {
        { "Instance",
          [](QMainWindow* window) {
              const QString id = firstInstanceId(window);
              if (auto* home = window->findChild<KubikHomePage*>(); home && !id.isEmpty()) {
                  emit home->instanceOpened(id);
              }
          } },
        { "Dialog-rename", action(KubikInstancePage::Action::Rename), true },
        { "Dialog-icon", action(KubikInstancePage::Action::ChangeIcon), true },
        { "Dialog-copy", action(KubikInstancePage::Action::Copy), true },
        { "Dialog-delete", action(KubikInstancePage::Action::Delete), true },
        { "Dialog-export-zip", action(KubikInstancePage::Action::ExportZip), true },
        { "Dialog-export-mrpack", action(KubikInstancePage::Action::ExportModrinth), true },
        { "Dialog-shortcut", action(KubikInstancePage::Action::CreateShortcut), true },
        { "Dialog-group", action(KubikInstancePage::Action::ChangeGroup), true },
        { "Dialog-offline-name", [](QMainWindow* window) { KubikAccounts::addOffline(window); }, true },
        { "Dialog-about", [](QMainWindow* window) { QMetaObject::invokeMethod(window, "on_actionAbout_triggered"); }, true },
        { "Instance-window-open",
          [](QMainWindow* window) {
              if (auto* page = window->findChild<KubikInstancePage*>()) {
                  emit page->manageRequested(page->instanceId(), "settings");
              }
          } },
        { "Instance-window",
          {},
          false,
          {},
          0,
          [](QMainWindow*) -> QWidget* {
              for (auto* widget : QApplication::topLevelWidgets()) {
                  if (qobject_cast<InstanceWindow*>(widget) && widget->isVisible()) {
                      return widget;
                  }
              }
              return nullptr;
          } },
    };
    static const bool dialogs = qEnvironmentVariable("KUBIKLAUNCHER_SNAPSHOT_DIALOGS") == QLatin1String("1");
    static const QList<Step>& steps = !KubikAccount::get()->isSignedIn() ? signedOut
                                      : dialogs                          ? dialogsWalk
                                      : playServer.isEmpty()             ? signedIn
                                                                         : playWalk;

    auto* timer = new QTimer(qApp);
    timer->setInterval(2500);
    QObject::connect(timer, &QTimer::timeout, qApp, [dir, timer] {
        static int shot = 0;
        static int step = 0;
        static bool pendingSave = false;
        static int waited = 0;
        static QString dialogName;

        // The setup wizard: every page in turn, and on the Microsoft page its skip confirmation once.
        if (auto* wizard = qobject_cast<QWizard*>(QApplication::activeModalWidget())) {
            static bool askedSkip = false;
            save(wizard, dir, ++shot, "Wizard-" + wizard->currentPage()->objectName());
            if (wizard->currentId() != wizard->pageIds().last()) {
                wizard->next();
            } else if (auto* skip = wizard->currentPage()->findChild<QPushButton*>("kubikSkip"); skip && !askedSkip) {
                askedSkip = true;
                QTimer::singleShot(0, skip, &QPushButton::click);
            } else {
                wizard->reject();
            }
            return;
        }
        // A menu a step opened (the profile card's): save it, then close it.
        if (auto* menu = qobject_cast<QMenu*>(QApplication::activePopupWidget())) {
            save(menu, dir, ++shot, dialogName.isEmpty() ? QString("Menu") : dialogName);
            dialogName.clear();
            menu->close();
            return;
        }
        // A dialog in front (one a step opened): save it, then dismiss it.
        if (auto* modal = qobject_cast<QDialog*>(QApplication::activeModalWidget())) {
            save(modal, dir, ++shot, dialogName.isEmpty() ? QString(modal->metaObject()->className()) : dialogName);
            dialogName.clear();
            modal->reject();
            return;
        }

        auto* window = mainWindow();
        if (!window) {
            return;
        }
        if (pendingSave) {
            const Step& current = steps[step];
            if (current.until && !current.until(window) && waited < current.maxTicks) {
                ++waited;
                return;
            }
            waited = 0;
            QWidget* what = current.target ? current.target(window) : nullptr;
            save(what ? what : window, dir, ++shot, current.name);
            if (what && what != window) {
                what->close();
            }
            pendingSave = false;
            ++step;
            return;
        }
        if (step >= steps.size()) {
            timer->stop();
            QApplication::quit();
            return;
        }
        const Step& current = steps[step];
        if (current.opensDialog) {
            // Run the step from the event loop: it calls exec(), whose nested loop keeps this timer
            // ticking so the branch above can save and close the dialog.
            dialogName = current.name;
            ++step;
            QTimer::singleShot(0, window, [window, prepare = current.prepare] { prepare(window); });
            return;
        }
        if (current.prepare) {
            current.prepare(window);
        }
        pendingSave = true;
    });
    timer->start();
#endif
}

}  // namespace KubikSnapshot
