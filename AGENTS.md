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
  `_UNICODE`.
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

## Required checks for the queue-derived Stations build

Run all of these before reporting an installed build:

```bash
python3 tools/validate_source.py
git diff --check
7z t dist/KenshiJobManagement-0.1.0-alpha.zip
```

Also inspect the final DLL strings. It must not contain
`getOwnedBuildingsH`, `ZoneMapContent::findAllBuildings`,
`SCANNING PLAYER STATIONS`, or `Player-owned station list`. Those belong to
the removed crash-prone world-discovery implementations. The Stations view is
derived only from exact targets in readable loaded-player permanent job queues.
