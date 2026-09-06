# Contributing

Thanks for taking a look. This is a small, deliberately narrow project — a single-file ReShade
addon that caps the frame rate. That narrowness is the design, not an oversight.

**Bug reports and questions are very welcome. Code contributions are not being accepted** — see
[Code contributions](#code-contributions) below for why.

## Ground rules

**The cap is one number, clamped to `[0, kMaxFps]`.** 0 means unlimited; anything above it is the
target frame rate. Nothing per-game, per-server or per-profile is hardcoded, and the overlay stays a
single control plus presets — PRs adding profiles, per-application detection or a settings file will
be declined regardless of how well they're written.

**No new dependencies.** Everything the addon needs is either vendored under `deps/` (headers
only) or a Windows system library. No package manager, no telemetry, no network calls.

**Version pins move together.** The ReShade SDK headers and the Dear ImGui headers must match:
SDK **v6.1.0** = addon **API 11**, paired with the **docking branch** of ImGui at
`IMGUI_VERSION_NUM 19040`. The old SDK is deliberate — it is the compatibility floor, and lower
means the addon loads on more ReShade builds, and plenty of users run an older one. Don't
"modernise" the pins. `reshade_overlay.hpp` has an `#error` that fires if the ImGui version
doesn't match exactly, and the release-branch ImGui headers won't work at all because the overlay
uses docking-only types. [BUILDING.md](BUILDING.md) has the refresh commands.

## Building and testing

Build steps are in [BUILDING.md](BUILDING.md). Windows and MSVC only.

Two things worth knowing before you spend time debugging:

- **On macOS or Linux, your editor's clang diagnostics will light up** — missing `imgui.h`,
  undeclared `reshade`, `std::chrono::duration` arity errors. That's clang lacking the Windows SDK
  and the CMake include paths, not broken code. It builds clean under MSVC. Don't "fix" it.
- **There is no test suite.** Verification is a real FiveM run: does ReShade load the addon, does
  the overlay appear, does the frame rate actually sit where you set it. Nothing else proves the
  pacing works, which is why "untested in-game" is always worth saying out loud.

CI builds every push on `windows-2022` and attaches the assembled bundle as an artifact, so a
build can be downloaded and tested without a local Windows toolchain.

## Code contributions

**Unsolicited pull requests are closed automatically, unmerged.** A bot does it, so don't read
anything into how fast it happens — it's not a judgement on the code. It's a licensing decision:
MintCap is source available under a licence the maintainer needs to be able to change and enforce
(see [LICENSE](LICENSE)), and that only works while one person holds the copyright to all of it.
Merging someone else's code would mean every future licence decision needed their agreement too, or
a contributor agreement to sign, and neither is worth it on a project this size.

So please don't spend your evening on a patch for this repo. What genuinely helps:

- **Bug reports**, especially with a `ReShade.log` attached.
- **Telling me the frame pacing misbehaves** on a setup I can't test — a specific GPU, a graphics
  pack, an unusual refresh rate.
- **Pointing out something wrong in the docs.** The install guide gets read by people who have
  never used ReShade, so confusing wording is a real bug.

If you have a change you think belongs in MintCap, open an issue describing it first. If it fits,
I'll implement it and credit you in the changelog.

Forking for your own use is fine and the source is right there. Publishing that fork is not — the
licence covers that.

Commit messages here follow [Conventional Commits](https://www.conventionalcommits.org/)
(`fix:`, `feat:`, `docs:`, `ci:`, `perf:`, `refactor:`).

## Releases

Maintainer only. Update `CHANGELOG.md`, then push a tag:

```sh
git tag vX.Y.Z && git push origin vX.Y.Z
```

`.github/workflows/release.yml` builds the addon at that version, bundles the latest official
ReShade, and attaches `MintCap_vX.Y.Z.zip` to the GitHub Release. The tag is the single
source of truth for the version — it flows into the DLL's version resource and the overlay label.

## Reporting bugs

Use the [issue forms](https://github.com/Nipeno/MintCap/issues/new/choose). Bugs and
questions both go through GitHub Issues so answers stay searchable. For security problems, see
[SECURITY.md](SECURITY.md) instead.

## Code of conduct

Participation is covered by our [Code of Conduct](CODE_OF_CONDUCT.md).
