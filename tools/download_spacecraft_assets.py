import argparse
import hashlib
import os
from pathlib import Path
import sys
import urllib.error
import urllib.request

MIR_SOYUZ_BASE_URL = "https://raw.githubusercontent.com/Utkarsh575/Smart-India-Hackathon-2022/main/public/mir"

MIR_SOYUZ_ASSETS = {
    "scene.gltf": "B8D0F6A482FFA4A28DCEE2360D5AB11ADBD481451D3F39EEB10F65CEDCD49B7C",
    "scene.bin": "61F4B26AC7514204E1AD5ACE0CC1DB72F0E2515B611ED512FA5F0F6549F85C2C",
    "textures/Mir_Antenna_baseColor.png": "5D2BABDAF615C17055A3D8B2C783F19E89380A9254D09CA0C1D765681E4EE78A",
    "textures/Mir_Antenna_metallicRoughness.png": "7BAD39AE2DD939C116B5B6E19DACC8CE5B96CBF114C0B39CC9CCC2D482E0CC32",
    "textures/Mir_White_Isolation_baseColor.png": "C4CA5C5B0D6F42DA9B7424E37835933A8B80493EB541F98AF71081BA02F9EA88",
    "textures/Progress_black_Isolation_metallicRoughness.png": "AF5DE87A18935C46D9007AA71E0F3ED24E6A43E16C9D04E88BA5686733CEC41D",
    "textures/Progress_black_Isolation_normal.png": "8EF5409ABEB8EC6662F4474B45E1EBFFB85CE0D3771530B1EB8F37F67BD63D46",
    "textures/Progress_color_baseColor.png": "4E93A916C9C43069958C57EC27FED501CA58AF2F994360DFB2370C9141D94A8C",
    "textures/Progress_color_metallicRoughness.png": "C2888E8F5A1654BC665A8A82736F6687548DB6660A278A9D19C5274A5877F247",
    "textures/Progress_color_normal.png": "CC2C16C73560DD79EB2944B32537802AACAEFE5A8D1FE4B8937B26A705EFBB66",
    "textures/Soyuz_black_Isolation_metallicRoughness.png": "B63C0E262FB069EB0EBB21B6DF7311D74CD89B47159FA84CCD2B2B4FA741C1C2",
    "textures/Soyuz_color_metallicRoughness.png": "94E69F1BD38DFD67CE8604AD75F1CD69842940139AB6E39544ACACC3A1BC1D29",
    "textures/mir_color_baseColor.png": "388FFE8017C755957A060CF65A4276D31EA0B26D433C8A49F1774900C249BEC3",
    "textures/mir_color_metallicRoughness.png": "C16410BF1CE01B8E1FB6B8E0E09EC3CED284AEF9718ED281831D96FB9301BD84",
    "textures/mir_color_normal.png": "6004B46318D77B6F0CB6CFAD018838E56E5E1697F29DC32B68BB96B89455ABF4",
}

DRAGON_ASSETS = {
    "scene.gltf": "E63A1F9E2F6A6ADB95CA27C2204F381EED7B51279D5C3159F83EA31BFC9502E7",
    "scene.bin": "B1DC10A329B1ABC40C4944F402E098B16848916961CCCD2418FCA9B7782E35D1",
    "textures/body_baseColor.png": "93B4B4584CF9A75100E12909ED6BF18311E683B43D9802C2BF88F1FB6B7D4574",
    "textures/body_metallicRoughness.png": "832FB455916A2D864EDB7E7E9AD5FA27133A6DF82540CCE65EF0F2D8856E1185",
    "textures/body_normal.png": "8E36259A682F9129E7971461A4157948EF362EA561026A496893A61DC49B4552",
    "textures/ground_baseColor.png": "65EEC4468BED87FE995A68CB4FAA50CF5F295826B14113A99CE1E7C1DE500880",
    "textures/ground_emissive.png": "A31859560110589E5DBEBCB0548C543E83F3ADF76896182FA7DE3E1635DADB0E",
    "textures/heatshield_baseColor.png": "2B34A77D00FEC2801B667AEEC0BABCB93A19922FE934BD2C2BB733A23638E8F8",
    "textures/heatshield_metallicRoughness.png": "2A8727014836E7B95F9BF77BACD95BFA3730EAB08E330D8FCFD79CF18A13D323",
    "textures/heatshield_normal.png": "C7BF03F4C3284897D935F9FD3A3EE853742FCC7260334181E0181EEC65072E46",
    "textures/trunk_baseColor.png": "E23B8D84914D76FA88C35263AAD9631436AC52C4592E43DCD5ADB2F81CEDE30D",
    "textures/trunk_metallicRoughness.png": "D0A511AC5E5E45BF178EA9F9051F24025574E577886471D5C59079A79A8E7F89",
    "textures/trunk_normal.png": "C9172720A015B5174CC59CC2FB0E8C980CEBE09304D7E29260BBCA67F5A12ECA",
}

def compute_sha256(path: Path) -> str:
    h = hashlib.sha256()
    with open(path, "rb") as f:
        while chunk := f.read(65536):
            h.update(chunk)
    return h.hexdigest().upper()

def verify_asset(path: Path, expected_hash: str) -> bool:
    if not path.is_file():
        return False
    actual = compute_sha256(path)
    return actual == expected_hash.upper()

def download_file(url: str, dest: Path, expected_hash: str, timeout: float = 30.0) -> tuple[bool, str]:
    dest.parent.mkdir(parents=True, exist_ok=True)
    tmp_path = dest.with_suffix(dest.suffix + ".part")
    if tmp_path.exists():
        tmp_path.unlink()

    req = urllib.request.Request(url, headers={"User-Agent": "astro-voyager/1.0"})
    try:
        with urllib.request.urlopen(req, timeout=timeout) as resp, open(tmp_path, "wb") as out_f:
            h = hashlib.sha256()
            while chunk := resp.read(65536):
                h.update(chunk)
                out_f.write(chunk)
            actual_hash = h.hexdigest().upper()
    except (urllib.error.URLError, urllib.error.HTTPError, TimeoutError, OSError) as exc:
        if tmp_path.exists():
            tmp_path.unlink()
        return False, f"network error: {exc}"

    if actual_hash != expected_hash.upper():
        if tmp_path.exists():
            tmp_path.unlink()
        return False, f"SHA256 mismatch: expected {expected_hash}, got {actual_hash}"

    os.replace(tmp_path, dest)
    return True, "success"

def main():
    parser = argparse.ArgumentParser(
        description="Verify or download spacecraft model assets with SHA256 provenance."
    )
    parser.add_argument(
        "--verify-only", "-v",
        action="store_true",
        help="Only verify SHA256 of existing local files; do not download."
    )
    parser.add_argument(
        "--force", "-f",
        action="store_true",
        help="Re-download files even if local file matches expected SHA256."
    )
    parser.add_argument(
        "--strict",
        action="store_true",
        help="Exit with non-zero status on download failure or missing assets (default: non-fatal)."
    )
    parser.add_argument(
        "--check-dragon",
        action="store_true",
        help="Also check SpaceX Dragon assets in blender/out/spacex_src/dragon."
    )
    parser.add_argument(
        "--target-dir",
        type=str,
        default=None,
        help="Override destination directory for Mir/Soyuz assets (default: blender/out/mir_src)."
    )
    args = parser.parse_args()

    repo_root = Path(__file__).resolve().parent.parent
    mir_dir = Path(args.target_dir) if args.target_dir else (repo_root / "blender" / "out" / "mir_src")
    dragon_dir = repo_root / "blender" / "out" / "spacex_src" / "dragon"

    print("=== Spacecraft Asset Provenance Checker ===")
    print(f"Target Mir/Soyuz directory: {mir_dir}")
    print(f"Mode: {'verify-only' if args.verify_only else 'download-and-verify'}")

    mir_dir.mkdir(parents=True, exist_ok=True)

    verified_count = 0
    downloaded_count = 0
    missing_count = 0
    failed_count = 0
    offline_errors = []

    for rel_name, expected_sha in MIR_SOYUZ_ASSETS.items():
        local_path = mir_dir / rel_name
        if local_path.is_file() and not args.force:
            actual = compute_sha256(local_path)
            if actual == expected_sha.upper():
                print(f"  [skip] {rel_name} (SHA256 verified)")
                verified_count += 1
                continue
            else:
                print(f"  [WARN] {rel_name} SHA256 mismatch (expected {expected_sha}, got {actual})")

        if args.verify_only:
            if local_path.is_file():
                print(f"  [FAIL] {rel_name} SHA256 mismatch")
                failed_count += 1
            else:
                print(f"  [MISSING] {rel_name}")
                missing_count += 1
            continue

        url = f"{MIR_SOYUZ_BASE_URL}/{rel_name}"
        print(f"  [GET ] {rel_name} <- {url}")
        ok, msg = download_file(url, local_path, expected_sha)
        if ok:
            print(f"  [OK  ] {rel_name} (downloaded and SHA256 verified)")
            downloaded_count += 1
            verified_count += 1
        else:
            if "network error" in msg:
                print(f"  [OFFLINE] {rel_name}: {msg}")
                offline_errors.append((rel_name, msg))
            else:
                print(f"  [FAIL] {rel_name}: {msg}")
                failed_count += 1

    # Check Dragon if requested or if directory exists
    dragon_verified = 0
    dragon_missing = 0
    if args.check_dragon or dragon_dir.exists():
        print(f"\nChecking SpaceX Dragon assets in {dragon_dir}...")
        for rel_name, expected_sha in DRAGON_ASSETS.items():
            dragon_path = dragon_dir / rel_name
            if dragon_path.is_file():
                actual = compute_sha256(dragon_path)
                if actual == expected_sha.upper():
                    print(f"  [OK  ] {rel_name} (SHA256 verified)")
                    dragon_verified += 1
                else:
                    print(f"  [FAIL] {rel_name} SHA256 mismatch (expected {expected_sha}, got {actual})")
                    failed_count += 1
            else:
                print(f"  [MISSING] {rel_name} (must be downloaded manually from Sketchfab; see docs/SPACECRAFT_ASSETS.md)")
                dragon_missing += 1

    print("\n=== Summary ===")
    print(f"Mir/Soyuz assets: {verified_count}/{len(MIR_SOYUZ_ASSETS)} verified ({downloaded_count} downloaded).")
    if dragon_dir.exists() or args.check_dragon:
        print(f"SpaceX Dragon assets: {dragon_verified}/{len(DRAGON_ASSETS)} verified.")

    if offline_errors:
        print(f"\nNotice: {len(offline_errors)} asset(s) could not be downloaded due to offline/network conditions.")
        print("Existing game models in res/models/ remain usable.")
        if args.strict:
            sys.exit(1)
        else:
            print("Graceful non-fatal exit (offline mode).")
            sys.exit(0)

    if failed_count > 0:
        print(f"\nERROR: {failed_count} file(s) failed verification!")
        sys.exit(1)

    if missing_count > 0 and args.strict:
        print(f"\nERROR: {missing_count} file(s) missing in strict mode!")
        sys.exit(1)

    print("\nAll checked spacecraft assets verified successfully.")
    sys.exit(0)

if __name__ == "__main__":
    main()
