# 0.1 field-test checklist

This build edits live AI state through reconstructed engine methods. Use a disposable save and keep the first run deliberately boring.

## Before launching

- Build `Release|x64` with the VC100 x64 toolset.
- Confirm the package contains the DLL, empty `.mod` marker, `RE_Kenshi.json`, and `README.txt`.
- Install to `mods\KenshiJobManagement\` and enable the mod.
- Keep other RE_Kenshi UI plugins enabled only when needed for compatibility testing.
- Back up the save.

## Startup and lifecycle

1. Reach the main menu and confirm no crash.
2. Load a disposable save.
3. Inspect `RE_Kenshi_log.txt` for the `0.1.0-alpha loaded` line.
4. Press `Ctrl+J`; the window should appear once, not flicker repeatedly.
5. Close it with both the window X and the Close button.
6. Reopen it with `Ctrl+J`.
7. Load another save while the window is open. The old window must be destroyed and the next open must show only the new save's selection.
8. Return to the main menu with the window open and confirm no crash.

## Read-only queue checks

1. Select a character with no permanent jobs. The list should show `[No permanent jobs]` and mutation buttons should be disabled.
2. Shift-assign three visibly different vanilla jobs.
3. Open or refresh the custom window and compare every row with Kenshi's vanilla job panel.
4. Change the selected character while the window remains open. It should update within roughly one quarter second.
5. Add or remove a job through vanilla controls while the window remains open. The custom list should follow without reopening.
6. Toggle vanilla Jobs and confirm the custom button caption follows.

## Jobs toggle

1. Click `Jobs: ON`; the selected character's vanilla Jobs state should turn off.
2. Click again; it should turn on.
3. Confirm another selected or nearby character is unaffected.
4. Trigger combat after toggling and confirm vanilla combat behavior still interrupts work normally.

## Remove one exact row

Test this before reordering.

1. Queue three distinct jobs A, B, and C.
2. Select B in the custom window and click Remove.
3. Expected result: A and C remain, in that order, in both interfaces.
4. Repeat with the first row, then the last row.
5. Repeat with two jobs that have the same visible name but different target stations. Only the selected target's row should disappear.
6. Change the vanilla queue immediately before clicking Remove. The plugin should refresh rather than blindly deleting a stale slot when row identity no longer matches.

## Reorder

`movePermajob(from, to)` is the least proven part of the first build.

1. Queue A, B, C.
2. Move B up. Expected queue: B, A, C.
3. Move B back down. Expected queue: A, B, C.
4. Move the first row down and the last row up.
5. Confirm Up is disabled on the first row and Down on the last.
6. Save, reload, and verify the reordered vanilla queue persisted.
7. Watch the active worker after moving its current top job. Record whether Kenshi immediately switches work, waits for the current action to finish, or behaves incorrectly.

If the actual ordering differs, stop testing reordering and attach the before/after log plus exact queue contents to an issue. Do not infer alternate index semantics from one test.

## Clear all

1. Queue at least three jobs.
2. Click Clear All once. It should change to Confirm Clear and remove nothing.
3. Wait more than four seconds. It should disarm and remove nothing.
4. Click twice within four seconds. Only the selected character's permanent jobs should be cleared.
5. Confirm immediate orders and other characters' queues are untouched.

## Stress and compatibility

- Repeat with 20 or more queue entries.
- Test while paused and at high game speed.
- Test an animal if Kenshi allows that animal to receive the same permanent jobs.
- Test UWE and Kaizo jobs with duplicate or unusual display names.
- Open/close the window repeatedly during squad switching.
- Test at multiple UI scales and resolutions.
- Test alongside other MyGUI/RE_Kenshi plugins.
- On CachyOS/Proton, verify `Ctrl+J` is delivered consistently and does not remain latched after focus changes.

## Report template

Include:

- Kenshi version;
- RE_Kenshi version;
- Windows or Proton version;
- active UI/AI mods;
- exact queue before the action;
- exact queue afterward;
- whether the vanilla panel and custom panel agreed;
- relevant `RE_Kenshi_log.txt` lines;
- whether save/reload preserved the result.
