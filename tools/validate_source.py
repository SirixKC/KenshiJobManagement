#!/usr/bin/env python3
"""Static validation for the source-first Kenshi Job Management package."""

from __future__ import annotations

import json
import sys
from pathlib import Path
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]

REQUIRED_FILES = (
    ".github/workflows/validate.yml",
    ".gitignore",
    "KenshiJobManagement.sln",
    "KenshiJobManagement.vcxproj",
    "KenshiJobManagement.vcxproj.filters",
    "LICENSE",
    "README.md",
    "docs/DESIGN.md",
    "docs/TESTING.md",
    "mod/KenshiJobManagement.mod",
    "mod/README.txt",
    "mod/RE_Kenshi.json",
    "scripts/build.ps1",
    "scripts/install.ps1",
    "src/KenshiJobManagement.cpp",
    "src/RuntimeAccess.inl",
    "src/JobView.inl",
    "src/JobActions.inl",
    "src/JobWindow.inl",
    "src/Hooks.inl",
)

SOURCE_TOKENS = (
    "__declspec(dllexport) void startPlugin()",
    "PlayerInterface::updateUT",
    "PlayerInterface::clearAndReset",
    "getPermajobCount",
    "getPermajobName",
    "getPermajobData",
    "movePermajob",
    "removePermajob",
    "clearPermajobs",
    "isJobsEnabled",
    "setJobsEnabled",
    "selectedObjectsChangedThisFrame",
    "createWidgetReal<MyGUI::Window>",
    '"Kenshi_ListBox"',
    "GetAsyncKeyState",
    "ValidateUiSlot",
    "CLEAR_CONFIRMATION_MS",
)


def fail(errors: list[str], message: str) -> None:
    errors.append(message)


def read_text(relative: str, errors: list[str]) -> str:
    path = ROOT / relative
    try:
        return path.read_text(encoding="utf-8")
    except OSError as exc:
        fail(errors, f"Could not read {relative}: {exc}")
        return ""


def validate_required_files(errors: list[str]) -> None:
    for relative in REQUIRED_FILES:
        if not (ROOT / relative).is_file():
            fail(errors, f"Missing required file: {relative}")


def validate_manifest(errors: list[str]) -> None:
    path = ROOT / "mod/RE_Kenshi.json"
    try:
        manifest = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        fail(errors, f"Invalid mod/RE_Kenshi.json: {exc}")
        return

    if manifest != {"Plugins": ["KenshiJobManagement.dll"]}:
        fail(errors, "RE_Kenshi manifest must load only KenshiJobManagement.dll")

    marker = ROOT / "mod/KenshiJobManagement.mod"
    try:
        if marker.stat().st_size != 0:
            fail(errors, "mod/KenshiJobManagement.mod must remain an empty FCS marker")
    except OSError as exc:
        fail(errors, f"Could not inspect mod marker: {exc}")


def validate_project(errors: list[str]) -> None:
    project_path = ROOT / "KenshiJobManagement.vcxproj"
    try:
        tree = ET.parse(project_path)
    except (OSError, ET.ParseError) as exc:
        fail(errors, f"Invalid Visual Studio project XML: {exc}")
        return

    root = tree.getroot()
    namespace = {"msb": "http://schemas.microsoft.com/developer/msbuild/2003"}

    configurations = {
        node.attrib.get("Include", "")
        for node in root.findall(".//msb:ProjectConfiguration", namespace)
    }
    if "Release|x64" not in configurations:
        fail(errors, "Visual Studio project must define Release|x64")

    toolsets = {
        (node.text or "").strip()
        for node in root.findall(".//msb:PlatformToolset", namespace)
    }
    if "v100" not in toolsets:
        fail(errors, "Visual Studio project must use the VC100 toolset")

    sources = {
        node.attrib.get("Include", "").replace("/", "\\")
        for node in root.findall(".//msb:ClCompile", namespace)
    }
    if "src\\KenshiJobManagement.cpp" not in sources:
        fail(errors, "Visual Studio project does not compile src\\KenshiJobManagement.cpp")

    dependencies = ";".join(
        (node.text or "")
        for node in root.findall(".//msb:AdditionalDependencies", namespace)
    )
    for dependency in ("kenshilib.lib", "MyGUIEngine_x64.lib", "User32.lib"):
        if dependency not in dependencies:
            fail(errors, f"Visual Studio project is missing linker dependency: {dependency}")

    project_text = read_text("KenshiJobManagement.vcxproj", errors)
    for package_file in (
        "RE_Kenshi.json",
        "KenshiJobManagement.mod",
        "README.txt",
    ):
        if package_file not in project_text:
            fail(errors, f"Post-build package does not mention {package_file}")


def validate_source(errors: list[str]) -> None:
    source_paths = (
        "src/KenshiJobManagement.cpp",
        "src/RuntimeAccess.inl",
        "src/JobView.inl",
        "src/JobActions.inl",
        "src/JobWindow.inl",
        "src/Hooks.inl",
    )
    source = "\n".join(read_text(path, errors) for path in source_paths)
    for token in SOURCE_TOKENS:
        if token not in source:
            fail(errors, f"Source is missing critical token: {token}")

    if "std::thread" in source or "CreateThread" in source:
        fail(errors, "0.1 UI code must remain single-threaded; MyGUI is UI-thread only")

    if source.count("KenshiLib::AddHook(") < 2:
        fail(errors, "Source must install both updateUT and clearAndReset hooks")

    if source.count("__try") < 8 or source.count("__except") < 8:
        fail(errors, "Engine pointer operations are not consistently guarded with SEH wrappers")

    if "g_rows[slot]" not in source or "taskData == currentTaskData" not in source:
        fail(errors, "Selected queue rows must be revalidated before mutation")


def validate_scripts_and_docs(errors: list[str]) -> None:
    build = read_text("scripts/build.ps1", errors)
    for token in (
        "KENSHILIB_DIR",
        "BOOST_INCLUDE_PATH",
        "MyGUIEngine_x64.lib",
        "Configuration=Release",
        "Platform=x64",
        "Compress-Archive",
    ):
        if token not in build:
            fail(errors, f"Build script is missing: {token}")

    readme = read_text("README.md", errors)
    for token in ("Ctrl+J", "source-first field-test", "disposable save"):
        if token not in readme:
            fail(errors, f"README must disclose or document: {token}")

    design = read_text("docs/DESIGN.md", errors)
    for token in ("OutpostScanner", "AssignmentMatrix", "OptionalPriorityScheduler"):
        if token not in design:
            fail(errors, f"Design roadmap is missing component: {token}")

    testing = read_text("docs/TESTING.md", errors)
    for token in ("Remove one exact row", "Reorder", "Clear all", "save"):
        if token not in testing:
            fail(errors, f"Testing checklist is missing coverage for: {token}")


def main() -> int:
    errors: list[str] = []
    validate_required_files(errors)
    validate_manifest(errors)
    validate_project(errors)
    validate_source(errors)
    validate_scripts_and_docs(errors)

    if errors:
        print("Source validation failed:", file=sys.stderr)
        for error in errors:
            print(f"  - {error}", file=sys.stderr)
        return 1

    print("Kenshi Job Management source tree validated successfully.")
    print("Note: this does not compile the VC100 DLL or replace in-game testing.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
