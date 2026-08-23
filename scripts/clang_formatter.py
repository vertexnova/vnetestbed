#!/usr/bin/env python3
"""
Clang formatter for C/C++. Recursively finds C/C++ in given folders (same scope as CI).

Usage:
    python scripts/clang_formatter.py <folder> [options]   # recursive under folder
    python scripts/clang_formatter.py all [options]        # recursive under src, include, samples, tests
    python scripts/clang_formatter.py --file <path> [options]

  --dry-run  Check only (fail if any file needs formatting; matches CI).
  Omit --dry-run to fix files in place.

Examples:
    python scripts/clang_formatter.py all --dry-run
    python scripts/clang_formatter.py samples              # all subfolders under samples/
    python scripts/clang_formatter.py --file samples/glfw_opengl/02_test_scene/demo_test_scene.cpp
"""

import argparse
import os
import subprocess
import sys
from functools import lru_cache
from pathlib import Path


def find_source_files(folder_path: Path) -> list:
    """Find all C/C++ source files recursively under the given folder."""
    source_files = []
    root_path = Path(folder_path).resolve()

    extensions = ('.h', '.cpp', '.mm', '.m', '.hpp', '.c')
    exclude_dirs = {'build', '.git', 'node_modules', 'CMakeFiles', '__pycache__'}

    for root, dirs, files in os.walk(root_path, topdown=True):
        # Recurse into all subdirs except excluded
        dirs[:] = [d for d in dirs if d not in exclude_dirs]
        for f in files:
            if f.lower().endswith(extensions):
                source_files.append(str(Path(root) / f))

    return source_files


def is_source_file(file_path: Path) -> bool:
    """Check if a file is a supported source file."""
    extensions = ('.h', '.cpp', '.mm', '.m', '.hpp', '.c')
    return file_path.suffix.lower() in extensions


@lru_cache(maxsize=1)
def get_clang_format_binary() -> str:
    """Use clang-format-17 when available (matches many VNE repos); otherwise clang-format."""
    for name in ('clang-format-17', 'clang-format'):
        try:
            subprocess.run([name, '--version'], capture_output=True, check=True)
            return name
        except (subprocess.CalledProcessError, FileNotFoundError):
            continue
    return 'clang-format'  # fallback for error message


def check_clang_format(binary: str) -> bool:
    """Check if clang-format is available."""
    try:
        subprocess.run([binary, '--version'], capture_output=True, check=True)
        return True
    except (subprocess.CalledProcessError, FileNotFoundError):
        return False


def run_clang_format(files: list, style_file: Path, dry_run: bool = False, *, binary: str) -> bool:
    """Run clang-format on the specified files."""
    if not files:
        print("No source files found to format.")
        return True

    print(f"Found {len(files)} source files to format (using {binary}).")

    base_cmd = [
        binary,
        '--style=file',
        '--fallback-style=Google',
    ]

    if dry_run:
        # Same as CI: fail if any file would be reformatted (--dry-run --Werror)
        print("DRY RUN - Checking for format violations (matches CI).")
        result = subprocess.run(
            [*base_cmd, '--dry-run', '--Werror', *files],
            capture_output=True,
            text=True,
        )
        if result.returncode != 0:
            if result.stderr:
                print(result.stderr, file=sys.stderr)
            print("One or more files need formatting. Run without --dry-run to fix.")
            return False
        print("All files are formatted correctly.")
        return True

    # In-place formatting
    for file_path in files:
        print(f"Formatting: {file_path}")
        try:
            subprocess.run(
                [*base_cmd, '-i', file_path],
                capture_output=True,
                text=True,
                check=True,
            )
            print(f"  ✓ Formatted successfully")
        except subprocess.CalledProcessError as e:
            print(f"  ✗ Error formatting {file_path}: {e}")
            if e.stderr:
                print(f"    stderr: {e.stderr}")
            return False
        except FileNotFoundError:
            print(f"  ✗ clang-format not found. Please install clang-format.")
            return False

    return True


def main():
    """Main function."""
    parser = argparse.ArgumentParser(
        description="Format C/C++ source files using clang-format (VneTestbed; CI dirs: src, include, samples, tests)",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python scripts/clang_formatter.py all --dry-run
  python scripts/clang_formatter.py samples
  python scripts/clang_formatter.py --file samples/glfw_opengl/02_test_scene/demo_test_scene.cpp
        """
    )

    # Create mutually exclusive group for folder vs file
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument(
        'folder',
        nargs='?',
        help='Folder to format (e.g. src, samples), or "all" for CI dirs (src, include, samples, tests)'
    )
    group.add_argument(
        '--file',
        help='Specific file to format (path relative to repo root)'
    )

    parser.add_argument(
        '--dry-run',
        action='store_true',
        help='Show what would be formatted without making changes'
    )
    parser.add_argument(
        '--verbose',
        action='store_true',
        help='Show verbose output'
    )

    args = parser.parse_args()

    # Get the project root directory
    script_dir = Path(__file__).parent
    project_root = script_dir.parent
    style_file = project_root / ".clang-format"

    # Check if .clang-format exists
    if not style_file.exists():
        print(f"Warning: .clang-format not found at {style_file}")
        print("Using fallback style: Google")

    print("VneTestbed Clang Formatter")
    print("=" * 40)
    if style_file.exists():
        print(f"Style file: {style_file}")
    print()

    if args.file:
        # Format specific file
        target_file = project_root / args.file

        if not target_file.exists():
            print(f"Error: File not found at {target_file}")
            sys.exit(1)

        if not is_source_file(target_file):
            print(f"Error: {target_file} is not a supported source file type.")
            print("Supported extensions: .h, .cpp, .mm, .m, .hpp, .c")
            sys.exit(1)

        print(f"Target file: {target_file}")
        print()

        source_files = [str(target_file)]

    else:
        # Format folder (or "all" = recursive over CI dirs: src, include, samples, tests)
        if (args.folder or "").lower() in ("all", "ci"):
            ci_dirs = ("src", "include", "samples", "tests")
            source_files = []
            for d in ci_dirs:
                p = project_root / d
                if p.is_dir():
                    source_files.extend(find_source_files(p))  # recursive under each dir
            source_files = sorted(set(source_files))
            print(f"Target: CI dirs (recursive) src, include, samples, tests — {len(source_files)} files")
        else:
            target_folder = (project_root / args.folder).resolve()
            if not target_folder.is_dir():
                print(f"Error: Target folder not found at {target_folder}")
                sys.exit(1)
            print(f"Target folder (recursive): {target_folder}")
            source_files = find_source_files(target_folder)
        print()

    if args.verbose:
        print("Source files found:")
        for file in source_files:
            print(f"  {file}")
        print()

    # Check clang-format availability (prefer clang-format-17 when present)
    binary = get_clang_format_binary()
    if not check_clang_format(binary):
        print("Error: clang-format not found. Please install clang-format (install clang-format-17 for pinned behavior).")
        print("  Ubuntu/Debian: sudo apt install clang-format-17")
        print("  macOS: brew install clang-format@17  (or llvm)")
        sys.exit(1)
    if binary != 'clang-format-17':
        print(f"Note: Using '{binary}'. For CI-identical formatting when CI pins 17, install clang-format-17.")

    # Run clang-format
    success = run_clang_format(source_files, style_file, args.dry_run, binary=binary)

    if success:
        print("\n✓ Formatting completed successfully!")
        if not args.dry_run:
            if args.file:
                print(f"Formatted 1 file: {args.file}")
            else:
                print(f"Formatted {len(source_files)} files in {args.folder}")
    else:
        print("\n✗ Formatting failed!")
        sys.exit(1)


if __name__ == "__main__":
    main()
