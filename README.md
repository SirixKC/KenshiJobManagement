# Kenshi Job Management

A native in-game job-management interface for Kenshi, built as a KenshiLib plugin loaded by RE_Kenshi.

## Current milestone: 0.1.0-alpha Stage 1 squad audit/editor + Stations board

Press **Ctrl+J** in game to open a native full-screen editor for the current
squad. This replaces the one-character popup. It audits and edits Kenshi's
existing permanent-job queues; it does not introduce a parallel scheduler.

The native HUD also has a persistent split-JOBS HUD entry: the live vanilla
`JOBS` control keeps its toggle behavior and a square sibling opens Job
Manager. The sibling is sized from the current native control, stays under the
Orders panel, and uses the tooltip `Open Job Manager (Ctrl+J)`. Its click is
deferred until after vanilla `updateUT`; `Ctrl+J` remains the fallback. The
entry is reacquired after a GUI rebuild, and reset restores the original JOBS
rectangle only when the same live control still has the plugin's split
rectangle. The packaged `gui/kjm-hud-icon.png` is loaded independently; the
`JM` caption remains a safe fallback if the texture is unavailable at runtime.

The Stage 1 window provides:

- a fully opaque full-screen backdrop for maximum text contrast;
- one member card for each current-squad member, with per-member live or cached
  state and Kenshi's native generated portrait;
- per-member `Jobs: ON`/`Jobs: OFF` and `Clear Queue` controls;
- narrow, high-contrast job cards that center Kenshi's live work text above `V`
  and the exact target name for station-display jobs, omit duplicate priority
  numbers, and fit long work or renamed-target text without clipping; a
  station-display card with an unavailable target is highlighted red and
  reuses the square station category artwork without stretching it; the icon
  `ImageBox` is directly 33% opaque (67% transparent) behind the queue text,
  and common machinery work is shown compactly as `Operating...`. Global
  Builder/Engineering, Medic, Robotics, and Rescue rows retain their exact
  stored targets for mutation verification but omit target text, arrow,
  unavailable tint, and station-target artwork from the Squad card. Role art
  is TaskType-driven: `JOB_BUILDER` uses optional `job-engineering.png`
  (falling back to `station-other.png`), `JOB_REPAIR_ROBOT` uses the existing
  skeleton-limb icon, and `JOB_MEDIC`, `FIND_AND_RESCUE`,
  `FIND_BED_AND_PUT_IN`, and the live permanent Rescue wrapper
  `FIND_AND_RESCUE_IF_THERES_BEDS` use optional `job-medic.png`. Role art uses the same
  direct 33% card-art alpha as station icons;
- cached gold hover outlines on every card with the same job type and exact
  target for station-display jobs. Global behavior cards compare task type
  only, without the incidental target, and mouse movement never polls
  Kenshi's queues;
- same-row drag reorder, backed by Kenshi's `movePermajob` method;
- multi-select across members and immediate `Remove Selected` or drop-to-remove,
  with no prompt or undo, stop-on-first-failure, and a partial-result report;
- each member's own top three supported base stats above 1;
- an `Options` page with broad station-category filters. These filters apply
  only to Stations; Squad Jobs always shows every job and uses all supported
  stats when selecting each member's top three. Category changes save
  immediately and persist across reload, reinstall, and update;
- native pause on open, prior pause/speed restoration on ordinary close, and
  preservation of a user's resumed speed if they unpause or change speed while
  the manager is open;
- a compact single-row bottom squad selector on `Squad Jobs`. It shows only
  active, nonempty player squads. Its order is the exact raw active/nonempty vanilla `TAB` order;
  it excludes `__DEAD__` and applies the exact selected highlight to the
  current squad;
- selector clicks queue a squad `HandleIdentity` only. The update path performs
  a fresh validated `setCurrentPlatoon` call and rereads the selected identity;
  no `Platoon` or `ActivePlatoon` pointer is retained in UI state;
- the selector has independent horizontal overflow. Plain wheel over the strip
  still scrolls the member rows, while Shift+wheel scrolls the strip; ordinary
  `TAB` remains vanilla-owned and moves the selector highlight after the native
  cycle;
- one-second incremental live-state checks, with no manual Refresh button;
- per-member unloaded or cached queues shown read-only, while other live member
  cards remain editable and a live queue remains editable when one target label
  is unavailable.

The same manager now has a **Stations** tab. It is a category-grouped card grid,
not a member-by-station board. Each card is a verified player-owned
workstation (or an assigned natural resource node, the explicit non-owned
exception). The strict workstation allowlist excludes walls, lights, chairs,
and other objects that only expose a generic task. Exact queue targets are
joined to the same card; Engineer, Medic, Robotics, and Rescue jobs remain on
Squad Jobs because their stored subjects do not define a station scope.

Cards are grouped by category. Every category header shows `+` or `-`, its
station count, and its unassigned count; collapse state persists. Within a
category, unassigned cards appear first, followed by case-insensitive
alphabetical exact station names. A card shows the exact (including renamed)
name, a translucent category icon, the number of unique assigned people, and a
red blocking status only when the station cannot work. A usable station with
nobody assigned has a thin yellow outline and a large red `X`. An idle bench's
zero power allocation is not an outage: `NO POWER` appears only when an active
consumer still has unmet power demand, while a manual switch-off appears as
`POWER OFF`. The area is shown on a card and in the detail view only when more
than one area is present.
Destroyed stations remain visible for cleanup, but new assignment is disabled;
existing verified assignments can still be removed.

Clicking a card opens a wide, fully opaque detail modal. Assigned workers are
listed on the left and available workers are always visible on the right. Each
side has its own scrollbar. Portraits use a fixed column and normal text is
pure white. Both lists sort by relevant skill (highest first), except Training
stations sort lowest first so the workers who benefit most appear first. Ties
then use total permanent jobs and name. Each row shows the worker name, relevant skill
value, and total job count. An OPERATE_STORAGE/hauling row is hidden only when that
same worker also has a non-hauling exact-target station job; otherwise the
hauling assignment remains represented. Click a candidate or press `Enter` for
an immediate, verified assignment. Kenshi's automatic-machine operation uses
its native operation-plus-hauling bundle, and storage defaults are normalized
to `OPERATE_STORAGE`. A station with an unsupported assignment contract keeps
its available-worker pane disabled; the player can still right-click an assigned person
to remove verified existing exact-target station jobs for that worker. There
is no Apply button and no undo.

Successful assignment changes update only the affected card, detail list,
category count, and worker cache. Card positions stay frozen until the detail
modal closes, and the changed card/detail entry receives blue recent-change
feedback. Stale queues, load transitions, and failed verification remain
fail-closed. Squad Jobs keeps its separate same-row drag reorder; Stations has
no drag-to-transfer interaction.

The Squad Jobs roster uses enlarged `80x80` portraits and stacks each member's
three displayed skills vertically in a larger font. The portrait remains
`80x80` and sits at `y=11`; the name, condition, and skills block starts at
`y=8/33/49`, with a 48-pixel skills block in a 120-pixel row, so all three
skill lines clear the member-row border and bottom controls.
While the manager is open, its opaque backdrop keeps
the world from reducing text contrast and mouse-wheel input is reserved for the
manager rather than changing the game camera. On Squad Jobs, the wheel scrolls
vertically over every control and Shift+wheel scrolls the job columns
horizontally. Ctrl-click multi-selection keeps the pointer live even when Ctrl
is also bound to Kenshi's native mouse-rotate control.

The station pass starts when the tab is first opened. It brackets a borrowed
player ownership record copy into plugin-owned scalar data, then resolves the
entire bounded candidate list before showing the grid. Each candidate uses its
own guarded scanner call. Normalized station results append internally in
value-only batches of up to 16, but the card grid is built only once after the
pass. This trades a short button-click pause for a complete board without
streaming or repeated widget rebuilds. Very large ownership lists can take
longer than an ordinary frame. Squad-tab edits mark the board dirty but defer
that synchronous refresh until Stations is opened. The pass also deduplicates exact target handles
copied from readable permanent queues. A direct ownership
candidate appears only when its live building is resolved, its handle identity
matches, it is player-owned, and it passes the strict station allowlist.
Assigned natural resource nodes remain the deliberate non-owned exception, and
assigned player targets with unknown metadata retain the `Other / Unclassified`
fallback. An unloaded or unreadable assigned target stays on the Squad Jobs tab
with its red warning, while Stations omits it. It reports failed assigned
targets, keeps ownership-record validation warnings separate,
and stops at the 2,048 final-station cap with a visible truncation warning. It
does not enumerate zones, towns, or unrelated world buildings. Player-owned
workstations with no readable queue assignment remain visible with a thin
yellow outline and a large red `X`. The borrowed source copy has a separate
8,192-record safety cap.
Kenshi's global Builder/Engineering, Medic, Robotics, Rescue, and Put-in-bed
jobs remain in the Squad Jobs queue and total job count, but they never create
station cards because their stored targets do not define their work scope. Their
exact stored targets remain in the queue snapshot for mutation verification;
Squad card presentation omits target text, arrow, unavailable tint, and
station-target artwork, and hover grouping compares task type without the
incidental target. TaskType-driven role icons are presentation-only and do not
enter the station-target cache or Stations projection.
The shared Options page controls only station-category visibility. Its default
station categories are Crafting, Refining, Farming, Mining, Research, and
Other / Unclassified; Training, Storage / Hauling, and Defense start disabled.

The nine broad station categories use a shared set of simple 2-to-4-color
pictograms: anvil, furnace, wheat, pickaxe, research book, training dummy,
crate, shield, and gear. Exact stable building/functionality identities can
select more specific material or bench artwork for copper ore, iron ore,
UWE copper plates, iron plates, steel bars, copper alloy plates, electronics,
crossbows, and skeleton limbs. The live display name is never used for this
classification, so building-rename mods do not change the icon. Every subtype
falls back to its broad category icon when no stable match is available. The
same artwork identifies building-target jobs on both tabs.

The UI uses exact live task and target data. It does not guess a target from a
job name. Role presets and camera/world highlighting remain later milestones;
station assignment is limited to the verified detail-modal actions above.

The architecture and deferred work are documented in
[`docs/DESIGN.md`](docs/DESIGN.md).

## Validation status

This is an **alpha field-test build**. It has completed repeated in-game tests on
Kenshi Steam 1.0.65 with RE_Kenshi 0.3.4, including squad queue editing,
player-station discovery and assignment, squad switching, portraits, job art,
and UI reset/reopen flows. The included validator checks the source tree,
manifest, project XML, MyGUI dependency, and critical job calls, but it cannot
prove every save or mod combination safe. Keep a backup and test on a
disposable save first.

## Build requirements

- Windows build environment;
- Visual Studio 2019 or newer as the IDE;
- the Visual C++ 2010 **x64** toolset (`v100`);
- RE_Kenshi 0.3.4 or newer;
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
  gui\
    station-crafting.png
    ... eight more broad station-category icons
    ... nine station visual subtype icons
    job-engineering.png
    job-medic.png
    kjm-hud-icon.png

dist\KenshiJobManagement-0.1.0-alpha.zip
```

## Install and first test

Copy the generated folder to `<Kenshi>\mods\KenshiJobManagement\`, enable it in
the launcher, load a disposable save with a small squad, at least one player
outpost, and several Shift-assigned jobs, and press **Ctrl+J**.

Test single-member edits before batch removal and Squad Jobs same-row drag
reorder. After each
action, verify the custom window against Kenshi's vanilla job panel, record the
pause/speed state before and after opening and closing, inspect both tabs, and
read `RE_Kenshi_log.txt`. The complete disposable-save field checklist is in
[`docs/TESTING.md`](docs/TESTING.md).

## Design rule

> New management UI, vanilla queues, vanilla AI.

The plugin should not replace Kenshi's GOAP system. It should translate clearer player intent into ordinary task types and stable target handles, then let Kenshi execute those jobs normally.

## License

GPL-3.0-only, matching KenshiLib's license.
