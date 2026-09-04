# R1Q2v2 (RENEGADE)

A modern Windows build of r1ch's R1Q2 Quake II client. Updated by **h0s3r**.

It still looks and plays like R1Q2. The menus, console, and movement stay classic. A few Q2PRO ideas are in (public server list, MD3 view weapons) without turning this into a cheat client or a heavy effects pack.

Build string: **8012-RENEGADE**.

## Play

1. Install Quake II (Steam is fine).
2. Copy these files into that folder, next to `quake2.exe`:
   - `R1Q2v2.exe`, `r1q2ded.exe`, `gamex86.dll`
   - `ref_r1gl.dll`, `ref_gl.dll`
   - `z.dll`, `libpng16.dll`, `jpeg62.dll`
3. Launch `R1Q2v2.exe`.

You still need the usual game data (`baseq2` and so on). This project does not ship the retail or Steam content.

## Menus

Click a row or use the arrow keys. Left-click / right-arrow moves a toggle or slider forward; right-click / left-arrow moves it back. Enter also cycles a toggle.

The console and notify text scale with resolution (about 2× at 1080p). `con_scale 0` is auto; set it to 1–6 to lock a size.

### Video Options

OpenGL / R1GL:

- Driver, video mode, screen size, brightness, fullscreen
- **Vsync** (off by default)
- **Max fps** slider (about 60–300; default 250)
- Texture quality, 8-bit textures, sync every frame
- **Lightmap** brightness
- **Anisotropy** (1 / 2 / 4 / 8 / 16; default 16)
- **Texture sharpen** (LOD bias on world textures; 0 = off)
- **HUD scale** (`gl_hudscale`, about 0.5–2.5)
- **HUD at top** (`scr_hud_top`) — flips status bar / layout `yb` to the top
- **Shadows**
- **Dynamic lights**

Escape applies driver / resolution / fullscreen. The FX toggles and sliders apply as you change them. Cancel leaves without applying the video-mode change.

New installs default to your desktop resolution. New Game starts a map from the main menu.

### High-res textures

R1GL already loads drop-in replacements next to retail WALs (keep the WALs for UV size):

```
baseq2/textures/<name>.png|.jpg|.tga
baseq2/env/<skyname>rt.png   (also bk lf ft up dn; jpg/tga work too)
```

Try order is png → jpg → tga → wal. Same-gamedir **paks beat loose files** — put overrides in a later pak or a mod gamedir. Needs `libpng16.dll` / `jpeg62.dll` next to the exe.

Useful cvars: `gl_ext_max_anisotropy` (or alias `gl_anisotropy`), `gl_texture_lodbias` (0 … -2), `gl_linear_mipmaps 1`, `gl_texture_formats "png jpg tga"`.

### Multiplayer

**Join Network Server** lists public games from q2servers.com, plus LAN. Empty rows stay blank while the list fills in.

- Enter — connect
- F — add or remove the selected server in the Address Book
- Space — refresh
- [ / ] — page

**Address Book** is the favorite list (`adr0`–`adr15`). It is separate from the public browser.

### Options → R1Q2 settings

DirectInput mouse, XP mouse acceleration fix, deferred model loading, **Q2Pro movement** (sync physics / `cl_async 0` for classic jump feel; off keeps stock R1Q2 async), automatic demo record, and Xania rail trail. These are not written to `config.cfg` — put anything you want to keep in `autoexec.cfg`.

## Building

On Windows, run `build-windows.cmd` (VS2022 Build Tools, CMake, Ninja, vcpkg). Output lands in `build\bin\`.

## Credits

- **r1ch** — original R1Q2 (http://www.r1ch.net/stuff/r1q2/)
- **h0s3r** — this RENEGADE / R1Q2v2 update
- Public archive: https://github.com/tastyspleen/r1q2-archive
- Ideas (not a full port): https://github.com/q2pro/q2pro

If you reuse code from this fork, keep credit for r1ch and say clearly that it is a modified build.
