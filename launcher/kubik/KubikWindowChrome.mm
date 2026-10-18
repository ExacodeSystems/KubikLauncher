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

#include "KubikWindowChrome.h"

#include <QEvent>
#include <QMouseEvent>
#include <QWidget>
#include <QWindow>

#import <AppKit/AppKit.h>

namespace {

// The traffic lights' strip: AppKit's standard title bar height.
constexpr int TITLE_BAR_INSET = 28;

/**
 * With the content under the title bar, Qt's widgets receive the clicks that
 * used to drag the window. This gives them back: a press in the top strip, on
 * a spot no button claims, moves the window, and a double-click zooms it the
 * way the title bar would.
 */
class TitleStripDragFilter final : public QObject {
   public:
    using QObject::QObject;

   protected:
    bool eventFilter(QObject* watched, QEvent* event) override {
        auto* widget = qobject_cast<QWidget*>(watched);
        if (!widget || (event->type() != QEvent::MouseButtonPress && event->type() != QEvent::MouseButtonDblClick)) {
            return false;
        }
        auto* mouse = static_cast<QMouseEvent*>(event);
        if (mouse->button() != Qt::LeftButton || mouse->position().y() >= TITLE_BAR_INSET) {
            return false;
        }
        QWidget* window = widget->window();
        if (event->type() == QEvent::MouseButtonDblClick) {
            NSView* view = reinterpret_cast<NSView*>(window->winId());
            [view.window zoom:nil];
            return true;
        }
        if (QWindow* handle = window->windowHandle()) {
            handle->startSystemMove();
            return true;
        }
        return false;
    }
};

}  // namespace

namespace KubikWindowChrome {

void apply(QWidget* window) {
    // Qt 6.9+: the client area covers the title bar, and the title bar draws no background of its own.
    window->setWindowFlag(Qt::ExpandedClientAreaHint, true);
    window->setWindowFlag(Qt::NoTitleBarBackgroundHint, true);

    // The one thing Qt has no flag for: the title text, which would otherwise sit on top of the page.
    NSView* view = reinterpret_cast<NSView*>(window->winId());
    if (NSWindow* nsWindow = view.window) {
        nsWindow.titleVisibility = NSWindowTitleHidden;
    }

    window->installEventFilter(new TitleStripDragFilter(window));
}

int titleBarInset() {
    return TITLE_BAR_INSET;
}

void ignoreSafeArea(QWidget* root) {
    root->setAttribute(Qt::WA_ContentsMarginsRespectsSafeArea, false);
    for (auto* child : root->findChildren<QWidget*>()) {
        child->setAttribute(Qt::WA_ContentsMarginsRespectsSafeArea, false);
    }
}

}  // namespace KubikWindowChrome
