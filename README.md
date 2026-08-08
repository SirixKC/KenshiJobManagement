# Kenshi Job Management

A native in-game job-management interface for Kenshi, built as a KenshiLib plugin loaded by RE_Kenshi.

## Current milestone: 0.1.0-alpha queue field test

Press **Ctrl+J** in game to open a native Kenshi window for the currently selected player character. The first build deliberately operates on Kenshi's existing permanent-job queue rather than introducing a parallel scheduler.

It currently supports:

- live inspection of the selected character's permanent-job queue;
- toggling that character's vanilla **Jobs** state;
- moving a selected job up or down with `movePermajob`;
- removing one exact queue slot with `removePermajob`;
- guarded clear-all using `clearPermajobs`;
- automatic redraw when the selected character or vanilla queue changes;
- cleanup when `PlayerInterface::clearAndReset` runs during game-state transitions.

The larger outpost interface will grow on top of this proven adapter. Station discovery, worker-to-machine assignment, batch operations, role presets, and an optional priority scheduler are tracked in [`docs/DESIGN.md`](docs/DESIGN.md).

## Validation status

This is a **source-first field-test build**. It has not yet been compiled with the VC100 toolchain or run inside Kenshi. The included validator checks the source tree, manifest, project XML, MyGUI dependency, and critical job calls, but it cannot establish runtime calling conventions or prove `movePermajob` behavior. Keep a backup and test on a disposable save first.

## Build requirements

- Windows build environment;
- Visual Studio 2019 or newer as the IDE;
- the Visual C++ 2010 **x64** toolset (`v100`);
- RE_Kenshi 0.3.1 or newer;
- KenshiLib headers and libraries, including the real Git LFS copies of `kenshilib.lib` and `MyGUIEngine_x64.lib`;
- Boost 1.60 headers and VC100 libraries from the KenshiLib dependency package.

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

dist\KenshiJobManagement-0.1.0-alpha.zip
```

## Install and first test

Copy the generated folder to `<Kenshi>\mods\KenshiJobManagement\`, enable it in the launcher, load a disposable save, select a recruit with several Shift-assigned jobs, and press **Ctrl+J**.

Test removal before reordering. After each action, verify both the new window and Kenshi's vanilla job panel, then inspect `RE_Kenshi_log.txt`. The complete field checklist is in [`docs/TESTING.md`](docs/TESTING.md).

## Design rule

> New management UI, vanilla queues, vanilla AI.

The plugin should not replace Kenshi's GOAP system. It should translate clearer player intent into ordinary task types and stable target handles, then let Kenshi execute those jobs normally.

## License

GPL-3.0-only, matching KenshiLib's license.
