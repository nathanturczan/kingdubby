#!/usr/bin/env python3
"""
fix-moduleinfo.py - Fix JUCE-generated moduleinfo.json

JUCE generates moduleinfo.json with trailing commas, which is invalid JSON.
Some DAW hosts (like certain Ableton versions) reject plugins with invalid JSON.

Usage:
    python3 fix-moduleinfo.py path/to/moduleinfo.json

This script:
1. Reads the JSON file
2. Removes trailing commas before } and ]
3. Validates the result is valid JSON
4. Writes back the fixed content
"""

import json
import re
import sys
from pathlib import Path


def fix_trailing_commas(content: str) -> str:
    """Remove trailing commas before } and ]"""
    # Remove trailing commas before closing braces
    content = re.sub(r',\s*}', '}', content)
    # Remove trailing commas before closing brackets
    content = re.sub(r',\s*]', ']', content)
    return content


def main():
    if len(sys.argv) < 2:
        print("Usage: python3 fix-moduleinfo.py <path/to/moduleinfo.json>")
        print("\nFixes JUCE-generated moduleinfo.json by removing trailing commas.")
        sys.exit(1)

    filepath = Path(sys.argv[1])

    if not filepath.exists():
        print(f"Error: File not found: {filepath}")
        sys.exit(1)

    # Read original content
    original = filepath.read_text()

    # Fix trailing commas
    fixed = fix_trailing_commas(original)

    # Validate it's now valid JSON
    try:
        parsed = json.loads(fixed)
    except json.JSONDecodeError as e:
        print(f"Error: Could not parse JSON after fix: {e}")
        sys.exit(1)

    # Write back with proper formatting
    filepath.write_text(json.dumps(parsed, indent=2))

    print(f"Fixed: {filepath}")


if __name__ == "__main__":
    main()
