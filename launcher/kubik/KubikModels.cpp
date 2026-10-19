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

#include "KubikModels.h"

#include <QCryptographicHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRandomGenerator>
#include <QRegularExpression>

namespace Kubik {

namespace {

std::optional<QJsonObject> objectOf(const QByteArray& json)
{
    QJsonParseError error{};
    const auto doc = QJsonDocument::fromJson(json, &error);
    if (error.error != QJsonParseError::NoError || !doc.isObject()) {
        return std::nullopt;
    }
    return doc.object();
}

QString str(const QJsonObject& o, const char* key)
{
    const auto v = o.value(QLatin1String(key));
    return v.isString() ? v.toString() : QString();
}

std::optional<Profile> profileOf(const QJsonValue& value)
{
    if (!value.isObject()) {
        return std::nullopt;
    }
    const auto o = value.toObject();
    Profile p;
    p.key = str(o, "key");
    p.label = str(o, "label");
    p.minecraft = str(o, "minecraft");
    p.loader = str(o, "loader");
    p.modpackVersionId = str(o.value("modpack").toObject(), "versionId");
    if (p.key.isEmpty() || p.minecraft.isEmpty()) {
        return std::nullopt;
    }
    return p;
}

std::optional<Server> serverOf(const QJsonObject& o)
{
    Server s;
    s.id = str(o, "id");
    if (s.id.isEmpty()) {
        return std::nullopt;
    }
    s.name = str(o, "name");
    s.address = str(o, "address");
    s.port = o.value("port").toInt();
    s.edition = str(o, "edition");
    s.loader = str(o, "loader");
    s.loaderName = str(o, "loaderName");
    s.version = str(o, "version");
    s.status = str(o, "status");
    s.plan = str(o, "plan");
    s.locked = o.value("locked").toBool();
    s.lockReason = str(o, "lockReason");
    s.modpackName = str(o, "modpackName");
    s.profile = profileOf(o.value("profile"));
    s.unplayable = str(o, "unplayable");
    s.modCount = o.value("modCount").toInt();
    s.dashboardUrl = str(o, "dashboardUrl");
    return s;
}

// A sha512 is 128 hex digits. Anything else would make the checksum check pass on nothing.
bool isSha512(const QString& s)
{
    static const QRegularExpression re("^[0-9a-f]{128}$");
    return re.match(s).hasMatch();
}

// A bare file name, never a path into somewhere else in the instance. The same rule the api's
// isSafeJarName applies to the server's disk, so a name the server accepted is one we accept.
bool isSafeFilename(const QString& s)
{
    static const QRegularExpression re(R"(^[^/\\\x{0}]+\.jar$)");
    return s.size() <= 191 && !s.startsWith('.') && !s.contains("..") && re.match(s).hasMatch();
}

}  // namespace

std::optional<User> parseUser(const QByteArray& json)
{
    const auto o = objectOf(json);
    if (!o) {
        return std::nullopt;
    }
    // POST /token answers { token, user }; GET /me answers the user itself.
    const QJsonObject u = o->contains("user") ? o->value("user").toObject() : *o;
    User user{ str(u, "id"), str(u, "name"), str(u, "email") };
    if (user.id.isEmpty()) {
        return std::nullopt;
    }
    return user;
}

std::optional<QList<Server>> parseServers(const QByteArray& json)
{
    const auto o = objectOf(json);
    if (!o || !o->value("servers").isArray()) {
        return std::nullopt;
    }
    QList<Server> out;
    for (const auto& v : o->value("servers").toArray()) {
        if (auto s = serverOf(v.toObject())) {
            out.append(*s);
        }
    }
    return out;
}

std::optional<Server> parseServer(const QByteArray& json)
{
    const auto o = objectOf(json);
    return o ? serverOf(*o) : std::nullopt;
}

std::optional<Pack> parsePack(const QByteArray& json)
{
    const auto o = objectOf(json);
    if (!o) {
        return std::nullopt;
    }
    Pack pack;
    pack.revision = str(*o, "revision");
    const auto server = o->value("server").toObject();
    pack.serverId = str(server, "id");
    pack.serverName = str(server, "name");
    pack.address = str(server, "address");
    pack.status = str(server, "status");
    const auto profile = profileOf(o->value("profile"));
    pack.minecraft = str(*o, "minecraft");
    if (pack.revision.isEmpty() || pack.serverId.isEmpty() || !profile || pack.minecraft.isEmpty()) {
        return std::nullopt;
    }
    pack.profile = *profile;

    const auto loader = o->value("loader").toObject();
    pack.loader = str(loader, "id");
    pack.loaderBuild = str(loader, "build");

    if (o->value("modpack").isObject()) {
        const auto m = o->value("modpack").toObject();
        ModpackFile file{ str(m, "versionId"), str(m, "projectId"), str(m, "name"),
                          str(m, "url"),       str(m, "sha512"),    static_cast<qint64>(m.value("size").toDouble()) };
        if (file.versionId.isEmpty() || file.url.isEmpty()) {
            return std::nullopt;
        }
        pack.modpack = file;
    }

    if (!o->value("mods").isArray()) {
        return std::nullopt;
    }
    for (const auto& v : o->value("mods").toArray()) {
        const auto m = v.toObject();
        Mod mod{ str(m, "projectId"),
                 str(m, "versionId"),
                 str(m, "filename"),
                 str(m, "url"),
                 str(m, "sha512").toLower(),
                 static_cast<qint64>(m.value("size").toDouble()),
                 str(m, "client") == "required" };
        // All or nothing: a sync applies the pack, so one unreadable mod refuses the whole of it.
        if (mod.url.isEmpty() || !isSha512(mod.sha512) || !isSafeFilename(mod.filename)) {
            return std::nullopt;
        }
        pack.mods.append(mod);
    }
    for (const auto& v : o->value("unmanaged").toArray()) {
        const auto u = v.toObject();
        pack.unmanaged.append({ str(u, "filename"), str(u, "reason") });
    }
    return pack;
}

ApiError parseApiError(int status, const QByteArray& body)
{
    ApiError error{ status, QString(), QString() };
    if (const auto o = objectOf(body)) {
        const auto e = o->value("error").toObject();
        error.code = str(e, "code");
        error.reason = str(e, "reason");
    }
    if (error.code.isEmpty()) {
        error.code = status == 0 ? "offline" : status == 401 ? "unauthorized" : "unavailable";
    }
    return error;
}

int compareBuilds(const QString& a, const QString& b)
{
    static const QRegularExpression nonDigits("[^0-9]+");
    const auto pa = a.split(nonDigits, Qt::SkipEmptyParts);
    const auto pb = b.split(nonDigits, Qt::SkipEmptyParts);
    for (qsizetype i = 0; i < std::max(pa.size(), pb.size()); ++i) {
        const qlonglong x = i < pa.size() ? pa[i].toLongLong() : -1;
        const qlonglong y = i < pb.size() ? pb[i].toLongLong() : -1;
        if (x != y) {
            return x < y ? -1 : 1;
        }
    }
    return QString::compare(a, b);
}

QString loaderComponentUid(const QString& loader)
{
    if (loader == "fabric") {
        return "net.fabricmc.fabric-loader";
    }
    if (loader == "quilt") {
        return "org.quiltmc.quilt-loader";
    }
    if (loader == "forge") {
        return "net.minecraftforge";
    }
    if (loader == "neoforge") {
        return "net.neoforged";
    }
    return {};
}

QString makePkceVerifier()
{
    QByteArray bytes(32, Qt::Uninitialized);
    QRandomGenerator::system()->fillRange(reinterpret_cast<quint32*>(bytes.data()), bytes.size() / 4);
    return QString::fromLatin1(bytes.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));
}

QString pkceChallenge(const QString& verifier)
{
    const auto digest = QCryptographicHash::hash(verifier.toLatin1(), QCryptographicHash::Sha256);
    return QString::fromLatin1(digest.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));
}

}  // namespace Kubik
