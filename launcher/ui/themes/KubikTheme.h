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

#include "FusionTheme.h"

/**
 * The kubik.gg dashboard's look: near-black "block" surfaces, a lit top-left
 * edge, a solid zero-blur lip under each block, grass green for go. Qt Style
 * Sheets have no box-shadow, but the dashboard never blurs a shadow, so every
 * one of its edges is a border here (`:/kubik/kubik.qss`).
 */
class KubikTheme : public FusionTheme {
   public:
    virtual ~KubikTheme() {}

    void apply(bool initial) override;
    QString id() override;
    QString name() override;
    QString tooltip() override;
    bool hasStyleSheet() override;
    QString appStyleSheet() override;
    QPalette colorScheme() override;
    double fadeAmount() override;
    QColor fadeColor() override;
};
