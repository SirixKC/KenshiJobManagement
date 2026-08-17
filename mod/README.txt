Kenshi Job Management 0.1.0-alpha
=================================

This is an alpha field-test build for RE_Kenshi 0.3.4+ / KenshiLib.

Open the Stage 1 squad audit/editor:
  Ctrl+J

The vanilla HUD JOBS row also contains a square Job Manager sibling. It uses
the live JOBS control size, remains under the Orders panel, and shows the
tooltip `Open Job Manager (Ctrl+J)`. Its click is deferred until after vanilla
updateUT; Ctrl+J remains the fallback. GUI rebuilds reacquire the entry, and
reset restores the original JOBS rectangle only when its live rectangle still
matches the plugin split. The packaged `gui/kjm-hud-icon.png` is loaded
independently; the button uses a `JM` caption if the texture is unavailable at
runtime.

The native full-screen window replaces the old one-character popup. Its primary
backdrop is fully opaque so cards, controls, and text remain easy to read. It shows
the current squad's live or per-member cached permanent-job queues. Each member
has a Jobs toggle and a Clear Queue control. Each station-display card centers
Kenshi's live job text above a `V` and the exact target name, with no synthetic
`Job:` or `Target:` prefix and no duplicate priority number:
  <live work type>
  V
  <live target name>
Long work or renamed-target text fits without clipping. If a station-display
target cannot be resolved, the target line is `Target unavailable` and the whole
card is highlighted red. Building-target cards reuse the square station-category
artwork without stretching it. The icon ImageBox itself is 33% opaque (67%
transparent) behind the card text; the dark overlay is separate. Global
Builder/Engineering, Medic, Robotics, and Rescue rows retain their exact stored
targets internally for mutation verification, but their Squad cards omit target
text, arrow, unavailable tint, and station-target artwork. Role art is selected by
TaskType: `JOB_BUILDER` uses optional `job-engineering.png` with a
`station-other.png` fallback; `JOB_REPAIR_ROBOT` uses the existing skeleton-limb
icon; and `JOB_MEDIC`, `FIND_AND_RESCUE`, `FIND_BED_AND_PUT_IN`, and the live
permanent Rescue wrapper `FIND_AND_RESCUE_IF_THERES_BEDS` use optional
`job-medic.png`. Role icons use the same direct 33% card-art alpha. `Operating Machine` and
`Operating Automatic Machine` display as `Operating...`; their complete Kenshi
text remains in the tooltip.
Hover a station-display card to outline every displayed card with the same job
type and exact target. Global behavior cards group by task type without the
incidental target. Matching groups are cached when captured job identities
change. Member portraits use Kenshi's generated portrait and native overlay
order; the portrait remains `80x80` at `y=11`, with the name/condition/skills
block at `y=8/33/49` and a 48-pixel skills block in a 120-pixel row.
An empty live queue shows `[No permanent jobs]`; an unavailable queue shows
`Queue unavailable (read-only)`.

Use same-row drag to reorder one member's queue. Select exact rows across
members for immediate Remove Selected, or drop a row onto the remove target.
Both actions have no prompt and no undo. A batch stops at its first failure and
reports the partial result. Each member shows its own top three supported base
stats above 1. The Options page contains broad station-category filters only;
they apply to Stations, while Squad Jobs always shows every job and uses all
supported stats when selecting each member's top three. Category changes save
immediately and persist across reload, reinstall, and update.
The manager invokes native pause on open, restores prior pause/speed on an
ordinary close, and preserves a user's resumed speed if they change it while
the manager is open. Each TAB press advances once on the main manager and
skips Kenshi's internal `__DEAD__` holding squad; Options and Clear Queue
modals block TAB through the native
`PlayerInterface::cycleSquad` hook. Unloaded or cached queues are read-only per
member. Other live member cards remain editable, and a live queue stays
editable if only its target is unavailable.

SQUAD JOBS SELECTOR
-------------------
The `Squad Jobs` tab has a compact single-row selector along its bottom edge.
It copies active, nonempty player squads in the exact raw active/nonempty
vanilla `TAB` order. Empty squads and Kenshi's internal `__DEAD__` squad are
excluded. The current squad has the exact selected highlight.

A click queues only the squad's value identity. The update path uses a fresh validated `setCurrentPlatoon` path.
It rereads the result before refreshing the
jobs view and retains no `Platoon` or `ActivePlatoon` engine pointer. The strip
has independent horizontal overflow: plain wheel over the strip still scrolls
the member rows, while Shift+wheel scrolls only the strip. Ordinary `TAB`
cycling remains vanilla-owned and moves the selector highlight after Kenshi's
native cycle.

STATIONS TAB
------------
The `Stations` tab is a category-grouped card grid, not a member-by-station
board. It shows verified player-owned workstations and exact station targets
from readable player queues. The strict player-owned workstation allowlist
excludes walls, lights, chairs, and any other object accepted only because it
has a generic task. An assigned natural resource node is the one explicit
non-owned exception. Builder/Engineering, Medic, Robotics, Rescue, and Put-in-bed remain on
Squad Jobs and never create a station card. Their exact stored targets remain
available for mutation verification, but Squad card presentation omits target
text, arrow, unavailable tint, and station-target artwork. Their incidental
subjects never create a Stations projection.

Category headers show persistent `+`/`-` collapse state, station count, and
unassigned count. Within each category, unassigned cards appear first, then
cards sort alphabetically by exact station name. A card shows the exact name
(including a rename-mod name), a translucent category icon, unique assigned
people, and a red blocking status only when work is prevented. A usable card
with nobody assigned has a thin yellow outline and a large red X. An idle
bench's zero power allocation is not an outage: NO POWER requires active unmet
demand, while a manual switch-off appears as POWER OFF. The area appears only
when the result contains more than one area.
Destroyed stations remain visible for cleanup, but new assignment is disabled;
existing verified assignments can still be removed.

Click a card to open a wide, fully opaque detail modal. Assigned workers stay
on the left and available workers remain visible on the right, with independent
scrollbars. Portraits have a fixed column and normal text is pure white. Both
lists sort by relevant skill from highest to lowest, except Training stations
sort from lowest to highest. Ties use total permanent jobs, then name. Each row
shows name, skill/value, and total jobs. An OPERATE_STORAGE/hauling entry is hidden only when that same worker has
a non-hauling exact-target station job. Click a candidate or press `Enter` for
an immediate verified assignment. Automatic-machine work uses Kenshi's native
operation-plus-hauling bundle; storage tasks normalize to `OPERATE_STORAGE`.
An unsupported assignment contract disables the available pane instead of
guessing a task. Right-click an assigned person to immediately remove all verified
existing exact-target station jobs for that worker. There is no Apply button
and no undo.

After a verified change, update only the affected card, detail list, category
count, and worker cache. Keep the card position fixed until the modal closes
and show blue recent-change feedback. A stale queue, load transition, or failed
verification is fail-closed. Squad Jobs retains same-row drag reorder; Stations
has no drag-to-transfer interaction.

The main manager backdrop is fully opaque. While the manager is open, the
mouse wheel scrolls Squad Jobs vertically over every control. Hold Shift while
using the wheel to scroll its job columns horizontally. Manager wheel input
does not also change the game camera. Ctrl-click multi-selection keeps the
pointer live even when Ctrl is bound to Kenshi's native mouse-rotate control.

The station pass merges verified live player-owned station-relevant buildings
from Kenshi's borrowed ownership records with exact targets from readable
loaded-player permanent job queues. It does not enumerate zones, towns, or
unrelated city buildings. Assigned natural resource nodes are the deliberate
exception because Kenshi does not make them player-owned; assigned unknown
player targets may remain in `Other / Unclassified`. Player-owned workstations
with no readable queue assignment remain visible with a thin yellow outline and
a large red `X`.
Loaded assigned targets remain visible when incomplete, unpowered, broken,
disabled, being dismantled, or destroyed; a red status marks any state that
prevents work. Invalid or unresolvable handles are excluded.
`Other / Unclassified` can be hidden in the shared Options page. Category
filters affect only station visibility. The default enabled station
categories are Crafting, Refining, Farming, Mining, Research, and Other /
Unclassified.
The nine broad station categories use simple 2-to-4-color pictograms: anvil,
furnace, wheat, pickaxe, research book, training dummy, crate, shield, and
gear. Stable building/functionality identities can select more specific icons
for copper ore, iron ore, UWE copper plates, iron plates, steel bars, copper alloy
plates, electronics, crossbows, and skeleton limbs. The live display name is
not used for this choice, so building-rename mods remain respected. An
unmatched subtype falls back to its broad category icon. The same artwork is
used on both tabs.

The target pass starts when the tab is first opened. It brackets the borrowed
ownership source into scalar plugin-owned records and copies exact building
targets referenced by loaded permanent jobs. It resolves the complete bounded
candidate list before showing the grid; each candidate remains isolated in one
guarded call. Normalized results append internally in value-only batches of up
to 16, then the visible station cards are built once. This causes a short
button-click pause instead of streaming an incomplete board. Squad-tab edits
defer this refresh until Stations is opened. An unloaded or unreadable assigned target remains visible
with a red warning on Squad Jobs, but Stations omits it and reports the
target-read failure. An unloaded ownership record is omitted without one red
error per record; a validation fault reports one ownership-pass warning. The
final 2,048-station result cap and separate 8,192-record ownership copy cap are
visible warnings. The scanner never retains a borrowed source pointer or an
engine object pointer across a step, never enumerates zones or unrelated world
containers, and never uses a generic default task to classify a station.
Camera centering, roles, and world highlights are outside this view.

SAFETY
------
This build has completed repeated in-game tests on Kenshi Steam 1.0.65 with
RE_Kenshi 0.3.4, including Squad Jobs, Stations, assignment changes, squad
switching, and UI reset/reopen flows. It remains alpha software. Use a
disposable save for the first test and keep a backup before installation. Test
single-member Remove and Clear Queue before multi-member batch removal or Squad
Jobs same-row drag reorder. Remove Selected and drop-to-remove
are immediate and irreversible. Inspect RE_Kenshi_log.txt after the first run
and after every failed mutation.

INSTALL
-------
Place this entire folder at:
  <Kenshi>\mods\KenshiJobManagement\

Enable KenshiJobManagement in the launcher and ensure RE_Kenshi 0.3.4 or newer
is installed:
  https://github.com/BFrizzleFoShizzle/RE_Kenshi/releases

The detailed disposable-save checklist, including the Stations field test, is
in docs\TESTING.md.

License: GPL-3.0-only.
