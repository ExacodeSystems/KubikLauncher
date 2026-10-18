#!/bin/sh
# Compiles KubikLauncher's bundled translations (.ts -> .qm). Run after editing any .ts here, and
# commit the .qm files with it: they are built into the app through kubik.qrc, which is what lets
# TranslationsModel's bundled mode work offline and from the very first start.
#
#   prism_id.ts  Prism Launcher's Indonesian, from PrismLauncher/Translations (Apache-2.0, see
#                LICENSE-prism-translations.txt). Refresh it from upstream when merging Prism.
#   kubik_id.ts  KubikLauncher's own strings and the Prism strings it reworded, in Indonesian.
#   kubik_en.ts  English plural forms ("1 mod", "12 mods") for the same strings.
#
# Usage: build.sh [path to lrelease]   (default: lrelease on PATH)
set -e
cd "$(dirname "$0")"
LRELEASE="${1:-lrelease}"
for ts in prism_id kubik_id kubik_en; do
    "$LRELEASE" -silent "$ts.ts" -qm "$ts.qm"
done
