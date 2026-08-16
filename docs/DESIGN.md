# Design notes

## Product goal

Turn outpost labor management into a safe audit of the jobs that Kenshi already
owns. Stage 1 is a full-screen, current-squad job audit/editor. The current
Stations milestone adds a second all-squad station matrix and supports guarded
movement of an existing assignment between members without changing the queue
architecture. It replaces the old one-character popup with
one view of every member in the current squad, while preserving Kenshi's
permanent-job queues and GOAP execution.

Stage 1 answers two questions:

1. Which members are in the current squad, and are their Jobs states enabled?
2. What exact job and target does each member have in each queue slot, and in
   what priority order?

It is an audit/editor, not a new scheduler. The plugin presents and edits
Kenshi's live permanent jobs. It does not invent assignments from a station
catalog.

## Non-negotiable architecture rule

**The plugin owns presentation and intent translation. Kenshi owns jobs,
serialization, and AI execution.**

The first implementation writes through the public reconstructed methods on
`Character` and `OrdersReceiver`. It does not mutate `lektor<Tasker*>`
containers directly, create a second queue, or replace GOAP. Task targets are
read through Kenshi's `TaskMatch` adapter and copied into stable `hand` values;
the plugin does not assume a raw `Tasker` subject-field offset or retain a raw
`Tasker` subject pointer across refreshes or mutations.

## Stage 1 full-screen squad audit/editor

`Ctrl+J` opens a native full-screen management window for the current squad.
The window must not open a separate one-character popup for the selected
character. A squad member remains in the view when another member is selected;
the editor is a squad snapshot that refreshes from live game state.

### Layout and labels

The layout is a readable vertical list of member cards or rows. The current
source labels the major controls and states as follows:

- The primary full-screen backdrop is fully opaque. Foreground member rows,
  job cards, controls, portraits, and text remain fully opaque.
- The top bar starts as `Current squad`, then shows
  `<squad name>  |  <N> member(s)`. A squad-level read-only snapshot appends
  `  |  read-only snapshot`.
- The member-column header is exactly `SQUAD MEMBER  |  TOP ENABLED STATS`.
- The priority rail labels columns as `Priority 1`, `Priority 2`, and so on.
  The bottom controls are `Remove Selected (N)`, `Options`, and `Close`; while
  dragging, the remove target reads `DROP TO REMOVE (N)`.
- Each member header shows the live member name, the live `Jobs: ON` or
  `Jobs: OFF` state, that member's `Jobs` toggle, and that member's `Clear
  Queue` control. Its portrait uses Kenshi's generated character image and
  the same background/overlay depth order as the vanilla portrait layout.
- Each member independently shows up to three of its own enabled base stats
  whose values are above 1. The three stats are selected per member; they are
  not the top three stats across the squad and are not an aggregate squad
  value. The three entries are stacked vertically in a 16-pixel font beside
  an enlarged 80-pixel portrait.
- Each queue slot is a narrow, high-contrast selectable and draggable card. It has no
  repeated `DRAG` label and no priority number inside the card; priority is
  shown once in the column header. A target-bearing card centers this live text:

  ```text
  <Kenshi's live job text>
  V
  <live target name>
  ```

  The first line is Kenshi's live job text with only its duplicate leading
  `<priority>:` presentation prefix removed. The target line is reduced only as
  needed to fit a narrow card. Keep the unmodified raw text in the snapshot for
  row identity and mutation validation. Do not replace a target with a guessed
  building category or combine two distinct targets that
  share a visible job name. Long work text wraps or uses a smaller stock font
  instead of clipping. The target line has no synthetic `Target:` prefix. If
  Kenshi cannot resolve that target, it shows `Target unavailable`, highlights
  the whole card red, and keeps the otherwise-live row editable. A member with
  no jobs shows `[No permanent jobs]`. An unavailable queue shows `Queue
  unavailable (read-only)` and is read-only.
- A building-target card centers the same station-category artwork in an
  `88 x 88` square behind its text and applies the same dark overlay as the
  Stations header. The icon `ImageBox` itself is set to `33%` opacity, which
  makes it `67%` transparent; the overlay has its own independent tint and
  depth. It never stretches the square texture across the wider card. The
  nine broad categories use one simple 2-to-4-color pictogram set: anvil,
  furnace, wheat, pickaxe, research book, training dummy, crate, shield, and
  gear. Exact stable building/functionality identities can select visual
  subtypes for copper ore, iron ore, UWE copper plates, iron plates, steel bars,
  copper alloy plates, electronics, crossbows, and skeleton limbs. The live
  display name is never used for subtype classification, so rename mods do not
  change the artwork. An unmatched subtype falls back to its broad category
  icon. Exact target categories are cached and resolved at no more than one
  uncached queue target per UI tick; card creation does not enumerate the
  world or perform an unbounded target pass. `Operating Machine` and
  `Operating Automatic Machine` display as `Operating...`, while the raw live
  text remains unchanged for tooltips, identity, and mutation validation.
- Hovering a job card draws a gold outline around every displayed card with
  the same `TaskType` and stable target identity. The group index is rebuilt
  from the captured squad snapshot only when the window opens or a roster or
  job identity changes. Mouse movement never reads Kenshi's live queues.
- A member with no qualifying displayed stats shows `No enabled stats above 1`;
  a cached or unloaded member shows `Stats unavailable`.
- The footer has no persistent instruction or current-squad message. It remains
  available for actionable mutation results, settings failures, and the first
  failure in a batch. There is no manual Refresh button; live changes arrive
  through the incremental checks.

The top queue slot is the highest Kenshi priority. Empty queues show
`[No permanent jobs]`, and unavailable controls are visibly disabled.

### Edits and selection

Each member has independent `Jobs` and `Clear Queue` controls. Toggling `Jobs`
calls Kenshi's native Jobs state for that member. `Clear Queue` opens a
`Confirm Clear Queue` modal. Its body is
`Are you sure you wish to remove all jobs?` followed by
`Remove all <N> permanent jobs from <member>?`; its buttons are exactly `Yes`
and `No`, and `Esc` closes it. The modal shows the member name and reviewed row
count, then validates a fingerprint of the full ordered queue immediately
before accepting `Yes`; the fingerprint covers every row, not only the
reviewed count.
If the queue changed, the clear is rejected and the view is refreshed. `Clear
Queue` never clears immediate orders or another member's queue.

`Options` changes one global preference and saves it immediately. The preference
persists across reload, reinstall, and update. Its defaults are **Sciences**
and **Trades**, and it controls which base stats are eligible for each
member's own top-three display. If the write fails, the current display still
updates and the status line shows exactly: `Displayed-stat options were applied,
but settings.ini could not be saved.`

The Options modal is titled `Displayed Stats`; its help text is `Each member
shows their three highest enabled base stats above 1. Changes save globally
now.` Its controls are `Select All`, `Clear All`, `Reset Default`, and `Close`.

The list supports multi-selection across members. A selected item is an exact
member-plus-queue-row identity, not just a visible row number or job name.
`Remove Selected` removes those exact rows immediately, with no prompt and no
undo. Dropping a row onto the remove target has the same immediate,
irreversible behavior. Each operation revalidates the member, queue, row
identity, job data, and target immediately before removal. It stops at the
first failed removal. It must report which removals succeeded, which item
failed, and which items remain selected; it must not claim that later items
were removed.

Reordering uses same-row drag only. A job may be dragged to another position
in that same member's queue and is translated to Kenshi's native
`movePermajob` operation. Dragging between member rows is not an assignment
operation and must be rejected. The editor revalidates the source and target
positions before moving. After a fully verified successful move, the existing
station projection is patched in place for the source and destination rows.
This preserves station columns, scan progress, filters, collapsed squads,
scroll positions, and selection. A normal full refresh remains the fail-closed
fallback when verification is partial, a member disappears, or the projection
cannot be patched.

### Game controls and refresh

The manager invokes Kenshi's native pause when it opens and records the prior
pause and speed state. On an ordinary close, it restores that prior state. If
the user unpauses or changes speed while the manager is open, the manager
records that as an explicit requested resume and closes while preserving the
user's requested pause/speed instead of restoring the old state.

One `TAB` press performs exactly one manager-owned current-squad cycle. If the
next squad is Kenshi's internal `__DEAD__` holding squad, continue through the
native order until the next manageable squad. Never render `__DEAD__`. An
`Options` modal or `Clear Queue` Yes/No/Esc modal owns keyboard input and
blocks TAB from cycling the current squad until the modal closes.

While the window is open, perform an incremental live-state check once per
second. Compare squad membership, loaded state, Jobs state, each member's
stats, queue size, queue row identity, labels, and order. Update only changed
member cards or rows; do not rebuild or mutate unchanged cards every tick.
There is no manual Refresh button.

If one squad member is unloaded or only its cached queue can be shown, replace
that member's live snapshot with its own last-safe cached snapshot and mark
only that member read-only. Its condition is `Cached / unavailable`, its
`Jobs`, `Clear Queue`, drag, and removal controls are disabled, and other live
member cards remain editable. If the whole active squad cannot be enumerated,
cached members use `Cached / unloaded` and the heading adds
`read-only snapshot`; a missing cache uses `Unavailable member` and
`Unavailable`. A live member with an unresolved target is not read-only: show
`Target unavailable` and keep safe queue actions enabled. Never call
reconstructed engine methods on an unloaded or cached object and never use a
stale pointer after a load, reset, upgrade, or dismantle. Return each card to
live editing only after a fresh safe resolution.

## 0.1 adapter and Stage 1 implementation boundary

The initial adapter proves safe squad-wide queue inspection and mutation before
outpost discovery is added:

1. Hook `PlayerInterface::updateUT`, which runs on the UI thread.
2. Hook `PlayerInterface::clearAndReset` to destroy plugin widgets and cached
   member state at game-state transitions.
3. Hook `PlayerInterface::cycleSquad`: pass through native current-squad
   cycling on the main manager, but block it while the Options or Clear Queue
   modal owns keyboard input.
4. Edge-detect `Ctrl+J` and create the native full-screen MyGUI window.
5. Resolve the current squad and each member through stable `hand` values.
6. Snapshot each loaded member's permanent jobs using
   `getPermajobCount`, `getPermajobName`, `getPermajob`, and
   `getPermajobData`.
7. Pass each transient task through Kenshi's `TaskMatch` adapter and copy its
   subject to a stable target `hand`; do not inspect a raw `Tasker` subject
   offset or retain a raw subject pointer.
8. Render exact current UI labels, member-specific top-three base stats, and
   per-member read-only fallbacks for unloaded or cached queues.
9. Revalidate the selected member and exact row immediately before every
   toggle, clear, remove, batch remove, or move. A live queue remains editable
   when only its target label is unavailable.
10. Call `movePermajob`, `removePermajob`, `clearPermajobs`, or
   `OrdersReceiver::setJobsEnabled` through guarded wrappers.
11. Apply `Remove Selected` and drop-to-remove immediately, without a prompt or
   undo. Stop a cross-member removal batch on its first failure and report the
   partial result.
12. Show `Clear Queue` as a Yes/No/Esc modal with the member name and reviewed
   count, and validate the full queue fingerprint before clearing.
13. Save `Options` changes immediately to one global preference and load that
    preference across reload, reinstall, and update.
14. Report the exact settings write failure while retaining the current-session
    display change: `Displayed-stat options were applied, but settings.ini could
    not be saved.`
15. Invoke native pause on open, restore the prior pause/speed on ordinary
    close, and preserve an explicit user-requested resumed speed.
16. Give each `TAB` edge exactly one manager-owned cycle on the main manager,
    skip the internal `__DEAD__` squad, and let Options and Clear Queue modals
    block it. Run incremental checks at one-second cadence without a manual
    Refresh button.
17. Ask the character to reconsider its current AI action and flag the vanilla
    selection UI for refresh after a successful mutation.
18. Keep MyGUI calls on the UI thread during
    `PlayerInterface::clearAndReset` cleanup.

Every engine read or write that dereferences reconstructed runtime objects
lives in a small SEH wrapper. MyGUI calls remain on the UI thread.

## Player-station and assignment Stations tab milestone

The manager has a second tab, `Stations`, beside `Squad Jobs`. Opening the
manager always starts on `Squad Jobs`; switching to `Stations` is explicit.
The station tab is primarily an information view. It can move one existing
exact station assignment between two readable loaded member queues. It cannot
invent a job, change its target, directly reorder the destination, center the
camera, highlight buildings in the world, or expose roles.

### Matrix and roster

The station view is a frozen spreadsheet-style matrix:

- Columns are all qualifying stations in every currently loaded area.
- Rows contain every loaded player character across every loaded squad.
- Squad groups and member rows keep Kenshi's vanilla faction, squad, and member
  order. Loaded squads start expanded. Unloaded squads start collapsed with a
  `Live data unavailable` state; a partially loaded squad has a single
  `<N> members unavailable` placeholder.
- The left roster is frozen while the station columns scroll horizontally. The
  area/category headers and station headers are frozen while member rows scroll
  vertically. Both axes use virtualized widgets so only visible rows and
  columns are created. The station header strip also supports left-button
  click-and-drag panning; a meaningful drag is not treated as a station click.
- A member row keeps the portrait, name, condition, Jobs state, permanent-job
  count, and the same filtered top-three skills used by `Squad Jobs`. It shows
  `NO PERMANENT JOBS` in red for an empty queue, `N JOBS` otherwise, and a
  `JOBS OFF` badge when the vanilla Jobs state is off. A queue that cannot be
  read remains visible as `JOBS UNAVAILABLE`; this does not disable viewing or
  hide the worker. Its top-three skills and portrait use the same enlarged
  vertical layout as the Squad Jobs roster.

### Player-owned and assigned targets and ordering

The board merges two exact sources. It brackets the player faction's borrowed
ownership records into plugin-owned scalar POD, then reconstructs at most one
record per UI update. It also copies exact building targets from readable
permanent queues belonging to loaded player members and deduplicates those
stable handles. A direct ownership candidate is included only after a valid
handle, loaded Building, exact live-handle identity, player ownership, and
station-relevant classification all pass. Assigned natural resource nodes are
the deliberate non-owned exception. Assigned player targets whose metadata is
unknown retain the `Other / Unclassified` fallback. An unresolved or unloaded
assigned target remains visible in Squad Jobs but is omitted here because the
live target cannot be proven.
Loaded assigned targets remain visible when destroyed, incomplete, unpowered,
broken, disabled, or dismantling.

No zone container, town list, or unrelated world-building list is traversed. No
borrowed ownership source pointer or live building, squad, or character pointer
is cached between resolution steps. Direct-owned non-work buildings are
excluded. Public and city buildings are excluded even when a player queue
references them, except for assigned natural resource nodes. This avoids
city-wide noise while allowing the board to show unassigned player stations.

The station projection excludes the exact generic permanent task types
`JOB_BUILDER` (Engineer), `JOB_MEDIC` (Medic), `JOB_REPAIR_ROBOT` (Robotics),
and `FIND_AND_RESCUE` (Rescue). Their stored subjects do not limit the scope of
those jobs. They remain visible and editable in Squad Jobs and remain included
in the worker's total permanent-job count, but they never create a station
column or assignment cell. Filter these task types, not localized job labels.

Columns are ordered by area name, then category, then exact station name. The exact
Kenshi name is used, including a name supplied by a building-rename mod. If
two stations in one area have the same name, the presentation adds `#1`, `#2`,
and so on without renaming the engine objects. The station categories are:

`Crafting`, `Refining`, `Farming`, `Mining`, `Research`, `Training`,
`Storage / Hauling`, `Defense`, and `Other / Unclassified`.

Each station card shows its category label, exact name, and relevant
skill. A station with no known skill says `Relevant skill: None`; an unknown
classification says `Relevant skill: Unknown`. A known-skill station is visible
only when its skill is enabled in the shared Options page. Unknown-skill
stations are controlled by the `Other / Unclassified` category checkbox.
Category filters affect only the station view. Skill filters affect both the
station columns and the top-three worker skills.

### Assignment cells and status

An empty cell means no detected permanent assignment. An assigned cell shows
one compact, left-aligned card per permanent job using the exact queue priority and a
compact work label, such as `1  Hauling...` and `2  Operating...`. The full
order text remains in the cell tooltip. Light gray, 33%-opaque divider lines
mark the station-column gaps in both the frozen headers and scrolling matrix.
The cell also shows the worker's station-relevant permanent base skill, even when that skill
is not in the worker's top three or is one or lower. Hovering a cell shows the
exact job text, station name, priority, squad, relevant skill/value, and any
blocking station status. Hovering a station header highlights its column and
the corresponding worker row. Up to five jobs are shown directly; a larger
pathological queue adds an explicit `+N more jobs` row, with every job retained
in the tooltip. A short card click keeps the existing station-column selection
behavior. Dragging one exact card to another visible loaded member row moves
that assignment to the destination member and keeps the source building as its
target. A drop can use either the frozen roster row or any matrix cell in that
row.

Assignment-card movement is deferred to the next manager update. No engine
method runs inside a MyGUI callback. The action stores stable handles and full
source/destination queue snapshots. It then reacquires both loaded characters,
validates each full queue fingerprint independently, rejects an identical
task-type-plus-target job already on the destination, and resolves a fresh
building pointer. It calls `Character::addJob(task, building, true, true,
buildingPosition)` to append a permanent destination job without clearing the
queue. The destination gain is verified before the exact source row is
removed. If removal fails, the destination job remains and a nonblocking
partial-failure message tells the player that a duplicate can remain. The
plug-in does not attempt rollback. If both queue mutations and their
post-mutation reads succeed, the projection is updated directly from those
verified snapshots; it does not restart the target scan or rebuild unrelated
station columns. Partial or stale results use the full refresh fallback.

While the manager exists, the update hook clears Kenshi's camera-wheel input
before vanilla UI processing. MyGUI continues to receive wheel input for the
manager's own scroll views.

Blocking states are red only when they prevent work. Direct-owned stations can
appear without an assignment and show `UNASSIGNED` in the station header;
unreadable queues remain visible as unavailable worker data but cannot
contribute assigned target columns.

### Lazy target resolution and safety limits

The station candidate pass starts only when `Stations` is opened for the first
time. Each UI update consumes no more than one stable assigned target or one
copied ownership record. It pauses when the tab is hidden and resumes when the
tab is shown again. Results fill the matrix progressively and the banner
remains prominent until the pass finishes:

```text
READING PLAYER STATION CANDIDATES - RESULTS INCOMPLETE
Candidate <N> of <M>
```

If an assigned target cannot be read, resolution continues and shows a red
incomplete warning with the failed-target count. An unloaded ownership record is
silently omitted; a validation fault reports one ownership-pass warning. The
final station result stops at 2,048 unique stations, while the borrowed source
copy is separately capped at 8,192 records. Reopening the manager starts a new
player-station session. Queue changes from
`Squad Jobs` rebuild both the target list and assignment joins. A successful
Stations transfer is the exception: its verified source and destination rows
are patched without restarting the target pass. External queue changes,
failed verification, or a load transition use the normal full refresh path.
Jobs state and condition refresh at the existing one-second cadence. Changing a
filter rebuilds visible columns immediately.

The shared Options page has clearly separated `CHARACTER SKILL FILTERS` and
`STATION CATEGORY FILTERS` sections and saves changes immediately. Station
category defaults are enabled for Crafting, Refining, Farming, Mining,
Research, and Other / Unclassified; Training, Storage / Hauling, and Defense
are disabled. Both tabs open this same page.

## Why the Squad Jobs tab remains squad-wide while Stations is all-squad

Queue mutation is independent of building ownership, active-zone lifetime,
machine function, valid task-target pairings, and multiple-base selection. A
full-screen squad audit gives players a complete, concrete view of existing
vanilla queues. The Stations matrix can then join those exact queues
to discovered world targets without making the editable queue UI depend on
station discovery.

The following are explicitly later work:

- creating a new station job when no source assignment exists;
- station-cell removal or station-side priority reordering;
- role presets or abstract role scheduling;
- an unassigned-building highlight or demand scanner;
- camera centering and world-space station highlighting;
- automatic station target assignment and an optional priority scheduler.

## Planned components after Stage 1

### CharacterJobAdapter

Reads a queue into immutable rows and exposes guarded operations:

- move an exact row within one member;
- remove an exact row;
- remove an exact selected set immediately, with no prompt or undo, stopping
  on failure;
- clear one member's queue after the full-fingerprint Yes/No/Esc modal;
- toggle Jobs;
- later, add a validated task and target.

Rows retain copied task type, Kenshi's exact current job text,
target availability/label, and the stable target `hand` produced by the
`TaskMatch` adapter. They retain the raw task address only as a numeric,
never-dereferenced identity token for the current snapshot. They do not retain
a raw `Tasker` subject pointer or depend on a raw subject-field offset. Queue identity and full-queue
fingerprints are rechecked through fresh guarded reads immediately before
mutation.

### Ownership scanner boundary

The earlier `OutpostScanner` name now refers to this bounded production
scanner; it does not imply a zone-wide world walk.

The production scanner uses the validated borrowed ownership-record path. It
copies only the source vtable address and five scalar hand fields into
plugin-owned storage. It never asks Kenshi to populate an output container and
never frees or retains the borrowed source. The ownership source copy is capped
at 8,192 records; the final normalized station result remains capped at 2,048.
Each copied record is reconstructed and checked with `isValid`, `getBuilding`,
an exact `Building::getHandle` identity comparison, and `isThePlayer` before
metadata is read. A failed loaded-object check is omitted as an expected live
inventory gap; validation faults are reported once as an incomplete ownership
pass. World highlights, roles, new assignment creation, and an unassigned
building toggle remain deferred.

The current normalized station result contains a stable target handle, area
identity/name, category, exact display name, relevant skill, and blocking
status, plus the exact permanent-job assignments joined from loaded player
characters. It preserves renamed target names, excludes destroyed or invalid
targets, caps the result at 2,048, and reports an assigned-target failure
without aborting later candidates.

Its normalized target identity is represented by:

```cpp
struct WorkTarget
{
    hand target;
    BuildingFunction function;
    TaskType recommendedTask;
    std::string displayName;
    std::string areaName;
    StationCategory category;
    StationVisualSubtype visualSubtype;
    std::string relevantSkillName;
    bool blocking;
    std::string blockingStatus;
    std::vector<StationAssignmentSnapshot> assignments;
};
```

The scanner must prefer stable `hand` values over long-lived raw building
pointers. It should reacquire objects on refresh because zones unload,
buildings upgrade, and structures are dismantled.

### JobRuleRegistry

Maps functional building categories to ordinary player job types, then permits
exceptions by FCS identity. Generic function-based matching should cover normal
modded production buildings without a name list; UWE/Kaizo oddities can live in
small override data rather than in UI code.

### AssignmentMatrix

The current matrix contains rows for every loaded player character
across all loaded squads and columns for discovered stations. It keeps the
vanilla squad/member order, groups rows by squad, and joins cells by exact
target handle and queue priority. It supports shared skill/category filters,
progressive scan warnings, frozen headers, and virtualized horizontal/vertical
scrolling. Cell and short card clicks select/highlight a column. Assignment
cards can move an existing exact job to another loaded member through the
validated add-verify-remove action. The matrix does not invent a task or target
and does not reorder existing destination jobs.

### OptionalPriorityScheduler

A later RimWorld-style category matrix may convert abstract priorities such as
Farming 1 or Hauling 3 into concrete target jobs. It must remain optional and
must not continuously fight Kenshi's own decisions.

## Safety rules

- Never touch MyGUI from a worker thread.
- Never retain an unvalidated engine pointer across a load/reset transition.
- Resolve task subjects through the `TaskMatch` adapter; never assume a raw
  `Tasker` subject offset or retain a raw task/subject pointer in UI state.
- Never mutate the permanent-job container directly when a game method exists.
- Revalidate member and queue-row identity immediately before removing or
  moving it.
- Treat destroyed, upgraded, unloaded, or replaced targets as expected state
  changes. A missing target label does not make an otherwise live queue
  read-only; unloaded or cached queues remain read-only until reacquired.
- Cap one rendered queue at 64 rows. Larger queues show the first 64 rows as
  an explicit read-only safety view, so malformed state cannot create an
  unbounded MyGUI widget tree.
- Stop a multi-member removal batch at the first failure and preserve the
  remaining selection.
- Keep same-row Squad Jobs reorder separate from cross-member station transfer.
- Defer every station-card transfer outside MyGUI callbacks. Reacquire stable
  member/building handles and verify the full source and destination queues
  independently before mutation. Add and verify the destination first; remove
  only the exact source row afterward. Never roll back a verified destination
  add if source removal fails. After a fully verified success, patch only the
  affected station roster rows and preserve the existing board projection;
  use a full refresh only when the patch preconditions fail.
- Build station columns from verified player-owned station-relevant building
  records and exact building targets in readable loaded player queues. Do not
  enumerate zones or unrelated world containers.
- Copy only scalar ownership-record fields into plugin-owned storage. Bracket
  the borrowed source header, reconstruct one candidate per UI update, and
  require `isValid`, `getBuilding`, exact live-handle identity, and
  `isThePlayer` before reading direct-owned metadata. Never retain source or
  engine object pointers between refreshes or game-state transitions.
- Bound the borrowed ownership copy at 8,192 records and the final station
  result at 2,048 unique stations. Resolve one stable candidate per UI update
  and show incomplete results until resolution completes. A failed assigned
  target must not abort later candidates.
- Reopen the manager to start a fresh player-station candidate pass. Do not
  retain raw `Building`, `Platoon`, or `Character` pointers between refreshes
  or game-state transitions.
- Leave combat, rescue, self-preservation, and immediate orders to vanilla
  Kenshi.
- `Remove Selected` and drop-to-remove are immediate, irreversible actions with
  no prompt or undo; exact row revalidation is their safety gate.
- Require the Yes/No/Esc full-fingerprint modal for `Clear Queue`.
- If settings persistence fails, keep the in-session display change but show the
  exact `Displayed-stat options were applied, but settings.ini could not be
  saved.` status.
- Invoke and restore native pause/speed according to the open/close rules, and
  preserve a user's explicit resumed speed.
- Test on disposable saves until save/load/import behavior is proven.

## Roadmap

### 0.1: Stage 1 squad audit/editor field test

- full-screen current-squad window;
- Kenshi's live job text and exact live target names;
- per-member Jobs toggle and full-fingerprint Clear Queue modal;
- same-row drag reorder;
- cross-member multi-select and drop-to-remove with stop-on-failure and no
  prompt or undo;
- each member's top three enabled base stats above 1;
- immediate global Options preference, default Sciences + Trades;
- native pause/speed open/close behavior and Kenshi `TAB` cycling;
- unloaded/cached read-only fallback, but editable live queues with unavailable
  target labels;
- one-second incremental refresh and transition cleanup;
- source validator and build package.

### 0.1: Player-station and assignment Stations matrix field test

- second `Stations` tab with the manager opening on `Squad Jobs`;
- all loaded player characters across all loaded squads, grouped in vanilla
  squad/member order;
- verified player-owned station-relevant buildings plus exact loaded building
  targets referenced by readable player permanent jobs;
- exact renamed station names, categories, relevant skills, queue priorities,
  Jobs state, conditions, and blocking status;
- stable visual subtypes for material-output stations and special benches, with
  broad-category fallback artwork and rename-independent classification;
- shared skill filters and station category filters with the documented defaults;
- lazy one-candidate-per-update enrichment with progressive results, ownership
  validation warning, assigned-target failure warning, and 2,048-result
  truncation warning;
- frozen roster/headers, virtualized vertical/horizontal scrolling, station
  category labels, short-click column selection, and guarded assignment-card
  transfer between loaded members with an in-place verified-success projection
  patch and a fail-closed full-refresh fallback;
- disposable-save field checklist and source validation for the new files.

The production ownership pass can show unassigned player stations. Complete
world highlights, safe full-world discovery, and the unassigned world-highlight
toggle remain gated on separate validated contracts.

### 0.2: reliable target assignment

- inspect a selected world object;
- ask Kenshi for valid player-task probability;
- add one concrete permanent job through the game method;
- display target identity and validation failures.

### 0.3: editable assignment board

- station cell assignment and removal through validated native task methods;
- batch assignment to selected workers;
- queue conflicts and duplicate warnings;
- role presets that expand into concrete jobs;
- camera centering and unassigned-building highlight.

### 0.4+: optional scheduling

- abstract work-category priorities;
- demand detection and worker selection;
- skill/distance recommendations;
- multiple loaded outposts and diagnostics.
