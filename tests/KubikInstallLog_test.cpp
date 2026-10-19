#include <QTest>

#include <kubik/KubikPlay.h>

using namespace Kubik;

class KubikInstallLogTest : public QObject {
    Q_OBJECT

   private slots:
    void stampsEachLineWithItsTime()
    {
        InstallLog log;
        QCOMPARE(log.add("Installing Fabric 26.2…", QTime(12, 3, 4)).value_or(QString()), QString("[12:03:04] Installing Fabric 26.2…"));
    }

    void keepsTheStepNotItsCounter()
    {
        // Prism's download task reports progress as status lines; a new game's assets are thousands of files.
        InstallLog log;
        const QTime t(9, 0, 0);
        QVERIFY(log.add("Executing 4 task(s) (5 out of 3000 are done)", t).has_value());
        QVERIFY(!log.add("Executing 4 task(s) (6 out of 3000 are done)", t).has_value());
        QVERIFY(!log.add("Executing 3 task(s) (2999 out of 3000 are done)", t).has_value());
        QVERIFY(log.add("Getting Fabric 26.2 ready", t).has_value());
        // The same step again after another one is news, and is kept.
        QVERIFY(log.add("Executing 1 task(s) (0 out of 2 are done)", t).has_value());
    }

    void foldsHashLikeRunsLikeNumbers()
    {
        // A download names each file by its checksum: one step, thousands of names.
        InstallLog log;
        const QTime t(9, 0, 0);
        QVERIFY(log.add("Verifying asset 96a84af9fad5619657dd5ea6995888f1", t).has_value());
        QVERIFY(!log.add("Verifying asset 1934a01d11c9f49124ccc57772805011", t).has_value());
        QVERIFY(log.add("Verifying library fabric-loader", t).has_value());
    }

    void dropsEmptyLinesAndStartsAgainAfterClear()
    {
        InstallLog log;
        const QTime t(9, 0, 0);
        QVERIFY(!log.add("   ", t).has_value());
        QVERIFY(log.add("Checking Alpha SMP…", t).has_value());
        QVERIFY(!log.add("Checking   Alpha SMP…", t).has_value());
        log.clear();
        QVERIFY(log.add("Checking Alpha SMP…", t).has_value());
    }
};

QTEST_GUILESS_MAIN(KubikInstallLogTest)

#include "KubikInstallLog_test.moc"
