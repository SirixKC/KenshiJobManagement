# Ownership scanner diagnostic

This diagnostic isolates `Ownerships::getOwnedBuildingsH()` from the normal
Stations view. It is not a release feature and it must not enter the release
ZIP.

The diagnostic code is compiled only when `KJM_SCANNER_PROBE` is defined. A
normal Release build remains the queue-derived, scanner-free Job Manager.

## Safety model

- It reuses the existing `PlayerInterface::updateUT` hook. It does not install
  a second hook or plugin.
- It queries only `Faction::factionOwnerships`. It does not inspect platoon
  ownership objects.
- It runs only after a hotkey edge on the main game thread.
- It never frees or clears the returned `lektor<hand>`.
- It never calls a method on a returned `hand`.
- A world reset abandons all retained game pointers without accessing them.
- The probe is one-shot. Restart Kenshi before repeating stage 1.

The intentionally retained result can leak one small allocation until process
exit. Use this build only for one controlled test session.

## Build

Use the permanent VC100 x64 environment documented in `AGENTS.md`. The tracked
helpers are:

```text
diagnostics/build/compile-v100.cmd
diagnostics/build/link-v100.cmd
```

Run them from the repository root:

```bash
wine cmd /c 'Z:\home\sirix\Documents\Projects\KenshiJobManager\diagnostics\build\compile-v100.cmd'
wine cmd /c 'Z:\home\sirix\Documents\Projects\KenshiJobManager\diagnostics\build\link-v100.cmd'
```

The output is:

```text
build/ScannerProbe/KenshiJobManagement.dll
build/ScannerProbe/KenshiJobManagement.pdb
```

The probe uses `/O2 /Oi /Gy /GL /Zi /Oy- /DKJM_SCANNER_PROBE` and links with
`/LTCG`. KenshiLib's `GetRealAddress()` requires whole-program optimization;
without it, Kenshi stops at an assertion during plugin startup.

## Install

Close Kenshi completely. Back up the current live DLL outside the mod folder,
then copy only the diagnostic DLL over the existing live DLL:

```text
<Kenshi>/mods/KenshiJobManagement/KenshiJobManagement.dll
```

Do not change `RE_Kenshi.json`, the `.mod` marker, GUI assets, or
`settings.ini`. Do not load a second copy of the plugin.

## Test sequence

Use a disposable save. Load the save completely and pause Kenshi. Keep the Job
Manager window closed during the probe.

1. Press `Ctrl+Shift+F10` once.
   - Resolves the player faction and its faction-level `Ownerships` object.
   - Calls `getOwnedBuildingsH()` once.
   - Does not read or free the output.
2. If Kenshi remains open, release the keys and press `Ctrl+Shift+F10` again.
   - Reads only the source and output `lektor` headers.
   - Logs `count`, `maxSize`, `stuff`, and whether the pointers alias.
3. If Kenshi remains open, release the keys and press `Ctrl+Shift+F10` a
   third time.
   - Revalidates the header.
   - Reads at most eight raw `hand` records.
   - Does not resolve any building.

Do not reload or import a save between stages. If the world resets, restart
Kenshi before another probe.

## Logs and debugger exports

Look for `[KJM OwnershipProbe]` in `RE_Kenshi_log.txt`. Each line includes the
Windows thread ID. Copy the log before restarting Kenshi after a crash.

The diagnostic DLL exports these debugger entry points:

```text
KJM_ScannerProbe_RequestStage1
KJM_ScannerProbe_RequestInspect
KJM_ScannerProbe_RequestReadHandles
KJM_ScannerProbe_GetState
```

The first three exports only request work. The engine-facing operation runs on
the next `PlayerInterface::updateUT` call, on the normal game thread.

Probe states are:

```text
 0  idle
 1  stage 1 returned
 2  stage 2 completed
 3  stage 3 completed
-1  guarded failure
-2  abandoned after identity change, alias detection, or world reset
```

## Restore

Close Kenshi and restore the stable DLL. Verify that its SHA-256 matches the
package or the recorded pre-test hash before continuing normal play.
