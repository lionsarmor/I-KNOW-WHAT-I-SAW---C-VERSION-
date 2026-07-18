#!/usr/bin/env bash
# =========================================================================
# Build fresh Windows + Linux binaries and upload them to Steam via SteamPipe.
#
#   tools/steam/upload.sh              build + upload (build lands OFF all branches)
#   tools/steam/upload.sh --preview    dry run: build, stage, validate, upload NOTHING
#   tools/steam/upload.sh --branch X   promote the uploaded build to branch X
#   tools/steam/upload.sh --no-build   skip `make dist`, use whatever is in dist/
#
# Config (App ID, depot IDs, login) lives in tools/steam/steam.conf.
# =========================================================================
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$HERE/../.." && pwd)"
CONF="${STEAM_CONF:-$HERE/steam.conf}"

# ---- args ---------------------------------------------------------------
PREVIEW=0
DO_BUILD=1
BRANCH_OVERRIDE=""
BRANCH_SET=0
while [ $# -gt 0 ]; do
	case "$1" in
		--preview|-p) PREVIEW=1 ;;
		--no-build)   DO_BUILD=0 ;;
		--branch)     BRANCH_OVERRIDE="${2:-}"; BRANCH_SET=1; shift ;;
		-h|--help)    grep '^#' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
		*) echo "unknown arg: $1" >&2; exit 2 ;;
	esac
	shift
done

# ---- config -------------------------------------------------------------
[ -f "$CONF" ] || { echo "missing $CONF" >&2; exit 1; }
# shellcheck source=/dev/null
. "$CONF"
[ "$BRANCH_SET" = 1 ] && BRANCH="$BRANCH_OVERRIDE"
BRANCH="${BRANCH:-}"

fail=0
for v in APPID DEPOT_CONTENT; do
	val="${!v:-0}"
	case "$val" in
		''|*[!0-9]*) echo "steam.conf: $v is not set to a numeric ID (got '${val}')" >&2; fail=1 ;;
		0)           echo "steam.conf: $v is still 0 -- fill it in from Steamworks" >&2; fail=1 ;;
	esac
done
[ "$fail" = 0 ] || exit 1
# STEAM_USER is only needed for an actual upload; a local --preview never logs
# in. It's enforced just before the steamcmd call below.

# ---- tools --------------------------------------------------------------
STEAMCMD="${STEAMCMD:-$(command -v steamcmd || true)}"
if [ -z "$STEAMCMD" ] && [ "$PREVIEW" = 0 ]; then
	cat >&2 <<-EOF
	steamcmd not found. Install it, or set STEAMCMD=/path/to/steamcmd.sh
	  Ubuntu/Debian:  sudo add-apt-repository multiverse && sudo apt install steamcmd
	  Or download:    https://developer.valvesoftware.com/wiki/SteamCMD
	(You can still run with --preview to build + validate the staging locally.)
	EOF
	exit 1
fi

VERSION="$(sed -n 's/^VERSION[[:space:]]*?\{0,1\}=[[:space:]]*//p' "$ROOT/Makefile" | head -1)"
VERSION="${VERSION:-0.0.0}"
GITSHA="$(git -C "$ROOT" rev-parse --short HEAD 2>/dev/null || echo nogit)"
DESC="I Know What I Saw demo v${VERSION} (${GITSHA})"

# ---- 1. build -----------------------------------------------------------
if [ "$DO_BUILD" = 1 ]; then
	echo ">> make dist"
	make -C "$ROOT" dist
fi
[ -f "$ROOT/dist/windows/iknowwhatisaw.exe" ] || { echo "dist/windows/iknowwhatisaw.exe missing -- run without --no-build" >&2; exit 1; }
[ -f "$ROOT/dist/linux/iknowwhatisaw" ]       || { echo "dist/linux/iknowwhatisaw missing -- run without --no-build" >&2; exit 1; }

# ---- 2. stage content ---------------------------------------------------
WORK="$ROOT/build/steam"
CONTENT_WINDOWS="$WORK/content/windows"
CONTENT_LINUX="$WORK/content/linux"
SCRIPTS="$WORK/scripts"
OUTPUT="$WORK/output"
rm -rf "$WORK"
mkdir -p "$CONTENT_WINDOWS" "$CONTENT_LINUX" "$SCRIPTS" "$OUTPUT"

cp "$ROOT/dist/windows/iknowwhatisaw.exe" "$ROOT/dist/windows/SDL2.dll" "$CONTENT_WINDOWS/"
cp -a "$ROOT/dist/linux/." "$CONTENT_LINUX/"   # -a preserves the +x bit

echo ">> staged content:"
echo "   windows: $(ls "$CONTENT_WINDOWS" | tr '\n' ' ')"
echo "   linux:   $(ls "$CONTENT_LINUX"   | tr '\n' ' ')"

# ---- 3. generate concrete VDFs (absolute paths -> no relative-path pain) --
subst() {
	sed -e "s#@APPID@#$APPID#g" \
	    -e "s#@DEPOT_CONTENT@#$DEPOT_CONTENT#g" \
	    -e "s#@DESC@#$DESC#g" \
	    -e "s#@BRANCH@#$BRANCH#g" \
	    -e "s#@PREVIEW@#$PREVIEW#g" \
	    -e "s#@BUILDOUTPUT@#$OUTPUT#g" \
	    -e "s#@CONTENTROOT@#$WORK/content#g" \
	    -e "s#@SCRIPTS@#$SCRIPTS#g" \
	    "$1"
}
subst "$HERE/app_build.vdf.tmpl"     > "$SCRIPTS/app_build.vdf"
subst "$HERE/depot_content.vdf.tmpl" > "$SCRIPTS/depot_content.vdf"

echo ">> generated $SCRIPTS/app_build.vdf  (appid $APPID, depot $DEPOT_CONTENT, branch '${BRANCH:-<none>}', preview $PREVIEW)"

# ---- 4. upload ----------------------------------------------------------
if [ "$PREVIEW" = 1 ] && [ -z "$STEAMCMD" ]; then
	echo ">> PREVIEW (no steamcmd): staging + VDFs generated, nothing uploaded."
	echo "   inspect: $SCRIPTS/app_build.vdf"
	exit 0
fi

[ -n "${STEAM_USER:-}" ] || { echo "steam.conf: STEAM_USER is empty -- set your Steam account username to upload" >&2; exit 1; }

echo ">> steamcmd run_app_build  (preview=$PREVIEW)"
"$STEAMCMD" +login "$STEAM_USER" +run_app_build "$SCRIPTS/app_build.vdf" +quit

echo ">> done. Check SteamPipe -> Builds:  https://partner.steamgames.com/apps/builds/$APPID"
[ -z "$BRANCH" ] && echo "   Build is uploaded but OFF all branches -- promote it there when ready."
