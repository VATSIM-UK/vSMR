#!/usr/bin/env python3

import argparse
import subprocess
from pathlib import Path
import sys

# File extensions to format
EXTENSIONS = {".c", ".cpp", ".h", ".hpp"}

# Directories to skip (customize as needed)
SKIP_DIRS = {
    ".git",
    "build",
    "euroscope",
    "_ref"

}


def find_source_files(root: Path):
    for path in root.rglob("*"):
        if any(part in SKIP_DIRS for part in path.parts):
            continue
        if path.suffix in EXTENSIONS and path.is_file():
            yield path


def run_clang_format(files, apply_changes: bool):
    failed_files = []

    for file in files:
        if apply_changes:
            cmd = ["clang-format", "-i", "--style=file", str(file)]
        else:
            # --dry-run + -Werror makes clang-format exit non-zero if formatting is needed
            cmd = ["clang-format", "--dry-run", "--Werror", "--style=file", str(file)]

        result = subprocess.run(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )

        if result.returncode != 0:
            failed_files.append(file)

    return failed_files


def main():
    parser = argparse.ArgumentParser(
        description="Check or apply clang-format to a C/C++ project"
    )
    parser.add_argument(
        "path",
        nargs="?",
        default=".",
        help="Project root directory (default: current directory)",
    )
    parser.add_argument(
        "--apply",
        action="store_true",
        help="Apply formatting changes (default is check-only)",
    )

    args = parser.parse_args()
    root = Path(args.path).resolve()

    if not root.exists():
        print(f"Error: path does not exist: {root}")
        sys.exit(1)

    files = list(find_source_files(root))

    if not files:
        print("No source files found.")
        return

    print(f"Found {len(files)} files.")

    failed = run_clang_format(files, apply_changes=args.apply)

    if args.apply:
        print("Formatting applied.")
    else:
        if failed:
            print("\nThe following files need formatting:")
            for f in failed:
                print(f"  {f}")
            sys.exit(1)
        else:
            print("All files are properly formatted.")


if __name__ == "__main__":
    main()
