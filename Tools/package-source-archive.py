#!/usr/bin/env python3
"""Create source-only Orbit checkpoint or incremental development archives."""

from __future__ import annotations

import argparse
import hashlib
import json
import pathlib
import re
import zipfile
from datetime import datetime, timezone

ROOT = pathlib.Path(__file__).resolve().parents[1]
ARCHIVE_DIR = ROOT / "Builds" / "Source Archives"
STATE_FILE = ARCHIVE_DIR / ".source-archive-state.json"


def eligible_files() -> list[pathlib.Path]:
    files: set[pathlib.Path] = set()
    for name in (
        "CMakeLists.txt",
        "README.md",
        "PROGRESS.md",
        "LICENSE",
        "DEPENDENCIES.md",
        ".gitmodules",
        "BUILD_HISTORY.md",
        "ORBIT_DAW_FOUNDATION_PLAN_001.md",
        "ORION_LEARN_TODO.md",
        "ORION_NATIVE_EFFECTS_COMPATIBILITY.md",
        "ORION_VISUAL_TARGETS.md",
        "DEV_BUILD_COUNTER.json",
    ):
        path = ROOT / name
        if path.is_file():
            files.add(path)
    for folder in ("Source", "Tools"):
        for path in (ROOT / folder).rglob("*"):
            if not path.is_file() or path.name == "__pycache__":
                continue
            if path.suffix in {".cpp", ".h", ".hpp", ".mm", ".m", ".py", ".sh", ".md", ".json"}:
                files.add(path)
    return sorted(files, key=lambda p: p.relative_to(ROOT).as_posix().lower())


def digest(path: pathlib.Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def snapshot() -> dict[str, str]:
    return {path.relative_to(ROOT).as_posix(): digest(path) for path in eligible_files()}


def safe_name(value: str) -> str:
    return re.sub(r"[^A-Za-z0-9_.-]+", "_", value)


def write_archive(name: str, files: list[pathlib.Path], summary: str, baseline: str) -> pathlib.Path:
    ARCHIVE_DIR.mkdir(parents=True, exist_ok=True)
    target = ARCHIVE_DIR / name
    if target.exists():
        raise SystemExit(f"Refusing to overwrite source archive: {target}")
    with zipfile.ZipFile(target, "x", compression=zipfile.ZIP_DEFLATED, compresslevel=8) as archive:
        for path in files:
            archive.write(path, path.relative_to(ROOT).as_posix())
        archive.writestr("CHANGE_SUMMARY.md", summary.rstrip() + "\n")
        archive.writestr("BASELINE.txt", baseline.rstrip() + "\n")
    return target


def main() -> None:
    parser = argparse.ArgumentParser()
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--checkpoint", action="store_true", help="Save a complete source checkpoint and reset the incremental baseline")
    group.add_argument("--dev-iteration", action="store_true", help="Save only changes since the last source checkpoint/iteration")
    parser.add_argument("--label", help="Optional checkpoint label, e.g. Build_025")
    args = parser.parse_args()

    files = eligible_files()
    current = snapshot()
    counter = json.loads((ROOT / "DEV_BUILD_COUNTER.json").read_text())
    build = int(counter["product_checkpoint_build"])
    version = counter["product_checkpoint_version"]
    if args.checkpoint:
        label = safe_name(args.label or f"Build_{build:03d}")
        name = f"Orbit_{label}_Source.zip"
        selected = files
        summary = (
            f"# Orbit {label} source checkpoint\n\n"
            f"Complete Orbit-owned source/configuration checkpoint for product Build {build}, version {version}. "
            "This archive contains no compiled app, executable, build output, cache, or vendored JUCE/Tracktion dependency source.\n\n"
            "The project uses CMake with JUCE and Tracktion Engine as pinned external dependencies; see DEPENDENCIES.md and README.md.\n"
        )
        baseline = f"Complete source checkpoint. Product Build {build}, version {version}. Created {datetime.now(timezone.utc).isoformat(timespec='seconds')}."
    else:
        state = json.loads(STATE_FILE.read_text()) if STATE_FILE.exists() else {"files": {}, "archive": "(no baseline)"}
        previous = state.get("files", {})
        changed_names = [path for path, sha in current.items() if previous.get(path) != sha]
        removed_names = sorted(set(previous) - set(current))
        if not changed_names and not removed_names:
            print("No source changes since the last archive; no ZIP created.")
            return
        iteration = int(counter.get("successful_dev_iterations", 0))
        name = f"Orbit_Dev_Iteration_{iteration:03d}_Source.zip"
        by_name = {path.relative_to(ROOT).as_posix(): path for path in files}
        selected = [by_name[item] for item in changed_names]
        summary_lines = [
            f"# Orbit development iteration {iteration:03d}",
            "",
            f"Incremental source archive for product Build {build}, version {version}.",
            f"Baseline archive: {state.get('archive', '(no baseline)')}.",
            "",
            "Changed or added files:",
            *(f"- `{item}`" for item in changed_names),
        ]
        if removed_names:
            summary_lines.extend(["", "Removed since baseline (listed for context; no file payload):", *(f"- `{item}`" for item in removed_names)])
        summary = "\n".join(summary_lines)
        baseline = f"Incremental against {state.get('archive', '(no baseline)')}. Files in this ZIP use project-relative paths. Created {datetime.now(timezone.utc).isoformat(timespec='seconds')}."

    target = write_archive(name, selected, summary, baseline)
    state_data = {"archive": name, "files": current}
    temporary = STATE_FILE.with_suffix(".tmp")
    temporary.write_text(json.dumps(state_data, indent=2) + "\n")
    temporary.replace(STATE_FILE)
    print(f"Source archive: {target}")
    print(f"Files: {len(selected)} project files + CHANGE_SUMMARY.md + BASELINE.txt")
    print(f"Size: {target.stat().st_size:,} bytes")


if __name__ == "__main__":
    main()
