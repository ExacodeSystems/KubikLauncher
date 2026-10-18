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

#include "KubikTheme.h"

#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QFont>
#include <QObject>
#include <QRegularExpression>

#include "kubik/KubikBrand.h"

using KubikBrand::color;

void KubikTheme::apply(bool initial)
{
    KubikBrand::registerFonts();
    QFont font = QApplication::font();
    font.setFamily(KubikBrand::bodyFamily());
    // One body size on every OS (Windows defaults to 9pt, macOS to 13pt): 14px reads comfortably
    // and is the size the rest of the type scale in kubik.qss is measured against.
    font.setPixelSize(14);
    QApplication::setFont(font);
    FusionTheme::apply(initial);
}

QString KubikTheme::id()
{
    return "kubik";
}

QString KubikTheme::name()
{
    return QObject::tr("Kubik");
}

QString KubikTheme::tooltip()
{
    return QObject::tr("The kubik.gg look");
}

QPalette KubikTheme::colorScheme()
{
    QPalette p;
    p.setColor(QPalette::Window, color("void.raised"));
    p.setColor(QPalette::WindowText, color("bone"));
    p.setColor(QPalette::Base, color("void"));
    p.setColor(QPalette::AlternateBase, color("void.panel"));
    p.setColor(QPalette::ToolTipBase, color("void.panel"));
    p.setColor(QPalette::ToolTipText, color("bone"));
    p.setColor(QPalette::PlaceholderText, color("bone.faint"));
    p.setColor(QPalette::Text, color("bone"));
    p.setColor(QPalette::Button, color("void.panel"));
    p.setColor(QPalette::ButtonText, color("bone"));
    p.setColor(QPalette::BrightText, color("heart"));
    p.setColor(QPalette::Link, color("sky"));
    p.setColor(QPalette::LinkVisited, color("sky.soft"));
    p.setColor(QPalette::Highlight, color("grass"));
    p.setColor(QPalette::HighlightedText, color("void"));
    // Fusion draws its bevels and frames from these five. They are surface tones, not edge tones,
    // so the frames Fusion still draws (combo boxes, spin boxes, check boxes) melt into their fill:
    // the launcher separates surfaces by tone, never by an outline.
    p.setColor(QPalette::Light, color("block"));
    p.setColor(QPalette::Midlight, color("void.panel"));
    p.setColor(QPalette::Mid, color("void.panel"));
    p.setColor(QPalette::Dark, color("void.raised"));
    p.setColor(QPalette::Shadow, color("void"));
    return fadeInactive(p, fadeAmount(), fadeColor());
}

double KubikTheme::fadeAmount()
{
    return 0.5;
}

QColor KubikTheme::fadeColor()
{
    return color("void.raised");
}

bool KubikTheme::hasStyleSheet()
{
    return true;
}

QString KubikTheme::appStyleSheet()
{
    QFile file(QStringLiteral(":/kubik/kubik.qss"));
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "KubikTheme: cannot read the stylesheet:" << file.errorString();
        return {};
    }
    QString qss = QString::fromUtf8(file.readAll());

    // @font.display@ and friends name the bundled faces; every other @name@ is a
    // palette colour, written as the dashboard's Tailwind name ("@void.panel@").
    qss.replace("@font.display@", KubikBrand::displayFamily());
    qss.replace("@font.body@", KubikBrand::bodyFamily());
    qss.replace("@font.mono@", KubikBrand::monoFamily());
    static const QRegularExpression token(QStringLiteral("@([a-z]+(?:\\.[a-z]+)?)@"));
    QString out;
    qsizetype last = 0;
    for (auto it = token.globalMatch(qss); it.hasNext();) {
        const auto match = it.next();
        out += qss.mid(last, match.capturedStart() - last);
        out += color(match.captured(1)).name();
        last = match.capturedEnd();
    }
    out += qss.mid(last);
    return out;
}
