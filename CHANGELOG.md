# Changelog

All notable changes to this project are documented here.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and this project
adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

Every release bundles whatever ReShade was latest at build time; the exact version is recorded in
`reshade-version.txt` inside the zip.

## [Unreleased]

## [1.0.0] - 2026-09-05

First release. MintCap is a ReShade add-on that holds your game at a frame rate you pick.

### Added
- **An adjustable FPS cap.** Type any number into the overlay, or click a preset
  (Off / 30 / 60 / 120 / 144 / 240). **0 means unlimited** - the add-on stops pacing entirely.
  Values are clamped to `[0, 1000]`, including anything read back from a hand-edited `ReShade.ini`.
- **Accurate pacing.** A high-resolution waitable timer covers the bulk of each frame's wait, and a
  short busy-wait covers the final ~0.5 ms, because a plain `Sleep` stutters at Windows' timer
  granularity. Overshoot is compensated frame to frame, and a stall longer than one frame
  re-anchors instead of burst-rendering to catch up.
- **Your setting persists**, in ReShade's own config under `[MintCap]` - no extra file. So does the
  overlay window's position and dock slot, which ReShade stores for us.
- **A log you can send.** Load, graphics-API detection, the first presented frame and every failure
  are written to `ReShade.log`, tagged `[MintCap]`. A healthy launch is about five lines, and
  nothing is written per frame.
- **`Enable-ReShade.bat`** for FiveM users: FiveM blocks ReShade 5+ until acknowledged once, and
  this computes the machine-specific ID and writes the single `CitizenFX.ini` line for you, using
  the same Windows INI API FiveM reads it with. It is idempotent and touches nothing else.
- **`Install Guide.html`** in the zip - the whole setup, start to finish.

### Notes
- Built for **FiveM** first - the install guide opens on the FiveM path and the bundle carries the
  `Enable-ReShade.bat` fix. It is a plain ReShade add-on, so it also works in any other game
  ReShade runs on; the guide has a second path for those.
- **Pure mode friendly.** Pure mode blocks modified game files. This is not one - it is a ReShade
  add-on, and FiveM has built-in support for loading ReShade.
- Targets ReShade add-on **API 11**, so it loads on ReShade **6.1.0 and newer** - deliberately low,
  since a newer ReShade happily loads an older add-on but not the reverse. The bundled `dxgi.dll`
  is the latest official ReShade.
- No network access, no telemetry, no ads. The only time it opens your browser is when you click
  the GitHub button. It does not read or modify game files.
- The CRT is linked statically, so the add-on does not need the VC++ redistributable installed.
- **Source available, not open source** (see `LICENSE`): free to use, read, build and modify for
  yourself; redistribution and commercial use need written permission. The bundled ReShade
  (BSD 3-Clause) and Dear ImGui (MIT) keep their own licences and ship with the zip.

[Unreleased]: https://github.com/Nipeno/MintCap/compare/v1.0.0...HEAD
[1.0.0]: https://github.com/Nipeno/MintCap/releases/tag/v1.0.0
