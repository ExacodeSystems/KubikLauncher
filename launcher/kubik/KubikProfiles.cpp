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

#include "KubikProfiles.h"

#include <QHash>
#include <QJsonDocument>
#include <QJsonObject>

#include "Application.h"
#include "BaseInstance.h"
#include "InstanceList.h"
#include "Json.h"
#include "minecraft/MinecraftInstance.h"

namespace Kubik {

namespace {

QJsonObject choices()
{
    return QJsonDocument::fromJson(APPLICATION->settings()->get("KubikServerInstances").toString().toUtf8()).object();
}

QJsonObject lastPlayedMap()
{
    return QJsonDocument::fromJson(APPLICATION->settings()->get("KubikServerLastPlayed").toString().toUtf8()).object();
}

// Which server each running game joined. Not saved: a game outlives nothing but this process.
QHash<QString, QString>& playing()
{
    static QHash<QString, QString> map;
    return map;
}

}  // namespace

QString profileOf(BaseInstance* instance)
{
    return instance ? instance->settings()->get("KubikProfile").toString() : QString();
}

QList<BaseInstance*> instancesFor(const QString& profileKey)
{
    QList<BaseInstance*> out;
    if (profileKey.isEmpty()) {
        return out;
    }
    auto* list = APPLICATION->instances();
    for (int i = 0; i < list->count(); ++i) {
        auto* instance = list->at(i);
        if (profileOf(instance) == profileKey) {
            out.append(instance);
        }
    }
    std::sort(out.begin(), out.end(), [](BaseInstance* a, BaseInstance* b) { return a->lastLaunch() > b->lastLaunch(); });
    return out;
}

BaseInstance* rememberedInstance(const QString& serverId, const QString& profileKey)
{
    const QString id = choices().value(serverId).toString();
    auto* instance = id.isEmpty() ? nullptr : APPLICATION->instances()->getInstanceById(id);
    // A remembered instance that no longer fits (the server moved to another version) is no answer.
    return instance && profileOf(instance) == profileKey ? instance : nullptr;
}

void rememberInstance(const QString& serverId, BaseInstance* instance)
{
    if (!instance || serverId.isEmpty()) {
        return;
    }
    auto map = choices();
    map.insert(serverId, instance->id());
    APPLICATION->settings()->set("KubikServerInstances", QString::fromUtf8(QJsonDocument(map).toJson(QJsonDocument::Compact)));

    auto servers = serversOf(instance);
    if (!servers.contains(serverId)) {
        servers.append(serverId);
        instance->settings()->set("KubikServers", Json::fromStringList(servers));
    }
}

QStringList serversOf(BaseInstance* instance)
{
    return instance ? Json::toStringList(instance->settings()->get("KubikServers").toString()) : QStringList();
}

QDateTime lastPlayed(const QString& serverId)
{
    const auto value = lastPlayedMap().value(serverId);
    return value.isDouble() ? QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(value.toDouble())) : QDateTime();
}

void recordLaunch(const QString& serverId, BaseInstance* instance)
{
    if (serverId.isEmpty()) {
        return;
    }
    auto map = lastPlayedMap();
    map.insert(serverId, static_cast<double>(QDateTime::currentMSecsSinceEpoch()));
    APPLICATION->settings()->set("KubikServerLastPlayed", QString::fromUtf8(QJsonDocument(map).toJson(QJsonDocument::Compact)));
    if (instance) {
        playing().insert(instance->id(), serverId);
    }
}

QString playingServer(BaseInstance* instance)
{
    return instance && instance->isRunning() ? playing().value(instance->id()) : QString();
}

QString iconForLoader(const QString& loader)
{
    if (loader == "fabric") {
        return "fabricmc";
    }
    if (loader == "quilt") {
        return "quiltmc";
    }
    if (loader == "forge") {
        return "forge";
    }
    if (loader == "neoforge") {
        return "neoforged";
    }
    return "grass";
}

}  // namespace Kubik
