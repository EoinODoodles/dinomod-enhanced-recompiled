#!/usr/bin/env python3
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import toml
import json

def main():
    parser = argparse.ArgumentParser(description="Create a Thunderstore package from a mod project directory.")
    parser.add_argument("dir", type=str, help="The mod project directory to generate a package for.")

    args = parser.parse_args()

    # Do all path lookups from the mod directory
    os.chdir(Path(args.dir).resolve())

    # Load configs
    mod_toml_path = Path("mod.toml")
    if not mod_toml_path.exists():
        print(f"❌ Missing mod.toml at {mod_toml_path.absolute()}")
        exit(1)
    with open("mod.toml", "r", encoding="utf-8") as mod_toml:
        mod = toml.load(mod_toml)
    
    thunderstore_json_path = Path("thunderstore.json")
    if not thunderstore_json_path.exists():
        print(f"❌ Missing thunderstore.json at {thunderstore_json_path.absolute()}")
        exit(1)
    with open("thunderstore.json", "r", encoding="utf-8") as thunderstore_json:
        manifest = json.load(thunderstore_json)
    
    # Verify mod configs are sync'd
    if mod["manifest"]["version"] != manifest["version_number"]:
        print(f"❌ Version number is not synchronized between mod.toml and thunderstore.json. {mod["manifest"]["version"]} vs {manifest["version_number"]}")
        exit(1)

    print("✔️ mod.toml and thunderstore.json are synchronized.")
    
    # Verify thumbnail
    thumbnail_path = Path("thumb.png")
    if not thumbnail_path.exists():
        print(f"❌ Missing thumb.png at {thumbnail_path.absolute()}")
        exit(1)

    print("✔️ thumb.png exists.")

    # Verify README
    readme_path = Path("README.md")
    if not readme_path.exists():
        print(f"❌ Missing README.md at {readme_path.absolute()}")
        exit(1)

    print("✔️ README.md exists.")

    # Verify CHANGELOG
    changelog_path = Path("CHANGELOG.md")
    if not changelog_path.exists():
        print(f"⚠️ Missing CHANGELOG.md at {changelog_path.absolute()}")
    else:
        print("✔️ CHANGELOG.md exists.")
    
    # Build
    print("Running build...")
    subprocess.check_call(["make"])
    print("Building NRM...")
    subprocess.check_call(["../RecompModTool", "mod.toml", "build"])

    # Create package
    build_dir = Path("build")
    pkg_dir = build_dir.joinpath("thunderstore_package")
    if pkg_dir.exists():
        shutil.rmtree(pkg_dir)
    pkg_dir.mkdir()

    shutil.copy(thunderstore_json_path, pkg_dir.joinpath("manifest.json"))
    shutil.copy(thumbnail_path, pkg_dir.joinpath("icon.png"))
    shutil.copy(readme_path, pkg_dir.joinpath("README.md"))
    if changelog_path.exists():
        shutil.copy(changelog_path, pkg_dir.joinpath("CHANGELOG.md"))

    nrm_path = build_dir.joinpath(f"{mod["inputs"]["mod_filename"]}.nrm")
    shutil.copy(nrm_path, pkg_dir.joinpath(nrm_path.name))

    zip_path = build_dir.joinpath(f"{manifest["name"]}-{manifest["version_number"]}.zip")
    shutil.make_archive(zip_path.with_suffix("").as_posix(), "zip", pkg_dir)

    print(f"Created Thunderstore package at {zip_path.absolute()}")

if __name__ == "__main__":
    main()
