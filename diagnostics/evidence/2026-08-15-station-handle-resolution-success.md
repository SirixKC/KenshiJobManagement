# Station handle resolution probe — 2026-08-15

## Result

The complete allocation-free ownership path passed in Kenshi 1.0.65. A
borrowed ownership record was reconstructed as a local `hand`, resolved to a
loaded `Building`, verified against the building's live handle, and checked
with `Building::isThePlayer()`. Kenshi remained stable through all eight
stages and for approximately one minute afterward.

## Build

- Branch: `agent/station-scanner-probe`
- Commit: `a9c6c82`
- DLL SHA-256:
  `783b4600948682d666aac214477c9e4889a1b955ab16b647b4fbe2c7c6a7c47e`
- Compiler: VC100 x64 with `/GL`
- Link: `/LTCG`

## Relevant log excerpt

```text
78.705 [KJM OwnershipProbe] tid=364 stage=3 complete copied=365 BUILDING=365 truncated=no source-count=365 owner-header-pre-post-exact=yes
79.905 [KJM OwnershipProbe] tid=364 stage=4 complete; constructor fields and vtable match exactly
81.052 [KJM OwnershipProbe] tid=364 stage=5 complete hand.isValid=true; no Building pointer was requested
82.330 [KJM OwnershipProbe] tid=364 stage=6 complete handValid=true buildingFound=true; returned pointer was not dereferenced or retained
83.459 [KJM OwnershipProbe] tid=364 stage=7 complete exact Building handle verified=true; no pointer retained
84.741 [KJM OwnershipProbe] tid=364 stage=8 complete exact Building verified; isThePlayer=true; no pointer retained
```

## Confirmed boundaries

- Borrowing `Faction::factionOwnerships->stuff` and copying scalar fields did
  not allocate, modify, or release engine memory.
- The five-field `hand` constructor produced the expected identity and
  runtime vtable.
- `hand::isValid()` completed successfully.
- `hand::getBuilding()` returned a loaded building for the tested record.
- `Building::getHandle()` matched the source vtable and all five identity
  fields exactly.
- `Building::isThePlayer()` completed and returned `true`.
- Each stage freshly resolved and bracket-validated the owner, source header,
  and selected record.
- No engine pointer or reference escaped its guarded leaf function.

## Production constraint

Production discovery can use the same borrowed-list path on the main update
thread. It must copy only scalar identities into plugin-owned storage and
resolve at most one identity per update. It must never call
`getOwnedBuildingsH()`, retain a borrowed pointer, change the source list, or
release engine memory.
