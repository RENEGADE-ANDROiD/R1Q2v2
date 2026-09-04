# R1Q2 (RENEGADE fork)

A work-in-progress update of r1ch's R1Q2 Quake II client, based on the public source archive.

The goal is a client that still feels like R1Q2, builds on modern Windows, and picks up a few solid ideas from Q2PRO — without turning into a cheat client or a heavy effects pack.

## Play

1. You need a normal Quake II install (Steam is fine).
2. Put the build next to your Quake II exe. On this machine that is:
   `D:\SteamLibrary\steamapps\common\Quake 2`
3. Run **R1Q2** from the desktop shortcut, or launch `r1q2.exe` from that folder.

You still need the usual game data (`baseq2` and so on). This project does not ship the retail or Steam content.

Build string: **8012-RENEGADE** (so you can tell this apart from stock R1Q2).

## What we're aiming for

- **Classic look** — Quake-style fonts, console, and menus stay.
- **R1Q2 movement by default** — optional Q2PRO-style movement later, as a setting.
- **Modern Windows build** — CMake / VS2022 instead of old VS6 projects.
- **Better renderer over time** — cleaner OpenGL inspired by Q2PRO, still looking like Quake II.
- **Server browser with favorites** — planned, using the old multiplayer menus as a base.
- **Fair play** — no wallhacks, player wireframes, or bloom tricks that reveal enemies. Heavy weather-style FX are out of scope.

## What's in so far

- CMake build for the game DLL, dedicated server, and client (see `BUILD-WINDOWS.md` if you are compiling).
- Fork marked in `build.h` as `8012-RENEGADE`.
- Work in progress on a proper `ref_r1gl` renderer so the client can start with matching OpenGL code from this tree.

Expect rough edges while the renderer and menus catch up.

## Credits

- **r1ch** — original R1Q2 (http://www.r1ch.net/stuff/r1q2/).
- Public archive: https://github.com/tastyspleen/r1q2-archive
- Ideas (not a full port): https://github.com/q2pro/q2pro

If you reuse code from this fork, keep credit for r1ch and say clearly that it is a modified build.