#!/usr/bin/env python3
"""
Update string.csv to add xrefs column from StringPool analysis.

Usage:
    1. Copy stringpool_xrefs.txt from Windows temp dir to this directory
    2. Run: python3 update_string_csv.py

The script will:
- Read resources/string.csv
- Read stringpool_xrefs.txt (idx|xref1 xref2 ...)
- Add xrefs as third column
- Write back to resources/string.csv
"""

import os
import sys

def load_xrefs(xrefs_path: str) -> dict:
    """Load xrefs data from file."""
    xrefs = {}
    with open(xrefs_path, 'r', encoding='utf-8') as f:
        for line in f:
            line = line.strip()
            if not line or '|' not in line:
                continue
            idx_str, xref_str = line.split('|', 1)
            try:
                idx = int(idx_str)
                xrefs[idx] = xref_str
            except ValueError:
                continue
    return xrefs

def update_csv(csv_path: str, xrefs: dict):
    """Update CSV file with xrefs column."""
    lines = []
    with open(csv_path, 'r', encoding='utf-8') as f:
        for line in f:
            line = line.rstrip('\n\r')
            if not line:
                continue

            # Parse existing CSV line: idx,string_value
            parts = line.split(',', 1)
            if len(parts) < 2:
                lines.append(line)
                continue

            try:
                idx = int(parts[0])
                string_val = parts[1]

                # Get xrefs for this idx
                xref_str = xrefs.get(idx, '')

                # Build new line with 3 columns: idx,string_value,xrefs
                new_line = f"{idx},{string_val},{xref_str}"
                lines.append(new_line)
            except ValueError:
                lines.append(line)

    # Write back
    with open(csv_path, 'w', encoding='utf-8', newline='\n') as f:
        for line in lines:
            f.write(line + '\n')

def main():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    repo_dir = os.path.dirname(script_dir)

    csv_path = os.path.join(repo_dir, 'resources', 'string.csv')
    xrefs_path = os.path.join(script_dir, 'stringpool_xrefs.txt')

    if not os.path.exists(xrefs_path):
        print(f"Error: {xrefs_path} not found")
        print("Please copy stringpool_xrefs.txt from Windows temp dir to this directory")
        sys.exit(1)

    if not os.path.exists(csv_path):
        print(f"Error: {csv_path} not found")
        sys.exit(1)

    print(f"Loading xrefs from {xrefs_path}...")
    xrefs = load_xrefs(xrefs_path)
    print(f"Loaded {len(xrefs)} idx entries")

    print(f"Updating {csv_path}...")
    update_csv(csv_path, xrefs)
    print("Done!")

if __name__ == '__main__':
    main()
