# No Black Bars Fix for Red Dead Redemption 2

Full-screen cinematic cutscenes in RDR2 story mode - **without the black bars, and without the stuck missions** the old NoBlackBars mod causes. Works on ultrawide.

**Download:** [Releases](../../releases) · **Support:** [Buy me a coffee on Ko-fi](https://ko-fi.com/gamelover77511)

## Why a fix?
The old `NoBlackBars.asi` (2020) removes the bars on every frame. Some missions wait for the bars to come in before their cutscene starts, so with that mod they never start and the game hangs before the scene.

This fix removes the bars **only while a cutscene is really running**: the bars appear for a moment (so the mission starts), then disappear for the rest of the scene.

## Requirements
[Script Hook RDR2](http://www.dev-c.com/rdr2/scripthookrdr2/) (Alexander Blade) with its ASI loader (`dinput8.dll`). Story mode only.

## Install
1. Delete the old `NoBlackBars.asi` if you have it.
2. Copy `NoBlackBarsFix.asi` and `NoBlackBarsFix.ini` next to `RDR2.exe`.
3. Play. Bars show for under a second at a cutscene start, then they're gone.

## Settings (`NoBlackBarsFix.ini`)
| Setting | Default | What it does |
|---|---|---|
| StartDelayMs | 750 | How long the bars stay at a cutscene start. Raise to 1500 if a scene still gets stuck. |
| EndGraceMs | 1000 | Raise if bars flash back between shots. |
| ToggleKeyF | 0 | 1-12 = an F-key to switch the fix on/off; 0 = no key. |
| Log | 0 | 1 = write `NoBlackBarsFix.log` next to `RDR2.exe` for bug reports. |

## Build from source
One C file, no dependencies. With [zig](https://ziglang.org/) (`pip install ziglang` works too):
```
zig cc -target x86_64-windows-gnu -shared -O2 -s -o NoBlackBarsFix.asi src/NoBlackBarsFix.c
```

## Bugs / custom mods
Open an [issue](../../issues) with `NoBlackBarsFix.log` (set `Log=1`), or ask on my [discussion board](https://github.com/pavanpotluri27-sketch/pavanpotluri27-sketch/discussions). **I build custom mods on request.**

## More from me
[Moon Glow](https://github.com/pavanpotluri27-sketch/rdr2-moon-glow) · [Ghost Mode](https://github.com/pavanpotluri27-sketch/rdr2-ghost-mode) · [Nexus (RUDHRAAA)](https://www.nexusmods.com/profile/RUDHRAAA) · [YouTube](https://www.youtube.com/@Bounty_Bandit_Gaming) · [☕ Ko-fi](https://ko-fi.com/gamelover77511)

## Credits
Alexander Blade (Script Hook RDR2) · alloc8or (native database)

## License
MIT. See [LICENSE](LICENSE).
