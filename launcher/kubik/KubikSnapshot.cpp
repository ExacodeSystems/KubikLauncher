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
#include "translations/TranslationsModel.h"
#include "ui/instanceview/InstanceView.h"
#include "ui/kubik/KubikHomePage.h"
#include "ui/kubik/KubikInstanceDelegate.h"
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
};

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

    static const QList<Step> steps = {
        { "Home", {} },
        { "Home-hover",
          [](QMainWindow* window) {
              auto* home = window->findChild<KubikHomePage*>();
              auto* view = window->findChild<InstanceView*>();
              if (home && view && view->model()->rowCount() > 0) {
                  home->delegate()->setHoveredIndex(view->model()->index(0, 0));
                  view->viewport()->update();
              }
          } },
        { "Home-search",
          [](QMainWindow* window) {
              if (auto* search = window->findChild<QLineEdit*>("kubikSearch")) {
                  search->setText("vanilla");
              }
          } },
        { "Home-search-none",
          [](QMainWindow* window) {
              if (auto* search = window->findChild<QLineEdit*>("kubikSearch")) {
                  search->setText("no such instance");
              }
          } },
        // Play with no Microsoft account: the launch must stop at the "add an account" prompt.
        { "Play-no-account",
          [](QMainWindow* window) {
              if (auto* search = window->findChild<QLineEdit*>("kubikSearch")) {
                  search->clear();
              }
              const QString id = firstInstanceId(window);
              if (auto* home = window->findChild<KubikHomePage*>(); home && !id.isEmpty()) {
                  emit home->playRequested(id);
              }
          },
          true },
        { "Instance",
          [](QMainWindow* window) {
              const QString id = firstInstanceId(window);
              if (auto* home = window->findChild<KubikHomePage*>(); home && !id.isEmpty()) {
                  emit home->instanceOpened(id);
              }
          } },
        { "Servers",
          [](QMainWindow* window) {
              if (auto* sidebar = window->findChild<KubikSidebar*>()) {
                  sidebar->setCurrentPage(KubikSidebar::Servers);
              }
          } },
        { "NewInstance",
          [](QMainWindow* window) {
              if (auto* home = window->findChild<KubikHomePage*>()) {
                  emit home->newInstanceRequested(QString());
              }
          },
          true },
        { "Settings",
          [](QMainWindow* window) {
              if (auto* sidebar = window->findChild<KubikSidebar*>()) {
                  emit sidebar->settingsRequested();
              }
          },
          true },
        // Switching language at runtime: everything on screen must follow without a restart.
        { "Home-language-switched",
          [](QMainWindow* window) {
              auto* translations = APPLICATION->translations();
              translations->selectLanguage(translations->selectedLanguage() == "id" ? "en_US" : "id");
              if (auto* sidebar = window->findChild<KubikSidebar*>()) {
                  sidebar->setCurrentPage(KubikSidebar::Home);
              }
          } },
    };

    auto* timer = new QTimer(qApp);
    timer->setInterval(2500);
    QObject::connect(timer, &QTimer::timeout, qApp, [dir, timer] {
        static int shot = 0;
        static int step = 0;
        static bool pendingSave = false;
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
            save(window, dir, ++shot, steps[step].name);
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
