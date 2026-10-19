#!/usr/bin/env bash
# Runs the Debug build of KubikLauncher against your local kubik.gg (pnpm dev), from anywhere.
#
#   scripts/run-local.sh                 local api + dashboard, offline play allowed, ~/KubikLauncher-test2
#   scripts/run-local.sh --fresh         a new empty data dir, so the first-run wizard shows again
#   scripts/run-local.sh --prod          kubik.gg itself instead of your local stack
#   scripts/run-local.sh --no-offline    what players see until Mojang approves the sign-in
#   scripts/run-local.sh -- <args>       anything after -- goes to the launcher
#
# It builds first when anything under launcher/ is newer than the app (a build with nothing to do
# still takes ~15s, in macdeployqt), and warns when the local api is not answering.
set -euo pipefail

usage() {
    cat <<'EOF'
Usage: scripts/run-local.sh [options] [-- launcher args]

  --prod            Use kubik.gg (api.kubik.gg) instead of your local stack.
  --no-offline      Without KUBIKLAUNCHER_DEV_OFFLINE: an offline profile then needs a
                    Microsoft account, as for players.
  --fresh           Start in a new, empty data dir (printed), to see the first run again.
  -d, --data DIR    Data dir to use (default: $KUBIK_DATA, else ~/KubikLauncher-test2).
  --build           Build before running, even if nothing changed.
  --no-build        Never build; run whatever is in build/.
  -h, --help        Show this.

Environment:
  KUBIK_GG_DIR      The kubik.gg checkout whose .env gives WEB_PORT and API_PORT
                    (default: ~/Documents/Projects/examc/project-saas-minecraft-server).
  KUBIK_DATA        Default data dir.
EOF
}

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
kubik_gg="${KUBIK_GG_DIR:-$HOME/Documents/Projects/examc/project-saas-minecraft-server}"
data="${KUBIK_DATA:-$HOME/KubikLauncher-test2}"
prod=0
offline=1
build=auto
launcher_args=()

while [[ $# -gt 0 ]]; do
    case "$1" in
        --prod) prod=1 ;;
        --no-offline) offline=0 ;;
        --fresh) data="$(mktemp -d -t kubiklauncher)" ;;
        -d | --data)
            [[ $# -ge 2 ]] || { echo "error: $1 needs a directory" >&2; exit 2; }
            data="$2"
            shift
            ;;
        --build) build=always ;;
        --no-build) build=never ;;
        -h | --help) usage; exit 0 ;;
        --) shift; launcher_args=("$@"); break ;;
        *) echo "error: unknown option $1 (see --help)" >&2; exit 2 ;;
    esac
    shift
done

if [[ "$(uname)" != Darwin ]]; then
    echo "error: this script runs the macOS app bundle; on another OS run build/ yourself" >&2
    exit 1
fi

app="$root/build/Debug/kubiklauncher.app/Contents/MacOS/kubiklauncher"

# Build when the app is missing or older than its sources.
if [[ $build == auto ]]; then
    if [[ ! -x "$app" ]] || [[ -n "$(find "$root/launcher" "$root/libraries" "$root/buildconfig" "$root/CMakeLists.txt" \
        -newer "$app" -type f -print -quit 2>/dev/null)" ]]; then
        build=always
    fi
fi
if [[ $build == always ]]; then
    if [[ ! -f "$root/build/CMakeCache.txt" ]]; then
        echo "error: build/ is not configured yet; configure it with CMake first" >&2
        exit 1
    fi
    log="$root/build/run-local-build.log"
    echo "Building (Debug)… log: $log"
    # The exit status decides, not a grep of the output: a filtered log once hid a FAILED step.
    if ! cmake --build "$root/build" --config Debug >"$log" 2>&1; then
        echo "error: the build failed. The end of the log:" >&2
        tail -n 40 "$log" >&2
        exit 1
    fi
fi
if [[ ! -x "$app" ]]; then
    echo "error: no app at $app (try without --no-build)" >&2
    exit 1
fi

# The local origins come from kubik.gg's own .env, where WEB_PORT is the one knob for the dashboard.
port_from_env() {
    local key="$1" fallback="$2" value=""
    if [[ -f "$kubik_gg/.env" ]]; then
        value="$(grep -E "^${key}=" "$kubik_gg/.env" | tail -n 1 | cut -d= -f2- | tr -d "\"' \r")"
    fi
    echo "${value:-$fallback}"
}

env_args=(-u KUBIKLAUNCHER_API_URL -u KUBIKLAUNCHER_WEB_URL -u KUBIKLAUNCHER_DEV_OFFLINE)
if [[ $prod == 1 ]]; then
    where="kubik.gg"
else
    api="http://localhost:$(port_from_env API_PORT 8787)"
    web="http://localhost:$(port_from_env WEB_PORT 3000)"
    env_args+=("KUBIKLAUNCHER_API_URL=$api" "KUBIKLAUNCHER_WEB_URL=$web")
    where="$api (sign-in at $web)"
    if ! curl -fsS --max-time 2 "$api/healthz" >/dev/null 2>&1; then
        echo "warning: kubik.gg's api is not answering at $api. Home will show an error until it is:"
        echo "         cd \"$kubik_gg\" && pnpm dev"
    fi
fi
if [[ $offline == 1 ]]; then
    env_args+=(KUBIKLAUNCHER_DEV_OFFLINE=1)
fi

mkdir -p "$data"
echo "KubikLauncher → $where"
echo "  data:    $data"
echo "  offline: $([[ $offline == 1 ]] && echo "allowed (KUBIKLAUNCHER_DEV_OFFLINE=1)" || echo "needs a Microsoft account")"
exec env "${env_args[@]}" "$app" -d "$data" ${launcher_args[@]+"${launcher_args[@]}"}
