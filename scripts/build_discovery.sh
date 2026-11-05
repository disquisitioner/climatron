#!/usr/bin/env bash
# Build from <sketch>/scripts, independent of the caller's working directory.
# Usage: ./scripts/build_discovery.sh --version 1.2.3 [--application-name Climatron]
#        [--build-root /path/to/builds] [--libraries /path/to/libraries]
set -euo pipefail
export LC_ALL=C
usage() {
    printf '%s\n' 'Usage: build_discovery.sh --version VERSION [--application-name NAME] [--build-root DIR] [--libraries DIR]'
}
die() { printf 'Error: %s\n' "$*" >&2; exit 1; }
VERSION=''
APPLICATION_NAME='Climatron'
BUILD_ROOT="${BUILD_ROOT:-${HOME:?HOME must be set}/arduinobuild}"
LIB_PATH="${LIB_PATH:-/run/media/eric/windu/Users/eric/Dropbox/make/arduino/libraries}"
while (($#)); do
    case "$1" in
        --version|--application-name|--build-root|--libraries)
            (($# >= 2)) || die "Missing value for $1"
            case "$1" in
                --version) VERSION=$2 ;;
                --application-name) APPLICATION_NAME=$2 ;;
                --build-root) BUILD_ROOT=$2 ;;
                --libraries) LIB_PATH=$2 ;;
            esac
            shift 2 ;;
        -h|--help) usage; exit 0 ;;
        *) usage >&2; die "Unknown argument: $1" ;;
    esac
done
[[ "$VERSION" =~ ^[0-9A-Za-z][0-9A-Za-z.+_-]{0,30}$ ]] || die 'Version must be 1-31 ASCII release-identifier characters.'
[[ "$APPLICATION_NAME" =~ ^[0-9A-Za-z][0-9A-Za-z\ .+_-]{0,30}$ ]] || die 'Application name must be 1-31 allowed ASCII characters.'
command -v arduino-cli >/dev/null || die 'arduino-cli is required.'
command -v python3 >/dev/null || die 'Python 3 is required.'
SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)
SKETCH_PATH=$(cd -- "$SCRIPT_DIR/.." && pwd -P)
SKETCH_NAME=${SKETCH_PATH##*/}
[[ -f "$SKETCH_PATH/$SKETCH_NAME.ino" ]] || die "No $SKETCH_NAME.ino found in $SKETCH_PATH"
[[ -d "$LIB_PATH" ]] || die "Library directory not found: $LIB_PATH (mount the drive or use --libraries)."
[[ -n "$BUILD_ROOT" ]] || die 'Build root cannot be empty.'
mkdir -p -- "$BUILD_ROOT"
BUILD_ROOT=$(cd -- "$BUILD_ROOT" && pwd -P)
BUILD_DIR="$BUILD_ROOT/$SKETCH_NAME"
mkdir -p -- "$BUILD_DIR"
BOARD='esp32:esp32:esp32:PartitionScheme=min_spiffs'
ELF_PATH="$BUILD_DIR/$SKETCH_NAME.ino.elf"
STAMP_SCRIPT="$SCRIPT_DIR/stamp_app_descriptor.py"
[[ -f "$STAMP_SCRIPT" ]] || die "Missing descriptor hook: $STAMP_SCRIPT"
printf 'Sketch: %s\nBuild:  %s\nApp:    %s %s\n' "$SKETCH_PATH" "$BUILD_DIR" "$APPLICATION_NAME" "$VERSION"
# Encode arguments to keep paths, spaces and quotes out of the recipe parser.
# The helper receives ELF, version and name through this inherited environment.
export CLIMATRON_DESCRIPTOR_CONFIG
CLIMATRON_DESCRIPTOR_CONFIG=$(python3 - "$STAMP_SCRIPT" "$ELF_PATH" "$VERSION" "$APPLICATION_NAME" <<'PY'
import base64, json, sys
print(base64.b64encode(json.dumps(sys.argv[1:]).encode()).decode())
PY
)
DESCRIPTOR_HOOK="recipe.hooks.linking.postlink.90.pattern=python3 -c \"import base64,json,os,runpy,sys; a=json.loads(base64.b64decode(os.environ['CLIMATRON_DESCRIPTOR_CONFIG'])); sys.argv=[a[0],'stamp']+a[1:]; runpy.run_path(a[0],run_name='__main__')\""
# Delete only the ELF so metadata-only changes force linking, retaining objects.
rm -f -- "$ELF_PATH"
printf '%s\n' '--- Starting Linux Compile with library discovery ---'
if arduino-cli compile --fqbn "$BOARD" \
    --jobs 0 \
    --libraries "$LIB_PATH" \
    --build-path "$BUILD_DIR" \
    --build-property "compiler.c.elf.extra_flags=-Wl,-Map,$BUILD_DIR/output.map" \
    --build-property "$DESCRIPTOR_HOOK" \
    -v "$SKETCH_PATH"; then
    python3 "$STAMP_SCRIPT" verify "$BUILD_DIR/$SKETCH_NAME.ino.bin" "$VERSION" "$APPLICATION_NAME"
else
    status=$?
    printf 'Compile failed (exit code %s). Check the library path, FQBN and output above.\n' "$status" >&2
    exit "$status"
fi
printf '%s\n' '--- Compile Success! ---'
# Compile only. For a separate upload, select the actual Linux device using
# `arduino-cli board list` (e.g. /dev/serial/by-id/..., /dev/ttyUSB0 or /dev/ttyACM0).
