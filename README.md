# Kenshi Job Management

A native in-game job-management interface for Kenshi, built as a KenshiLib plugin loaded by RE_Kenshi.

## Current milestone: 0.1.0-alpha Stage 1 squad audit/editor + Stations audit

Press **Ctrl+J** in game to open a native full-screen editor for the current
squad. This replaces the one-character popup. It audits and edits Kenshi's
existing permanent-job queues; it does not introduce a parallel scheduler.

The Stage 1 window provides:

- a fully opaque full-screen backdrop for maximum text contrast;
- one member card for each current-squad member, with per-member live or cached
  state and Kenshi's native generated portrait;
- per-member `Jobs: ON`/`Jobs: OFF` and `Clear Queue` controls;
- narrow, high-contrast job cards that center Kenshi's live work text above `V`
  and the exact target name, omit duplicate priority numbers, and fit long
  work or renamed-target text without clipping; a card with an unavailable
  target is highlighted red; building-target cards reuse the square station
  category artwork without stretching it; the icon `ImageBox` is directly
  33% opaque (67% transparent) behind the queue text, and common machinery
  work is shown compactly as `Operating...`;
- cached gold hover outlines on every card with the same job type and exact
  target, without polling Kenshi's queues on mouse movement;
- same-row drag reorder, backed by Kenshi's `movePermajob` method;
- multi-select across members and immediate `Remove Selected` or drop-to-remove,
  with no prompt or undo, stop-on-first-failure, and a partial-result report;
- each member's own top three enabled base stats above 1;
- an `Options` preference with **Sciences + Trades** defaults, saved immediately
  as one global preference across reload, reinstall, and update; if `settings.ini`
  cannot be saved, the current display still applies and reports
  `Displayed-stat options were applied, but settings.ini could not be saved.`;
- native pause on open, prior pause/speed restoration on ordinary close, and
  preservation of a user's resumed speed if they unpause or change speed while
  the manager is open;
- one squad change per `TAB` press in the main manager, while Kenshi's internal
  `__DEAD__` holding squad is skipped;
- one-second incremental live-state checks, with no manual Refresh button;
- per-member unloaded or cached queues shown read-only, while other live member
  cards remain editable and a live queue remains editable when one target label
  is unavailable.

The same manager now has a **Stations** tab. It builds a frozen,
spreadsheet-style matrix for every readable loaded player character across the
player squads. Kenshi's vanilla squad and member order is kept; squads are
grouped and collapsible. Station columns merge player-owned station-relevant
buildings with exact building targets already referenced by those members'
permanent job queues. Areas, categories, exact (including renamed) target
names, relevant skills, permanent queue priorities, Jobs state, and blocking
status are shown. Existing station jobs can be moved between loaded members by
dragging an assignment card. A verified successful transfer patches only the
source and destination rows in place, so the current columns, scan progress,
scroll positions, collapsed squads, and selection remain stable. A full
projection refresh remains the fail-closed fallback for stale, partial, or
externally changed data.

Both rosters use an enlarged portrait and stack each member's three displayed
skills vertically in a larger font. While the manager is open, its opaque backdrop keeps the world from
reducing text contrast and mouse-wheel input is reserved for the manager rather
than changing the game camera.

The station pass starts lazily when the tab is first opened. It brackets a
borrowed player ownership record copy into plugin-owned scalar data, then
resolves no more than one candidate per UI update. It also deduplicates exact
target handles copied from readable permanent queues. A direct ownership
candidate appears only when its live building is resolved, verified as
player-owned, and classified as station-relevant. Assigned natural resource
nodes remain the deliberate non-owned exception, and assigned player targets
with unknown metadata retain the `Other / Unclassified` fallback. An unloaded
or unreadable assigned target stays on the Squad Jobs tab with its red warning,
while Stations omits it. The matrix fills as results arrive and shows an
explicit incomplete-results banner while it runs. It reports failed assigned
targets, keeps ownership-record validation warnings separate, and stops at the
2,048 final-station cap with a visible truncation warning. It does not
enumerate zones, towns, or unrelated world buildings.
Player-owned work stations with no readable queue assignment remain visible
and are marked `UNASSIGNED`.
Kenshi's global Engineer, Medic, Robotics, and Rescue jobs remain in the Squad
Jobs queue and total job count, but they never create station columns or
assignment cells because their stored targets do not define their work scope.
When one worker has several jobs for the same station, the matrix gives each
job its own compact, left-aligned card with the exact queue priority and a compact work
label, for example `1  Hauling...` and `2  Operating...`; the tooltip retains
the full order text. Drag one card to another loaded member row to move that
exact permanent job while keeping its source station. The move appends to the
destination queue. Light gray, 33%-opaque lines separate station columns.
The tab supports frozen roster/headers, vertical and horizontal scrolling, and
virtualized rows and columns. Dragging the station header strip pans it left or
right without selecting a station. The shared Options page controls character skill
filters and station-category filters. Its default station categories are
Crafting, Refining, Farming, Mining, Research, and Other / Unclassified;
Training, Storage / Hauling, and Defense start disabled.

The nine broad station categories use a shared set of simple 2-to-4-color
pictograms: anvil, furnace, wheat, pickaxe, research book, training dummy,
crate, shield, and gear. Exact stable building/functionality identities can
select more specific material or bench artwork for copper ore, iron ore,
UWE copper plates, iron plates, steel bars, copper alloy plates, electronics,
crossbows, and skeleton limbs. The live display name is never used for this
classification, so building-rename mods do not change the icon. Every subtype
falls back to its broad category icon when no stable match is available. The
same artwork identifies building-target jobs on both tabs.

The Stations tab can move an existing exact station assignment between loaded,
readable player-member queues. It cannot create a new station assignment or
change the job target. Camera centering, world highlights, roles, station new
assignment creation, and an unassigned-building toggle remain planned follow-up
work.

The UI uses exact live task and target data. It does not guess a target from a
job name. New assignment creation, role presets, camera/world highlighting,
and an unassigned-building toggle remain later milestones.

The architecture and deferred work are documented in
[`docs/DESIGN.md`](docs/DESIGN.md).

## Validation status

This is a **field-test build**. It has been compiled successfully as an x64 DLL with the VC100 toolchain against KenshiLib 0.4.0, but it has not yet completed in-game testing. The included validator checks the source tree, manifest, project XML, MyGUI dependency, and critical job calls, but it cannot prove runtime hook behavior or the reconstructed `movePermajob` and `removePermajob` index semantics. Keep a backup and test on a disposable save first.

## Build requirements

- Windows build environment;
- Visual Studio 2019 or newer as the IDE;
- the Visual C++ 2010 **x64** toolset (`v100`);
- RE_Kenshi 0.3.1 or newer;
- KenshiLib headers and libraries, including the real Git LFS copies of
  `KenshiLib.lib`, `MyGUIEngine_x64.lib`, and `OgreMain_x64.lib`;
- Boost 1.60 headers (the Boost thread library is not linked).

Set these environment variables:

```powershell
$env:KENSHILIB_DIR = 'C:\path\to\KenshiLib'
$env:BOOST_INCLUDE_PATH = 'C:\path\to\boost_1_60_0'
```

Then build `Release|x64`:

```powershell
.\scripts\build.ps1
```

A successful build creates:

```text
dist\KenshiJobManagement\
  KenshiJobManagement.dll
  KenshiJobManagement.mod
  RE_Kenshi.json
  README.txt
  gui\
    station-crafting.png
    ... eight more broad station-category icons
    ... nine station visual subtype icons

dist\KenshiJobManagement-0.1.0-alpha.zip
```

## Install and first test

Copy the generated folder to `<Kenshi>\mods\KenshiJobManagement\`, enable it in
the launcher, load a disposable save with a small squad, at least one player
outpost, and several Shift-assigned jobs, and press **Ctrl+J**.

Test single-member edits before batch removal and drag reorder. After each
action, verify the custom window against Kenshi's vanilla job panel, record the
pause/speed state before and after opening and closing, inspect both tabs, and
read `RE_Kenshi_log.txt`. The complete disposable-save field checklist is in
[`docs/TESTING.md`](docs/TESTING.md).

## Design rule

> New management UI, vanilla queues, vanilla AI.

The plugin should not replace Kenshi's GOAP system. It should translate clearer player intent into ordinary task types and stable target handles, then let Kenshi execute those jobs normally.

## License

GPL-3.0-only, matching KenshiLib's license.
