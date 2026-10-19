#include <QTest>

#include <kubik/KubikSyncPlan.h>

using namespace Kubik;

namespace {

QString sha(char c)
{
    return QString(128, QChar(c));
}

Mod mod(const QString& filename, char hash, const QString& project = {}, bool required = true)
{
    return Mod{ project.isEmpty() ? "p-" + filename : project,
                "v-" + filename,
                filename,
                "https://cdn.modrinth.test/" + filename,
                sha(hash),
                10,
                required };
}

Pack pack(const QString& serverId, QList<Mod> mods)
{
    Pack p;
    p.serverId = serverId;
    p.revision = "rev-" + serverId;
    p.profile.key = "fabric-1.21.1";
    p.minecraft = "1.21.1";
    p.loader = "fabric";
    p.mods = std::move(mods);
    return p;
}

/** The folder after a plan is applied, the way KubikSyncTask applies it. */
QList<LocalFile> apply(QList<LocalFile> local, const SyncPlan& plan)
{
    auto find = [&local](const QString& name) {
        return std::find_if(local.begin(), local.end(), [&](const LocalFile& f) { return f.filename == name; });
    };
    for (const auto& name : plan.remove) {
        if (auto it = find(name); it != local.end()) {
            local.erase(it);
        }
    }
    for (const auto& name : plan.enable) {
        find(name)->enabled = true;
    }
    for (const auto& name : plan.disable) {
        find(name)->enabled = false;
    }
    for (const auto& m : plan.place) {
        local.append({ m.filename, true, m.projectId, m.sha512 });
    }
    return local;
}

QStringList enabledNames(const QList<LocalFile>& local)
{
    QStringList out;
    for (const auto& f : local) {
        if (f.enabled) {
            out.append(f.filename);
        }
    }
    out.sort();
    return out;
}

}  // namespace

class KubikSyncPlanTest : public QObject {
    Q_OBJECT

   private slots:
    void firstSyncDownloadsAndPlacesEverything()
    {
        const auto plan = planSync({}, pack("a", { mod("create.jar", 'a'), mod("jei.jar", 'b') }), {}, {});
        QCOMPARE(plan.download.size(), 2);
        QCOMPARE(plan.place.size(), 2);
        QVERIFY(plan.remove.isEmpty());
        QCOMPARE(plan.next.serverId, QString("a"));
        QCOMPARE(plan.next.placed.size(), 2);
    }

    void aSecondSyncOfTheSameServerChangesNothing()
    {
        const auto p = pack("a", { mod("create.jar", 'a') });
        auto first = planSync({}, p, {}, {});
        const auto folder = apply({}, first);
        const auto again = planSync(first.next, p, folder, { sha('a') });
        QVERIFY(!again.changesFolder());
        QVERIFY(again.download.isEmpty());
    }

    void switchingServersSwapsOnlyTheServerLayer()
    {
        // Two Fabric servers sharing one instance: A runs Create + JEI, B runs JEI + Waystones.
        const auto a = pack("a", { mod("create.jar", 'a'), mod("jei.jar", 'b') });
        const auto b = pack("b", { mod("jei.jar", 'b'), mod("waystones.jar", 'c') });
        const QList<LocalFile> player{ { "minimap.jar", true, "p-minimap", {} } };

        auto toA = planSync({}, a, player, {});
        auto folder = apply(player, toA);
        QCOMPARE(enabledNames(folder), (QStringList{ "create.jar", "jei.jar", "minimap.jar" }));

        auto toB = planSync(toA.next, b, folder, { sha('a'), sha('b') });
        QCOMPARE(toB.remove, QStringList{ "create.jar" });
        QCOMPARE(toB.place.size(), 1);
        QCOMPARE(toB.place.first().filename, QString("waystones.jar"));
        folder = apply(folder, toB);
        QCOMPARE(enabledNames(folder), (QStringList{ "jei.jar", "minimap.jar", "waystones.jar" }));

        // And back to A: Create comes from the cache, nothing is downloaded twice.
        auto backToA = planSync(toB.next, a, folder, { sha('a'), sha('b'), sha('c') });
        QVERIFY(backToA.download.isEmpty());
        folder = apply(folder, backToA);
        QCOMPARE(enabledNames(folder), (QStringList{ "create.jar", "jei.jar", "minimap.jar" }));
    }

    void anotherVersionOfTheSameModIsSwapped()
    {
        const auto a = pack("a", { mod("create-0.5.jar", 'a', "create") });
        const auto b = pack("b", { mod("create-0.6.jar", 'b', "create") });
        auto toA = planSync({}, a, {}, {});
        auto folder = apply({}, toA);
        auto toB = planSync(toA.next, b, folder, { sha('a') });
        QCOMPARE(toB.remove, QStringList{ "create-0.5.jar" });
        QCOMPARE(toB.place.first().filename, QString("create-0.6.jar"));
        QCOMPARE(enabledNames(apply(folder, toB)), QStringList{ "create-0.6.jar" });
    }

    void theSameFileNameWithNewBytesIsReplaced()
    {
        auto first = planSync({}, pack("a", { mod("x.jar", 'a') }), {}, {});
        auto folder = apply({}, first);
        auto second = planSync(first.next, pack("a", { mod("x.jar", 'b') }), folder, { sha('a') });
        QCOMPARE(second.remove, QStringList{ "x.jar" });
        QCOMPARE(second.place.size(), 1);
        QCOMPARE(second.download.size(), 1);
    }

    void thePlayersOwnJarsAreNeverRemoved()
    {
        const QList<LocalFile> player{ { "sodium.jar", true, "sodium", {} }, { "shaders-helper.jar", false, {}, {} } };
        const auto plan = planSync({}, pack("a", { mod("create.jar", 'a') }), player, {});
        QVERIFY(plan.remove.isEmpty());
        QVERIFY(plan.disable.isEmpty());
        QCOMPARE(plan.next.placed.size(), 1);
        QCOMPARE(plan.next.placed.first().filename, QString("create.jar"));
    }

    void aConflictingPlayerJarIsDisabledThenRestored()
    {
        // The player installed Sodium 0.5 themselves; server A needs Sodium 0.6; server B needs neither.
        const QList<LocalFile> player{ { "sodium-0.5.jar", true, "sodium", {} } };
        const auto a = pack("a", { mod("sodium-0.6.jar", 'a', "sodium") });
        const auto b = pack("b", { mod("jei.jar", 'b') });

        auto toA = planSync({}, a, player, {});
        QCOMPARE(toA.disable, QStringList{ "sodium-0.5.jar" });
        QCOMPARE(toA.next.displaced, QStringList{ "sodium-0.5.jar" });
        auto folder = apply(player, toA);
        QCOMPARE(enabledNames(folder), QStringList{ "sodium-0.6.jar" });

        auto toB = planSync(toA.next, b, folder, { sha('a') });
        QCOMPARE(toB.enable, QStringList{ "sodium-0.5.jar" });
        QCOMPARE(toB.remove, QStringList{ "sodium-0.6.jar" });
        QVERIFY(toB.next.displaced.isEmpty());
        QCOMPARE(enabledNames(apply(folder, toB)), (QStringList{ "jei.jar", "sodium-0.5.jar" }));
    }

    void aPlayerJarThatIsTheSameFileIsLeftAsTheirs()
    {
        const QList<LocalFile> player{ { "jei.jar", true, "p-jei.jar", sha('b') } };
        const auto plan = planSync({}, pack("a", { mod("jei.jar", 'b') }), player, {});
        QVERIFY(!plan.changesFolder());
        QVERIFY(plan.next.placed.isEmpty());
    }

    void aPlayerJarWithAServerModsNameButOtherBytesMakesWay()
    {
        const QList<LocalFile> player{ { "jei.jar", true, {}, sha('z') } };
        const auto plan = planSync({}, pack("a", { mod("jei.jar", 'b') }), player, {});
        QCOMPARE(plan.disable, QStringList{ "jei.jar" });
        QCOMPARE(plan.place.size(), 1);
    }

    void aRequiredModThePlayerSwitchedOffGoesBackOn()
    {
        auto first = planSync({}, pack("a", { mod("create.jar", 'a', {}, true), mod("jei.jar", 'b', {}, false) }), {}, {});
        auto folder = apply({}, first);
        for (auto& f : folder) {
            f.enabled = false;  // the player disabled both on the Mods page
        }
        const auto again = planSync(first.next, pack("a", first.place), folder, { sha('a'), sha('b') });
        // Create is required to join; JEI is optional, so the player's choice stands.
        QCOMPARE(again.enable, QStringList{ "create.jar" });
        QVERIFY(again.place.isEmpty());
    }

    void aJarSyncPlacedButThePlayerDeletedIsPlacedAgain()
    {
        auto first = planSync({}, pack("a", { mod("create.jar", 'a') }), {}, {});
        const auto again = planSync(first.next, pack("a", { mod("create.jar", 'a') }), {}, { sha('a') });
        QCOMPARE(again.place.size(), 1);
        QVERIFY(again.download.isEmpty());
        QVERIFY(again.remove.isEmpty());
    }

    void theSidecarRoundTrips()
    {
        ManagedState state{ "srv", "rev", { { "create.jar", sha('a') } }, { "sodium.jar" } };
        const auto back = readManagedState(writeManagedState(state));
        QVERIFY(back.has_value());
        QCOMPARE(back->serverId, state.serverId);
        QCOMPARE(back->placed.size(), 1);
        QCOMPARE(back->placed.first().sha512, sha('a'));
        QCOMPARE(back->displaced, state.displaced);
        QVERIFY(!readManagedState("not json").has_value());
        QVERIFY(!readManagedState(R"({"formatVersion": 2})").has_value());
    }

    void tellsServerModsFromThePlayersInEitherForm()
    {
        ManagedState state;
        state.placed = { { "fabric-api.jar", sha('a') }, { "sodium.jar", sha('b') } };
        QVERIFY(isManagedFile(state, "fabric-api.jar"));
        QVERIFY(isManagedFile(state, "sodium.jar.disabled"));
        QVERIFY(!isManagedFile(state, "my-minimap.jar"));
        QVERIFY(!isManagedFile(state, "fabric-api.jar.bak"));
        QVERIFY(!isManagedFile(ManagedState{}, "fabric-api.jar"));
    }
};

QTEST_GUILESS_MAIN(KubikSyncPlanTest)

#include "KubikSyncPlan_test.moc"
