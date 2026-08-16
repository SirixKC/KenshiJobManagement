# Borrowed ownership-list diagnostic

This diagnostic tests read-only access to the player faction's existing
`Ownerships::stuff` list. It does not call `getOwnedBuildingsH()`, allocate an
engine output container, or release engine memory.

It is not a release feature and it must not enter the release ZIP. The code is
compiled only when `KJM_SCANNER_PROBE` is defined. A normal Release build
remains the queue-derived, scanner-free Job Manager.

The prior allocation-and-release test is recorded in
`diagnostics/evidence/2026-08-15-ownership-release-crash.md`.

## Safety model

- The probe reuses the existing `PlayerInterface::updateUT` hook.
- It reads only `Faction::factionOwnerships->stuff` on the main game thread.
- It requires the resolved faction to be the player faction.
- It validates `count`, `maxSize`, `stuff`, owner identity, and the source
  header before and after each copy.
- Stages 1-3 copy only scalar `hand` fields into fixed plugin-owned POD arrays.
- It never stores an engine pointer. Stored pointer values are numeric identity
  markers only and are never dereferenced later.
- Stage 4 constructs one local `hand` from five scalar identity fields and
  verifies its fields and vtable. It calls no `hand` method.
- Stage 5 constructs that same record again and calls only `hand::isValid()`.
- Stage 6 requests `hand::getBuilding()` but does not dereference or retain the
  returned pointer.
- Stage 7 resolves the building again and verifies its live `getHandle()`
  identity. Stage 8 repeats that verification before calling only
  `Building::isThePlayer()`.
- No `Building*` or `hand*` leaves a guarded leaf function.
- It never modifies, clears, destroys, or releases the borrowed list.
- A world reset clears only plugin-owned POD state.

The probe is fail-closed, not crash-proof. Use a disposable save. Keep Kenshi
paused and keep the Job Manager closed during the test.

## Build

Use the permanent VC100 x64 environment documented in `AGENTS.md`:

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

Close Kenshi completely. Copy only the diagnostic DLL over the existing live
`KenshiJobManagement.dll`. Do not change `RE_Kenshi.json`, the `.mod` marker,
GUI assets, or settings. Do not load a second copy of the plugin.

## Test sequence

Load a disposable save completely. Pause Kenshi and keep the Job Manager
closed. Fully release the keys between presses.

1. Press `Ctrl+Shift+F10` once.
   - Resolves the player faction and `Ownerships` object.
   - Reads the borrowed `Ownerships::stuff` header twice.
   - Requires both reads to match exactly.
2. Press `Ctrl+Shift+F10` again.
   - Re-resolves the same owner.
   - Copies at most eight records' scalar fields.
   - Requires the source header to remain unchanged across the copy.
3. Press `Ctrl+Shift+F10` a third time.
   - Re-resolves the same owner.
   - Copies at most 8,192 records' scalar fields into fixed plugin storage.
   - Logs the copied count, `BUILDING` count, and truncation state.
   - Requires the source header to remain unchanged across the copy.
4. Press `Ctrl+Shift+F10` a fourth time.
   - Revalidates the owner, header, and record zero against stage 3.
   - Constructs one local `hand` through the five-field engine constructor.
   - Verifies all five scalar fields and the vtable.
   - Calls no `hand` method.
5. Press `Ctrl+Shift+F10` a fifth time.
   - Revalidates the same borrowed record.
   - Constructs it again and verifies the result.
   - Calls only `hand::isValid()`.
   - Revalidates the borrowed owner, header, and record after the call.
6. Press `Ctrl+Shift+F10` a sixth time.
   - Reconstructs and validates record zero again.
   - Calls `hand::getBuilding()`.
   - Logs only whether a pointer was returned. It does not dereference or
     retain that pointer.
7. If stage 6 found a loaded building, press `Ctrl+Shift+F10` a seventh time.
   - Resolves the building again from the scalar identity.
   - Reads its live `getHandle()` reference and verifies the vtable and all
     five identity fields.
   - Retains no pointer or reference.
8. Press `Ctrl+Shift+F10` an eighth time.
   - Repeats fresh resolution and exact handle verification.
   - Calls only `Building::isThePlayer()` and logs the Boolean result.
   - Retains no pointer or reference.

There is no ninth stage. This build has no engine-container allocation or
release operation.

## Logs and debugger exports

Look for `[KJM OwnershipProbe]` in `RE_Kenshi_log.txt`. Each line includes the
Windows thread ID.

The diagnostic DLL exports request-only entry points:

```text
KJM_ScannerProbe_RequestStage1
KJM_ScannerProbe_RequestInspect
KJM_ScannerProbe_RequestReadHandles
KJM_ScannerProbe_RequestConstructHand
KJM_ScannerProbe_RequestValidateHand
KJM_ScannerProbe_RequestGetBuilding
KJM_ScannerProbe_RequestVerifyBuilding
KJM_ScannerProbe_RequestCheckOwnership
KJM_ScannerProbe_GetState
```

The engine-facing work runs on the next normal `PlayerInterface::updateUT`
call.

Probe states are:

```text
 0  idle
 1  stable borrowed header captured
 2  up to eight scalar records copied
 3  full bounded scalar copy completed
 4  record zero reconstructed and verified
 5  record zero `hand::isValid()` call completed
 6  `hand::getBuilding()` completed
 7  exact live Building handle verified
 8  `Building::isThePlayer()` completed
-1  guarded failure
-2  abandoned after an invalid or changing header
```

## Restore

Close Kenshi and restore the stable DLL. Verify that its SHA-256 matches the
package or recorded pre-test hash before normal play.
