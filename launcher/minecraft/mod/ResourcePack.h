#pragma once

#include <optional>

#include "minecraft/mod/DataPack.h"

#include <QImage>
#include <QMutex>
#include <QPixmap>
#include <QPixmapCache>

class Version;

/* TODO:
 *
 * Store localized descriptions
 * */

class ResourcePack : public DataPack {
   public:
    ResourcePack(QFileInfo file_info) : DataPack(file_info) {}

    /** Gets, respectively, the lower and upper versions supported by the set pack format. */
    QMap<std::pair<int, int>, std::pair<Version, Version>> mappings() const override;

    /**
     * The other direction (KubikLauncher writes its own pack's pack.mcmeta per instance): the pack format
     * the given Minecraft version reads, or nothing for a version this table does not know yet.
     */
    static std::optional<std::pair<int, int>> formatFor(const QString& minecraftVersion);
    /** The newest format in the table: the best guess for a version newer than all of it. */
    static std::pair<int, int> newestFormat();
};
