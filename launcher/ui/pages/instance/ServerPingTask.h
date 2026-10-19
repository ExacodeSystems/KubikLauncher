#pragma once

#include <QObject>
#include <QString>

#include <tasks/Task.h>

class ServerPingTask : public Task {
    Q_OBJECT
   public:
    explicit ServerPingTask(QString domain, int port) : Task(), m_domain(domain), m_port(port) {}
    ~ServerPingTask() override = default;
    int m_outputOnlinePlayers = -1;
    // KubikLauncher's server tiles: the player limit, and the server's icon (PNG bytes; empty if it has none).
    int m_outputMaxPlayers = -1;
    QByteArray m_outputFavicon;
    // And its message of the day, as plain text, for the server page's join pictures.
    QString m_outputMotd;

   private:
    QString m_domain;
    int m_port;

   protected:
    virtual void executeTask() override;
};
