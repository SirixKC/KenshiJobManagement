# General job-transfer live promotion evidence

Date: 2026-08-17

The cross-member and cross-squad transfer path was built with the VC100 x64
probe switch and installed for live Kenshi testing before Release enablement.

- Probe DLL SHA-256:
  `92417a6d8b63ad3b2d9a64dd2f2eb30bb40e40e69eabac28c8587fd661270655`
- The probe used `KJM_GENERAL_JOB_TRANSFER_PROBE`, not the Release verified
  switch.
- VC100 compile and `/LTCG` link completed with exit code 0.
- `tools/validate_source.py` and `git diff --check` passed.
- The final probe DLL contained the general-transfer diagnostic marker and none
  of the four forbidden world-scanner strings.
- After live testing, the user approved promotion to `main` with: “great lets
  push to main”.

This record explains why the tracked Release project defines
`KJM_GENERAL_JOB_TRANSFER_VERIFIED`. It does not replace the repeatable
regression matrix in `docs/TESTING.md`. Remove the verified define if a future
change fails that matrix.
