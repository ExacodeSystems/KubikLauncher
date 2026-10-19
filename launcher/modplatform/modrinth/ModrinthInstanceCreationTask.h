#pragma once

#include <optional>

#include <QByteArray>
#include <QCryptographicHash>
#include <QQueue>
#include <QString>
#include <QUrl>
#include <QVector>

#include "BaseInstance.h"
#include "InstanceTask.h"

class Resource;

class ModrinthCreationTask final : public InstanceTask {
    Q_OBJECT
    struct File {
        QString path;

        QCryptographicHash::Algorithm hashAlgorithm;
        QByteArray hash;
        QQueue<QUrl> downloads;
        bool required = true;
    };

   public:
    ModrinthCreationTask(const QString& stagingPath,
                         bool trustedSource,
                         SettingsObject* globalSettings,
                         QWidget* parent,
                         QString id,
                         QString versionId = {},
                         QString originalInstanceId = {});
    ~ModrinthCreationTask() override;

    bool abort() override;

    void createInstance();
    void executeTask() override;

    /**
     * KubikLauncher: take the pack's optional files enabled, without asking. Used when a pack is
     * installed to join a kubik.gg server, where a mod the pack calls optional may be one the server
     * needs (docs/plans/phase-10-kubik-launcher.md §10.1 in kubik.gg).
     */
    void setOptionalFilesEnabled(bool enabled) { m_optionalFilesEnabled = enabled; }

   private slots:
    void finishInstall();

   private:
    bool parseManifest(const QString&, std::vector<File>&, bool setInternalData = true, bool showOptionalDialog = true);

    void ensureMetaLoop();
    void setManagedPack(BaseInstance* instance);

    [[nodiscard]] bool promptForUntrustedMods();

   private:
    QWidget* m_parent = nullptr;
    bool m_trustedSource;

    QString m_minecraftVersion, m_fabricVersion, m_quiltVersion, m_forgeVersion, m_neoForgeVersion;
    QString m_managedId, m_managedVersionId, m_managedName;

    std::vector<File> m_files;
    Task::Ptr m_task;

    std::optional<BaseInstance*> m_oldInstance;
    std::unique_ptr<MinecraftInstance> m_newInstance;

    QString m_rootPath = "minecraft";
    bool m_optionalFilesEnabled = false;

    QHash<QString, Resource*> m_resources;
};
