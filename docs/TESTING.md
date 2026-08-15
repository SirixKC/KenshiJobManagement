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
- **station** means one displayed world building/node column. Short clicks only
  select a column. Dragging an exact assignment card can move that job to a
  different loaded member.
- **assignment view incomplete** means the matrix is still resolving known
  queue targets, a target failed, or the 2,048-target safety cap was reached.
  Treat the result as incomplete until the
  banner clears and record any warning.

## Before launching

- Build `Release|x64` with the VC100 x64 toolset.
- Confirm the package contains the DLL, canonical blank FCS `.mod` marker,
  `RE_Kenshi.json`, and `README.txt`.
- Install to `mods\KenshiJobManagement\` and enable the mod.
- Keep other RE_Kenshi UI plugins enabled only when needed for compatibility
  testing.
- Make a disposable-save backup.
- Prepare a small squad with at least three members. Give two members
  distinguishable permanent jobs, and give one member no permanent jobs.
- If possible, place the squad at a player outpost with at least one crafting,
  farming, mining, or refining station. Keep one assigned natural iron or
  copper node available for the exact-target exception test.
- If possible, include two jobs with the same visible job name but different
  targets. This is needed to test exact row identity.
- Note each member's enabled base stats and values, the current game pause
  state, game speed, squad membership, and Jobs state before opening the
  manager.
- Record the current Options preference. On a clean installation, verify that
  its documented defaults are **Sciences** and **Trades**.

## Startup, layout, pause ownership, and lifecycle

1. Reach the main menu and confirm no crash.
2. Load the disposable save.
3. Inspect `RE_Kenshi_log.txt` for the `0.1.0-alpha loaded` line.
4. Record the pause state and speed immediately before opening the manager.
5. Press `Ctrl+J`. The native full-screen current-squad manager should appear
   once. It must not open a one-character popup.
   Confirm the primary backdrop is fully opaque and member rows, job cards,
   controls, portraits, and text stay fully opaque.
   Move the mouse wheel over the manager and confirm the game camera does not
   zoom. Confirm manager scroll views still respond to the wheel.
6. Confirm the manager invokes Kenshi's native pause on open. Record the new
   pause state and speed.
7. Confirm the heading resolves to `<squad name>  |  <N> member(s)` (or starts
   as `Current squad`) and appends `  |  read-only snapshot` only for a
   squad-level cached snapshot. Confirm the member-column header is exactly
   `SQUAD MEMBER  |  TOP ENABLED STATS`.
8. Confirm every current-squad member has one card or row, including the
   member with no jobs. Confirm the member name and `Jobs: ON` or `Jobs: OFF`
   state match Kenshi. Confirm each portrait shows that member's actual
   character image, not only the gray `Background/Normal` layer, and compare
   it with the same member in Kenshi's vanilla squad interface.
9. Confirm each member shows its own top three enabled base stats whose values
   are above 1. The three displayed stats must be computed per member, not
   selected as three aggregate stats across the squad. Confirm the three skills
   are stacked vertically in a larger, readable font and do not overlap the
   member controls.
10. Confirm each queue entry is a narrow, high-contrast selectable card with
   same-row drag behavior, no repeated `DRAG` label, and no priority number
   inside the card. Confirm `Jobs: ON` and `Clear Queue` captions stay inside
   their buttons at the smaller control font.
   Priority must appear once in the column header. A target-bearing card must
   center:

   ```text
   <Kenshi's live job text>
   V
   <live target name>
   ```

   Compare the labels with the vanilla job panel and the actual target in the
   world. The card may remove only a duplicate leading `<priority>:` from the
   work text. The job and target lines must have no synthetic prefixes. An
   unresolved target must show `Target unavailable`, tint the whole card red,
   and remain editable; no target means no arrow or target line. Confirm long
   work or renamed-target text wraps or scales down and does not clip or leave
   the card bounds.
   Confirm building-target cards fill in with the same category artwork used
   on Stations. The simple 2-to-4-color artwork must remain square and centered,
   with no horizontal stretching. Inspect the icon itself: its `ImageBox` must
   be `33%` opaque (`67%` transparent), independent of the dark overlay and
   parent/card alpha. Confirm categories fill progressively without a long UI
   stall.
   `Operating Machine` and `Operating Automatic Machine` must both display as
   `Operating...`; hover them and confirm the tooltip retains the full label.
   Select an unavailable-target card and confirm both its red warning and its
   selected state remain clear.
   Hover a job that appears on multiple members with the same exact target.
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

## Options and per-member stats

`Options` controls one global preference. It is not a separate filter stored on
each member.

1. Open `Options` from the main manager. Confirm the defaults on a clean
   installation are **Sciences** and **Trades**.
   Confirm the modal title is `Displayed Stats`, its help text says
   `Each member shows their three highest enabled base stats above 1. Changes
   save globally now.`, and its controls are `Select All`, `Clear All`,
   `Reset Default`, and `Close`.
2. Change the enabled base-stat preference and close the Options modal. Confirm
   the change saves immediately without a separate Apply button.
3. Confirm every member now computes its own top three enabled base stats above
   1 from that one preference. A member with fewer than three qualifying stats
   must show only the qualifying stats; do not fill the list with stats at 1 or
   below.
4. Confirm changing Options changes display data only. It must not add/remove
   jobs, change queue order or targets, change Jobs state, or change pause/speed.
5. Press `TAB` while Options is open. The modal must block Kenshi's
   current-squad cycling until it closes.
6. If `settings.ini` cannot be written, confirm the current display still
   applies and the manager status says exactly `Displayed-stat options were
   applied, but settings.ini could not be saved.`. Record that persistence is
   not proven for that change.
7. Save and reload the game. Confirm the changed preference remains; it must not
   change during reload.
8. Reinstall the same package over the disposable test installation. Confirm
   the preference remains.
9. Apply an update package without deleting the preference. Confirm the
   preference remains after the update as well.
10. Restore the default preference and record the result for the field report.

## Stations tab: queue-derived assignment matrix

Use a save with at least one player outpost, several station types, multiple
squads, and one permanent job aimed at a natural iron or copper node when
possible. Keep the vanilla building names visible so renamed-building support
can be checked with a second save or rename mod.

1. Open the manager with `Ctrl+J`. Confirm it opens on `Squad Jobs`. Click
   `Stations` and confirm the tab does not change the current squad or the
   selected character.
2. Confirm the first station view starts a lazy assigned-target pass. While it
   runs, the prominent banner says
   `READING ASSIGNED JOB TARGETS - RESULTS INCOMPLETE`, shows `Target <N> of
   <M>`, and fills station columns as exact queue targets resolve. Confirm the
   pass consumes no more than one stable target per UI update. Switch to `Squad
   Jobs`, then back; confirm target resolution pauses while hidden and resumes
   later. Do not treat the matrix as complete while the banner is present.
3. Confirm the matrix contains loaded building targets referenced by readable
   permanent queues from every loaded player squad. Confirm an unrelated,
   unassigned, public, or city building does not appear, even when a player
   queue references it. Confirm an assigned natural resource node can appear as
   the explicit non-owned exception. Confirm an unavailable target remains red
   on Squad Jobs, is omitted from Stations, and increments the failed-target
   warning because ownership cannot be verified. Confirm destroyed, incomplete, unpowered,
   broken, disabled, and dismantling assigned targets also remain visible with
   their blocking status.
4. Confirm station columns are grouped and ordered by area, category, and exact
   station name.
   Confirm a building renamed by a rename mod uses that exact name. If two
   names match in one area, confirm only the presentation adds `#1`, `#2`.
   Check material-output stations and special benches: copper ore, iron ore,
   UWE copper plates when UWE is loaded, iron plates, steel bars, copper alloy plates, electronics,
   crossbows, and skeleton limbs must use their specific pictograms on both
   tabs. Rename one of these buildings and confirm its icon does not change;
   an unknown or modded identity must fall back to its broad category icon.
5. Confirm each header shows a category label, exact station name, relevant
   skill, and blocking status when the station cannot work. Confirm known-skill
   stations disappear when that skill is disabled, while no-skill stations
   show `Relevant skill: None`. Confirm unknown stations are controlled by
   `Other / Unclassified`. Inspect the station header icon itself: its
   `ImageBox` must be `33%` opaque (`67%` transparent), independent of the
   dark overlay, and the same direct alpha rule must hold on Squad Jobs.
6. Confirm all loaded player characters from all loaded squads appear in
   vanilla order. Loaded squads start expanded. Collapse and reopen a squad;
   confirm only its member rows hide. An unloaded squad must start collapsed
   with `Live data unavailable`; a partly loaded squad must show its loaded
   members and one `<N> members unavailable` placeholder.
7. In the frozen roster, confirm each member shows the native portrait, name,
   condition (including dead/unconscious), Jobs state, permanent job count,
   `NO PERMANENT JOBS` or `N JOBS`, and the same filtered top-three skills as
   `Squad Jobs`. A queue-read failure must show `JOBS UNAVAILABLE` without
   hiding the member or blocking the matrix.
8. Confirm assigned cells show one compact left-aligned card per permanent job, with its
   exact queue priority and compact work label such as `Hauling...` or
   `Operating...`. Give one member `Hauling to` and `Operating machine` jobs for
   the same station and confirm they appear on separate rows. Confirm the cell
   tooltip retains the full order text. Confirm light gray, 33%-opaque divider
   lines separate the station columns in both the frozen header and scrolling
   matrix. Confirm the cell also shows the worker's station-relevant
   permanent base skill, including a value of 1 or below and a skill outside
   the top three. If a disposable test queue has more than five jobs for one
   station, confirm the cell shows `+N more jobs` and its tooltip still lists
   every job. Empty cells remain empty. Hover a
   cell and confirm the tooltip includes exact job text, station, priority,
   squad, relevant skill/value, and blocking status. Hover a station header
   and confirm its column and matching row highlight. Confirm there is no
   `UNASSIGNED` state in this milestone: a column exists only when at least one
   readable loaded queue references the target.
9. Give loaded members the generic Engineer, Medic, Robotics, and Rescue jobs.
   Confirm all four remain visible on Squad Jobs and in each worker's total job
   count, but none creates a Stations column or assignment cell. If the same
   target also has a station-specific job, confirm only that specific job is
   shown in the cell.
10. Short-click a station header, empty cell, or assignment card. Confirm it
   selects/highlights only the column. Drag one assignment card to a different
   loaded member's roster row, then repeat using a matrix cell in that member's
   row. Confirm the exact source job is appended to the destination queue with
   the same station target, then removed from the source. Confirm the complete
   destination row/cells highlight green while hovered. After a fully verified
   successful transfer, confirm only the source and destination rows change:
   station columns, scan progress/banner, filters, collapsed squads, scroll
   positions, header widgets, and selection must not reset or visibly rebuild.
   A same-member drop,
   unreadable queue, duplicate task type plus target, or drop outside a member
   row must make no change and show a nonblocking status. Jobs OFF, dead, and
   unconscious members remain valid when their live queue is readable.
11. During a station-card drag, switch tabs, close the window, press Esc, and
   attempt wheel scrolling. Confirm the drag cancels, capture clears, no widget
   is destroyed during its callback, and no mutation occurs. In a disposable
   test, change either queue before the deferred action runs and confirm the
   full-fingerprint check cancels the move. If source removal is forced to
   fail after a verified destination add, confirm the duplicate remains and
   the partial-failure status is explicit; there is no rollback.
   Make a separate queue change through Kenshi's vanilla controls after the
   transfer completes. Confirm that this external change uses the normal full
   projection refresh path, while the successful manager transfer did not
   restart the target scan or reset the board state.
12. Test vertical member scrolling and horizontal station scrolling. Confirm
    the roster and headers stay frozen and that scrolling remains usable with
    more rows and columns than fit on screen. Drag a station header or the
    gap between headers left and right. Confirm the matrix and scrollbar move,
    newly exposed columns remain visible during a normal full-width drag, the
    drag does not select a station, and a short click still selects one.
    Open/close the tab repeatedly
    and watch `RE_Kenshi_log.txt` for widget growth or access violations.
13. Disable and re-enable skills and station categories in `Options`. Confirm
    the page has clearly marked `CHARACTER SKILL FILTERS` and `STATION CATEGORY
    FILTERS`, both tabs open the same page, filters save immediately, and
    filters rebuild only the display. Restore the documented default categories
    (Crafting, Refining, Farming, Mining, Research, Other / Unclassified).
14. If a station target read fails, confirm target resolution continues and
    shows a red incomplete warning with the failed-target count. Use a stress
    save if possible to reach 2,048 assigned targets; confirm the pass stops
    and shows `Assigned target list truncated at 2,048. Results are
    incomplete.` Record the target count, station count, warning text, and
    whether any later target was skipped unexpectedly.

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
   exactly once, a held key does not repeat, and `TAB` does not traverse widget
   focus or trigger a queue mutation. Put an internal `__DEAD__` squad between
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
- each member's displayed stats and enabled Options preference;
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
- loaded squads/members shown in the station matrix, collapsed/unavailable
  groups, omitted unavailable targets, renamed station names, filters, and
  assignment priority cells tested;
- station-card transfer source/destination names, task, exact station target,
  both preflight fingerprints, and final queue result; also confirm all other
  station-tab input did not change Jobs state, station state, or camera;
- relevant `RE_Kenshi_log.txt` lines;
- whether save/reload/reinstall/update preserved the Options preference;
- whether save/reload preserved the queue result.
