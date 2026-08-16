# Ownership handle construction probe — 2026-08-15

## Result

One scalar ownership record was reconstructed through Kenshi's five-field
`hand` constructor. The constructor returned the expected object address, all
five scalar identity fields and the runtime vtable matched, and
`hand::isValid()` returned `true`. Kenshi remained stable.

## Build

- Branch: `agent/station-scanner-probe`
- Commit: `c471edb`
- DLL SHA-256:
  `3392c8c3197f438327e9b9422ad883e9aa124c86044a5720974101ddbf2ede5c`

## Relevant log excerpt

```text
97.158 [KJM OwnershipProbe] tid=364 stage=3 complete copied=365 BUILDING=365 truncated=no source-count=365 owner-header-pre-post-exact=yes
98.449 [KJM OwnershipProbe] tid=364 stage=4 before five-field hand constructor; no hand method will run
98.449 [KJM OwnershipProbe] tid=364 stage=4 complete; constructor fields and vtable match exactly
99.693 [KJM OwnershipProbe] tid=364 stage=5 before fresh constructor and hand.isValid; no Building call will run
99.693 [KJM OwnershipProbe] tid=364 stage=5 complete hand.isValid=true; no Building pointer was requested
```

## Confirmed boundaries

- The five-argument constructor ABI is correct for the tested runtime.
- The constructor returned the supplied aligned storage address.
- The reconstructed vtable matched the live source-record vtable.
- Type, container, container serial, index, and serial all matched.
- `hand::isValid()` completed and returned `true`.
- The owner header and selected borrowed record matched before and after each
  operation.
- No `Building*` was requested or retained.

## Next boundary

Test `hand::getBuilding()`, exact `Building::getHandle()` verification, and
`Building::isThePlayer()` in separate stages with fresh reconstruction and
borrowed-record validation around each operation.
