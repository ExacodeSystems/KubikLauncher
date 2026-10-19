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
#include <QTimer>

#include <functional>

#include "Application.h"
#include "InstanceList.h"
#include "kubik/KubikAccount.h"
#include "translations/TranslationsModel.h"
#include "ui/instanceview/InstanceView.h"
#include "ui/kubik/KubikHomePage.h"
#include "ui/kubik/KubikInstanceDelegate.h"
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
};

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
        { "Settings",
          [](QMainWindow* window) {
              if (auto* sidebar = window->findChild<KubikSidebar*>()) {
                  emit sidebar->settingsRequested();
              }
          },
          true },
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
        { "Instances-after-play",
          [](QMainWindow* window) {
              if (auto* sidebar = window->findChild<KubikSidebar*>()) {
                  sidebar->setCurrentPage(KubikSidebar::Instances);
              }
          } },
        { "Shared-server", [](QMainWindow* window) { openServer(window, shareServer); } },
    };
    static const QList<Step>& steps = !KubikAccount::get()->isSignedIn() ? signedOut : playServer.isEmpty() ? signedIn : playWalk;

    auto* timer = new QTimer(qApp);
    timer->setInterval(2500);
    QObject::connect(timer, &QTimer::timeout, qApp, [dir, timer] {
        static int shot = 0;
        static int step = 0;
        static bool pendingSave = false;
        static int waited = 0;
        static QString dialogName;

        // A dialog in front (the setup wizard, or one a step opened): save it, then dismiss it.
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
            save(window, dir, ++shot, current.name);
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
