# Kenshi Job Management

Kenshi Job Management adds a native, full-screen interface for auditing and
editing Kenshi's permanent jobs. It works through RE_Kenshi and keeps Kenshi's
normal job queues and AI in charge.

**Version: 0.1.0-alpha**

## Disclaimer

This is an alpha field-test release. It has been tested for many hours on the
author's machine, but it is not guaranteed safe for every save, mod list, or
game version. Back up your save and use a disposable save before trying it.
Actions that change jobs may have no undo.

I use Radiant Dark True UI, so the layout may look different with other UI
mods. The manager includes Vanilla light and Dark UI-friendly appearance
options.

## Requirements

- Kenshi Steam 1.0.65 x64 (the tested game version);
- RE_Kenshi 0.3.4 or newer;
- Windows 64-bit.

Install [RE_Kenshi from GitHub](https://github.com/BFrizzleFoShizzle/RE_Kenshi/releases)
or [Nexus Mods](https://www.nexusmods.com/kenshi/mods/847) before installing
this mod.

## Features

### Squad Jobs

- Open the manager with **Ctrl+J** or the Job Manager button beside Kenshi's
  vanilla `JOBS` HUD control.
- View every active, nonempty player squad in Kenshi's normal `TAB` order.
- See native portraits, each member's Jobs ON/OFF state, permanent-job queues,
  exact live targets, and the member's top supported base stats.
- Reorder jobs within a member by dragging. Select several jobs with
  Ctrl-click or Shift-click, copy them with Ctrl+C, and paste them to selected
  recipients with Ctrl+V.
- Select recipients by clicking portraits or the bottom squad selector.
  Ctrl-click toggles people or whole squads. Job selection and recipient
  selection are separate modes.
- Add missing Rescue, Put in Bed, Medic, Robotics, and Splinting jobs with the
  compact Add Medic/Robotics button. `Prioritize Healing` reorders existing
  healing jobs.
- Use `Remove Invalid Jobs` to clean up unresolved fixed-target jobs.
  `Clear Queue` asks for confirmation; individual removal is immediate.
- Unloaded or unavailable queues are shown read-only. A live queue with an
  unavailable target remains visible and editable.

### Stations

- Browse verified player-owned workstations and assigned natural resource nodes
  in a category-grouped card grid.
- See exact station names, including renamed buildings, area, assignment count,
  and blocking status. Walls, lights, chairs, and generic non-work objects are
  not treated as workstations.
- Open a station for assigned and available workers. Click a candidate or press
  Enter to assign a worker; right-click an assigned worker to remove verified
  station jobs.
- Workers sort by relevant skill. Training stations sort lowest skill first so
  the workers who benefit most appear first.
- Station categories and the Vanilla/Dark appearance mode are saved in
  `Options`.

The game pauses while the manager is open and restores the prior pause/speed
state when it closes, unless you deliberately resume or change speed.

## Controls

| Action | Control |
| --- | --- |
| Open manager | `Ctrl+J` or the HUD Job Manager button |
| Select one job | Click a job card |
| Add/remove a job from selection | `Ctrl`-click a job card |
| Select a same-member range | `Shift`-click a job card |
| Reorder a job | Drag within that member's queue |
| Copy selected jobs | `Ctrl+C` |
| Paste copied jobs | Select recipients, then `Ctrl+V` |
| Select one recipient | Click a portrait |
| Toggle recipients | `Ctrl`-click portraits or squad buttons |
| Switch squad | Click a bottom squad button or use vanilla `TAB` |
| Scroll squad rows | Mouse wheel |
| Scroll the squad selector | `Shift` + mouse wheel over the selector |
| Open station details | Click a station card |
| Assign a station worker | Click the worker or press `Enter` |
| Remove station jobs | Right-click an assigned worker |

Stations has no drag-to-transfer action. Job removal, queue clearing,
copy/paste, healing actions, and station assignments change live game state;
review the status message before making another change.

## Installation

1. Install RE_Kenshi 0.3.4 or newer.
2. Download `KenshiJobManagement-0.1.0-alpha.zip`.
3. Extract or copy its `KenshiJobManagement` folder to
   `<Kenshi>\mods\KenshiJobManagement\`.
4. Keep `KenshiJobManagement.dll`, `KenshiJobManagement.mod`,
   `RE_Kenshi.json`, and the `gui` folder together.
5. Enable the mod in the Kenshi launcher.
6. Load a backed-up disposable save and press `Ctrl+J`.

The package includes the plugin DLL, the RE_Kenshi manifest, the Kenshi mod
marker, and the GUI artwork. If the HUD artwork is unavailable, the button
falls back to a `JM` caption.

## Build from source

The supported release build uses the Visual C++ 2010 (`v100`) x64 toolset.
You also need KenshiLib 0.4.0 with `KenshiLib.lib`, `MyGUIEngine_x64.lib`,
and `OgreMain_x64.lib`, plus Boost 1.60 headers.

From a Visual Studio Developer PowerShell, set the dependency paths and build
`KenshiJobManagement.sln` (which builds `KenshiJobManagement.vcxproj`):

```powershell
$env:KENSHILIB_DIR = 'C:\path\to\KenshiLib'
$env:BOOST_INCLUDE_PATH = 'C:\path\to\boost_1_60_0'
msbuild .\KenshiJobManagement.sln /m /t:Build /p:Configuration=Release /p:Platform=x64
```

The project writes the ready-to-package folder to
`dist\KenshiJobManagement\`. Create the ZIP from that folder after a successful
build. Do not use MinGW or a newer MSVC ABI for the release DLL.

## Reporting bugs and suggestions

Please report bugs, slowdowns, crashes, or strange text on the
[GitHub issue tracker](https://github.com/SirixKC/KenshiJobManagement/issues).
Include:

- Kenshi and RE_Kenshi versions;
- the save/mod setup and clear reproduction steps;
- whether the problem occurred in Squad Jobs or Stations;
- a screenshot and relevant `RE_Kenshi_log.txt` lines.

Do not attach a valuable campaign save. Use a copy or a disposable test save.

## Credits and license

Codex generated the code and GUI icons. I wrote the prompts, tested the mod in
game, took the screenshots, and made the preview art.

Kenshi and its assets belong to Lo-Fi Games. RE_Kenshi and KenshiLib are
separate projects with their own licenses and release notes.

This project is licensed under **GPL-3.0-only**.
