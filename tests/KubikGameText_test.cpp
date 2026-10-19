#include <QJsonDocument>
#include <QJsonObject>
#include <QTest>

#include <kubik/KubikGameText.h>

using namespace Kubik;

namespace {

QJsonValue component(const char* json)
{
    return QJsonDocument::fromJson(json).object().value("description");
}

}  // namespace

class KubikGameTextTest : public QObject {
    Q_OBJECT

   private slots:
    void readsTheLanguageOptionsTxtSets()
    {
        QCOMPARE(gameLanguage("version:4440\nlang:id_id\nfov:0.0\n"), QString("id_id"));
        // Older versions wrote the region in capitals.
        QCOMPARE(gameLanguage("lang:id_ID\n"), QString("id_id"));
    }

    void fallsBackToMinecraftsDefault()
    {
        QCOMPARE(gameLanguage(""), QString("en_us"));
        QCOMPARE(gameLanguage("resourcePacks:[\"vanilla\"]\n"), QString("en_us"));
        QCOMPARE(gameLanguage("lang:\n"), QString("en_us"));
    }

    void namesTheButtonsInTheGamesLanguage()
    {
        QCOMPARE(gameLabels("id_id").multiplayer, QString("Bermain Bersama"));
        QCOMPARE(gameLabels("id_id").joinServer, QString("Gabung"));
        QCOMPARE(gameLabels("en_us").joinServer, QString("Join Server"));
        // A language this table does not carry gets English, never an empty button.
        QCOMPARE(gameLabels("ja_jp").serverAddress, QString("Server Address"));
    }

    void readsAPlainStringMotd()
    {
        QCOMPARE(plainMotd(component(R"({"description":"A Minecraft Server"})")), QString("A Minecraft Server"));
    }

    void joinsATextComponentsParts()
    {
        const auto motd =
            plainMotd(component(R"({"description":{"text":"Alpha ","extra":[{"text":"SMP","bold":true},"\n",{"text":"Survival"}]}})"));
        QCOMPARE(motd, QString("Alpha SMP\nSurvival"));
    }

    void dropsFormattingCodes() { QCOMPARE(plainMotd(component(R"({"description":"§aGreen §lbold"})")), QString("Green bold")); }

    void answersNothingForNoDescription() { QCOMPARE(plainMotd(QJsonValue()), QString()); }
};

QTEST_GUILESS_MAIN(KubikGameTextTest)

#include "KubikGameText_test.moc"
