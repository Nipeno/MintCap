<!-- HEADS UP: MintCap does not accept unsolicited pull requests - they get closed unmerged.
     This is a licensing constraint, not a code-quality one. Please open an issue instead:
     https://github.com/Nipeno/MintCap/issues   Details: CONTRIBUTING.md#code-contributions
     This template exists for the maintainer's own PRs. -->

## What & why
<!-- Short summary of the change and the reason for it. -->

Closes #

## Type
- [ ] Bug fix
- [ ] Addon behavior / pacing
- [ ] CI / release tooling
- [ ] Docs

## Checklist
- [ ] **Cap stays clamped to `[0, kMaxFps]`** — 0 means unlimited, and nothing per-game is hardcoded
- [ ] Licence and notices untouched; no new deps / telemetry
- [ ] If touching ReShade/ImGui headers: version pins kept in sync (SDK v6.1.0 = API 11, ImGui docking `19040`) — the low pin is deliberate, it's the compatibility floor
- [ ] Builds clean in GitHub Actions (`windows-2022`)
- [ ] Docs updated if user-facing behavior changed (README / BUILDING / `packaging/Install Guide.html`)

## Testing
<!-- How was this verified? Real FiveM run? CI artifact? Note what's still UNtested. -->
