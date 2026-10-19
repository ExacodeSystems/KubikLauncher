#include <QTest>

#include <kubik/KubikModels.h>

using namespace Kubik;

namespace {

// The fixtures are written with single quotes and turned into JSON by json(): moc tokenises this
// file, and raw string literals full of quotes and braces make it lose the test class.
QByteArray json(const char* text)
{
    return QByteArray(text).replace('\'', '"');
}

const QByteArray SHA = QByteArray(128, 'a');

QByteArray packJson(const QByteArray& mods, const QByteArray& modpack = "null")
{
    return json(
               "{ 'revision': 'r1',"
               "  'server': { 'id': 's1', 'name': 'Fabric world', 'address': 'steve.kubik.gg', 'status': 'RUNNING' },"
               "  'profile': { 'key': 'fabric-1.21.1', 'label': 'Fabric 1.21.1', 'minecraft': '1.21.1',"
               "               'loader': 'fabric', 'modpack': null },"
               "  'minecraft': '1.21.1',"
               "  'loader': { 'id': 'fabric', 'build': '0.16.9' },"
               "  'unmanaged': [{ 'filename': 'dropped.jar', 'reason': 'unknown-origin' }],"
               "  'someFutureField': true,") +
           "\"modpack\": " + modpack + ", \"mods\": " + mods + "}";
}

QByteArray modJson(const QByteArray& filename, const QByteArray& sha512 = SHA)
{
    return json(
               "{ 'projectId': 'P1', 'versionId': 'V1', 'url': 'https://cdn.modrinth.test/x.jar', 'size': 10,"
               "  'client': 'required', 'filename': '") +
           filename + "\", \"sha512\": \"" + sha512 + "\" }";
}

const QByteArray MODPACK_JSON = json(
    "{ 'versionId': 'AbC12345', 'projectId': 'cabinPrj', 'name': 'CABIN 1.4', 'url': 'https://cdn.modrinth.test/c.mrpack', 'sha512': 'x', "
    "'size': 42 }");

const QByteArray SERVERS_JSON = json(
    "{ 'servers': ["
    "  { 'id': 's1', 'name': 'Fabric world', 'address': 'steve.kubik.gg', 'port': null, 'edition': 'java',"
    "    'loader': 'fabric', 'loaderName': 'Fabric', 'version': '1.21.1', 'status': 'SUSPENDED', 'plan': 'free',"
    "    'locked': false, 'lockReason': null, 'modpackName': null,"
    "    'profile': { 'key': 'fabric-1.21.1', 'label': 'Fabric 1.21.1', 'minecraft': '1.21.1', 'loader': 'fabric',"
    "                 'modpack': null },"
    "    'unplayable': null, 'modCount': 3, 'dashboardUrl': 'https://kubik.gg/dashboard/servers/s1' },"
    "  { 'id': 's2', 'name': 'Bedrock', 'address': 'b.kubik.gg', 'port': 19132, 'edition': 'bedrock',"
    "    'loader': 'bedrock', 'version': 'latest', 'status': 'RUNNING', 'profile': null, 'unplayable': 'bedrock' },"
    "  { 'name': 'no id, skipped' }"
    "]}");

const QByteArray REFUSAL_JSON = json("{ 'error': { 'code': 'unplayable', 'reason': 'modpack-unpinned' } }");

}  // namespace

class KubikModelsTest : public QObject {
    Q_OBJECT

   private slots:
    void parsesAPack()
    {
        const auto pack = parsePack(packJson("[" + modJson("create.jar") + "]"));
        QVERIFY(pack.has_value());
        QCOMPARE(pack->serverId, QString("s1"));
        QCOMPARE(pack->address, QString("steve.kubik.gg"));
        QCOMPARE(pack->profile.key, QString("fabric-1.21.1"));
        QCOMPARE(pack->loader, QString("fabric"));
        QCOMPARE(pack->loaderBuild, QString("0.16.9"));
        QVERIFY(!pack->modpack.has_value());
        QCOMPARE(pack->mods.size(), 1);
        QVERIFY(pack->mods.first().required);
        QCOMPARE(pack->unmanaged.first().reason, QString("unknown-origin"));
    }

    void refusesAPackWithAModItCannotVerifyOrPlaceSafely_data()
    {
        QTest::addColumn<QByteArray>("mod");
        QTest::newRow("no hash") << modJson("create.jar", "");
        QTest::newRow("short hash") << modJson("create.jar", "abc");
        QTest::newRow("a path, not a name") << modJson("../config/evil.jar");
        QTest::newRow("a subfolder") << modJson("sub/create.jar");
        QTest::newRow("not a jar") << modJson("create.zip");
        QTest::newRow("hidden file") << modJson(".create.jar");
    }

    void refusesAPackWithAModItCannotVerifyOrPlaceSafely()
    {
        QFETCH(QByteArray, mod);
        QVERIFY(!parsePack(packJson("[" + modJson("ok.jar") + "," + mod + "]")).has_value());
    }

    void readsAModpack()
    {
        const auto pack = parsePack(packJson("[]", MODPACK_JSON));
        QVERIFY(pack.has_value());
        QVERIFY(pack->modpack.has_value());
        QCOMPARE(pack->modpack->versionId, QString("AbC12345"));
        QCOMPARE(pack->modpack->projectId, QString("cabinPrj"));
        QCOMPARE(pack->modpack->size, qint64(42));
    }

    void parsesTheServerList()
    {
        const auto servers = parseServers(SERVERS_JSON);
        QVERIFY(servers.has_value());
        QCOMPARE(servers->size(), 2);
        QVERIFY(servers->at(0).isAsleep());
        QCOMPARE(servers->at(0).profile->label, QString("Fabric 1.21.1"));
        QCOMPARE(servers->at(0).modCount, 3);
        QVERIFY(!servers->at(1).profile.has_value());
        QCOMPARE(servers->at(1).unplayable, QString("bedrock"));
        QCOMPARE(servers->at(1).port, 19132);
    }

    void readsARefusalsCode()
    {
        const auto e = parseApiError(422, REFUSAL_JSON);
        QCOMPARE(e.code, QString("unplayable"));
        QCOMPARE(e.reason, QString("modpack-unpinned"));
        QCOMPARE(parseApiError(0, {}).code, QString("offline"));
        QCOMPARE(parseApiError(401, "<html>").code, QString("unauthorized"));
        QCOMPARE(parseApiError(502, "<html>").code, QString("unavailable"));
    }

    void pkceMatchesTheRfc()
    {
        // RFC 7636 Appendix B.
        QCOMPARE(pkceChallenge("dBjftJeZ4CVP-mB92K27uhbUJU1p1r_wW1gFWFOEjXk"), QString("E9Melhoa2OwvFrEMTJguCHaoeK1t8URWbuGJSstw-cM"));
        const auto v = makePkceVerifier();
        QCOMPARE(v.size(), 43);
        QVERIFY(v != makePkceVerifier());
    }

    void comparesBuildsByTheirNumbers()
    {
        QVERIFY(compareBuilds("0.16.10", "0.16.9") > 0);
        QVERIFY(compareBuilds("21.1.9", "21.1.77") < 0);
        QCOMPARE(compareBuilds("47.3.10", "47.3.10"), 0);
    }

    void namesEachLoadersComponent()
    {
        QCOMPARE(loaderComponentUid("fabric"), QString("net.fabricmc.fabric-loader"));
        QCOMPARE(loaderComponentUid("neoforge"), QString("net.neoforged"));
        QVERIFY(loaderComponentUid("").isEmpty());
    }

    void sortsHomeUpFirstThenRecentThenByName()
    {
        const auto server = [](const QString& id, const QString& name, const QString& status) {
            Server s;
            s.id = id;
            s.name = name;
            s.status = status;
            return s;
        };
        QList<Server> servers = {
            server("1", "Zeta", "SUSPENDED"), server("2", "beta", "RUNNING"),  server("3", "Alpha", "RUNNING"),
            server("4", "Gamma", "RUNNING"),  server("5", "Delta", "STOPPED"), server("6", "Eta", "SUSPENDED"),
        };
        // Gamma was played an hour ago, Zeta (asleep) a minute ago; the rest never.
        const QHash<QString, qint64> played = { { "4", 1000 }, { "1", 5000 } };
        sortForHome(servers, [&played](const QString& id) { return played.value(id, 0); });
        QStringList order;
        for (const auto& s : servers) {
            order << s.name;
        }
        QCOMPARE(order, QStringList({ "Gamma", "Alpha", "beta", "Zeta", "Delta", "Eta" }));
    }
};

QTEST_GUILESS_MAIN(KubikModelsTest)

#include "KubikModels_test.moc"
