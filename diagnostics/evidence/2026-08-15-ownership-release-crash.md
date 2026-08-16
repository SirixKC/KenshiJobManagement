# Ownership output release probe — 2026-08-15

## Result

`Ownerships::getOwnedBuildingsH(lektor<hand>&)` returned successfully on
Kenshi Steam 1.0.65. The returned header and eight sampled `hand` records were
read successfully. Kenshi terminated when the probe released the returned
buffer through
`Ogre::CategorisedAllocPolicy<Ogre::MEMCATEGORY_GENERAL>::deallocateBytes()`.

The log reached the checkpoint immediately before the allocator call. It did
not reach the checkpoint after the call. The explicit SEH handler did not log
an exception. Kenshi did not create a new crash dump.

## Build

- Branch: `agent/station-scanner-probe`
- Commit: `ee9f6af`
- DLL SHA-256:
  `6e30a800bb959ac32b97148a9c04f05d1c0bd5f7e551ae9d87ba75387d3db733`
- Compiler: VC100 x64 cross-compiler
- Compile: `/O2 /Oi /Gy /GL /Zi /Oy- /MD /EHsc`
- Link: `/LTCG /DEBUG`

## Relevant log excerpt

```text
208.648 [KJM OwnershipProbe] tid=364 stage=1 before-call player=00000000051B9550 faction=000000005600DF70 ownerships=0000000044577620 out=00006FFFF70F28C0 sizeof(lektor<hand>)=0x18
208.648 [KJM OwnershipProbe] tid=364 stage=1 entering getOwnedBuildingsH; output will not be read or freed
208.648 [KJM OwnershipProbe] tid=364 stage=1 returned; output remains unread and intentionally unfreed
210.264 [KJM OwnershipProbe] tid=364 stage=2 source object=0000000044577678 vtable=00000001416947E8 count=365 max=640 stuff=00000000D6DCC0F0
210.264 [KJM OwnershipProbe] tid=364 stage=2 output object=00006FFFF70F28C0 vtable=00006FFFF70D4CE8 count=365 max=365 stuff=0000000118D0EB90
210.264 [KJM OwnershipProbe] tid=364 stage=2 output-aliases-source=no output-object-alias=no output-sane=yes
211.911 [KJM OwnershipProbe] tid=364 stage=3 complete; output remains retained for the release test
214.071 [KJM OwnershipProbe] tid=364 stage=4 before-release output=0000000118D0EB90 count=365 max=365 source=0000000044577678 source-stuff=00000000D6DCC0F0
214.071 [KJM OwnershipProbe] tid=364 stage=4 freeing retained output through Ogre general allocator
214.076 WASDCombatPlugin: WASDCombatPlugin: unloaded
```

## Confirmed boundaries

- The call returned on the normal `PlayerInterface::updateUT` thread.
- `sizeof(lektor<hand>)` was `0x18`.
- The source header was sane: `count=365`, `maxSize=640`.
- The output header was sane: `count=365`, `maxSize=365`.
- The source and output objects and capacity spans did not overlap.
- Eight output `hand` records had the expected `hand` vtable and
  `itemType::BUILDING` (`type=0`).
- The probe detached and zeroed all retained output state before the allocator
  call, so the crash was not caused by a later retry or global destructor.

## Current safety decision

Do not release a buffer returned by this API through the Ogre general
allocator in the plugin. Do not use this allocation-returning path in the
production scanner until KenshiLib defines and validates its ownership and
release contract. The next probe must avoid temporary engine-owned output
allocation entirely.

## Post-test disassembly finding

Disassembly of the active Steam executable explains the allocator mismatch.
`Ownerships::getOwnedBuildingsH` at `0x1405AD010` forwards
`&this->stuff` to the generic `lektor<hand>` deep-copy helper. The engine
reserve/copy path allocates through executable thunk `0x140ED6504`, which
imports MFC100U `operator new`. Its replacement/free path calls thunk
`0x140ED64FE`, which imports MFC100U `operator delete`.

The reconstructed `lektor<T>` header instead exposes Ogre's
`STLAllocator<T, CategorisedAllocPolicy<GENERAL>>` and does not declare the
engine deleting destructor. Calling the Ogre/Ned pooling deallocator on the
MFC allocation therefore used the wrong allocator family. This matches the
observed immediate termination.

The source allocator vtable observed in the probe was `0x1416947E8`; its
deleting-destructor path leads to `0x1401672E0` and also frees `[object+0x10]`
through the matching MFC delete thunk. This is not a documented or exported
KenshiLib API. The plugin must not call a guessed MFC ordinal, a hard-coded
game address, `delete[]`, or the reconstructed object's implicit destructor.
The supported fix is for KenshiLib to expose the correct release operation or
repair the reconstructed container behavior.
