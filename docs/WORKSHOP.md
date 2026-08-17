# Steam Workshop release sheet

This page is the operator checklist and ready-to-paste copy for the first
Workshop release. The author supplies the preview art.

## Workshop fields

- Title: `Kenshi Job Management [RE_Kenshi]`
- Version: `0.1.0-alpha`
- Tags: `Gameplay`, `GUI`, and `RE_Kenshi`
- Initial visibility: `Private`
- Required dependency: RE_Kenshi 0.3.4 or newer
- RE_Kenshi: https://github.com/BFrizzleFoShizzle/RE_Kenshi/releases
- Source: https://github.com/SirixKC/KenshiJobManagement

## Ready-to-paste description

```text
[h1]Kenshi Job Management[/h1]

[b]Alpha field-test release. Back up your save before first use.[/b]

Kenshi Job Management adds a full-screen interface for auditing and editing
permanent jobs without replacing Kenshi's job queues or AI.

[h2]Requirements[/h2]

[b]RE_Kenshi 0.3.4 or newer is required.[/b]
[url=https://github.com/BFrizzleFoShizzle/RE_Kenshi/releases]Download RE_Kenshi[/url]

Tested on Kenshi Steam 1.0.65 x64 with RE_Kenshi 0.3.4.

[h2]Features[/h2]

[list]
[*]View every permanent job for Kenshi's current squad.
[*]Reorder jobs by dragging within a member's queue.
[*]Toggle Jobs, remove selected rows across members, or clear one queue.
[*]See native portraits and each member's three highest supported stats.
[*]Switch active squads from the bottom strip or use vanilla TAB cycling.
[*]Audit player-owned workstations by category, assignment count, and status.
[*]Open a station to review assigned workers and assign or remove workers.
[*]Respect renamed buildings and exact building identities.
[/list]

[h2]Controls[/h2]

[list]
[*]Open: Ctrl+J or the Job Manager button beside the vanilla JOBS control.
[*]Squad Jobs: mouse wheel scrolls vertically; Shift+wheel scrolls jobs horizontally.
[*]Ctrl-click selects multiple job cards without activating Kenshi's mouse-rotate mode.
[*]Stations: click a station card for worker details.
[*]TAB remains Kenshi's normal squad-cycle control.
[/list]

[h2]Install[/h2]

[olist]
[*]Install RE_Kenshi 0.3.4 or newer.
[*]Subscribe to this item.
[*]Enable KenshiJobManagement in the Kenshi launcher.
[*]Confirm the main menu displays the RE_Kenshi version line.
[*]Load a backed-up save and open the manager with Ctrl+J.
[/olist]

Remove, Clear Queue, and station assignment changes are immediate. There is no
undo. The manager uses Kenshi's native permanent queues and pause system.

[h2]Source and license[/h2]

[url=https://github.com/SirixKC/KenshiJobManagement]Source code and issue tracker[/url]

GPL-3.0-only. Kenshi and its assets belong to Lo-Fi Games.
```

## Private first upload

1. Back up the clean package and the save used for the smoke test.
2. Copy only the current `dist/KenshiJobManagement` contents into
   `<Kenshi>/mods/KenshiJobManagement`. Do not carry `.bak`, logs, settings, or
   old probe DLLs into the upload folder.
3. Add the author's final preview through the FCS upload window. Kenshi
   community guidance recommends a square JPG no larger than 800x800. Steam
   accepts common web image formats; keep additional previews below 1 MB.
4. Start Kenshi from Steam and select `Game Editor`. Do not launch the FCS
   executable directly for an upload session.
5. Open `KenshiJobManagement` as the active mod, save once, select `Steam
   Workshop`, paste the fields above, and upload with `Private` visibility.
6. Find the private item under
   `steamapps/workshop/content/233860/<Workshop ID>`.
7. Confirm the downloaded item contains `KenshiJobManagement.dll`,
   `RE_Kenshi.json`, `KenshiJobManagement.mod`, `README.txt`, and all GUI PNGs.
   Hash the downloaded DLL and compare it with the release DLL.
8. Test from the downloaded Workshop folder with current RE_Kenshi. Verify the
   launcher entry, startup log, Ctrl+J, HUD button, both tabs, one safe reorder,
   one station detail view, save reload, and return to the main menu.
9. Do not make the item public until the Workshop download retains the DLL and
   this clean-install smoke test passes. If Steam or FCS strips the DLL, keep
   the Workshop item private and distribute the binary package through the
   GitHub release with explicit manual-install instructions.

Kenshi upload guidance: launch the FCS through Steam's `Game Editor`, open the
active mod, and use its `Steam Workshop` button:
https://steamcommunity.com/app/233860/discussions/0/1639790664929442171/

Steam preview API reference:
https://partner.steamgames.com/doc/api/isteamugc
