#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTest>

#include <kubik/KubikGameFiles.h>
#include <minecraft/ServersDat.h>

#include <tag_compound.h>
#include <tag_list.h>
#include <tag_primitive.h>
#include <tag_string.h>

using namespace Kubik;

namespace {

nbt::tag_compound entry(const QString& name, const QString& ip)
{
    nbt::tag_compound server;
    server.put("name", nbt::tag_string(name.toStdString()));
    server.put("ip", nbt::tag_string(ip.toStdString()));
    return server;
}

nbt::tag_compound list(std::initializer_list<nbt::tag_compound> servers)
{
    nbt::tag_list entries(nbt::tag_type::Compound);
    for (const auto& server : servers) {
        entries.push_back(nbt::tag_compound(server));
    }
    nbt::tag_compound root;
    root.put("servers", std::move(entries));
    return root;
}

/** The list as "name@ip" strings, in order. */
QStringList names(const nbt::tag_compound& root)
{
    QStringList out;
    for (const auto& server : root.at("servers").as<nbt::tag_list>()) {
        const auto& c = server.as<nbt::tag_compound>();
        out << QString::fromStdString(c.at("name").as<nbt::tag_string>().get()) + "@" +
                   QString::fromStdString(c.at("ip").as<nbt::tag_string>().get());
    }
    return out;
}

const nbt::tag_compound& at(const nbt::tag_compound& root, int index)
{
    return root.at("servers").as<nbt::tag_list>().at(index).as<nbt::tag_compound>();
}

QJsonObject pack(const QByteArray& mcmeta)
{
    return QJsonDocument::fromJson(mcmeta).object().value("pack").toObject();
}

}  // namespace

class KubikGameFilesTest : public QObject {
    Q_OBJECT

   private slots:
    // ── servers.dat ──────────────────────────────────────────────────────────

    void addsTheServerToAnEmptyList()
    {
        nbt::tag_compound root;
        QVERIFY(upsertServerEntry(root, { "Alpha SMP", "alpha.kubik.gg", "PNG" }, {}));
        QCOMPARE(names(root), QStringList({ "Alpha SMP@alpha.kubik.gg" }));
        QVERIFY(at(root, 0).has_key("icon", nbt::tag_type::String));
    }

    void putsItFirstAndLeavesThePlayersEntries()
    {
        auto root = list({ entry("Hypixel", "mc.hypixel.net"), entry("Friend", "friend.example") });
        QVERIFY(upsertServerEntry(root, { "Alpha SMP", "alpha.kubik.gg", {} }, {}));
        QCOMPARE(names(root), QStringList({ "Alpha SMP@alpha.kubik.gg", "Hypixel@mc.hypixel.net", "Friend@friend.example" }));
    }

    void isTheSameAfterTheSecondJoin()
    {
        auto root = list({ entry("Hypixel", "mc.hypixel.net") });
        QVERIFY(upsertServerEntry(root, { "Alpha SMP", "alpha.kubik.gg", {} }, {}));
        QVERIFY(upsertServerEntry(root, { "Alpha SMP", "alpha.kubik.gg", {} }, "alpha.kubik.gg"));
        QCOMPARE(names(root), QStringList({ "Alpha SMP@alpha.kubik.gg", "Hypixel@mc.hypixel.net" }));
    }

    void movesOurEntryBackToTheTop()
    {
        auto root = list({ entry("Hypixel", "mc.hypixel.net"), entry("Old name", "alpha.kubik.gg") });
        QVERIFY(upsertServerEntry(root, { "Alpha SMP", "alpha.kubik.gg", {} }, "alpha.kubik.gg"));
        QCOMPARE(names(root), QStringList({ "Alpha SMP@alpha.kubik.gg", "Hypixel@mc.hypixel.net" }));
    }

    void replacesTheEntryItAddedForAnotherServer()
    {
        // A shared instance carries one server's mods at a time: Beta's entry would join with Alpha's.
        auto root = list({ entry("Beta Builds", "beta.kubik.gg"), entry("Hypixel", "mc.hypixel.net") });
        QVERIFY(upsertServerEntry(root, { "Alpha SMP", "alpha.kubik.gg", {} }, "beta.kubik.gg"));
        QCOMPARE(names(root), QStringList({ "Alpha SMP@alpha.kubik.gg", "Hypixel@mc.hypixel.net" }));
    }

    void neverTouchesThePlayersOwnEntryForTheSameAddress()
    {
        auto root = list({ entry("Hypixel", "mc.hypixel.net"), entry("My SMP", "Alpha.Kubik.gg:25565") });
        QVERIFY(!upsertServerEntry(root, { "Alpha SMP", "alpha.kubik.gg", {} }, {}));
        QCOMPARE(names(root), QStringList({ "Hypixel@mc.hypixel.net", "My SMP@Alpha.Kubik.gg:25565" }));
    }

    void keepsTagsItDoesNotKnow()
    {
        auto player = entry("Hypixel", "mc.hypixel.net");
        player.put("hidden", nbt::tag_byte(1));
        auto ours = entry("Alpha SMP", "alpha.kubik.gg");
        ours.put("acceptTextures", nbt::tag_byte(1));
        ours.put("icon", nbt::tag_string("favicon-from-the-server"));
        auto root = list({ player, ours });

        // Through the file format and back, as on disk.
        auto read = ServersDat::parse(ServersDat::serialize(root));
        QVERIFY(read);
        QVERIFY(upsertServerEntry(*read, { "Alpha SMP", "alpha.kubik.gg", "PNG" }, "alpha.kubik.gg"));
        read = ServersDat::parse(ServersDat::serialize(*read));
        QVERIFY(read);

        QCOMPARE(names(*read), QStringList({ "Alpha SMP@alpha.kubik.gg", "Hypixel@mc.hypixel.net" }));
        QVERIFY(at(*read, 0).has_key("acceptTextures", nbt::tag_type::Byte));
        // The icon Minecraft fetched from the server stays; ours is only for a new entry.
        QCOMPARE(QString::fromStdString(at(*read, 0).at("icon").as<nbt::tag_string>().get()), QString("favicon-from-the-server"));
        QVERIFY(at(*read, 1).has_key("hidden", nbt::tag_type::Byte));
    }

    void refusesAFileThatIsNotAServerList()
    {
        QVERIFY(!ServersDat::parse("not nbt at all"));
        QVERIFY(!ServersDat::parse({}));
    }

    // ── options.txt ──────────────────────────────────────────────────────────

    void writesTheLinesIntoAMissingFile()
    {
        QCOMPARE(enableResourcePack({}, "file/Kubik"), QString("resourcePacks:[\"vanilla\",\"file/Kubik\"]\n"));
    }

    void putsThePackRightAfterVanillaAndKeepsEveryOtherLine()
    {
        const QString before =
            "version:3955\nfov:0.25\nresourcePacks:[\"vanilla\",\"fabric\",\"file/"
            "Faithful.zip\"]\nincompatibleResourcePacks:[]\nlang:id_id\n";
        const QString after = enableResourcePack(before, "file/Kubik");
        QCOMPARE(after, QString("version:3955\nfov:0.25\nresourcePacks:[\"vanilla\",\"file/Kubik\",\"fabric\",\"file/Faithful.zip\"]\n"
                                "incompatibleResourcePacks:[]\nlang:id_id\n"));
    }

    void leavesAFileThatAlreadyHasItAlone()
    {
        const QString on = "resourcePacks:[\"vanilla\",\"file/Kubik\"]\r\nincompatibleResourcePacks:[]\r\n";
        QCOMPARE(enableResourcePack(on, "file/Kubik"), on);
    }

    void keepsWindowsLineEndings()
    {
        const QString before = "version:3955\r\nresourcePacks:[\"vanilla\"]\r\nincompatibleResourcePacks:[]\r\n";
        QCOMPARE(enableResourcePack(before, "file/Kubik"),
                 QString("version:3955\r\nresourcePacks:[\"vanilla\",\"file/Kubik\"]\r\nincompatibleResourcePacks:[]\r\n"));
    }

    void takesItOffTheIncompatibleListAnEarlierBuildPutItOn()
    {
        // Minecraft leaves a compatible pack found on this list switched off for the session.
        const QString before = "resourcePacks:[\"vanilla\",\"file/Kubik\"]\nincompatibleResourcePacks:[\"file/Kubik\",\"file/Old.zip\"]\n";
        QCOMPARE(enableResourcePack(before, "file/Kubik"),
                 QString("resourcePacks:[\"vanilla\",\"file/Kubik\"]\nincompatibleResourcePacks:[\"file/Old.zip\"]\n"));
    }

    void namesThePackTheWayEachVersionDoes()
    {
        QCOMPARE(resourcePackEntry("1.21.1", "Kubik"), QString("file/Kubik"));
        QCOMPARE(resourcePackEntry("1.13", "Kubik"), QString("file/Kubik"));
        QCOMPARE(resourcePackEntry("1.12.2", "Kubik"), QString("Kubik"));
        QCOMPARE(resourcePackEntry("26.2", "Kubik"), QString("file/Kubik"));
    }

    // ── pack.mcmeta ──────────────────────────────────────────────────────────

    void usesTheExactFormatOfAKnownVersion()
    {
        const auto old = pack(packMcmeta("1.20.1", "Kubik"));
        QCOMPARE(old.value("pack_format").toInt(), 15);
        QVERIFY(!old.contains("min_format"));

        const auto ranged = pack(packMcmeta("1.21.11", "Kubik"));
        QCOMPARE(ranged.value("pack_format").toInt(), 75);
        QCOMPARE(ranged.value("min_format").toArray().at(0).toInt(), 75);
        QCOMPARE(ranged.value("max_format").toArray().at(0).toInt(), 75);
    }

    void opensTheRangeForAVersionNewerThanTheTable()
    {
        const auto newer = pack(packMcmeta("99.1", "Kubik"));
        QVERIFY(newer.value("pack_format").toInt() >= 83);
        QCOMPARE(newer.value("min_format").toArray().at(0).toInt(), newer.value("pack_format").toInt());
        QCOMPARE(newer.value("max_format").toInt(), 1000);
        QCOMPARE(newer.value("description").toString(), QString("Kubik"));
    }

    void bundlesThePacksArt()
    {
        // Registered the way main.cpp does; an unregistered resourcepack.qrc shipped an empty pack once.
        Q_INIT_RESOURCE(resourcepack);
        for (const char* file :
             { "pack.png", "assets/minecraft/texts/splashes.txt", "assets/minecraft/textures/gui/menu_background.png",
               "assets/minecraft/textures/gui/options_background.png", "assets/minecraft/textures/gui/sprites/widget/button.png",
               "assets/minecraft/textures/gui/sprites/widget/button.png.mcmeta",
               "assets/minecraft/textures/gui/title/background/panorama_0.png",
               "assets/minecraft/textures/gui/title/background/panorama_overlay.png" }) {
            QVERIFY2(QFile::exists(QString(":/kubik/resourcepack/") + file), file);
        }
    }
};

QTEST_GUILESS_MAIN(KubikGameFilesTest)

#include "KubikGameFiles_test.moc"
