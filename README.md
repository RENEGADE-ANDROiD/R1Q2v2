# R1Q2v2 (RENEGADE fork)

A work-in-progress update of r1ch's R1Q2 Quake II client, based on the public source archive. Updated by **h0s3r**.

The goal is a client that still feels like R1Q2, builds on modern Windows, and picks up a few solid ideas from Q2PRO - without turning into a cheat client or a heavy effects pack.

## Play

1. You need a normal Quake II install (Steam is fine).
2. Put the build next to your Quake II exe. On this machine that is:
   `D:\SteamLibrary\steamapps\common\Quake 2`
3. Launch `R1Q2v2.exe` from that folder (make your own shortcut when you want).

Useful files: `R1Q2v2.exe`, `gamex86.dll`, `z.dll`, `ref_r1gl.dll` (also as `ref_gl.dll`), plus `libpng16.dll` / `jpeg62.dll` if present. You still need the usual game data (`baseq2` and so on). This project does not ship the retail or Steam content.

Build string: **8012-RENEGADE** (so you can tell this apart from stock R1Q2).

## What we're aiming for

- **Classic look** - Quake-style fonts, console, and menus stay.
- **R1Q2 movement by default** - optional Q2PRO-style movement later, as a setting.
- **Modern Windows build** - CMake / VS2022 instead of old VS6 projects.
- **Better renderer over time** - cleaner OpenGL inspired by Q2PRO, still looking like Quake II.
- **Server browser with favorites** - planned, using the old multiplayer menus as a base.
- **Fair play** - no wallhacks, player wireframes, or bloom tricks that reveal enemies. Heavy weather-style FX are out of scope. The client should work with modern anticheats or at least not trip them.

## What's in so far

- Fixed null-call ACCESS_VIOLATION during sound init: re-enabled `dsound.dll` LoadLibrary/GetProcAddress for `DirectSoundCreate` (left commented after the CMake port switched callers to `iDirectSoundCreate`).
- **Video modes:** expanded beyond the old 4:3 table — seeds common modern resolutions (720p/1080p/1440p/4K, etc.) and appends modes from Windows `EnumDisplaySettings`. Pick **1920x1080** under Video Options → video mode, then apply (or `gl_mode` to that index + `vid_restart`).
- **Menu scale / mouse:** `scr_menuscale` (0 = auto from height/480) scales classic Quake menus/fonts for high-res; mouse is released in menus (including fullscreen) and clicks hit the same scaled space the UI draws in.
- **Menu mouse clicks:** force absolute cursor via `IN_UpdateMenuMouse` (GetCursorPos/ScreenToClient) on draw and click; always deliver mouse keys to `M_Keydown` in menus; never skip WM mouse while menu/console even if DInput is active; refresh hover before select; qmenu hover uses parent X span.
- Multi-size Quake II window icon embedded in `R1Q2v2.exe`.
- Note (later): q2pro-like server browser not done yet.

- CMake build for the game DLL, dedicated server, client, and `ref_r1gl` (see `BUILD-WINDOWS.md` / `RENDERER.md` if you are compiling).
- Official binary name **R1Q2v2.exe**, product name **R1Q2v2**.
- Fork marked in `build.h` as `8012-RENEGADE`.

Expect rough edges while crashes and menus get ironed out.

## Credits

- **r1ch** - original R1Q2 (http://www.r1ch.net/stuff/r1q2/).
- **h0s3r** - this RENEGADE / R1Q2v2 update.
- Public archive: https://github.com/tastyspleen/r1q2-archive
- Ideas (not a full port): https://github.com/q2pro/q2pro

If you reuse code from this fork, keep credit for r1ch and say clearly that it is a modified build.
