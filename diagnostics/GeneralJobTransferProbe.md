# General permanent-job transfer probe

The generic transfer core is fail-closed in normal release builds until its
native `OrdersReceiver::addJob` path has passed this matrix in Kenshi.

Build a diagnostic DLL with `KJM_GENERAL_JOB_TRANSFER_PROBE`. Do not also set
`KJM_GENERAL_JOB_TRANSFER_VERIFIED`. Exercise the core through the normal Squad
drag UI and record each result code plus the before/after queues.

Required field tests:

- Move each global role with a null or ignored subject: rescue, put in bed,
  medic, robotics, and engineering.
- Move one fixed-target crafting, farming, mining, turret, training, and hauling
  job between two characters.
- Move an automatic-machine job that makes Kenshi add a native companion row.
  Confirm that both rows move as one bundle when dragging either the operating
  card or its immediately adjacent hauling companion. Confirm that a missing,
  reordered, or mismatched companion fails before either queue changes.
- Confirm a primary whose `TaskData::permaJob_Associated` would normalize to a
  different TaskType fails before either queue changes. Record which global
  roles, if any, use this native normalization.
- Drop at the first, middle, and last destination gaps.
- Reject a destination that already has the same global role.
- Reject a destination that already has the same TaskType and exact target.
- Change each source and destination queue after drop capture. Confirm that no
  source row is removed.
- Force or simulate a destination insertion failure. Confirm there is no
  compensating rollback: the verified destination copy remains for manual
  review and the source remains unchanged.
- Test an unavailable target and an unavailable member. Confirm failure without
  a crash or destructive source change.

For each drop, retain the `General transfer probe:` line from
`RE_Kenshi_log.txt`. It records the numeric result code, before-row counts,
verified transferred-row count and both mutation flags.

After all tests pass, define `KJM_GENERAL_JOB_TRANSFER_VERIFIED` in the release
compile command. Keep the probe macro disabled in packaged builds.

The transfer snapshots contain only scalar values, copied `hand` values,
`HandleIdentity`, and vectors of those values. They never retain `Character`,
`OrdersReceiver`, `Tasker`, `TaskData`, or other borrowed engine pointers.
