# R1Q2v2

A modern Windows build of r1ch's R1Q2 Quake II client. Updated by **h0s3r**.

It still looks and plays like R1Q2. The menus, console, and movement stay classic. A few Q2PRO ideas are in (public server list, MD3 view weapons, `.pkz` packs, layered crosshairs) without turning this into a cheat client or a heavy effects pack.

Console / version string: **R1Q2v2 (Build: 8056)** — bump `BUILD` in [`build.h`](build.h) each release.

## Play

1. Install Quake II (Steam is fine).
2. Copy these files into that folder, next to `quake2.exe`:
   - `R1Q2v2.exe`, `r1q2ded.exe`, `gamex86.dll`
   - `ref_r1gl.dll`, `ref_gl.dll`
   - `z.dll`, `libpng16.dll`, `jpeg62.dll`
   - **OpenAL (recommended):** `OpenAL32.dll` and `fmt.dll` (OpenAL Soft from vcpkg; `fmt.dll` is required next to the exe). Optional: `oal/hrtf/` + `oal/presets/`. Shipped in the [desktop tester zip](#tester-zip); R1Q2 loads `OpenAL32.dll` at runtime.
   - **HTTP downloads:** `libcurl.dll` (and `zlib1.dll` if present) next to the exe. TastySpleen and other R1 servers send `dlserver=`; without curl you get “built without USE_CURL, bad luck.”
   - `baseq2/r1q2v2_visual.cfg` (and optionally `pretty_r1q2v2.cfg`) into the install `baseq2` folder
3. Launch `R1Q2v2.exe`.

You still need the usual game data (`baseq2` and so on). This project does not ship the retail or Steam content.

## Docs

- **[CVARS.md](CVARS.md)** — useful console variables (video, HUD, input, net, R1Q2 extras) with defaults and short explainers

**Ultrawide (e.g. 3440x1440):** use `gl_mode -1`, `vid_forcewidth` / `vid_forceheight`, and prefer `vid_borderless 1` (or desktop-native size). There is no `r_customwidth`. If you `exec r1q2v2_visual.cfg`, clear its 1920x1080 force dims first — see [CVARS.md](CVARS.md).

## Menus

Click a row or use the arrow keys. Left-click / right-arrow moves a toggle or slider forward; right-click / left-arrow moves it back. Enter also cycles a toggle.

The console and notify text scale with resolution (about 2× at 1080p). `con_scale 0` is auto; set it to 1–6 to lock a size.

Column menus are centered under the banner. New installs default to your desktop resolution. New Game starts a map from the main menu. Damage / powerup full-screen blends default **off** (`cl_blend 0`, `gl_polyblend 0`).

### Options

- **Crosshair** style (`none` / `cross` / `dot` / `angle`) and **crosshair scale** (0.1–4, saved)
- **Crosshair setup** — color, alpha, health tint, and per-layer scales for `ch1`–`ch3` overlays (all saved)
- Sound, mouse, always-run, invert, free look
- **R1Q2 options** and customize controls

### Video Options

OpenGL / R1GL:

- Driver is **R1GL** only. Saved `vid_ref gl` / `ncgl` / `soft` remap to r1gl. Stock OpenGL and software are not listed.
- Video mode, screen size, brightness, fullscreen
- **Vsync** (off by default)
- **Max fps** slider (about 60–300; default 250)
- Texture quality, 8-bit textures, sync every frame
- **Lightmap** brightness
- **Anisotropy** (1 / 2 / 4 / 8 / 16; default 16)
- **Texture sharpen** (LOD bias on world textures; 0 = off)
- **HUD scale** (`scr_hudscale`, about 0.5–2.5) — stock status bar only; menus and custom server HUDs keep their authored layout
- **Layout menu scale** (`scr_layoutscale`, default auto) — Arena team select, TastySpleen/RA MOTD, help, scoreboards, inventory. `0` = height/480. Centered 320×240 canvas even at scale 1 (widescreen). Does not use `scr_hud_top` / `scr_hudwide` (stock numeric HUD still does).
- **HUD at top** (`scr_hud_top`) — flips the stock status bar to the top
- **Wide HUD** (`scr_hudwide`, on by default) — on 16:9, stock health/ammo sit left and armor/weapon sit right. 4:3 stays the old centered strip. Server/mod HUD layouts are unchanged.
- **Shadows**
- **Dynamic lights**

Widescreen Hor+ FOV (`cl_adjustfov`) and HUD opacity (`scr_alpha`) are under Options → R1Q2, not here. Both save.

Escape applies driver / resolution / fullscreen. The FX toggles and sliders apply as you change them. Cancel leaves without applying the video-mode change.

Campaign / local SP (`maxclients` 1): warp water that the map tagged TRANS33/66 is translucent again. Rocket Arena, DM, CTF, and coop stay classic opaque turb. `gl_wateralpha` is console-only and ignored in multiplayer.

Older configs that used `gl_hudscale` for HUD size are redirected to `scr_hudscale`; `gl_hudscale` is forced back to `1` so it no longer shrinks the whole 2D layer (including after a later `exec`).

### Packs and high-res textures

**`.pak` and `.pkz`** (ZIP packs, same as [Q2PRO](https://github.com/q2pro/q2pro)) both load from `baseq2` and mod gamedirs. Later packs override earlier ones. Drop-in examples that work out of the box:

- `Q3ArenaHUD.pkz` — Quake III–style status bar pics
- `quake2-neural-upscale-textures-*.pkz` — HD wall replacements
- Do **not** drop `zz_r1q2_titles.pkz` in `baseq2` yet — R1GL prefers the 2× PNG and `M_Main_Draw` still uses classic 40-unit tag pitch, so the plaque sits on GAME and the tags stack. Stock `pak0` PCX only. The generator script is `scripts/pack_r1q2_titles.py` (retail art, not in git) for when layout uses logical PCX sizes.

R1GL also loads loose replacements next to retail WALs (keep the WALs for UV size):

```
baseq2/textures/<name>.png|.jpg|.tga
baseq2/env/<skyname>rt.png   (also bk lf ft up dn; jpg/tga work too)
```

Try order is png → jpg → tga → wal. Same-gamedir **packs beat loose files** — put overrides in a later `.pak`/`.pkz` or a mod gamedir. Needs `z.dll` / `libpng16.dll` / `jpeg62.dll` next to the exe.

Useful cvars: `gl_ext_max_anisotropy` (or alias `gl_anisotropy`), `gl_texture_lodbias` (0 … −4 via the sharpen slider), `gl_linear_mipmaps 1`, `gl_texture_formats "png jpg tga"`, `gl_pic_formats "png jpg tga"`. Full list: [CVARS.md](CVARS.md).

### Multiplayer

**Join Network Server** lists public games from q2servers.com, plus LAN. Empty rows stay blank while the list fills in. The list is centered under the banner.

- Enter — connect
- F — add or remove the selected server in the Address Book
- Space — refresh
- Mouse wheel, [ / ], PgUp/PgDn, left/right — page through the full list (vanilla, Arena, q2rpg2, …)

**Address Book** is the favorite list (`adr0`–`adr15`). It is separate from the public browser. Favorites are stored in `baseq2/bookmarks.cfg` so they survive `exec Q2config.cfg` and gamedir switches.

**Multiplayer settings** (under Multiplayer) can tint enemy / team players on this client only, and optionally blend enemy lighting toward fullbright (`cl_enemyfullbright`: Off / 50% / 66.6%). 66.6% means two-thirds of the way from world light to fullbright, not a flat 66.6% lamp. Force color uses **Tint Opacity** (default 50%): original skin at 0%, mix in the middle, solid enemy/team color at 100%. White keeps the original skin at low opacity. Team vs enemy uses Arena `r2red`/`r2blue` or CTF `ctf_r`/`ctf_b` skins; FFA treats everyone else as enemy. Cvars: `cl_forcecolors`, `cl_enemycolor`, `cl_teamcolor`, `cl_enemyfullbright`.

Chat filter: `ignore` / `mute` a name or snippet (same command). List with `ignorelist`, drop with `unignore` / `unmute`. The list is stored in `ignore.txt`.

### Weapons and view

MD3 view weapons load alongside stock MD2. Missing skins try the model folder, `.skin` files, and `players/*/weapon` so rail / vwep pics do not drop to the notexture. `cl_vwep` still draws other players’ guns.

### Demos and locations

`cl_autorecord` writes a demo when a map starts (R1Q2 options). While playing a demo: `demopause` and `demospeed 0.1`–`8`.

Drop a `maps/<map>.loc` (or `locs/<map>.loc`) in the gamedir. Nearest name draws on the HUD when `loc_enable` is on. `addloc` / `saveloc` from the console.

### Sound

Builds with **`R1Q2_USE_OPENAL=ON`** (default) support OpenAL via `s_initsound 2`. R1Q2 loads **`OpenAL32.dll`** from the game folder (OpenAL Soft from vcpkg). That DLL also needs **`fmt.dll`** beside the exe — without it OpenAL fails to load. If OpenAL still fails, the client falls back to DirectSound.

Shipped presets use OpenAL + 44 kHz:

```
seta s_initsound "2"
seta s_khz "44"
```

Player-local and `ATTN_NONE` sources stay on the listener so your shots do not drift; world sounds stay positional. Optional: `s_openal_device ""` (default output), `alsoft.ini`, and `oal/hrtf/` for HRTF data.

### Tester zip

Regenerate the shareable Desktop package (binaries + visual cfgs + OpenAL runtime):

```powershell
py -3 scripts/pack_r1q2v2_desktop.py
```

Output: `R1Q2v2-8014-win32.zip` on your Desktop. Always includes `OpenAL32.dll`, `fmt.dll`, and `oal/hrtf` + `oal/presets` when those folders exist.

### Options → R1Q2 settings

DirectInput mouse, XP mouse acceleration fix, deferred model loading, **sync physics** (`cl_async 0` locks render to `cl_maxfps` for classic jump *timing* — this is **not** Q2PRO pmove; off keeps stock R1Q2 async and is the default feel), widescreen Hor+ FOV, HUD alpha, location names, automatic demo record, and Xania rail trail.

**Saved to `config.cfg`:** sync physics, Hor+ FOV, HUD alpha, location names, and everything under Crosshair setup. Mouse / defer / demo / rail on this page are still session-only unless you put them in `autoexec.cfg`. Details: [CVARS.md](CVARS.md).

## Building

On Windows, run `build-windows.cmd` (VS2022 Build Tools, CMake, Ninja, vcpkg). Requires **openal-soft** and **curl** (in `vcpkg.json`). Output lands in `build\bin\` including `OpenAL32.dll` and `libcurl.dll` beside `R1Q2v2.exe`.

If CMake skips manifest packages (e.g. after editing `vcpkg.json`), delete `build/CMakeCache.txt` and reconfigure with `-DVCPKG_MANIFEST_INSTALL=ON`, or run `vcpkg install --triplet x86-windows` from the repo root before building.

## Credits

- **r1ch** — original R1Q2 (http://www.r1ch.net/stuff/r1q2/)
- **h0s3r** — this R1Q2v2 update
- Public archive: https://github.com/tastyspleen/r1q2-archive
- Ideas (not a full port): https://github.com/q2pro/q2pro

If you reuse code from this fork, keep credit for r1ch and say clearly that it is a modified build.
