Kenshi Job Management 0.1.0-alpha
=================================

This is a field-test build for RE_Kenshi/KenshiLib.

Open the Stage 1 squad audit/editor:
  Ctrl+J

The native full-screen window replaces the old one-character popup. Its primary
backdrop is fully opaque so cards, controls, and text remain easy to read. It shows
the current squad's live or per-member cached permanent-job queues. Each member
has a Jobs toggle and a Clear Queue control. Each narrow, high-contrast card
centers Kenshi's live job text above a `V` and the exact target name, with no
synthetic `Job:` or `Target:` prefix and no duplicate priority number:
  <live work type>
  V
  <live target name>
Long work or renamed-target text fits without clipping. If a live target cannot be resolved, the
target line is `Target unavailable` and the whole card is highlighted red.
Building-target cards reuse the square station-category artwork without
stretching it. The icon ImageBox itself is 33% opaque (67% transparent) behind
the card text; the dark overlay is separate. `Operating Machine` and
`Operating Automatic Machine` display as
`Operating...`; their complete Kenshi text remains in the tooltip.
Hover a card to outline every displayed card with the same job type and exact
target. Matching groups are cached when captured job identities change.
Member portraits use Kenshi's generated portrait and native overlay order.
An empty live queue shows `[No permanent jobs]`; an unavailable queue shows
`Queue unavailable (read-only)`.

Use same-row drag to reorder one member's queue. Select exact rows across
members for immediate Remove Selected, or drop a row onto the remove target.
Both actions have no prompt and no undo. A batch stops at its first failure and
reports the partial result. Each member shows its own top three enabled base
stats above 1. Options changes save immediately to one global preference and
persist across reload, reinstall, and update; defaults are Sciences + Trades.
If the settings write fails, the display still applies for the session and the
status says:
  Displayed-stat options were applied, but settings.ini could not be saved.
The manager invokes native pause on open, restores prior pause/speed on an
ordinary close, and preserves a user's resumed speed if they change it while
the manager is open. Each TAB press advances once on the main manager and
skips Kenshi's internal `__DEAD__` holding squad; Options and Clear Queue
modals block TAB through the native
`PlayerInterface::cycleSquad` hook. Unloaded or cached queues are read-only per
member. Other live member cards remain editable, and a live queue stays
editable if only its target is unavailable.

STATIONS TAB
------------
The `Stations` tab is an assignment view. It shows every readable
loaded player character against station columns derived from exact targets in
loaded members' permanent job queues.
Rows keep Kenshi's vanilla squad and member order and can be collapsed by
squad. The frozen roster shows each portrait, condition, Jobs state, permanent
job count, and the same filtered top three skills as the squad tab. Each station
column shows its area, category label, exact station name (including names from
building-rename mods), relevant skill, blocking status, and permanent queue
assignments for each worker. The portrait is enlarged, and the top-three skill
list is stacked vertically in a larger font. If one worker has several jobs for the same station, each job
gets a separate compact left-aligned card such as `1  Hauling...` or
`2  Operating...`; the tooltip retains the full order text. Light gray,
33%-opaque lines separate station columns. Drag one assignment card to another
loaded member row to move the exact permanent job while keeping its station
target. The job appends to the destination queue.

After a verified successful transfer, the board patches the source and
destination rows in place. Station columns, scan progress, scroll positions,
collapsed squads, and selection remain stable. A full projection refresh is
used only as the fail-closed fallback when the projection cannot be patched or
the queue changed outside the manager.

The main manager backdrop is fully opaque. While the manager is open, the
mouse wheel scrolls its views without also changing the game camera.

No zones, ownership lists, towns, or unrelated city buildings are enumerated.
Only live player-owned buildings appear. Assigned natural resource nodes are
the deliberate exception because Kenshi does not make them player-owned.
Loaded assigned targets remain visible when incomplete,
unpowered, broken, disabled, or being dismantled; destroyed or invalid handles
are excluded. `Other / Unclassified`
can be hidden in the shared Options page. Skill filters affect both member
skills and station visibility. The default enabled station categories are
Crafting, Refining, Farming, Mining, Research, and Other / Unclassified.
The nine broad station categories use simple 2-to-4-color pictograms: anvil,
furnace, wheat, pickaxe, research book, training dummy, crate, shield, and
gear. Stable building/functionality identities can select more specific icons
for copper ore, iron ore, UWE copper plates, iron plates, steel bars, copper alloy
plates, electronics, crossbows, and skeleton limbs. The live display name is
not used for this choice, so building-rename mods remain respected. An
unmatched subtype falls back to its broad category icon. The same artwork is
used on both tabs.

The target pass starts when the tab is first opened. It copies exact building
targets referenced by loaded permanent jobs, then resolves at most one stable
handle per UI update. An unloaded or unreadable target remains visible with a
red warning on Squad Jobs, but Stations omits it because ownership cannot be
verified and reports the target-read failure. While it runs, the tab says
`READING ASSIGNED JOB TARGETS - RESULTS INCOMPLETE` and shows target progress.
Failed targets and the 2,048-target cap are reported as visible warnings. The matrix
uses frozen headers, vertical and horizontal scrolling, and virtualized rows
and columns. Drag the station header strip left or right to pan without
selecting a station. Global Engineer, Medic, Robotics, and Rescue jobs remain on the
Squad Jobs tab and in each worker's total job count, but never create station
columns or assignment cells. Transfers validate both full queues, add and
verify the destination first, and remove the exact source row last. A source
removal failure leaves the duplicate and reports it; there is no rollback. Camera
centering, safe full-world building discovery, world highlights, roles,
new assignment creation, and an unassigned-building toggle are planned later.

SAFETY
------
This build has been compiled successfully with the VC100 x64 toolchain, but it
has not yet completed in-game testing. Use a disposable save. Keep a backup
before installation. Test single-member Remove and Clear Queue before
multi-member batch removal or drag reorder. Remove Selected and drop-to-remove
are immediate and irreversible. Inspect RE_Kenshi_log.txt after the first run
and after every failed mutation.

INSTALL
-------
Place this entire folder at:
  <Kenshi>\mods\KenshiJobManagement\

Enable KenshiJobManagement in the launcher and ensure RE_Kenshi is installed.

The detailed disposable-save checklist, including the Stations field test, is
in docs\TESTING.md.

License: GPL-3.0-only.
