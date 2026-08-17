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
    "docs/WORKSHOP.md",
    "mod/KenshiJobManagement.mod",
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
    "mod/gui/job-engineering.png",
    "mod/gui/job-medic.png",
    "mod/gui/kjm-hud-icon.png",
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
    "src/HudButton.inl",
    "src/StationView.inl",
    "src/SquadSelector.inl",
    "diagnostics/evidence/2026-08-15-station-handle-resolution-success.md",
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
    "ROW_HEIGHT = 120",
    "CARD_HEIGHT = 88",
    "MEMBER_PORTRAIT_SIZE = 80",
    "MyGUI::IntCoord(7, 11, MEMBER_PORTRAIT_SIZE, MEMBER_PORTRAIT_SIZE)",
    "MEMBER_TEXT_LEFT, 8,",
    "MEMBER_TEXT_LEFT, 33,",
    "MEMBER_TEXT_LEFT, 49,",
    "MEMBER_TEXT_LEFT, 49, g_memberWidth - MEMBER_TEXT_LEFT - 7, 48)",
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
    "key->rotate = false",
    "RebuildJobHighlightCache",
    "ApplyCachedJobHighlightGroups",
    "SameJobHighlightKey",
    "row.hasTarget && IsStationDisplayJob(row.taskType)",
    "key.hasTarget = job.hasTarget && IsStationDisplayJob(job.taskType);",
    "showTarget && !row.targetAvailable",
    "SameHandleIdentity(left.target, right.target)",
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
    "PublishUnavailableSquadSelection",
    "unavailable.unavailable = true",
    # Bottom Squad Jobs selector: raw vanilla order, value-only records, and
    # fresh identity validation before the native squad mutation.
    "SquadSelectorEntry",
    "TryBuildSquadSelectorSnapshotGuarded",
    "getActivePlatoons",
    "activePlatoons->valid()",
    "memberCount <= 0",
    'activeName == "__DEAD__"',
    'platoon->stringID == "__DEAD__"',
    "SQUAD_SELECTOR_BUTTON_WIDTH = 150",
    "SQUAD_SELECTOR_BUTTON_HEIGHT = 34",
    "SQUAD_SELECTOR_BUTTON_GAP = 4",
    '"KJM_SquadSelectorRoot"',
    '"KJM_SquadSelectorViewport"',
    '"KJM_SquadSelectorCanvas"',
    '"KJM_SquadSelectorScroll"',
    '"KJM_SquadSelectorIndex"',
    "setStateSelected(selected)",
    "g_pendingSquadSelectionIdentity",
    "TrySelectSquadByIdentity",
    "TrySetCurrentPlatoonGuardedLeaf",
    "setCurrentPlatoon(platoon)",
    "BindSquadSelectorMouseWheelTree",
    '"KJM_SquadSelectorWheelRoot"',
    "if (!shift)",
    "if (g_squadSelectorMaximumOffset == 0)",
    "OnMouseWheel(widget, relative)",
    "g_squadSelectorOffset - relative * 80",
    "g_squadSelectorCanvas->setPosition(-g_squadSelectorOffset, 0)",
    "TryRefreshMemberByIdentity",
    "SameQueue",
    "ACTION_REMOVE_SELECTED",
    "MODAL_CLEAR",
    "MODAL_OPTIONS",
    "WritePrivateProfileStringA",
    # Player-station grouped grid and validated detail actions.
    "StationScanState",
    "STATION_SCAN_TARGET_LIMIT = 2048",
    "CollectAssignedStationTargets",
    "IsStationDisplayJob",
    "taskType != JOB_BUILDER",
    "taskType != JOB_MEDIC",
    "taskType != JOB_REPAIR_ROBOT",
    "taskType != FIND_AND_RESCUE",
    "taskType != FIND_BED_AND_PUT_IN",
    "taskType != FIND_AND_RESCUE_IF_THERES_BEDS",
    "JOB_ENGINEERING_ICON_FILE",
    "JOB_MEDIC_ICON_FILE",
    "g_jobEngineeringIconAvailable",
    "g_jobMedicIconAvailable",
    "GetGlobalJobIconResource",
    "roleIconResource",
    "assignedTargetHandles",
    "BeginStationScan",
    "StepStationScan",
    "RefreshStationAssignments",
    "STATION_OWNERSHIP_COPY_LIMIT = 8192",
    "sizeof(lektor<hand>) == 0x18",
    "offsetof(lektor<hand>, count) == 0x08",
    "offsetof(lektor<hand>, maxSize) == 0x0C",
    "offsetof(lektor<hand>, stuff) == 0x10",
    "sizeof(hand) == 0x20",
    "offsetof(hand, type) == 0x08",
    "offsetof(hand, container) == 0x0C",
    "offsetof(hand, containerSerial) == 0x10",
    "offsetof(hand, index) == 0x14",
    "offsetof(hand, serial) == 0x18",
    "TryCopyStationOwnershipRecords",
    "TryReadStationOwnershipHeaderOnce",
    "faction->factionOwnerships",
    "StationOwnershipSpanFits",
    "source->count",
    "source->maxSize",
    "source->stuff",
    "SameStationOwnershipHeader(after, current)",
    "output[index].vtable",
    "output[index].type",
    "output[index].container",
    "output[index].containerSerial",
    "output[index].index",
    "output[index].serial",
    "StationOwnedRecordMatchesHand",
    "reconstructed->isValid()",
    "reconstructed->getBuilding()",
    "building->getHandle()",
    "building->isThePlayer()",
    "STATION_OWNED_RECORD_UNLOADED",
    "STATION_OWNED_RECORD_FAULT",
    "STATION_OTHER",
    "GetStationCategoryName",
    "GetStationCategoryIconResource",
    "EnsureStationIconResources",
    "LoadStationCategorySettings",
    "SaveStationCategorySettings",
    "IsStationCategoryCollapsed",
    "SetStationCategoryCollapsed",
    '"Collapsed_%d"',
    "SetManagerTab",
    '"KJM_SquadTabButton"',
    '"KJM_StationTabButton"',
    "CreateStationView",
    "SetStationBoardSnapshot",
    "SetStationViewVisible",
    "RefreshStationView",
    "STATION_SCAN_PRESENTATION_BATCH_SIZE = 16",
    "TryCompleteStationScanImmediately",
    "while (!g_stationScan.complete && steps <= candidateCount)",
    "g_stationScan.pendingStations.clear()",
    "pendingStations",
    "PublishPendingStationResults",
    "state->stations.insert(",
    "scanProgressChanged",
    "!g_stationTabActive || !g_stationScan.started",
    "assignmentSupported",
    "STATION_GRID_CARD_WIDTH = 214",
    "STATION_GRID_CARD_HEIGHT = 128",
    "StationGridGroup",
    "StationGridCardBinding",
    '"KJM_StationCategoryHeader"',
    '"KJM_StationCard"',
    '"KJM_StationDetailModal"',
    '"KJM_StationDetailBackground"',
    "modalBackground->setAlpha(1.0f)",
    "modalBackground->setDepth(100)",
    "gui->createWidget<MyGUI::Widget>(",
    "input->addWidgetModal(g_stationView.modalPanel)",
    '"ASSIGNED WORKERS ("',
    '"AVAILABLE WORKERS ("',
    '"KJM_StationAssignmentUnsupported"',
    '"KJM_StationAvailableViewport"',
    "OnStationAvailableWheel",
    "label->setTextColour(MyGUI::Colour(1.0f, 1.0f, 1.0f))",
    "IsStationDetailOpen",
    "CloseStationDetail",
    "detailCloseRequested",
    "ACTION_TRANSFER_STATION_JOB",
    "TryAddStationPermanentJob",
    "SameQueueWithAppendedStationJob",
    "SameQueueWithExactJobRemoved",
    "TryPatchStationTransferProjection",
    "g_stationProjectionRefreshRequested",
    "RefreshStationTransferredMemberRows",
    "5038571-Universal Wasteland Expansion.mod",
    "IsStationInteractionDragArmed",
    "WrapToolTipCaption",
    "PARTIAL SCAN - RESULTS INCOMPLETE",
    "PLAYER STATION RESULT LIST TRUNCATED AT 2,048 - RESULTS INCOMPLETE",
    "SetStationCardAssignment",
    'assignment->setCaption("X")',
    "IsStationUsableAndUnassigned",
    "blockingStatusKnown",
    '"KJM_StationUnassignedTop"',
    "SetStationUnassignedOutlineVisible",
    "howMuchPowerDoYouWantNow() > 0.001f",
    '"POWER OFF"',
    "ACTION_ASSIGN_STATION",
    "ACTION_REMOVE_STATION_BUNDLE",
    "RequestAddStationAssignment",
    "RequestRemoveStationAssignment",
    # Options deliberately filters only the broad station categories. Squad
    # member stats are always selected from the complete supported list.
    "bool StationPassesFilters(const StationTargetSnapshot& station)",
    "return IsStationCategoryEnabled(station.category);",
    "void BuildTopSkills(Character* character, std::vector<SkillValue>* skillsOut)",
    "void OnOptionsMouseWheel(MyGUI::Widget*, int relative)",
    "category->eventMouseWheel +=",
    "case BCTYPE_TURRET:",
    "case BF_TRAINING:",
    "case BF_TURRET:",
    "station.category == STATION_TRAINING",
    "lowestSkillFirst ?",
    "left.relevantSkill < right.relevantSkill",
    "NormalizeStationTaskScalars",
    "TryResolveAndAddStationJobOnce",
    "IsExpectedStationAssignmentSuffix",
    "ProcessStationBundleRemovalRequest",
    "MarkStationDetailChange",
    "RefreshStationActionProjection",
    # Persistent native HUD JOBS entry and deferred manager activation.
    "gui->mainbar->ordersDataPanel",
    "chaseCheckBox",
    '"KJM_HudJobManagerButton"',
    '"Open Job Manager (Ctrl+J)"',
    '"Kenshi_Button1"',
    "TryReadLiveHudManagerButtonGuarded",
    "TryDestroyHudButtonWidgetGuarded",
    "TryEnsureHudManagerIconResourceGuarded",
    "g_hudManagerButtonRequest",
    "RestoreHudManagerButton",
    "g_hudSplitJobsCoord",
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

    # Role icons are intentionally small, opaque, project-style bitmaps. Keep
    # the package contract explicit without requiring Pillow in the validator.
    png_signature = b"\x89PNG\r\n\x1a\n"
    for relative in ("mod/gui/job-engineering.png", "mod/gui/job-medic.png"):
        path = ROOT / relative
        try:
            data = path.read_bytes()
        except OSError:
            continue
        if len(data) < 24 or data[:8] != png_signature:
            fail(errors, f"Role icon is not a valid PNG: {relative}")
            continue
        width = int.from_bytes(data[16:20], "big")
        height = int.from_bytes(data[20:24], "big")
        if (width, height) != (96, 96):
            fail(errors, f"Role icon must be 96x96: {relative} is {width}x{height}")


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
        "src\\SquadSelector.inl",
        "mod\\gui\\job-engineering.png",
        "mod\\gui\\job-medic.png",
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
        "src/HudButton.inl",
        "src/StationScanner.inl",
        "src/StationSettings.inl",
        "src/StationView.inl",
        "src/SquadSelector.inl",
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
            fail(errors, f"Station view must not use the forbidden world-discovery API: {token}")

    if "g_skillEnabled" in source:
        fail(errors, "Per-stat skill filters must not affect Squad Jobs or Stations")

    build_top_skills_start = source.find(
        "void BuildTopSkills(Character* character, std::vector<SkillValue>* skillsOut)")
    if build_top_skills_start >= 0:
        build_top_skills_end = source.find(
            "void ", build_top_skills_start + 8)
        if build_top_skills_end < 0:
            build_top_skills_end = len(source)
        if "g_skillEnabled" in source[build_top_skills_start:build_top_skills_end]:
            fail(errors, "BuildTopSkills must include all supported stats above 1")

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
        "SameQueue(before.jobs, action.sequence)",
        "TryResolveAndAddStationJobOnce(",
        "SameStationQueuePrefix(before, after)",
        "TryValidateStationActionTargetIdentity(",
        "SameQueueWithExactJobRemoved(",
    ):
        if token not in source:
            fail(errors, f"Exact queue revalidation is missing: {token}")

    scanner = read_text("src/StationScanner.inl", errors)
    for forbidden in (
        "if (usable->getDefaultTask() != NULL_TASK)",
        "if (building->getDefaultTask() != NULL_TASK)",
    ):
        if forbidden in scanner:
            fail(
                errors,
                "Direct-owned station relevance must not use a generic default task",
            )

    station_view = read_text("src/StationView.inl", errors)
    for token in (
        "(STATION_GRID_CARD_WIDTH - iconSize) / 2",
        "input->addWidgetModal(g_stationView.modalPanel)",
        "input->removeWidgetModal(g_stationView.modalPanel)",
        "g_stationView.fullRefreshRequested = true",
        "RefreshStationActionProjection(",
    ):
        if token not in station_view:
            fail(errors, f"Station grid safety or layout contract is missing: {token}")

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
        "2,048 final-station cap",
        "enumerate zones",
        "read-only",
        "33% opaque",
        "visual subtype icons",
        "broad station-category filters",
        "stations sort lowest first",
        "Successful assignment changes update only the affected card, detail list,",
        "persistent split-JOBS HUD entry",
        "deferred until after vanilla `updateUT`",
        "kjm-hud-icon.png",
        "compact single-row bottom squad selector",
        "exact raw active/nonempty vanilla `TAB` order",
        "fresh validated `setCurrentPlatoon`",
        "independent horizontal overflow",
        "Plain wheel over the strip",
        "Shift+wheel scrolls the strip",
        "exact selected highlight",
        "80x80",
        "sits at `y=11`",
        "portrait remains",
        "y=11",
        "name, condition, and skills block",
        "48-pixel skills block",
        "120-pixel row",
        "Builder/Engineering, Medic",
        "stored targets for mutation verification",
        "unavailable tint, and station-target artwork",
        "task type",
        "TaskType-driven",
        "job-engineering.png",
        "job-medic.png",
        "FIND_BED_AND_PUT_IN",
        "FIND_AND_RESCUE_IF_THERES_BEDS",
        "RE_Kenshi 0.3.4 or newer",
    ):
        if token not in readme:
            fail(errors, f"README must disclose or document: {token}")

    design = read_text("docs/DESIGN.md", errors)
    for token in (
        "OutpostScanner",
        "Grouped station grid",
        "OptionalPriorityScheduler",
        "Player-station and assignment Stations tab milestone",
        "one stable assigned target",
        "2,048 unique stations",
        "Other / Unclassified",
        "StationVisualSubtype",
        "ImageBox",
        "Training stations sort lowest first",
        "contains only `STATION CATEGORY FILTERS`",
        "projection patches only the affected card, detail list, category counts,",
        "normal fail-closed refresh path.",
        "Native HUD JOBS entry",
        "KJM_HudJobManagerButton",
        "same live root/control still has the plugin's split rectangle",
        "Bottom Squad Jobs selector",
        "exact raw active/nonempty vanilla `TAB` order",
        "fresh validated `setCurrentPlatoon`",
        "independent horizontal overflow",
        "exact selected highlight",
        "value-only",
        "portrait remains `80x80`",
        "frame at `y=11`",
        "skills block starts at `y=8/33/49`",
        "48 pixels high",
        "120-pixel row",
        "!IsStationDisplayJob",
        "exact stored targets remain in the snapshot for mutation verification",
        "target text, arrow, unavailable tint, and",
        "without the incidental target",
        "JOB_BUILDER",
        "JOB_REPAIR_ROBOT",
        "FIND_BED_AND_PUT_IN",
        "FIND_AND_RESCUE_IF_THERES_BEDS",
        "job-engineering.png",
        "job-medic.png",
        "do not enter the station-target cache",
    ):
        if token not in design:
            fail(errors, f"Design roadmap is missing component: {token}")

    testing = read_text("docs/TESTING.md", errors)
    for token in (
        "Same-row drag reorder",
        "Cross-member multi-select and drop-to-remove",
        "Clear Queue Yes/No/Esc modal and fingerprint",
        "Stations tab: player-station card grid and assignment detail",
        "opens a fully populated grid",
        "reach 2,048 unique stations",
        "people or a large red `X`",
        "interior must be fully opaque",
        "33%` opaque",
        "copper ore, iron ore",
        "without any per-skill setting",
        "sort from lowest to highest",
        "confirm only the affected",
        "save",
        "Native HUD JOBS entry",
        "Open Job Manager",
        "original JOBS",
        "JM",
        "Bottom Squad Jobs selector",
        "exact raw active/nonempty vanilla `TAB` order",
        "fresh validated `setCurrentPlatoon`",
        "independent horizontal overflow",
        "Plain wheel over the selector strip",
        "Hold Shift and",
        "exact selected highlight",
        "editable controls, cards, or jobs",
        "read-only unavailable snapshot",
        "The current squad is unavailable",
        "Global behavior cards and exact target verification",
        "Builder/Engineering, Medic",
        "!IsStationDisplayJob",
        "unavailable tint, and station-target artwork",
        "exact stored target retained",
        "without the incidental target",
        "FIND_BED_AND_PUT_IN",
        "FIND_AND_RESCUE_IF_THERES_BEDS",
        "job-engineering.png",
        "job-medic.png",
        "station-target artwork cache",
    ):
        if token not in testing:
            fail(errors, f"Testing checklist is missing coverage for: {token}")

    workshop = read_text("docs/WORKSHOP.md", errors)
    for token in (
        "Kenshi Job Management [RE_Kenshi]",
        "Gameplay`, `GUI`, and `RE_Kenshi`",
        "RE_Kenshi 0.3.4 or newer",
        "Private first upload",
        "KenshiJobManagement.dll",
        "steamapps/workshop/content/233860",
        "Do not make the item public",
    ):
        if token not in workshop:
            fail(errors, f"Workshop release sheet is missing: {token}")

    evidence = read_text(
        "diagnostics/evidence/2026-08-15-station-handle-resolution-success.md",
        errors,
    )
    for token in (
        "complete allocation-free ownership path passed",
        "hand.isValid=true",
        "exact Building handle verified=true",
        "isThePlayer=true",
        "No engine pointer or reference escaped",
    ):
        if token not in evidence:
            fail(errors, f"Stage 8 ownership evidence is missing: {token}")


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
