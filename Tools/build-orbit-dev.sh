#!/bin/bash
set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build/make"
COUNTER_FILE="$PROJECT_ROOT/DEV_BUILD_COUNTER.json"
HISTORY_FILE="$PROJECT_ROOT/BUILD_HISTORY.md"
CURRENT_APP="$PROJECT_ROOT/Builds/Current/Orbit.app"
STAGING_APP="$PROJECT_ROOT/Builds/Current/Orbit.next.app"
BUILD_SUMMARY="${1:-Development rebuild; see ORION_LEARN_TODO.md and the source archive.}"
BUILD_JOBS="${ORBIT_BUILD_JOBS:-2}"

# Reserve the next product build before compiling. A failed build consumes its
# number, so retries always receive a fresh product label.
RESERVED_BUILD="$(python3 - "$PROJECT_ROOT" "$BUILD_SUMMARY" <<'PY'
import datetime
import json
import pathlib
import re
import sys

root = pathlib.Path(sys.argv[1])
summary = sys.argv[2].replace("|", "/").replace("\n", " ").strip()
counter_path = root / "DEV_BUILD_COUNTER.json"
counter = json.loads(counter_path.read_text())
build = max(int(counter.get("product_checkpoint_build", 0)),
            int(counter.get("last_reserved_product_build", 0))) + 1
cmake_path = root / "CMakeLists.txt"
cmake = cmake_path.read_text()
current_version = re.search(r"project\(OrbitDAW VERSION ([0-9.]+) LANGUAGES C CXX\)", cmake)
if current_version is None:
    raise SystemExit("Could not read the current Orbit product version")
parts = [int(piece) for piece in current_version.group(1).split(".")]
if len(parts) != 3:
    raise SystemExit("Expected a three-part product version in CMakeLists.txt")
parts[2] += 1
version = ".".join(map(str, parts))

cmake, n1 = re.subn(r"project\(OrbitDAW VERSION [0-9.]+ LANGUAGES C CXX\)",
                    f"project(OrbitDAW VERSION {version} LANGUAGES C CXX)", cmake, count=1)
cmake, n2 = re.subn(r'BUILD_VERSION "[0-9]+"', f'BUILD_VERSION "{build}"', cmake, count=1)
cmake, n3 = re.subn(r'MACOSX_BUNDLE_BUNDLE_VERSION "[0-9]+"',
                    f'MACOSX_BUNDLE_BUNDLE_VERSION "{build}"', cmake, count=1)
if (n1, n2, n3) != (1, 1, 1):
    raise SystemExit("Could not align CMake product version/build metadata")
cmake_path.write_text(cmake)

main_path = root / "Source" / "Main.cpp"
main = main_path.read_text()
main, n1 = re.subn(r'buildLabel\.setText\("Build [0-9]+  -  v[0-9.]+"',
                   f'buildLabel.setText("Build {build}  -  v{version}"', main, count=1)
main, n2 = re.subn(r'getApplicationVersion\(\) override \{ return "[0-9.]+"; \}',
                   f'getApplicationVersion() override {{ return "{version}"; }}', main, count=1)
if (n1, n2) != (1, 1):
    raise SystemExit("Could not align app build label/version")
main_path.write_text(main)

readme_path = root / "README.md"
readme = readme_path.read_text()
readme = re.sub(r"The current source checkpoint is Build [0-9]+ / v[0-9.]+",
                f"The current source checkpoint is Build {build} / v{version}", readme, count=1)
readme_path.write_text(readme)

history_path = root / "BUILD_HISTORY.md"
history = history_path.read_text()
today = datetime.date.today().isoformat()
row = f"| {build} | {version} | {today} | {summary} | Build reserved; compilation in progress. |\n"
history += row
history_path.write_text(history)
counter["last_reserved_product_build"] = build
counter_path.write_text(json.dumps(counter, indent=2) + "\n")
print(build)
PY
)"

BUILD_RESERVED=1
finish_build() {
    local result=$?
    if [[ "$BUILD_RESERVED" == "1" ]]; then
        if [[ "$result" -ne 0 ]]; then
            python3 - "$HISTORY_FILE" "$RESERVED_BUILD" <<'PY'
import pathlib
import sys

path = pathlib.Path(sys.argv[1])
build = int(sys.argv[2])
lines = path.read_text().splitlines()
for index, line in enumerate(lines):
    if line.startswith(f"| {build} |") and "Build reserved; compilation in progress." in line:
        lines[index] = line.replace("Build reserved; compilation in progress.",
                                    "Build failed; product build number consumed.")
        break
path.write_text("\n".join(lines) + "\n")
PY
        fi
    fi
    return "$result"
}
trap finish_build EXIT

if [[ ! -f "$BUILD_DIR/Makefile" ]]; then
    echo "Build directory is missing. Configure it first with: cmake -S . -B build/make -G 'Unix Makefiles'" >&2
    exit 2
fi

cmake --build "$BUILD_DIR" --target Orbit -j "$BUILD_JOBS"

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
mv "$BUILT_APP" "$STAGING_APP"
touch "$STAGING_APP"
if [[ -e "$CURRENT_APP" ]]; then
    rm -rf "$CURRENT_APP"
fi
mv "$STAGING_APP" "$CURRENT_APP"
touch "$CURRENT_APP"

python3 - "$COUNTER_FILE" "$RESERVED_BUILD" "$(sed -n 's/^project(OrbitDAW VERSION \([0-9.]*\) LANGUAGES.*/\1/p' "$PROJECT_ROOT/CMakeLists.txt")" <<'PY'
import datetime
import json
import pathlib
import sys

counter_path = pathlib.Path(sys.argv[1])
data = json.loads(counter_path.read_text())
data["successful_dev_iterations"] = int(data.get("successful_dev_iterations", 0)) + 1
data["product_checkpoint_build"] = int(sys.argv[2])
data["product_checkpoint_version"] = sys.argv[3]
data["last_reserved_product_build"] = int(sys.argv[2])
data["last_successful_build_utc"] = datetime.datetime.now(datetime.timezone.utc).isoformat(timespec="seconds")
data["artifact_path"] = "Builds/Current/Orbit.app"
temporary_path = counter_path.with_suffix(counter_path.suffix + ".tmp")
temporary_path.write_text(json.dumps(data, indent=2) + "\n")
temporary_path.replace(counter_path)
print(f"Successful development iteration {data['successful_dev_iterations']} recorded as product Build {data['product_checkpoint_build']} / v{data['product_checkpoint_version']}.")
PY

python3 - "$HISTORY_FILE" "$RESERVED_BUILD" <<'PY'
import pathlib
import sys

path = pathlib.Path(sys.argv[1])
build = int(sys.argv[2])
history = path.read_text()
lines = history.splitlines()
for index, line in enumerate(lines):
    if line.startswith(f"| {build} |") and "Build reserved; compilation in progress." in line:
        lines[index] = line.replace("Build reserved; compilation in progress.", "Built successfully.")
        break
path.write_text("\n".join(lines) + "\n")
PY

BUILD_RESERVED=0
trap - EXIT
python3 - "$PROJECT_ROOT/README.md" "$COUNTER_FILE" <<'PY'
import json
import pathlib
import re
import sys
readme_path = pathlib.Path(sys.argv[1])
counter = json.loads(pathlib.Path(sys.argv[2]).read_text())
readme = readme_path.read_text()
readme = re.sub(r"local development iteration \*\*[0-9]+\*\*",
                f"local development iteration **{counter['successful_dev_iterations']}**", readme, count=1)
readme_path.write_text(readme)
PY

python3 "$PROJECT_ROOT/Tools/package-source-archive.py" --dev-iteration
