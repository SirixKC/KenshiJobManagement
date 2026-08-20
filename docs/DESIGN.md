# Design notes

## Product goal

Turn outpost labor management into a safe audit of the jobs that Kenshi already
owns. Stage 1 is a full-screen, multi-squad job audit/editor. The current
Stations milestone adds a category-grouped station card grid and a guarded
detail-modal assignment view without changing the queue architecture. It
replaces the old one-character popup with grouped views of every member in all
active, nonempty player squads, while preserving Kenshi's permanent-job queues and GOAP
execution.

Stage 1 answers two questions:

1. Which members are in each active player squad, and are their Jobs states enabled?
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

`Ctrl+J` opens a native full-screen management window for all active, nonempty
player squads.
The window must not open a separate one-character popup for the selected
character. A squad member remains in the view when another member is selected;
the editor is a squad snapshot that refreshes from live game state.

### Native HUD JOBS entry

The plugin keeps one persistent split-JOBS HUD entry beside Kenshi's live
`OrdersChaseButton`. It resolves `gui->mainbar->ordersDataPanel->chaseCheckBox`
from the current native Orders panel on every update, then creates a square
`KJM_HudJobManagerButton` sibling under that same Orders panel. The square uses
the live JOBS height and leaves at least 40 pixels for the native JOBS label;
incompatible or missing geometry fails closed and leaves `Ctrl+J` available.
The sibling has independent enabled state, inherits parent visibility, and
uses `Open Job Manager (Ctrl+J)` as its tooltip. Its callback sets only a
deferred request flag; the request is consumed after vanilla `updateUT` through
the existing `ToggleJobWindow` path.

GUI rebuilds abandon stale bindings and reacquire the current root and direct
child by name. Cleanup restores the saved native JOBS rectangle only if the
same live root/control still has the plugin's split rectangle; UI-modified
geometry is left untouched. The packaged `kjm-hud-icon.png` resource is loaded
independently from mandatory station assets, with a `JM` caption fallback.

### Layout and labels

The layout is a readable vertical list of member cards or rows. The current
source labels the major controls and states as follows:

- The primary full-screen backdrop follows the appearance setting. Vanilla mode
  is a warm light Kenshi-style surface with dark text; Dark UI mode keeps the
  existing dark surface and light text. Foreground member rows, job cards,
  controls, portraits, and text remain fully opaque.
- The top bar starts as `Current squad`, then shows
  `<squad name>  |  <N> member(s)`. A squad-level read-only snapshot appends
  `  |  read-only snapshot`.
- The member-column header is exactly `SQUAD MEMBER  |  TOP STATS`.
- The priority rail labels columns as `Priority 1`, `Priority 2`, and so on.
  The bottom controls are `Remove Selected (N)`, `Add Healing Jobs...`,
  `Prioritize Healing`, `Options`, and `Close`; while dragging, the remove
  target reads `DROP TO REMOVE (N)`. Batch results use a large, nonblocking
  toast that fades after about four seconds.
- Each member header shows the live member name, the live `Jobs: ON` or
  `Jobs: OFF` state, that member's `Jobs` toggle, and that member's `Clear
  Queue` control. Its portrait uses Kenshi's generated character image and
  the same background/overlay depth order as the vanilla portrait layout. The
  portrait remains `80x80` with its frame at `y=11`; the name, condition, and
  skills block starts at `y=8/33/49` and is 48 pixels high in a 120-pixel row,
  so all three skill lines clear the member-row border and bottom controls.
- Each member independently shows up to three of its own supported base stats
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
  unavailable (read-only)` and is read-only. This target-bearing presentation
  applies only when `IsStationDisplayJob` is true. Global Builder/Engineering,
  Medic, Robotics, and Rescue rows are identified by `!IsStationDisplayJob`:
  their exact stored targets remain in the snapshot for mutation verification,
  but Squad card presentation omits target text, arrow, unavailable tint, and
  station-target artwork. Role artwork is selected by exact TaskType: Engineer
  (`JOB_BUILDER`) uses optional `job-engineering.png` with the `station-other.png`
  fallback; Robotics (`JOB_REPAIR_ROBOT`) uses the existing skeleton-limb icon;
  Medic (`JOB_MEDIC`), Rescue (`FIND_AND_RESCUE`), Put in bed
  (`FIND_BED_AND_PUT_IN`), and the live permanent Rescue wrapper
  (`FIND_AND_RESCUE_IF_THERES_BEDS`) use optional `job-medic.png`. These role icons use
  the same direct 33% card-art alpha and do not enter the station-target cache.
  Their incidental subjects never create a Stations projection.
- A station-display building-target card centers the same station-category artwork in an
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
  icon. Exact target categories are cached. Card creation resolves up to 512
  deduplicated current-squad targets synchronously before the first frame
  renders; extraordinary overflow uses the neutral Other artwork immediately.
  It does not enumerate ownership or the world. `Operating Machine` and
  `Operating Automatic Machine` display as `Operating...`, while the raw live
  text remains unchanged for tooltips, identity, and mutation validation.
- Hovering a job card draws a gold outline around every displayed
  station-display card with the same `TaskType` and stable target identity.
  For global rows identified by `!IsStationDisplayJob`, hover grouping compares
  task type without the incidental target. The group index is rebuilt from the
  captured squad snapshot only when the window opens or a roster or job identity
  changes. Mouse movement never reads Kenshi's live queues.
- A member with no qualifying displayed stats shows `No stats above 1`;
  a cached or unloaded member shows `Stats unavailable`.
- The footer has no persistent instruction or current-squad message. It remains
  available for actionable mutation results, settings failures, and the first
  failure in a batch. There is no manual Refresh button; live changes arrive
  through the incremental checks.

The top queue slot is the highest Kenshi priority. Empty queues show
`[No permanent jobs]`, and unavailable controls are visibly disabled.

### Grouped multi-squad board

The Squad Jobs board copies the raw active-platoon order and publishes a
value-only `AllSquadsSnapshot`. Empty squads and `__DEAD__` are omitted. Each
squad has a synchronized header in both panes with a `+`/`-` collapse control;
collapsed groups create no member or queue rows. Collapse state is keyed only
by `HandleIdentity`, lasts for the manager session, and is pruned when a squad
leaves the active roster. Header callbacks defer the rebuild until the callback
returns, because the rebuild destroys the clicked widget tree. Expanded groups
have a 60-pixel gap (half of the 120-pixel member row) before the next squad,
which keeps group boundaries readable without blocking controls.

Every member widget has a parallel value-only squad/member identity binding.
Callbacks resolve that identity against the latest `g_allSquads` snapshot;
they never treat a visible row number as a durable engine identity. The current
squad remains `g_squad`, follows vanilla `TAB` or the bottom selector, and has
the selected group highlight. Selecting a squad does not hide the other groups.
`RefreshAllActiveSquadsSnapshot` runs on the one-second refresh. A failed roster
read retains the cached board but marks every cached row read-only before the UI
is rebuilt, so a stale editable row is never published.

### Bottom Squad Jobs selector

The `Squad Jobs` tab has a compact single-row selector along the bottom of the
member view. It copies the player's raw active-platoon list without sorting it,
then keeps only valid, active, nonempty squads. The selector preserves the exact raw active/nonempty vanilla `TAB` order. Empty entries, duplicate identities, and Kenshi's internal
`__DEAD__` holding squad are excluded. The selector is not a second squad
ordering or scheduler. Its button caption is the copied squad name, with the
stable string ID as the guarded fallback.

The published selector record is value-only: `HandleIdentity`, copied name, and
member count. It never retains a `Platoon*` or `ActivePlatoon*` pointer. A click
queues the selected identity and returns from the MyGUI callback. The update
path then performs a fresh active-list validation, checks faction, identity,
nonempty state, and `__DEAD__` state, calls guarded `setCurrentPlatoon`, and
rereads the current identity before refreshing the member view. If any check
fails, the current squad remains unchanged and the selector reports that the
target is no longer available.

The selector owns its own horizontal canvas, viewport, and scrollbar. This
independent horizontal overflow does not change the member-row scroll position.
Plain wheel over the strip still routes to the member rows; Shift+wheel scrolls
only the selector strip. Raw MyGUI wheel values are normalized to one small
notch before applying the scroll step, so a single physical wheel tick cannot
jump to the top or bottom. Scroll positions clamp at both ends and wheel input
never activates a selector button. The selector's plain and Ctrl-click actions
also manage whole-squad recipients: plain switches current squad and selects
all its members; Ctrl toggles all its members without switching current squad.
The button caption reports the selected recipient count, and recipient colors
are distinct from the current-squad state. Ordinary `TAB` remains
vanilla-owned: the native cycle runs once, the manager observes the completed
change after the native update, and then applies the exact selected highlight.
Manager modals block native cycling; the ordinary manager does not replace
Kenshi's `TAB` behavior.

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

`Options` contains one global set of station-category visibility preferences
and the persistent `Dark UI friendly font colors` toggle. Vanilla light mode
is the default: its warm Kenshi-style background uses dark readable text. When
the toggle is enabled, the manager keeps the existing dark background and
light text colors for Dark UI. The setting applies to an open window
immediately and saves for later sessions. Station preferences apply only to
Stations; Squad Jobs always shows every permanent job and computes each
member's top three supported base stats above 1 from all supported stats. The
Options modal has the nine station-category toggles, the appearance toggle,
`Reset Default`, and `Close`. The mouse wheel is routed through options to the
shared scroll view. If the settings write fails, the in-session display still
applies and the status identifies the save failure.

Recipient selection and job selection are separate. The recipient set starts empty
when the manager opens. A plain portrait click selects only that person;
Ctrl-click toggles that person's recipient state. A plain bottom squad button
switches the current squad and selects its complete roster. Ctrl-clicking a
bottom squad button toggles its complete roster without switching the current
squad. Headers only collapse or expand. Recipient markers use a distinct
visual treatment from the current-squad highlight, and a batch action with no
recipients is a safe no-op with a toast.

Job selection remains an exact member-plus-queue-row identity, not just a
visible row number or job name. Ctrl-click selects or toggles job cards. The
value-only clipboard stores the selected rows, their exact target identities,
their board order, and their source presentation sequence. `Ctrl+C` captures
that snapshot and `Ctrl+V` appends it to every selected recipient. The
clipboard survives source refreshes but is cleared when the manager closes or
the world resets. Each recipient skips semantic duplicates and reports the
added, skipped, and failed counts.

Reordering within one row translates the exact insertion gap to Kenshi's native
`movePermajob` operation. A multi-selected drag moves the selected jobs to one
non-source visible member and appends them at the end of that recipient's
queue, preserving the board order and exact targets captured at drag start.
The MyGUI drop callback captures only copied squad/member identities,
presentation sequences, the exact source slots, and the destination member.
On the next update tick, the deferred path fresh-validates every active-roster
identity and captures complete structural queues. If any selected job already
exists on the destination, the whole drag is rejected before mutation.
Otherwise it appends each job through Kenshi's API, verifies each native
one-row or companion-row suffix, revalidates both queues, and removes the
source rows only after all destination changes are verified.
Dragging either card in an adjacent native primary/secondary pair moves the
pair. A separated or ambiguous companion fails before mutation; the manager
does not guess how to repair a non-contiguous native bundle.

Cross-member transfer and batch actions are probe-gated. The field-test
project defines `KJM_GENERAL_JOB_TRANSFER_PROBE` and
`KJM_JOB_BATCH_ACTIONS_PROBE`; it must not define
`KJM_GENERAL_JOB_TRANSFER_VERIFIED` or any batch verified macro. The full
disposable-save matrix in `docs/TESTING.md` is required before release
promotion. There is no compensating rollback. An unexpected append, insertion
failure, duplicate, or later verification failure stops the transaction,
retains any verified destination copy, and tells the player to review both
queues. No raw `Tasker*`, borrowed queue pointer, or unverified source removal
crosses the transaction boundary.

`Add Healing Jobs...` and `Prioritize Healing` use the same selected recipient
set. Add Healing Jobs adds only missing Rescue, Put in Bed, Medic, Robotics,
and Splinting rows, then moves the role families to the front in this exact
order: Rescue, Put in Bed, Medic, Robotics, Splinting, Engineering. It skips
semantic duplicates. Prioritize Healing performs only the ordering step and
never creates a missing row. Both actions validate the fresh active roster,
capture complete queues, verify every native move, stop on the first
unverified result, and report the partial result. Engineering remains in this
priority design; the separate imported-engineering invalidity report is
deferred until a reproducible in-game pattern is available.

Batch action results use a large, nonblocking toast. The toast remains on
screen for about four seconds, does not block input, and states whether rows
were added, skipped, moved, or rejected.

After a fully verified successful move, the affected
worker cache and any matching station detail assignment list are refreshed
without changing unrelated card positions. A normal full refresh remains the
fail-closed fallback when verification is partial, a member disappears, or the
projection cannot be patched.

### Game controls and refresh

The manager invokes Kenshi's native pause when it opens and records the prior
pause and speed state. On an ordinary close, it restores that prior state. If
the user unpauses or changes speed while the manager is open, the manager
records that as an explicit requested resume and closes while preserving the
user's requested pause/speed instead of restoring the old state.

Kenshi remains the sole owner of ordinary `TAB` squad cycling. The hook calls
the original native cycle once, records only that a change occurred, and the
manager refreshes after the native update returns. If the next squad is
Kenshi's internal `__DEAD__` holding squad, continue through the native order
until the next manageable squad. Never render `__DEAD__`. An
`Options` modal or `Clear Queue` Yes/No/Esc modal owns keyboard input and
blocks native squad cycling until the modal closes.

While the window is open, perform a live-state check once per second. Compare
the complete value-only active-squad board, loaded state, Jobs state, each
member's stats, queue size, queue row identity, labels, and order. Keep the
existing widget tree when the board revision is unchanged. Cancel an armed drag
and rebuild the grouped board when roster, collapse, current-squad highlight,
or queue structure changes. There is no manual Refresh button.

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
13. Save station-category and appearance `Options` changes immediately to one
    global preference set and load them across reload, reinstall, and update.
14. Report an `Options` settings write failure while retaining the current
    session display change.
15. Invoke native pause on open, restore the prior pause/speed on ordinary
    close, and preserve an explicit user-requested resumed speed.
16. Let Kenshi own each ordinary `TAB` cycle, observe the completed native
    change, refresh the manager afterward, skip the internal `__DEAD__` squad,
    and let Options and Clear Queue modals block cycling. Run incremental
    checks at one-second cadence without a manual Refresh button.
17. Build the bottom selector from the raw active-platoon order as value-only
    records, omit empty and `__DEAD__` squads, and apply the exact current
    identity highlight. Queue selector clicks, then fresh-validate and call
    `setCurrentPlatoon` only after the click callback returns. Keep selector
    overflow and Shift+wheel independent from the member-row scroll.
18. Ask the character to reconsider its current AI action and flag the vanilla
    selection UI for refresh after a successful mutation.
19. Keep MyGUI calls on the UI thread during
    `PlayerInterface::clearAndReset` cleanup.

Every engine read or write that dereferences reconstructed runtime objects
lives in a small SEH wrapper. MyGUI calls remain on the UI thread.

## Player-station and assignment Stations tab milestone

The manager has a second tab, `Stations`, beside `Squad Jobs`. Opening the
manager always starts on `Squad Jobs`; switching to `Stations` is explicit.
Stations is a category-grouped card grid. It has no member-by-station board,
and it has no drag-to-transfer interaction. Squad Jobs keeps its independent
same-row queue drag reorder.

### Grouped station grid

The grid contains verified player-owned workstations and exact station targets
from readable loaded-player permanent queues. It uses these stable categories:

`Crafting`, `Refining`, `Farming`, `Mining`, `Research`, `Training`,
`Storage / Hauling`, `Defense`, and `Other / Unclassified`.

Each category is a persistent collapsible group. Its header shows `+` when
collapsed or `-` when expanded, the category's station count, and its
unassigned count. Within a category, unassigned cards appear first, followed
by case-insensitive alphabetical order of the exact station name. The card
order is frozen while its detail modal is open; a localized update cannot move
the card under the pointer. Category filters affect only the visible grid and
save through the shared Options page.

Each card shows:

- the exact live station name, including a name supplied by a rename mod;
- the stable category/subtype icon as a translucent background;
- the number of unique assigned people, or a large red `X` when none exist;
- a thin yellow outline when it has no assigned people and a readable,
  non-blocking work state;
- a red blocking status only when the station cannot work.

A readable destroyed station stays visible so existing assignments can be
cleaned up. It disables new assignment but permits verified exact-target
removal.

The area is shown on the card and in the detail summary only when more than one
area is present. An unknown relevant skill remains `Other / Unclassified` or
`Relevant skill: Unknown`; station visibility follows only the selected broad
category.
The nine broad categories use the existing pictograms and stable FCS/function
subtypes. The live display name never selects an icon, so renaming does not
change classification. Farming classification gives `BCTYPE_FARM` precedence
over later function labels, so Wheat Farm XL, Hemp Farm L, and Rock Carrot
hydroponics remain in Farming. A modded `BCTYPE_PRODUCTION` record is accepted
as Farming only when its `UseableStuff::getStatUsed()` value is
`STAT_FARMING` and the task contract normalizes safely. A generic default task
alone is never enough to classify a farm or a station.

### Player-owned and assigned targets and ordering

The projection merges two exact sources. It brackets the player faction's
borrowed ownership records into plugin-owned scalar POD and reconstructs one
record per guarded scanner call. The Stations button processes the complete
bounded list synchronously before it exposes the grid. It also copies exact building targets from readable permanent
queues belonging to loaded player members and deduplicates those stable handles.

A direct ownership candidate is included only after a valid handle, loaded
`Building`, exact live-handle identity, `isThePlayer` ownership, and the strict
station allowlist all pass. The allowlist is based on stable
`BuildingClassType`/`BuildingFunction` and supported task contracts. It must
not use a generic default task: walls, lights, chairs, and other interactive
furniture are excluded even when they expose one. Assigned natural resource
nodes are the deliberate non-owned exception. Assigned player targets whose
metadata is unknown retain the `Other / Unclassified` fallback. An unresolved
or unloaded assigned target remains visible in Squad Jobs but is omitted here
because its live target cannot be proven.

Loaded assigned targets remain visible when incomplete, unpowered, broken,
disabled, dismantling, or destroyed. A red status marks a state that prevents
work. A zero local power allocation on an idle bench is not a failure. `NO
POWER` requires positive unmet live demand after Kenshi's generator and battery
allocation; a manual station switch-off is `POWER OFF`. Invalid or unresolvable
handles are excluded. No zone
container, town list, or unrelated world-building list is traversed. No
borrowed ownership source pointer or live `Building`, squad, or character
pointer is cached between resolution steps. Public and city buildings are
excluded even when a player queue references them, except for assigned natural
resource nodes.

The projection excludes the exact generic permanent task types `JOB_BUILDER`
(Engineer), `JOB_MEDIC` (Medic), `JOB_REPAIR_ROBOT` (Robotics),
`FIND_AND_RESCUE` (Rescue), `FIND_BED_AND_PUT_IN` (Put in bed), and
`FIND_AND_RESCUE_IF_THERES_BEDS` (the live permanent Rescue wrapper), and
`SPLINT_JOB` (Splinting). Their
stored subjects do not limit those jobs'
scope. They remain visible and editable in Squad Jobs and count toward the
worker's total jobs, but never create a station card or assignment. Squad Jobs
keeps each exact stored target internally for row identity and mutation
verification even though the card hides target text, arrow, unavailable tint,
and station-target artwork. Filter the task types, not localized labels; the
TaskType-driven role icons remain presentation-only and never enter the
station-target cache or Stations projection.

### Station detail modal and assignment actions

A left click on a card opens a centered modal with the exact station name,
category, conditional area, relevant skill, and blocking status. Its interior
is fully opaque to preserve text contrast. It uses two independent vertical
panes: assigned workers on the left and available workers on the right. Both
panes remain visible and have separate wheel/scrollbar positions. Every person
row reserves a fixed portrait column and renders normal text in pure white.
The modal deduplicates assignments by worker. Assigned and available people are
sorted by relevant skill with known values first. Normal stations sort highest
first. Training stations sort lowest first so the least-skilled workers appear
first. Ties use total permanent jobs (lowest first), then case-insensitive name.
Each person row shows the name, relevant
skill/value, and total permanent-job count.

The assigned-person projection hides an `OPERATE_STORAGE`/hauling row only
when that same worker also has a non-hauling station job for the exact target.
If no such non-hauling exact-target job exists, the hauling assignment remains
represented. This rule is target- and worker-specific; it must not hide a
different station's hauling job or all hauling jobs globally.

The right pane lists every loaded, readable player character not already
assigned. It uses the same name, relevant skill, skill value, and total-job
fields and the same sort order. A station whose
stable contract does not expose a supported permanent task remains visible,
but its available pane shows that assignment is unsupported; it must not offer
a guessed task. Existing
verified exact-target assignments remain removable.

Clicking a candidate or pressing `Enter` requests an immediate assignment.
Scrolling never commits a row. The MyGUI callback stores only stable handles;
the manager update then reacquires the member and station, validates the full
member queue fingerprint, and validates the exact live station identity and
player-managed gate. It calls `Character::addJob(task, building, true, true,
buildingPosition)` exactly once. A native automatic-machine operation may append
its `OPERATE_STORAGE` hauling companion; the result must be verified as the
expected one- or two-row suffix. Storage `LOOT_TARGET` defaults are normalized
to the permanent `OPERATE_STORAGE` task before the call. Duplicate exact jobs,
unsupported stations, stale queues, queue limits, and load transitions fail
closed with no queue change where possible. There is no Apply button and no
undo.

Right-clicking an assigned person requests immediate removal of every exact
target station job for that worker, not only the first matching row. The full
queue is captured and revalidated before each native removal; removals proceed
from the end of the queue so indexes remain valid. Each removal is verified
against the before/after queue. A validation failure stops with an explicit
partial result and leaves the remaining jobs for review. This action also has
no Apply step and no undo.

After a verified add or removal, update only the affected station card, detail
list, category station/unassigned counts, and worker cache. Do not restart the
target scan or rebuild unrelated categories. Apply blue recent-change feedback
to the affected card and detail row. Keep the card order and position frozen
until the detail modal closes; then return to normal unassigned-first,
alphabetical ordering. External queue changes, failed verification, and load
transitions use the normal fail-closed refresh path.

While the manager exists, the update hook clears Kenshi's camera-wheel input
and native mouse-rotate command before vanilla UI processing. Kenshi binds
Left Ctrl to mouse rotation by default; suppressing only that native command
keeps the rendered pointer moving while MyGUI retains Ctrl for job-card
multi-selection. MyGUI continues to receive wheel input for the manager's own
scroll views.

Blocking states are red only when they prevent work. Direct-owned stations can
appear without an assignment and show a thin yellow outline plus a large red
`X` on the card. The detail list can still state `UNASSIGNED`. Unreadable
queues cannot contribute an assignment or an add candidate; they do not make a
verified station disappear.

### Synchronous target resolution and safety limits

The station candidate pass starts only when `Stations` is opened for the first
time. Each guarded call consumes one stable assigned target or copied ownership
record. The plug-in processes the finite candidate list synchronously and does
not expose the grid until the pass stops. Normalized results append internally
in value-only batches of up to 16. The card view then performs one category and
name sort and creates the visible grid once. This design accepts a short input
pause and avoids partial-board streaming and repeated widget rebuilds.
Queue edits made while Squad Jobs is visible mark the station projection dirty,
but the synchronous rebuild waits until the player opens Stations.

If an assigned target cannot be read, resolution continues and shows a red
incomplete warning with the failed-target count. An unloaded ownership record is
silently omitted; a validation fault reports one ownership-pass warning. The
final station result stops at 2,048 unique stations, while the borrowed source
copy is separately capped at 8,192 records. Reopening the manager starts a new
player-station session. Queue changes from `Squad Jobs` rebuild the target list
and assignment joins. A successful detail-modal add or removal is the
exception: its verified member, station, category counts, and worker cache are
patched without restarting the target pass. External queue changes, failed
verification, or a load transition use the normal fail-closed refresh path.
Jobs state and condition refresh at the existing one-second cadence. Changing a
filter rebuilds visible cards immediately while preserving each category's
persistent collapse state.

The shared Options page contains `STATION CATEGORY FILTERS` and the
`Dark UI friendly font colors` appearance toggle, and saves changes
immediately. Vanilla light mode is the default. Station-category defaults are
enabled for Crafting, Refining, Farming, Mining, Research, and Other /
Unclassified; Training, Storage / Hauling, and Defense are disabled. Both tabs
open this same page, but only Stations uses the station filters.

## Why the Squad Jobs tab remains squad-wide while Stations is target-wide

Queue mutation is independent of building ownership, active-zone lifetime,
machine function, valid task-target pairings, and multiple-base selection. A
full-screen squad audit gives players a complete, concrete view of existing
vanilla queues. The Stations card grid joins those exact queues to discovered
world targets without making the editable queue UI depend on station discovery.

The following remain outside this milestone:

- role presets or abstract role scheduling;
- an unassigned-building world highlight or demand scanner;
- camera centering and world-space station highlighting;
- an optional priority scheduler that continuously changes vanilla queues.

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
pass. The same stable identity gate is repeated before every detail-modal
assignment or removal. World highlights, roles, and an unassigned-building
world toggle remain outside this milestone.

The current normalized station result contains a stable target handle, area
identity/name, category, exact display name, relevant skill, and blocking
status, plus the exact permanent-job assignments joined from loaded player
characters. It preserves renamed target names, keeps a readable destroyed
station as a red blocking card, excludes invalid or unresolvable targets, caps
the result at 2,048, and reports an assigned-target failure without aborting
later candidates.

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

Maps the strict allowlisted building class/function pairs to ordinary player
job types, then permits small FCS-identity exceptions for supported modded
stations. It must reject walls, lights, chairs, and any generic-default-task
object. Storage defaults normalize to `OPERATE_STORAGE`; automatic machinery
records the native operation-plus-hauling bundle. Unknown or unsupported
contracts disable new assignment rather than guessing a task; verified
existing exact-target assignments remain removable.

### StationCardGrid and StationDetailModal

The current grid groups verified targets by category, keeps persistent `+`/`-`
collapse state, reports station and unassigned counts, and sorts unassigned
cards before case-insensitive exact names. Virtual widgets render only visible
cards. Card clicks open a centered detail modal. The modal joins unique workers
by exact target, sorts them by relevant skill, total jobs, and name, and offers
all loaded readable players with those fields. Add uses click/`Enter`; assigned
person right-click removes all exact-target station jobs. Both actions are
deferred from MyGUI callbacks and use queue validation plus native game methods.
The projection patches only the affected card, detail list, category counts,
and worker cache, shows blue recent-change feedback, and freezes card position
until the modal closes. The view never invents a task or target and has no
Apply/undo workflow.

### OptionalPriorityScheduler

A later optional scheduler may convert abstract priorities such as Farming 1 or
Hauling 3 into concrete target jobs. It must remain optional and must not
continuously fight Kenshi's own decisions.

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
- Keep same-row Squad Jobs reorder separate from Stations detail-modal actions;
  Stations has no drag-to-transfer path.
- Defer every station add/remove outside MyGUI callbacks. Reacquire stable
  member/building handles and verify the complete affected queue immediately
  before mutation. Add uses one native call and verifies the one- or two-row
  suffix, including an automatic operation-plus-`OPERATE_STORAGE` bundle.
  Right-click removal revalidates and verifies every exact-target row, in
  reverse index order, and stops on the first failure.
- After a fully verified success, patch only the affected card, detail list,
  category counts, and worker cache. Preserve the card position until the
  detail modal closes, apply blue recent-change feedback, and use a full
  refresh only when patch preconditions fail.
- Build station cards from verified player-owned station-relevant building
  records and exact building targets in readable loaded player queues. Enforce
  the class/function allowlist; walls, lights, chairs, and generic default-task
  objects are not stations. Do not enumerate zones or unrelated world
  containers. Assigned natural resource nodes are the one explicit exception.
- Copy only scalar ownership-record fields into plugin-owned storage. Bracket
  the borrowed source header, reconstruct one candidate per guarded call, and
  require `isValid`, `getBuilding`, exact live-handle identity, and
  `isThePlayer` before reading direct-owned metadata. Never retain source or
  engine object pointers between refreshes or game-state transitions.
- Bound the borrowed ownership copy at 8,192 records and the final station
  result at 2,048 unique stations. Process the finite copied candidate list
  synchronously, one guarded candidate per call, and append normalized values
  internally in batches of at most 16. Build the card grid once after the pass.
  A failed assigned target must not abort later candidates.
- Reopen the manager to start a fresh player-station candidate pass. Do not
  retain raw `Building`, `Platoon`, or `Character` pointers between refreshes
  or game-state transitions.
- Leave combat, rescue, self-preservation, and immediate orders to vanilla
  Kenshi.
- `Remove Selected` and drop-to-remove are immediate, irreversible actions with
  no prompt or undo; exact row revalidation is their safety gate.
- Require the Yes/No/Esc full-fingerprint modal for `Clear Queue`.
- If station-category settings persistence fails, keep the in-session display
  change and report the station-category save failure.
- Invoke and restore native pause/speed according to the open/close rules, and
  preserve a user's explicit resumed speed.
- Keep the Squad Jobs selector value-only. Do not retain `Platoon` or
  `ActivePlatoon` pointers. Preserve raw active/nonempty vanilla `TAB` order,
  exclude empty and `__DEAD__` squads, fresh-validate `setCurrentPlatoon`, and
  keep selector horizontal overflow separate from member-row scrolling.
- Test every queue mutation, appearance mode, recipient mode, and station
  classifier on disposable saves until save/load/import behavior is proven.
  Imported-engineering invalidity is intentionally deferred until a repeatable
  in-game pattern is documented.

## Roadmap

### 0.1: Stage 1 squad audit/editor field test

- full-screen grouped multi-squad window;
- Kenshi's live job text and exact live target names;
- per-member Jobs toggle and full-fingerprint Clear Queue modal;
- same-row drag reorder;
- empty-by-default recipient selection through portraits and whole-squad
  selector buttons;
- value-only Ctrl+C/Ctrl+V clipboard with exact targets, board order, duplicate
  skipping, and clear-on-close;
- multi-selected drag append to one non-source recipient with stop-on-failure;
- Add Healing Jobs and Prioritize Healing for the same recipients, with Rescue,
  Put in Bed, Medic, Robotics, Splinting, and Engineering ordering;
- large nonblocking result toast lasting about four seconds;
- each member's top three supported base stats above 1;
- immediate station-category Options preferences and Vanilla-default/Dark UI
  appearance toggle;
- native pause/speed open/close behavior and Kenshi `TAB` cycling;
- compact bottom Squad Jobs selector with exact raw active/nonempty vanilla
  `TAB` order, exact selected highlight, fresh validated `setCurrentPlatoon`,
  empty/`__DEAD__` exclusion, value-only records, independent horizontal
  overflow, normalized wheel steps, 60-pixel group gaps, and plain-wheel/
  Shift+wheel routing;
- unloaded/cached read-only fallback, but editable live queues with unavailable
  target labels;
- one-second incremental refresh and transition cleanup;
- source validator and build package.

### 0.1: Player-station and assignment Stations card-grid field test

- second `Stations` tab with the manager opening on `Squad Jobs`;
- strict player-owned workstation allowlist, excluding walls, lights, chairs,
  and generic default-task objects, plus the assigned natural-resource
  exception;
- exact loaded building targets referenced by readable player permanent jobs;
- category-grouped virtual card grid with persistent `+`/`-` collapse, station
  and unassigned counts, unassigned-first ordering, and alphabetical exact names;
- translucent stable category/subtype icons, conditional area labels, unique
  assigned-person counts, yellow usable-unassigned outlines, and red blocking
  status;
- centered detail modal with relevant-skill sorting and all loaded readable
  players showing name, skill/value, and total jobs;
- hauling redundancy rule, storage normalization, native automatic
  operation-plus-hauling bundle, immediate click/`Enter` add, and right-click
  removal of all exact-target station jobs;
- localized card/detail/category/worker-cache updates, frozen card positions
  until modal close, blue recent-change feedback, and no Apply/undo workflow;
- synchronous bounded candidate enrichment when the player opens Stations,
  internal value-only publication batches of 16, one visible grid build,
  ownership validation warning, assigned-target failure warning, and
  2,048-result/8,192-copy safety limits;
- disposable-save field checklist and source validation for the new files.

The production ownership pass can show unassigned player stations. Complete
world highlights, safe full-world discovery, and the unassigned world-highlight
toggle remain gated on separate validated contracts.

### 0.2: world target inspection and highlighting

- inspect a selected world object;
- display target identity and validation failures;
- optional camera centering and world-space highlighting through separate
  validated contracts.

### 0.3: assignment policy extensions

- role presets that expand into concrete jobs;
- batch assignment to selected workers after the single-action contract is
  proven;
- queue conflict recommendations and an optional unassigned-building world
  highlight.

### 0.4+: optional scheduling

- abstract work-category priorities;
- demand detection and worker selection;
- skill/distance recommendations;
- multiple loaded outposts and diagnostics.
