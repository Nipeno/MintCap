# MintCap

[![Build](https://github.com/Nipeno/MintCap/actions/workflows/build.yml/badge.svg)](https://github.com/Nipeno/MintCap/actions/workflows/build.yml)
[![Latest release](https://img.shields.io/github/v/release/Nipeno/MintCap)](https://github.com/Nipeno/MintCap/releases/latest)
[![License: source available](https://img.shields.io/badge/license-source%20available-93E9BE)](LICENSE)

**An FPS limiter for FiveM.** Type a number, and your frame rate holds there. Type **0** and there's
no cap at all. It's a small add-on for [ReShade](https://reshade.me), so it also works in any other
game ReShade runs on.

**Pure mode friendly.** Pure mode blocks modified game files. MintCap isn't one - it's a ReShade
add-on, and FiveM has built-in support for loading ReShade, so it runs on pure-mode servers.

## Quick start

1. **[Download the latest zip](https://github.com/Nipeno/MintCap/releases/latest)** and extract it.
2. Open **`Install Guide.html`** from the extracted folder.
3. Follow it. About two minutes, most of it drag-and-drop.

The guide opens on the **FiveM** path - your plugins folder, three files, and the one-time
"ReShade was blocked" fix - and has an **Any other game** tab if you're installing somewhere else.
The zip bundles ReShade, so it's the only download you need. Already running ReShade from a
graphics pack like NVE or QuantV? The guide tells you which file to skip.

Then press <kbd>Home</kbd> in-game (some setups use <kbd>Insert</kbd>), find the **MintCap** window,
and set your number.

## Picking a number

- **GTA:** its physics is tied to frame rate, so cars handle differently at different FPS. Racing
  servers commonly ask everyone to sit at 60 for exactly that reason - if you're on one, use the
  number they ask for.
- **Otherwise, match your monitor** (60, 144, 240...): frames your screen can't show are wasted work.
- **A steady lower number** often feels smoother than an unsteady high one. If your FPS swings
  between 80 and 140, capping at 80 can beat both.
- **Turn VSync off** in-game. Both it and MintCap wait on their own timing, and running the two
  together can stack those waits into stutter.

## Is it safe? What does it do?

The **full source code is public**, so anyone can read exactly what it does. It only:

- caps your frame rate to the number you set,
- remembers that number between launches,
- shows a link to the code.

**No ads, no tracking, no network access** - the only time it opens your browser is when *you* click
the GitHub button. It doesn't read or modify your game's files. The code is right here in this repo,
and there's a **View Source on GitHub** button inside the add-on. Read it, build it yourself, check
the two against each other - that's why it's published.

> **Bans and server rules are your risk.** MintCap loads into a running game process. Many games,
> platforms, servers and anti-cheat systems don't allow that, and the only people who decide are the
> ones running them - you could be kicked, suspended or permanently banned, and lose access to a
> game or an account. Check what's allowed where you play **before** you install it. MintCap is
> provided as is, with no warranty of any kind, and the author accepts no liability for any
> consequence of using it, bans included. See sections 8 and 9 of the [LICENSE](LICENSE).

## Support

**[Open an issue](https://github.com/Nipeno/MintCap/issues/new/choose)** - bugs and questions both go
through GitHub, so answers stay searchable for the next person who hits the same thing.

If the overlay never appears, attach your **`ReShade.log`**. It sits next to the ReShade file you
installed (for FiveM that's `%LOCALAPPDATA%\FiveM\FiveM.app\plugins`), the add-on writes to it, and
it usually says exactly what went wrong.

## Credits

- Developer: **Nipeno**
- Testers: **Beanz**, **Cenkov**, **PhatWraith**, **krispy lzz**, **Wraith**, **hachiro**

## For developers

Building it yourself and how it works: **[BUILDING.md](BUILDING.md)**.
Bug reports are welcome; code contributions aren't being accepted -
**[CONTRIBUTING.md](CONTRIBUTING.md)** explains why, and what helps instead.

## License

**Source available, not open source** - see [LICENSE](LICENSE).

- **Free to use**, on as many PCs as you like, including on a server you run.
- **Read it, build it, modify your own copy** - that's why the source is public.
- **Don't redistribute it.** No mirrors, no re-uploads, no bundling it into a pack. The
  [Releases page](https://github.com/Nipeno/MintCap/releases) is the only place to get it.
- **Don't sell it**, or put it behind a paid tier or perk. (Donations to a server whose players use
  it are fine.)
- Permission for anything on that list is one
  [request](https://github.com/Nipeno/MintCap/issues/new?template=permission_request.yml) away -
  say what you want to do and you'll get a straight answer.

The bundled ReShade (BSD 3-Clause) and Dear ImGui (MIT) keep their own licences, shipped in every
release alongside
[`THIRD-PARTY-NOTICES.txt`](third_party/THIRD-PARTY-NOTICES.txt) - what's inside each file, and
every project ReShade itself links.
