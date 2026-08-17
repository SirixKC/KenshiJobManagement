# 0.1 Stage 1 + Stations disposable-save field checklist

This build edits live AI state through reconstructed engine methods. Use a
disposable save for every test pass. Keep a backup from before installation and
do not use a valuable campaign save to investigate a failed mutation.

The test terms below are intentional:

- **member** means one current-squad character;
- **row** means one exact permanent-job queue entry for one member;
- **live label** means Kenshi's exact current job or target text, with no
  synthetic `Job:` or `Target:` prefix; an unresolved live target is
  `Target unavailable`, not a read-only queue;
- **live queue** means the member and its queue were safely resolved;
- **cached queue** means a last-safe queue shown after the member or queue
  became unavailable. Cached queues are read-only;
- `Remove Selected` and drop-to-remove are immediate, irreversible actions with
  no prompt and no undo. `Clear Queue` is the only queue removal with a
  Yes/No/Esc review modal.
- **station** means one displayed world building/node card. A short card click
  opens its centered detail modal; Stations has no drag-to-transfer action.
- **station view incomplete** means the card grid is still resolving copied
  ownership records or known queue targets, an assigned target failed, an
  ownership validation fault occurred, or the 2,048-result safety cap was
  reached. Treat the result as incomplete until the banner clears and record
  any warning.

## Before launching

- Build `Release|x64` with the VC100 x64 toolset.
- Confirm the package contains the DLL, canonical blank FCS `.mod` marker,
  and `RE_Kenshi.json`.
- Install to `mods\KenshiJobManagement\` and enable the mod.
- Keep other RE_Kenshi UI plugins enabled only when needed for compatibility
  testing.
- Make a disposable-save backup.
- Prepare a small squad with at least three members. Give two members
  distinguishable permanent jobs, and give one member no permanent jobs.
- If possible, place the squad at a player outpost with at least one crafting,
  farming, mining, or refining station. Keep one assigned natural iron or
  copper node available for the exact-target exception test.
- If possible, include a player-owned wall, light, and chair near the outpost.
  These must remain excluded from Stations even if a generic task is exposed.
- If possible, include two jobs with the same visible job name but different
  targets. This is needed to test exact row identity.
- Note each member's supported base stats and values, the current game pause
  state, game speed, squad membership, and Jobs state before opening the
  manager.
- Record the current station-category Options preference. On a clean
  installation, verify the documented station-category defaults.

### Native HUD JOBS entry

- At the main menu and after loading the disposable save, confirm the vanilla
  `JOBS` control has one square Job Manager sibling on its right. The sibling
  must follow the live JOBS height and leave the native label readable.
- Disable the native JOBS toggle and confirm only the native control becomes
  disabled; the sibling remains independently enabled while its Orders-panel
  parent is visible.
- Hover the sibling and confirm the tooltip is exactly `Open Job Manager
  (Ctrl+J)`. Click it and confirm the manager opens after the vanilla update;
  no duplicate or re-entrant window appears. Press `Ctrl+J` and confirm the
  same manager path still works as fallback.
- Resize or rebuild the HUD, then confirm the sibling is reacquired under the
  current Orders panel and remains square. Use a deliberately narrow or
  incompatible JOBS rectangle and confirm the split is abandoned without
  damaging the native JOBS control.
- Return to the main menu or reset the save. Confirm the original JOBS
  rectangle is restored when it still has the plugin split geometry. If a UI
  mod changed that rectangle, confirm cleanup leaves the changed geometry
  untouched. If the packaged icon is missing, confirm the sibling remains
  usable with its `JM` caption.

## Startup, layout, pause ownership, and lifecycle

1. Reach the main menu and confirm no crash.
2. Load the disposable save.
3. Inspect `RE_Kenshi_log.txt` for the `0.1.0-alpha loaded` line.
4. Record the pause state and speed immediately before opening the manager.
5. Press `Ctrl+J`. The native full-screen current-squad manager should appear
   once. It must not open a one-character popup.
   Confirm the primary backdrop is fully opaque and member rows, job cards,
   controls, portraits, and text stay fully opaque.
   With enough members and jobs to require both scrollbars, use the wheel over
   portraits, member text, job cards, Jobs ON/OFF, Clear Queue, blank panels,
   headers, action buttons, and both scrollbar regions. Plain wheel input must
   move only the member/job rows vertically. Shift+wheel must move only job
   cards and priority headers horizontally. Confirm both directions clamp at
   their ends, no control activates, and the game camera never zooms.
   Hold Ctrl and move the pointer across several job cards. The rendered
   pointer must follow continuously. Ctrl-click several cards and confirm each
   click toggles only the intended card while earlier selections remain.
   Repeat the pointer movement with Mouse3 held because it can share Kenshi's
   native mouse-rotate command. The camera must remain still and the pointer
   must not freeze, jump, or wait for a click before it updates.
6. Confirm the manager invokes Kenshi's native pause on open. Record the new
   pause state and speed.
7. Confirm the heading resolves to `<squad name>  |  <N> member(s)` (or starts
   as `Current squad`) and appends `  |  read-only snapshot` only for a
   squad-level cached snapshot. Confirm the member-column header is exactly
   `SQUAD MEMBER  |  TOP STATS`.
8. Confirm every current-squad member has one card or row, including the
   member with no jobs. Confirm the member name and `Jobs: ON` or `Jobs: OFF`
   state match Kenshi. Confirm each portrait shows that member's actual
   character image, not only the gray `Background/Normal` layer, and compare
   it with the same member in Kenshi's vanilla squad interface.
9. Confirm each member shows its own top three supported base stats whose values
   are above 1. The three displayed stats must be computed per member, not
   selected as three aggregate stats across the squad. Confirm the three skills
   are stacked vertically in a larger, readable font and do not overlap the
   member controls.
10. Confirm each queue entry is a narrow, high-contrast selectable card with
   same-row drag behavior, no repeated `DRAG` label, and no priority number
   inside the card. Confirm `Jobs: ON` and `Clear Queue` captions stay inside
   their buttons at the smaller control font.
   Priority must appear once in the column header. A station-display
   target-bearing card must
   center:

   ```text
   <Kenshi's live job text>
   V
   <live target name>
   ```

   Compare the labels with the vanilla job panel and the actual target in the
   world. The card may remove only a duplicate leading `<priority>:` from the
   work text. The job and target lines must have no synthetic prefixes. An
   unresolved station-display target must show `Target unavailable`, tint the
   whole card red, and remain editable; no station-display target means no
   arrow or target line. Confirm long work or renamed-target text wraps or
   scales down and does not clip or leave the card bounds.
   Confirm station-display building-target cards fill in with the same category artwork used
   on Stations. The simple 2-to-4-color artwork must remain square and centered,
   with no horizontal stretching. Inspect the icon itself: its `ImageBox` must
   be `33%` opaque (`67%` transparent), independent of the dark overlay and
   parent/card alpha. Open directly on Squad Jobs and cycle to another squad
   with many distinct targets. Every icon must be present in the first rendered
   view; icons must not fill in across later update frames or cause a long UI
   stall. A synthetic squad with more than 512 unique building targets may use
   the neutral Other icon for overflow cards, but those cards must not be blank.
   `Operating Machine` and `Operating Automatic Machine` must both display as
   `Operating...`; hover them and confirm the tooltip retains the full label.
   Select an unavailable station-display target card and confirm both its red
   warning and its selected state remain clear. Hover a station-display job
   that appears on multiple members with the same exact target.
   Confirm all matching cards, including the hovered card, receive a gold
   outline. A card with the same work type but a different target must not be
   outlined. Move away and confirm all outlines clear. Repeat with a selected
   card, an unavailable-target card, and a Jobs OFF row; each underlying state
   must remain readable.
11. Confirm an empty queue displays `[No permanent jobs]`. `Clear Queue` must be
    disabled, while the member's independent `Jobs: ON`/`Jobs: OFF` toggle stays
    available. A queue that cannot be read displays `Queue unavailable
    (read-only)`.
12. Confirm the bottom controls are exactly `Remove Selected (N)`, `Options`,
    and `Close`; there is no persistent instruction/current-squad footer and no
    manual Refresh button. Trigger one safe validation failure and confirm its
    actionable status still appears. Live changes must arrive from the
    incremental checks.
13. If a malformed or stress-test queue exceeds 64 permanent jobs, confirm the
    manager shows only the first 64 with `Queue exceeds safety limit
    (read-only)` and disables all edits for that member.
14. Close the manager without changing pause or speed. Confirm ordinary close
    restores the exact pause state and speed recorded in step 4.
15. Reopen it with `Ctrl+J`. Load another save while the manager is open. The
    old window and row identities must be discarded; the next view must contain
    only the new save's current squad.
16. Return to the main menu with the manager open and confirm no crash.

## Global behavior cards and exact target verification

Use a disposable save with loaded members who have Builder/Engineering, Medic,
Robotics, Rescue, and Find-and-put-in-bed permanent jobs. Include the live
`FIND_AND_RESCUE_IF_THERES_BEDS` Rescue wrapper when Kenshi creates it. Give
the global rows different stored subjects, including one subject that is
unavailable if the save permits it. The TaskType-to-icon mappings are:

- `JOB_BUILDER` -> optional `job-engineering.png`, with `station-other.png` as
  the fallback;
- `JOB_REPAIR_ROBOT` -> the existing skeleton-limb station icon;
- `JOB_MEDIC`, `FIND_AND_RESCUE`, `FIND_BED_AND_PUT_IN`, and
  `FIND_AND_RESCUE_IF_THERES_BEDS` -> optional `job-medic.png`.

Also add two station-display rows with the same task type but different exact
targets.

1. Confirm the global rows remain visible in Squad Jobs and count toward
   each worker's total jobs. Because these rows are identified by
   `!IsStationDisplayJob`, each card must omit target text, the `V` arrow,
   unavailable tint, and station-target artwork. Each card must show the
   correct TaskType-driven role icon, at the direct 33% card-art alpha, when
   its optional asset is available. The card and its tooltip must not expose
   the incidental subject, even when the stored subject is unavailable.
   None of the global rows may enter the station-target artwork cache or
   the Stations projection, including when their stored subject is a building.
2. Hover two global rows with the same task type but different incidental
   targets. Confirm both cards highlight because grouping compares task type
   without the incidental target. Hover the station-display rows with the same
   task type and different exact targets; only cards with the same exact target
   may highlight.
3. Select or start a same-row mutation for a global row, then change or replace
   its stored subject before the native call. The exact stored target retained
   in the snapshot must make revalidation fail closed: no wrong row is changed,
   and the queue remains unchanged. Restore the original subject and repeat the
   mutation successfully. Confirm the card still omits target text, arrow,
   unavailable tint, and station-target artwork after both refreshes. Repeat
   once for the `FIND_BED_AND_PUT_IN` row to prove that its exact stored target
   remains available for edit validation even though its card only shows the
   medic cross. Repeat for `FIND_AND_RESCUE_IF_THERES_BEDS` and confirm the
   live Rescue wrapper also shows the cross and never exposes its subject.

## Bottom Squad Jobs selector

Use a disposable save with several player squads in a deliberate vanilla order;
include one empty squad, one internal `__DEAD__` holding squad, and enough
nonempty squads to exceed the visible strip when possible.

1. On `Squad Jobs`, confirm the compact selector is one bottom row. Compare its
   buttons with the raw vanilla active-platoon list. It must preserve the exact raw active/nonempty vanilla `TAB` order and show only nonempty player squads,
   and omit empty entries and `__DEAD__`. It must not alphabetize or add a
   synthetic squad.
2. Confirm exactly the current squad has the exact selected highlight.
   Click another button and verify that the click first queues its value
   identity; after the
   update, a fresh validated `setCurrentPlatoon` selects that squad and the
   member rows and highlight agree. No queue mutation or intermediate pointer
   state may be visible.
3. Remove or empty the clicked squad before its queued update, or cause a load
   transition. The fresh validation must reject it, leave the current squad
   unchanged, and show a safe unavailable status. Reopen/reset the manager and
   confirm no stale `Platoon` or `ActivePlatoon` pointer is used.
4. Force a direct selector click or observed native `TAB` change to complete,
   then make the following job snapshot read fail. Confirm the manager clears
   the prior member/job widgets before publishing the new identity: no old
   editable controls, cards, or jobs may remain. The new view must be a
   read-only unavailable snapshot and show `The current squad is unavailable
   and has no session snapshot.`; test both the click and native-`TAB` paths.
5. Add enough squads to overflow the strip. Use its scrollbar and confirm only
   the selector moves horizontally; this independent horizontal overflow must
   leave member-row vertical and job-column horizontal offsets unchanged.
   Confirm both ends clamp without blank or
   duplicated buttons.
6. Move the pointer over the selector strip. Plain wheel over the selector strip
   must still scroll the member rows and must not move the strip. Hold Shift and
   wheel: confirm only the strip scrolls, the member rows do not move, no squad
   button activates, and both directions clamp at their ends.
7. Press `TAB` on the ordinary manager. Confirm Kenshi's native cycle runs once
   and the selector highlight follows after the native update. A held key must
   not double-cycle. Put `__DEAD__` between two live squads and confirm it is
   skipped and never appears in the selector. Options and Clear Queue/detail
   modals must block native `TAB` cycling.

## Options and station categories

`Options` controls one global set of station-category visibility preferences.
It does not filter Squad Jobs or member stats.

1. Open `Options` from the main manager. Confirm the modal is wider than the
   old stat-filter layout and contains only `STATION CATEGORY FILTERS`, the
   nine broad category buttons, `Reset Default`, and `Close`. Confirm the help
   text says that category filters apply only to Stations and Squad Jobs shows
   every permanent job while selecting each member's top three supported base
   stats.
2. Move the mouse over a category button and use the wheel. Confirm wheel
   input reaches the shared Options scroll view and does not move the game
   camera. Repeat at a low resolution if possible.
3. Toggle each station category. Confirm changes save immediately, rebuild
   only the Stations display, and do not add/remove jobs, change queue order or
   targets, change Jobs state, or change pause/speed. Confirm Squad Jobs still
   shows every job after each toggle.
4. Confirm each member computes its own top three supported base stats above 1
   from all supported stats. Old `[Skills]` / `Stat_*` settings must not hide a
   qualifying stat.
5. Press `TAB` while Options is open. The modal must block Kenshi's
   current-squad cycling until it closes.
6. If `settings.ini` cannot be written, confirm the current station display
   still applies and the manager reports a station-category settings failure.
7. Save and reload the game. Confirm the category choices remain. Reinstall
   the same package and apply an update without deleting settings; confirm the
   choices remain after both operations.
8. Press `Reset Default` and record the documented station-category defaults.

## Stations tab: player-station card grid and assignment detail

Use a save with at least one player outpost, several station types, multiple
squads, and one permanent job aimed at a natural iron or copper node when
possible. Keep the vanilla building names visible so renamed-building support
can be checked with a second save or rename mod.

1. Open the manager with `Ctrl+J`. Confirm it opens on `Squad Jobs`. Click
   `Stations` and confirm the tab does not change the current squad or selected
   character. Confirm Stations contains a grouped card grid, not a member
   roster, and has no station drag-to-transfer affordance.
2. Confirm the first click on `Stations` causes one short input pause and then
   opens a fully populated grid. Cards must not stream in afterward. Confirm
   each guarded call resolves only one copied ownership record or exact assigned
   target, normalized results append internally in batches of at most 16, and
   the visible grid is built once after the bounded pass. Inspect the completion
   log for candidate count and elapsed milliseconds. An extreme ownership list
   may take longer than one frame; it must remain finite and must not retain an
   engine pointer between candidates. Change a queue on Squad Jobs and confirm
   no hidden scan pause occurs; the board refreshes synchronously on the next
   Stations click.
3. Test the source gate. Verified player-owned workstations may appear, but
   walls, lights, chairs, public/city buildings, and other objects accepted
   only through a generic default task must not appear. An assigned natural
   resource node may appear as the one explicit non-owned exception. An assigned
   unknown player target may remain `Other / Unclassified`. An unavailable
   assigned target must remain red on Squad Jobs, be omitted from Stations, and
   increment the failed-target warning. Destroyed, incomplete, unpowered,
   broken, disabled, and dismantling assigned targets remain visible with their
   blocking status when their live target is readable. A readable destroyed
   station remains visible and red, shows assignment as unsupported in the
   available-worker pane, and still permits removal of verified existing
   assignments. An unloaded ownership record is
   omitted without one red error per record.
4. Confirm the nine categories are `Crafting`, `Refining`, `Farming`, `Mining`,
   `Research`, `Training`, `Storage / Hauling`, `Defense`, and `Other /
   Unclassified`. Each category header shows `+` or `-`, its station count, and
   its unassigned count. Collapse several categories, switch tabs, close and
   reopen the manager, and reload the save; confirm the documented collapse
   preference persists. Within every expanded category, unassigned cards come
   first, then exact names sort case-insensitively alphabetically.
5. Confirm each card shows the exact station name, including a rename-mod name,
   a translucent stable category/subtype icon, the number of unique assigned
   people or a large red `X`, and a red status only when the station cannot work.
   Confirm a usable unassigned station has a thin yellow outline. A recent blue
   change marker temporarily takes priority; the yellow outline returns when
   the detail modal closes. Leave several powered benches idle in a base with
   adequate power and confirm they do not show `NO POWER`. Give an active
   consumer unmet demand and confirm `NO POWER` is red; manually switch a
   consumer off and confirm `POWER OFF` is red. Structural faults remain red
   even when nobody is assigned.
   The area must appear on cards and detail summaries only when more than one
   area exists. Test a renamed material-output station and special benches:
   copper ore, iron ore, UWE copper plates when UWE is loaded, iron plates,
   steel bars, copper alloy plates, electronics, crossbows, and skeleton limbs
   use stable subtype icons. Renaming one must not change its icon; unknown or
   modded identities fall back to the broad category icon.
6. Disable and re-enable station categories in `Options`. Confirm category
   filters rebuild only the display, while Squad Jobs remains complete.
   Confirm the category and unassigned counts update with the visible filtered
   cards. Enable `Training` and `Defense`; confirm a player-owned training dummy
   appears under Training and a player-owned double-barrel harpoon turret appears
   under Defense without any per-skill setting. Restore the documented defaults
   (Crafting, Refining, Farming, Mining, Research, Other / Unclassified).
7. Give loaded members the generic Engineer, Medic, Robotics, and Rescue jobs.
   Confirm all four remain visible on Squad Jobs and count in each worker's
   total jobs, but none creates a Stations card or assigned person. If the same
   target also has a station-specific job, only that station-specific job is
   represented.
8. Click a station card. Confirm the manager remains open and a centered modal
   opens with exact name, category, conditional area, relevant skill, and
   blocking status. Its interior must be fully opaque; the station grid and
   game world must not show through it. `MyGUI.log` must not add a new
   `Modal widget must be root`.
   Confirm the modal is wider and keeps assigned workers on the left and
   available workers on the right. Portraits must occupy a fixed column without
   covering text. Normal title, summary, header, and worker text must be pure
   white. Assigned people appear once per worker. Confirm ordinary stations
   sort both panes by relevant skill from highest to lowest. Confirm Training
   stations reverse both panes and sort from lowest to highest. Known values
   remain ahead of unknown values; ties use total permanent jobs, then name.
   Give one worker both a non-hauling
   exact-target station job and `OPERATE_STORAGE` hauling: hide only that
   redundant hauling row for that worker. A hauling job without the same
   worker's non-hauling exact target remains represented. Never hide hauling
   globally or for a different target.
9. Confirm the always-visible right pane contains every loaded, readable player
   character not already assigned, including characters from other loaded
   squads. Each entry shows name, relevant skill/value, and total jobs using the
   same sort order. Scroll the two panes independently with their scrollbars and
   mouse wheel. An unloaded, cached, or unreadable station cannot be selected
   for a mutation. An unsupported assignment contract replaces candidates with
   an explicit unsupported message instead of guessing a task, while verified
   existing assignments remain removable.
10. Click an add candidate and repeat with keyboard `Enter`. Each action must
    be immediate from the player's perspective, with no Apply button and no
    undo. Verify the full queue and exact live station immediately before the
    native call. A supported ordinary station adds the expected one permanent
    job. An automatic-machine station uses Kenshi's native operation-plus-
    hauling bundle and the manager verifies the expected two-row suffix.
    Storage `LOOT_TARGET` defaults normalize to `OPERATE_STORAGE`. Duplicate
    exact jobs, unsupported stations, stale queues, full queues, invalid
    handles, or load transitions must fail closed and report no unsafe change.
11. Right-click an assigned person. Confirm the action immediately requests
    removal of all permanent station jobs whose target is that exact station
    for that exact worker, including multiple rows. It must not remove jobs for
    another target or another worker. Verify the full queue before each native
    removal, remove from the end, and check each before/after queue. Force a
    stale queue or removal failure if possible; the action must stop, report the
    verified partial result, and leave remaining rows for review. There is no
    Apply step and no undo.
12. After a verified add and a verified removal, confirm only the affected
    station card, detail list, category station/unassigned counts, and worker
    cache update. Do not restart the candidate pass or rebuild unrelated
    categories. Keep the card's position frozen while its modal is open. Show
    blue recent-change feedback on the affected card/detail entry. Close the
    modal and confirm normal unassigned-first/alphabetical ordering resumes.
    Make an external queue change through Kenshi and confirm that only this
    external/stale path uses the normal fail-closed full projection refresh.
13. Scroll each detail pane with its wheel region and scrollbar. Confirm one
    pane does not move the other and scrolling never assigns or removes a
    worker. Press `Enter` on a focused available candidate and
    confirm it commits immediately. Close the modal with its close control and
    `Esc`; confirm the grid returns with its category collapse and card order.
    Open and close the tab repeatedly while watching `RE_Kenshi_log.txt` for
    widget growth, access violations, or stale-pointer errors.
14. If a station target read fails, confirm later candidates still resolve and
    the banner reports the failed-target count. Use a stress save if possible to
    reach 2,048 unique stations; confirm the pass stops at the cap and records
    truncation. Record candidate count, published station count, category and
    unassigned counts, warning text, and whether any later target was skipped.

## Live refresh, native pause/speed, and TAB

1. Leave the manager open for at least five seconds. Confirm the live-state
   check runs at approximately one-second cadence and does not visibly rebuild
   unchanged member cards every frame.
2. Add or remove a job through vanilla controls while the manager remains open.
   The affected member's exact row and target labels should update within the
   next one-second check without a manual refresh.
3. Change squad membership. Confirm changed cards update while unchanged cards
   keep their selection and scroll state when safe.
4. Close the manager normally without changing the native controls. Confirm it
   restores the pause/speed state from before open.
5. Reopen the manager. While it is open, unpause with Kenshi's native control
   and change to a different native speed. Close the manager normally. Confirm
   it preserves the user's requested resumed speed and does not restore the
   pre-open pause/speed over that request.
6. Reopen and test the inverse: if the user pauses or changes speed again while
   the manager is open, close with the latest explicit user-requested state and
   verify that state remains.
7. On the main manager, tap `TAB` repeatedly. Confirm each press advances
   exactly once immediately and does not advance again during the next two
   seconds. Confirm rapid separate taps each advance once, a held key does not
   repeat, and `TAB` does not traverse widget focus or trigger a queue mutation.
   Put an internal `__DEAD__` squad between
   two live squads and confirm it is skipped and never rendered. Also test a
   dead-only/current-dead edge case; the manager must not present `__DEAD__` as
   an editable squad.
8. Open `Options`, then press `TAB`; repeat with the `Clear Queue` modal. Both
   modals must block TAB from cycling the current squad until the modal closes.
9. Test both paused and running states, and record any native input focus issue.

## Per-member Jobs toggle

For each of two loaded members:

1. Record the member's vanilla Jobs state.
2. Click that member's `Jobs` control once. Confirm only that member changes
   from `Jobs: ON` to `Jobs: OFF`, or vice versa, in both interfaces.
3. Click it again and confirm the original state returns.
4. Confirm another member's Jobs state and queue are unchanged.
5. Trigger combat after the toggle. Confirm vanilla combat, rescue, and
   self-preservation behavior still interrupts work normally.

## Clear Queue Yes/No/Esc modal and fingerprint

1. Give member A at least three distinct permanent jobs. Give member B at least
   one different job.
2. Click member A's `Clear Queue`. Confirm a modal opens with member A's exact
   name and a reviewed count equal to the full queue count. Confirm the title
   is `Confirm Clear Queue` and the body starts `Are you sure you wish to
   remove all jobs?` and names the member in `Remove all <N> permanent jobs
   from <member>?`. The buttons must be exactly `Yes` and `No`.
3. Press `No`. Confirm nothing changes. Open the modal again and press `Esc`.
   Confirm nothing changes and the modal closes.
4. Open it again. Before pressing `Yes`, verify the displayed reviewed count.
   The manager must retain an internal full ordered queue fingerprint that
   includes every row identity/task/target in order, not only the count.
5. Press `Yes` without changing the queue. Confirm only member A's permanent
   jobs are cleared. Member B's queue, immediate orders, combat state, and Jobs
   state must be untouched.
6. Restore member A's jobs. Open `Clear Queue`, then change the queue through
   vanilla controls before pressing `Yes` (add, remove, reorder, or change a
   target when possible). The full fingerprint must fail validation; no jobs
   may be cleared, and the view must refresh with a stale-queue status.
7. Repeat with a member that has no jobs. `Clear Queue` must stay disabled and
   must not open a modal or change any state.
8. Save, reload, and confirm a successful clear persists.

## Same-row drag reorder

Test this after the read-only and single-member checks, and record the queue
before every action.

1. Queue three distinct jobs A, B, and C for one loaded member. Confirm the
   vanilla order and the manager order are A, B, C, with A at highest priority.
2. Drag B above A in the same member's queue. Expected order: B, A, C.
3. Drag B back below A. Expected order: A, B, C.
4. Move the first row down and the last row up. Confirm each result in the
   vanilla panel.
5. Attempt to drag a row to another member. The operation must be rejected or
   have no effect; it must not add the job to the other member.
6. Change the vanilla queue immediately before dropping a dragged row. The
   manager must refresh or reject the stale move rather than applying it to a
   different row.
7. Watch the active worker after moving its current top job. Record whether
   Kenshi immediately switches work, waits for the current action to finish, or
   behaves incorrectly.
8. Save, reload, and confirm a successful reorder persists.

`movePermajob` is the least proven mutation in this field test. If the actual
ordering differs from the expected result, stop testing reorder and attach the
before/after log plus exact queue contents to an issue. Do not infer alternate
index semantics from one test.

## Cross-member multi-select and drop-to-remove

These operations are immediate and irreversible. Make a fresh disposable-save
backup first. There is no prompt and no undo.

The reconstructed `removePermajob(int)` parameter is treated as a queue index
by this build. Prove that assumption with step 3 before testing larger batches.
If Kenshi removes a different row, stop all removal testing and restore the
backup.

1. Give member A three jobs A1, A2, A3 and member B two jobs B1, B2. Include
   duplicate visible names with different targets if the game allows it.
2. Select A2 and B1. Confirm both cards change to the selected colour and the
   fixed action button reads `Remove Selected (2)`.
3. Invoke `Remove Selected`. It must remove the selected rows immediately,
   without a prompt. Expected result: only A2 and B1 disappear;
   A1/A3 and B2 remain in their original order.
4. Verify both members in the manager and in Kenshi's vanilla job panel. Confirm
   immediate orders and other members are unchanged. There must be no undo
   action or restore prompt.
5. Restore the test jobs. Drop one exact row onto the remove target. It must
   remove immediately with no prompt and no undo. Confirm only that row is gone.
6. Repeat with a selection containing the first and last rows of several
   members. Confirm exact identities are removed, not rows at the same numeric
   index on unrelated members.
7. Force or reproduce a stale row during execution by changing one selected
   member's vanilla queue. The batch must stop at the first failed removal. It
   must report successful removals and the failed row, leave later rows
   selected, and never claim later rows were removed.
8. Confirm a later retry operates on refreshed exact identities only. It must
   still have no prompt and no undo.
9. Save, reload, and confirm the partial or complete result reported by the
   manager.

## Live target-unavailable versus unloaded/cached read-only state

1. With a member and queue still loaded, make one target unavailable or use a
   job whose target cannot be resolved, if the test environment permits it.
2. Confirm the row displays `Target unavailable` but remains editable. Test
   a safe selection, same-row reorder, or immediate removal. The manager must
   not make an otherwise-live queue read-only only because its target label is
   unavailable.
3. Move the squad across a zone boundary or otherwise cause one member/queue
   to unload, if the test environment permits it.
4. Confirm only the affected member remains visible with its last-safe identity,
   condition `Cached / unavailable`, and a cached queue. Confirm its queue is
   read-only: `Jobs`, `Clear Queue`, drag, and removal controls are disabled.
   Other live member cards must remain editable.
5. Attempting a mutation on the cached queue must not crash, call into an
   unloaded object, or affect another member.
6. Confirm loaded members remain editable and their labels continue to refresh.
7. If the whole active squad becomes unavailable, confirm cached members show
   condition `Cached / unloaded` and the heading appends `read-only snapshot`.
   Move back into a loaded area or otherwise reacquire the member. Only after a
   fresh safe resolution should editing controls return.
8. Save/reload while the fallback is visible. Confirm stale row pointers are
   discarded and the queue remains read-only until safe live data exists.

## Stress and compatibility

- Repeat with 20 or more queue entries across several members.
- Test while paused and at each native game speed.
- Test an animal if Kenshi allows that animal to receive the same permanent
  jobs.
- Test UWE and Kaizo jobs with duplicate or unusual display names.
- Open/close the manager repeatedly during squad switching.
- Test at multiple UI scales and resolutions; confirm the full-screen layout
  keeps labels and controls readable.
- Test alongside other MyGUI/RE_Kenshi plugins.
- On CachyOS/Proton, verify `Ctrl+J`, `TAB`, native pause/speed controls, and
  the Options/Clear modal keys are delivered consistently and do not remain
  latched after focus changes.
- Watch `RE_Kenshi_log.txt` for access violations, failed row validation,
  unexpected batch continuation, stale-fingerprint clears, or window errors.

## Report template

Include:

- Kenshi version;
- RE_Kenshi version;
- Windows or Proton version;
- active UI/AI mods;
- save-backup name and whether the save was disposable;
- current squad members and their loaded/cached state;
- each member's displayed top-three supported stats and station-category
  Options preference;
- pause state and game speed before open, on open, and after close;
- whether the user changed pause/speed while the manager was open;
- exact queue before the action, including each job and target label;
- exact queue afterward;
- selected rows and batch order for immediate multi-select removal;
- the first failed row and the partial-result message, if a batch stopped;
- Clear Queue member name, reviewed count, and fingerprint result, if used;
- exact visible status text, including any settings save failure message;
- whether the vanilla panel and custom panel agreed;
- Stations tab target count, completion state, station count, failed-target
  count, truncation state, and every visible warning;
- station categories, persistent collapse state, station/unassigned counts,
  unassigned-first/alphabetical card order, omitted unavailable targets,
  renamed station names, filters, conditional areas, icons, and blocking cards;
- station detail name, assigned-person sort, hauling suppression case,
  assigned-left/available-right pane entries and independent scrolling
  (name/skill/value/total jobs), exact target,
  normalized task, automatic operation-plus-hauling bundle, right-click
  exact-target removal result, partial failures, blue recent-change feedback,
  and localized projection updates; also confirm other station-tab input did
  not change Jobs state, unrelated cards, or camera;
- relevant `RE_Kenshi_log.txt` lines;
- whether save/reload/reinstall/update preserved the station-category Options
  preference;
- whether save/reload preserved the queue result.
