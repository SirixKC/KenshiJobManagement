# Kenshi Job Manager Agent Notes

## Permanent local build environment

This machine has a persistent VC100 x64 build environment. Do not download or
recreate it under `/tmp` unless these paths are missing.

- Build root: `/home/sirix/.local/share/kjm-build`
- Visual C++ 2010 toolchain:
  `/home/sirix/.local/share/kjm-build/v100-toolchain/vc_stdx86`
- VC root:
  `/home/sirix/.local/share/kjm-build/v100-toolchain/vc_stdx86/Program Files/Microsoft Visual Studio 10.0/VC`
- Windows SDK 7.1:
  `/home/sirix/.local/share/kjm-build/v100-toolchain/sdkbuild/Program Files/Microsoft SDKs/Windows/v7.1`
- KenshiLib/Boost dependency root:
  `/home/sirix/.local/share/kjm-build/dependencies`
- KenshiLib headers and libraries:
  `/home/sirix/.local/share/kjm-build/dependencies/KenshiLib/Include` and
  `/home/sirix/.local/share/kjm-build/dependencies/KenshiLib/Libraries`
- The import library filename is `KenshiLib.lib` (use this spelling in local
  checks; Windows lookup itself is case-insensitive).
- Boost 1.60 headers:
  `/home/sirix/.local/share/kjm-build/dependencies/boost_1_60_0`

The compiler was extracted from the official Windows SDK 7.1 x64 image. The
source image was verified with SHA-1
`9203529f5f70d556a60c37f118a95214e6d10b5a` before extraction. KenshiLib is
version 0.4.0. Build with the VC10 `x86_amd64` cross-compiler. Do not substitute
MinGW or a newer MSVC ABI for a release DLL.

## Wine build on this machine

The ignored local helpers are:

- `build/Release/compile-v100.cmd`
- `build/Release/link-v100.cmd`

They already reference the permanent paths above. From the repository root,
run:

```bash
wine cmd /c 'Z:\home\sirix\Documents\Projects\KenshiJobManager\build\Release\compile-v100.cmd'
wine cmd /c 'Z:\home\sirix\Documents\Projects\KenshiJobManager\build\Release\link-v100.cmd'
```

Both commands must exit with code 0. Warnings from reconstructed KenshiLib,
MyGUI, and Ogre headers are expected. The linker must consume only
`build/Release/KenshiJobManagement.obj`; never re-add stale
`StationZoneAdapter.obj` or `StationOwnershipAdapter.obj` files.

If the ignored command files are missing, recreate them with these settings:

- `PATH`: VC `bin/x86_amd64`, Visual Studio `Common7/IDE`, then
  `vc_stdx86/Win/System`.
- `INCLUDE`: VC `include`, SDK `Include`, KenshiLib `Include`, KenshiLib
  `Include/ogre`, then Boost 1.60.
- `LIB`: VC `lib/amd64`, SDK `Lib/x64`, then KenshiLib `Libraries`.
- Compile flags: `/nologo /c /O2 /Oi /Gy /GL /MD /EHsc /W4`, with
  `WIN32_LEAN_AND_MEAN`, `NOMINMAX`, `BOOST_ALL_NO_LIB`,
  `BOOST_ERROR_CODE_HEADER_ONLY`, `NDEBUG`, `_CONSOLE`, `UNICODE`, and
  `_UNICODE`. The verified Release build also defines
  `KJM_GENERAL_JOB_TRANSFER_VERIFIED`.
- Link flags: `/nologo /DLL /MACHINE:X64 /SUBSYSTEM:CONSOLE /LTCG
  /INCREMENTAL:NO /DEBUG /OPT:REF /OPT:ICF`.
- Link libraries: `KenshiLib.lib MyGUIEngine_x64.lib OgreMain_x64.lib
  User32.lib`.

## Package and live installation

The package directory is `dist/KenshiJobManagement`. Copy the rebuilt DLL and
the current files from `mod/` into it. Update the ZIP while the command working
directory is `dist`; otherwise `7z` can add an incorrect leading `dist/`
directory:

```bash
7z u -tzip KenshiJobManagement-0.1.0-alpha.zip KenshiJobManagement
```

The live mod directory is:

`/home/sirix/.local/share/Steam/steamapps/common/Kenshi/mods/KenshiJobManagement`

Before replacing the live DLL, confirm that `kenshi_x64.exe` is not running.
After installation, compare the SHA-256 hashes of the build, package, and live
DLL. They must be identical.

## Required checks for the player-station and assignment Stations build

Run all of these before reporting an installed build:

```bash
python3 tools/validate_source.py
git diff --check
7z t dist/KenshiJobManagement-0.1.0-alpha.zip
```

Also inspect the final DLL strings. It must not contain
`getOwnedBuildingsH`, `ZoneMapContent::findAllBuildings`,
`SCANNING PLAYER STATIONS`, or `Player-owned station list`. Those belong to
the removed crash-prone world-discovery implementations. The Stations view
merges verified player-owned station-relevant BUILDING records from the
bracketed borrowed ownership storage with exact targets in readable loaded-
player permanent job queues. The strict player-owned workstation allowlist
excludes walls, lights, chairs, and generic default-task objects. Ownership
records are copied as scalar POD only and reconstructed one per guarded scanner
call. The Stations button processes the finite copied candidate list
synchronously before it displays the grid. Normalized results are published
internally in value-only batches of at most 16, followed by one grid build.
Never retain a borrowed source pointer or release engine memory. Assigned natural
resource nodes remain the explicit non-owned exception. The final station
result remains capped at 2,048; the source ownership copy uses its separate
8,192-record safety cap.

## Multi-squad Squad Jobs and transfer safety

The Squad Jobs tab publishes one value-only `AllSquadsSnapshot` in the exact
raw active/nonempty vanilla `TAB` order. It excludes empty squads and
`__DEAD__`. `g_squad` remains Kenshi's current squad and controls the selected
group highlight, bottom selector, `TAB`, and `Prioritize Core Jobs` target.
Squad headers collapse or expand only. Their MyGUI callback must defer the
widget rebuild until after the callback returns.

Visible member widgets bind through copied squad/member `HandleIdentity`
values. Never use a visible row index as an engine identity. Reacquire a fresh
active-roster snapshot before a non-current member mutation. A failed roster
refresh may retain cached values only after every cached member is marked
read-only. Cancel an armed drag before any grouped-board rebuild. Collapsed
groups must not create member or job widgets.

Same-member drops use verified `movePermajob`. A single cross-member MyGUI drop
captures only copied identities, presentation sequences, the exact source slot,
and the exact destination gap. On the next update tick, the deferred transaction
must verify both active-roster identities, capture and verify both complete
`GeneralJobQueueValue` snapshots, reject semantic duplicates, verify the native
appended row or two-row companion bundle, position it, revalidate both sides,
and remove the source last. Never retain `Tasker*`, `TaskData*`, or a borrowed
queue pointer. Multiple selected jobs remain remove-only.

General transfer passed its field probe. The tracked Release project defines
`KJM_GENERAL_JOB_TRANSFER_VERIFIED`; `KJM_GENERAL_JOB_TRANSFER_PROBE` is only
for a separate diagnostic build. Never define both. If transfer code changes or
the disposable-save matrix in `docs/TESTING.md` fails, remove the verified
Release define until the path is proven again. There is no compensating
rollback. After an interrupted add or insertion, retain the verified
destination copy, leave the source unchanged when possible, and require manual
review. Do not weaken post-mutation verification to make a probe pass.

`Prioritize Core Jobs` reorders existing roles only, in this order: Find and
Rescue, Find and Put in Bed, Medic, Robotics, Engineering. It never adds a
missing job. It targets only the current squad, validates stable member order,
and verifies the complete queue after every native move. Stop the remaining
batch on the first unverified result.
