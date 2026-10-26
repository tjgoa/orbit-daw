#!/bin/bash
set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build/make"
COUNTER_FILE="$PROJECT_ROOT/DEV_BUILD_COUNTER.json"
CURRENT_APP="$PROJECT_ROOT/Builds/Current/Orbit.app"
STAGING_APP="$PROJECT_ROOT/Builds/Current/Orbit.next.app"

if [[ ! -f "$BUILD_DIR/Makefile" ]]; then
    echo "Build directory is missing. Configure it first with: cmake -S . -B build/make -G 'Unix Makefiles'" >&2
    exit 2
fi

cmake --build "$BUILD_DIR" --target Orbit -j 4

mkdir -p "$(dirname "$CURRENT_APP")"
bundle_digest() {
    python3 - "$1" <<'PY'
import hashlib
import pathlib
import sys

root = pathlib.Path(sys.argv[1])
digest = hashlib.sha256()
for path in sorted((p for p in root.rglob("*") if p.is_file()), key=lambda p: p.relative_to(root).as_posix()):
    relative = path.relative_to(root).as_posix()
    if relative.startswith("Contents/_CodeSignature/"):
        continue
    digest.update(relative.encode("utf-8"))
    digest.update(path.read_bytes())
print(digest.hexdigest())
PY
}

BUILT_APP="$BUILD_DIR/Orbit_artefacts/Debug/Orbit.app"
if [[ ! -d "$BUILT_APP" ]]; then
    echo "CMake completed without producing the Orbit app bundle: $BUILT_APP" >&2
    exit 3
fi

if [[ -d "$CURRENT_APP" ]] && [[ "$(bundle_digest "$BUILT_APP")" == "$(bundle_digest "$CURRENT_APP")" ]]; then
    echo "The compiled app is unchanged; leaving the current app, timestamp, archive set, and iteration count untouched."
    exit 0
fi

rm -rf "$STAGING_APP"
ditto "$BUILT_APP" "$STAGING_APP"
touch "$STAGING_APP"
if [[ -e "$CURRENT_APP" ]]; then
    rm -rf "$CURRENT_APP"
fi
mv "$STAGING_APP" "$CURRENT_APP"
touch "$CURRENT_APP"

python3 - "$COUNTER_FILE" <<'PY'
import datetime
import json
import pathlib
import sys

counter_path = pathlib.Path(sys.argv[1])
data = json.loads(counter_path.read_text())
data["successful_dev_iterations"] = int(data.get("successful_dev_iterations", 0)) + 1
data["last_successful_build_utc"] = datetime.datetime.now(datetime.timezone.utc).isoformat(timespec="seconds")
temporary_path = counter_path.with_suffix(counter_path.suffix + ".tmp")
temporary_path.write_text(json.dumps(data, indent=2) + "\n")
temporary_path.replace(counter_path)
print(f"Successful local iteration {data['successful_dev_iterations']} recorded; product remains Build {data['product_checkpoint_build']} / v{data['product_checkpoint_version']}.")
PY

python3 "$PROJECT_ROOT/Tools/package-source-archive.py" --dev-iteration
