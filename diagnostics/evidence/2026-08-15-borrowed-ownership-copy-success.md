# Borrowed ownership-list copy probe — 2026-08-15

## Result

The allocation-free probe read the player faction's existing
`Ownerships::stuff` list and copied all scalar `hand` fields successfully.
Kenshi remained stable after all three stages.

## Build

- Branch: `agent/station-scanner-probe`
- Commit: `114895b`
- DLL SHA-256:
  `00aa688d32648d2c257659947395e1484a669446a14b308590a88bed12e6f223`
- Compiler: VC100 x64 with `/GL`
- Link: `/LTCG`

## Relevant log excerpt

```text
88.696 [KJM OwnershipProbe] tid=368 stage=1 before resolving player faction and Ownerships::stuff
88.696 [KJM OwnershipProbe] tid=368 stage=1 complete owner=00000000446CC120 faction=000000005616DF70 source=00000000446CC178 source-vtable=00000001416947E8 count=365 max=640 stuff=00000000D3B9CB40 owner-header-exact=yes
90.660 [KJM OwnershipProbe] tid=368 stage=2 before re-resolve and bracket-copy of up to eight hand records
90.661 [KJM OwnershipProbe] tid=368 stage=2 complete copied=8 source-count=365 owner-header-pre-post-exact=yes
92.685 [KJM OwnershipProbe] tid=368 stage=3 before re-resolve and bracket-copy of up to 8192 hand records
92.685 [KJM OwnershipProbe] tid=368 stage=3 complete copied=365 BUILDING=365 truncated=no source-count=365 owner-header-pre-post-exact=yes
```

## Confirmed boundaries

- The borrowed source header was sane and stable.
- Re-resolving the player faction produced the same owner and source identity.
- Copying eight scalar records completed with an exact pre/post header match.
- Copying all 365 scalar records completed with an exact pre/post header match.
- Every copied record had `itemType::BUILDING` (`type=0`).
- The 8,192-record safety cap did not truncate the result.
- No `hand` object was constructed and no `hand` method was called.
- No engine output container was allocated, retained, changed, or released.

## Next boundary

Test one copied handle at a time. Keep `hand::isValid`, `hand::getBuilding`,
exact returned-handle verification, and `Building::isThePlayer` in separate
diagnostic stages before this path enters production code.
