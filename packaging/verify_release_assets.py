#!/usr/bin/env python3
"""
verify_release_assets.py — Verifies release asset completeness and SHA256 integrity.
Used in CI and local distribution packaging checks.
"""

import os
import sys
import glob
import hashlib
from pathlib import Path

def compute_sha256(filepath: Path) -> str:
    h = hashlib.sha256()
    with open(filepath, "rb") as f:
        while chunk := f.read(65536):
            h.update(chunk)
    return h.hexdigest().lower()

def verify_assets(dist_dir: str) -> bool:
    dist = Path(dist_dir)
    if not dist.exists():
        print(f"[FAIL] Distribution directory does not exist: {dist}")
        return False

    print(f"[*] Scanning distribution assets in: {dist.resolve()}")
    all_files = list(dist.glob("*"))
    for f in all_files:
        size_mb = f.stat().st_size / (1024 * 1024)
        print(f"    - {f.name} ({size_mb:.2f} MB)")

    # Check checksums file if present
    checksum_file = dist / "SHA256SUMS.txt"
    if checksum_file.exists():
        print(f"[*] Validating checksums against {checksum_file.name}...")
        with open(checksum_file, "r", encoding="utf-8") as f:
            for line in f:
                line = line.strip()
                if not line or line.startswith("#"):
                    continue
                parts = line.split()
                if len(parts) >= 2:
                    expected_hash = parts[0].lower()
                    target_name = parts[1].lstrip("*")
                    target_file = dist / target_name
                    if not target_file.exists():
                        print(f"[FAIL] Checksum references missing file: {target_name}")
                        return False
                    actual_hash = compute_sha256(target_file)
                    if actual_hash != expected_hash:
                        print(f"[FAIL] Hash mismatch for {target_name}: expected {expected_hash}, got {actual_hash}")
                        return False
                    print(f"  [OK] Hash verified: {target_name}")
    else:
        print("[WARN] No SHA256SUMS.txt found in target directory.")

    print("[SUCCESS] All inspected release assets verified.")
    return True

if __name__ == "__main__":
    target = sys.argv[1] if len(sys.argv) > 1 else "dist"
    ok = verify_assets(target)
    sys.exit(0 if ok else 1)
