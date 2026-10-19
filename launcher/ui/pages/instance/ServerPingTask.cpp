#include <QFutureWatcher>

#include <Json.h>
#include "McClient.h"
#include "McResolver.h"
#include "Result.h"
#include "ServerPingTask.h"
#include "kubik/KubikGameText.h"

namespace {
unsigned getOnlinePlayers(const QJsonObject& data)
{
    auto res = Json::requireObject(data, "players").and_then([](const auto& v) { return Json::requireInteger(v, "online"); });
    if (!res) {
        qWarning() << "server ping failed to parse response" << res.error();
        return 0;
    }
    return res.value();
}
}  // namespace

void ServerPingTask::executeTask()
{
    qDebug() << "Querying status of" << QString("%1:%2").arg(m_domain).arg(m_port);

    // Resolve the actual IP and port for the server
    McResolver* resolver = new McResolver(nullptr, m_domain, m_port);
    connect(resolver, &McResolver::succeeded, this, [this](QString ip, int port) {
        qDebug().nospace().noquote() << "Resolved address for " << m_domain << ": " << ip << ":" << port;

        // Now that we have the IP and port, query the server
        McClient* client = new McClient(nullptr, m_domain, ip, port);

        connect(client, &McClient::succeeded, this, [this](QJsonObject data) {
            m_outputOnlinePlayers = getOnlinePlayers(data);
            m_outputMaxPlayers = data.value("players").toObject().value("max").toInt(-1);
            // "data:image/png;base64,…", the 64x64 server-icon.png a server may send.
            const QString favicon = data.value("favicon").toString();
            if (const auto comma = favicon.indexOf(','); favicon.startsWith("data:image/png;base64,") && comma > 0) {
                m_outputFavicon = QByteArray::fromBase64(favicon.mid(comma + 1).toLatin1());
            }
            m_outputMotd = Kubik::plainMotd(data.value("description"));
            qDebug() << "Online players:" << m_outputOnlinePlayers;
            emitSucceeded();
        });
        connect(client, &McClient::failed, this, [this](QString error) { emitFailed(error); });

        // Delete McClient object when done
        connect(client, &McClient::finished, this, [client]() { client->deleteLater(); });
        client->getStatusData();
    });
    connect(resolver, &McResolver::failed, this, [this](QString error) { emitFailed(error); });

    // Delete McResolver object when done
    connect(resolver, &McResolver::finished, resolver, &McResolver::deleteLater);
    resolver->ping();
}
