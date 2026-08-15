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
    "mod/gui/station-copper-ore.png",
    "mod/gui/station-iron-ore.png",
    "mod/gui/station-copper-plates.png",
    "mod/gui/station-iron-plates.png",
    "mod/gui/station-steel-bars.png",
    "mod/gui/station-copper-alloy-plates.png",
    "mod/gui/station-electronics.png",
    "mod/gui/station-crossbow.png",
    "mod/gui/station-skeleton-limbs.png",
    "scripts/build.ps1",
    "scripts/install.ps1",
    "src/KenshiJobManagement.cpp",
    "src/RuntimeAccess.inl",
    "src/JobView.inl",
    "src/JobActions.inl",
    "src/JobWindow.inl",
    "src/Hooks.inl",
    "src/StationScanner.inl",
    "src/StationSettings.inl",
    "src/StationAssets.inl",
    "src/StationView.inl",
)

SOURCE_TOKENS = (
    "__declspec(dllexport) void startPlugin()",
    "PlayerInterface::updateUT",
    "PlayerInterface::clearAndReset",
    "getPermajobCount",
    "getPermajobName",
    "getPermajobData",
    "TaskMatch",
    "getCurrentPlatoon",
    "PlayerInterface::cycleSquad",
    "getActivePlatoon",
    "movePermajob",
    "removePermajob",
    "clearPermajobs",
    "isJobsEnabled",
    "setJobsEnabled",
    "OrderData",
    "PortraitManager",
    "setImageWidget",
    "portraitBackground->setDepth(5)",
    "portrait->setDepth(3)",
    "portraitBound",
    "getStat(stat, true)",
    "userPause(true)",
    "setGameSpeed",
    "selectedObjectsChangedThisFrame",
    "createWidget<MyGUI::Window>",
    '"Kenshi_ScrollBarV"',
    '"Kenshi_ScrollBarH"',
    "GetAsyncKeyState",
    "REFRESH_INTERVAL_MS = 1000",
    "MAX_SAFE_JOB_ROWS = 64",
    "ROW_HEIGHT = 116",
    "CARD_HEIGHT = 88",
    "MEMBER_PORTRAIT_SIZE = 80",
    "CARD_WIDTH = 168",
    '"KJM_CardArrow"',
    '"KJM_CardSelectionTint"',
    '"KJM_CardUnavailableTint"',
    "StripLeadingPriorityPrefix",
    "WrapCardJobCaption",
    "SetFittedCardTargetCaption",
    "client->setInheritsAlpha(false)",
    "background->setAlpha(1.0f)",
    "key->mWheel = 0",
    "RebuildJobHighlightCache",
    "ApplyCachedJobHighlightGroups",
    "SameJobHighlightKey",
    "eventMouseSetFocus",
    '"KJM_CardHighlightTop"',
    '"KJM_JobCategoryBackground"',
    '"KJM_JobCategoryOverlay"',
    "TickJobStationCategoryCache",
    "TryResolveJobStationCategory",
    "StationVisualSubtype",
    "STATION_VISUAL_COPPER_ORE",
    "STATION_VISUAL_IRON_ORE",
    "STATION_VISUAL_COPPER_PLATES",
    "STATION_VISUAL_IRON_PLATES",
    "STATION_VISUAL_STEEL_BARS",
    "STATION_VISUAL_COPPER_ALLOY_PLATES",
    "STATION_VISUAL_ELECTRONICS",
    "STATION_VISUAL_CROSSBOW",
    "STATION_VISUAL_SKELETON_LIMBS",
    "TryReadStationVisualSubtype",
    "GetStationVisualIconResource",
    "STATION_VISUAL_ICON_FILES",
    "TrySkipDeadCurrentSquad",
    "TryCallOriginalCycleSquad",
    "TryRefreshMemberByIdentity",
    "SameQueue",
    "ACTION_REMOVE_SELECTED",
    "MODAL_CLEAR",
    "MODAL_OPTIONS",
    "WritePrivateProfileStringA",
    # Queue-derived Stations tab and validated assignment transfer.
    "StationScanState",
    "STATION_SCAN_TARGET_LIMIT = 2048",
    "CollectAssignedStationTargets",
    "IsStationDisplayJob",
    "taskType != JOB_BUILDER",
    "taskType != JOB_MEDIC",
    "taskType != JOB_REPAIR_ROBOT",
    "taskType != FIND_AND_RESCUE",
    "assignedTargetHandles",
    "BeginStationScan",
    "StepStationScan",
    "RefreshStationAssignments",
    "STATION_OTHER",
    "GetStationCategoryName",
    "GetStationCategoryIconResource",
    "EnsureStationIconResources",
    "LoadStationCategorySettings",
    "SaveStationCategorySettings",
    "SetManagerTab",
    '"KJM_SquadTabButton"',
    '"KJM_StationTabButton"',
    "CreateStationView",
    "SetStationBoardSnapshot",
    "SetStationViewVisible",
    "RefreshStationView",
    "STATION_COLUMN_WIDTH",
    "STATION_MEMBER_ROW_HEIGHT = 116",
    '"KJM_StationAssignmentCard"',
    '"KJM_StationAssignmentOverflow"',
    "GetStationAssignmentWorkLabel",
    "GetCompactStationAssignmentWorkLabel",
    '"KJM_StationColumnDividerHeader"',
    '"KJM_StationColumnDividerBody"',
    "EnsureStationHeaderDragBuffer",
    "OnStationHeaderDrag",
    "IsStationHeaderDragArmed",
    "ACTION_TRANSFER_STATION_JOB",
    "OnStationAssignmentDrag",
    "TryAddStationPermanentJob",
    "SameQueueWithAppendedStationJob",
    "SameQueueWithExactJobRemoved",
    "TryPatchStationTransferProjection",
    "g_stationProjectionRefreshRequested",
    "RefreshStationTransferredMemberRows",
    "processedStationTarget",
    "5038571-Universal Wasteland Expansion.mod",
    "IsStationInteractionDragArmed",
    "WrapToolTipCaption",
    "STATION_OVERSCAN",
    "READING ASSIGNED JOB TARGETS - RESULTS INCOMPLETE",
    "ASSIGNED TARGET LIST TRUNCATED AT 2,048 - RESULTS INCOMPLETE",
    '"JOBS UNAVAILABLE"',
)

EMPTY_FCS_MOD_MARKER = bytes.fromhex(
    "11 00 00 00 1e 00 00 00 01 00 00 00 00 00 00 00 "
    "00 00 00 00 00 00 00 00 00 00 00 00 01 00 00 00 "
    "00 00 00 00 00 00 09 00 00 00 00 00 00 00"
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
        if marker.read_bytes() != EMPTY_FCS_MOD_MARKER:
            fail(
                errors,
                "mod/KenshiJobManagement.mod must be the canonical blank FCS mod marker",
            )
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
    for dependency in (
        "KenshiLib.lib",
        "MyGUIEngine_x64.lib",
        "OgreMain_x64.lib",
        "User32.lib",
    ):
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
        "src/StationAssets.inl",
        "src/StationScanner.inl",
        "src/StationSettings.inl",
        "src/StationView.inl",
    )
    source = "\n".join(read_text(path, errors) for path in source_paths)
    for token in SOURCE_TOKENS:
        if token not in source:
            fail(errors, f"Source is missing critical token: {token}")

    # The requested transparency belongs to each icon ImageBox.  A tint or
    # parent alpha is not an equivalent substitute, and both tabs must apply
    # the rule independently.
    for relative in ("src/JobView.inl", "src/StationView.inl"):
        text = read_text(relative, errors)
        if "setAlpha(0.33f)" not in text:
            fail(errors, f"{relative} must set direct station icon opacity to 33%")
        if "setInheritsAlpha(false)" not in text:
            fail(errors, f"{relative} must keep station icon alpha independent")

    if "std::thread" in source or "CreateThread" in source:
        fail(errors, "0.1 UI code must remain single-threaded; MyGUI is UI-thread only")

    for token in (
        "getOwnedBuildingsH",
        "CaptureStationOwnedBuildingHandles",
        "ZoneMapContent::findAllBuildings",
        "getAllActiveZonesT",
    ):
        if token in source:
            fail(errors, f"Queue-derived Stations view must not enumerate the world: {token}")

    if source.count("KenshiLib::AddHook(") < 3:
        fail(
            errors,
            "Source must install updateUT, clearAndReset, and cycleSquad hooks",
        )

    if source.count("__try") < 8 or source.count("__except") < 8:
        fail(errors, "Engine pointer operations are not consistently guarded with SEH wrappers")

    for token in (
        "TryRefreshMemberByIdentity(action.member, &fresh)",
        "SameQueue(fresh.jobs, action.sequence)",
        "FindJobSlot(fresh, candidate.selected.job)",
        "SameJob(fresh.jobs[liveSlot], candidate.selected.job)",
        "SameQueue(source.jobs, action.sequence)",
        "SameQueue(destination.jobs, action.destinationSequence)",
        "SameHandleIdentity(action.job.target, action.stationTarget)",
        "character->addJob(taskType, building, true, true, position)",
        "TryRemovePermajob(sourceBeforeRemove.handle, verifiedSourceSlot)",
    ):
        if token not in source:
            fail(errors, f"Exact queue revalidation is missing: {token}")

    if "->permajobs" in source or ".permajobs" in source:
        fail(errors, "Source must not mutate or inspect the permanent-job container directly")

    if "bytes + 0x10" in source:
        fail(errors, "Task targets must use Kenshi's TaskMatch adapter, not a raw ABI offset")

    if "MODAL_REMOVE" in source:
        fail(errors, "Remove Selected must remain immediate and must not open a modal")

    if 'setCaption("DRAG")' in source or '"KJM_CardDragHint"' in source:
        fail(errors, "Compact job cards must not repeat a visible DRAG label")

    if '"KJM_CardPriority"' in source:
        fail(errors, "Priority must appear only in the column header, not inside a card")

    if "Showing Kenshi's current squad in vanilla member order." in source:
        fail(errors, "The routine current-squad footer message must remain hidden")

    if source.count("g_closeRequested = true;") < 2 or "if (g_closeRequested)" not in source:
        fail(errors, "MyGUI window destruction must remain deferred outside event callbacks")


def validate_scripts_and_docs(errors: list[str]) -> None:
    build = read_text("scripts/build.ps1", errors)
    for token in (
        "KENSHILIB_DIR",
        "BOOST_INCLUDE_PATH",
        "MyGUIEngine_x64.lib",
        "OgreMain_x64.lib",
        "Configuration=Release",
        "Platform=x64",
        "Compress-Archive",
    ):
        if token not in build:
            fail(errors, f"Build script is missing: {token}")

    readme = read_text("README.md", errors)
    for token in (
        "Ctrl+J",
        "field-test build",
        "disposable save",
        "Stations",
        "2,048 assigned targets",
        "does not enumerate zones",
        "read-only",
        "33% opaque",
        "visual subtype icons",
        "patches only the source and destination rows in place",
    ):
        if token not in readme:
            fail(errors, f"README must disclose or document: {token}")

    design = read_text("docs/DESIGN.md", errors)
    for token in (
        "OutpostScanner",
        "AssignmentMatrix",
        "OptionalPriorityScheduler",
        "Assignment-derived Stations tab milestone",
        "one stable queue target",
        "2,048 assigned targets",
        "Other / Unclassified",
        "cannot show unassigned stations",
        "StationVisualSubtype",
        "ImageBox",
        "projection is patched in place",
        "full refresh only when the patch preconditions fail",
    ):
        if token not in design:
            fail(errors, f"Design roadmap is missing component: {token}")

    testing = read_text("docs/TESTING.md", errors)
    for token in (
        "Same-row drag reorder",
        "Cross-member multi-select and drop-to-remove",
        "Clear Queue Yes/No/Esc modal and fingerprint",
        "Stations tab: queue-derived assignment matrix",
        "READING ASSIGNED JOB TARGETS - RESULTS INCOMPLETE",
        "Assigned target list truncated at 2,048",
        "33%` opaque",
        "specific pictograms",
        "successful transfer, confirm only the source and destination rows change",
        "save",
    ):
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
