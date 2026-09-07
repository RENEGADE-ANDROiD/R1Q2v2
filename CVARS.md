# R1Q2v2 console variables

Useful client settings for **R1Q2v2 (Build 8041+)**. Defaults are engine defaults unless noted. This is not every cvar in the tree â€” only ones players typically want.

## How to use

- Console: `` ` `` then `cvar value` (example: `scr_menuscale 2.5`)
- `seta name value` â€” same, and marks the cvar for `config.cfg` when archived
- Put keepers in `baseq2/autoexec.cfg` so they survive menu / `config.cfg` rewrites
- `writeconfig` / `writecfg` â€” save binds + archived cvars to `<gamedir>/config.cfg` immediately (same as quit; no filename arg)
- Many Video Options rows already map to the cvars below

**Not in this client:** Q2PRO-only names like `cl_movement_feel_mode`, `cl_mouse_behavior_mode`, `scr_scale` (HUD uses `scr_hudscale`). There is no Q2PRO pmove. For classic jump *timing* use **sync physics** (`cl_async 0`) â€” that only locks render FPS to `cl_maxfps`. Default `cl_async 1` is stock R1Q2 async and is the natural R1Q2 feel.

---

## Video / display

R1Q2v2 uses **`gl_mode`** + **`vid_forcewidth`** / **`vid_forceheight`** (not Q2PROâ€™s `vid_geometry`). On first run, `gl_mode` and `sw_mode` default to your **desktop resolution index** in the mode list (`VID_GetDesktopModeIndex`).

| Cvar | Default | Notes |
|------|---------|--------|
| `vid_ref` | `r1gl` | Renderer module. Prefer `r1gl` (`ref_r1gl.dll`). |
| `vid_fullscreen` | `0` | `1` fullscreen, `0` windowed. |
| `vid_borderless` | `0` | `1` = borderless windowed fullscreen (no exclusive `ChangeDisplaySettings`). **Archived.** Also used automatically when the requested size matches the current desktop mode. Prefer this on ultrawide if exclusive fullscreen fails. Alias intent: same idea as a `vid_desktopfs` flag. |
| `gl_mode` | desktop index | OpenGL mode list index. `-1` = custom size (uses force dims or desktop if unset). Indexed modes use the mode table first; invalid indexed values fall back to the previous mode (often 320Ã—240 if mode `0`). Custom `-1` stays sticky when force dims are set. |
| `sw_mode` | desktop index | Software renderer mode index (video menu). Keep in sync with `gl_mode` if you use both paths. |
| `vid_forcewidth` | `0` | Target width in pixels. **Archived.** When non-zero, overrides the width from `gl_mode` (including indexed modes, not only `-1`). Added to the mode menu list on init / `vid_restart`. |
| `vid_forceheight` | `0` | Target height in pixels. **Archived.** When non-zero, overrides the height from `gl_mode`. |
| `gl_swapinterval` | `0` | Frame present / sync. **`0`** = unlocked (can tear). **`1`** = vsync (no tear; may add input lag). **`-1`** = adaptive sync / late-tear when the GPU supports it (VRR / FreeSync / Adaptive Sync); falls back to `1` if not. Video menu: off / on / adaptive. Does not change exclusive FS mode. |
| `vid_gamma` | `1.0` | Display gamma / brightness path used by the video menu. |
| `viewsize` | `100` | 3D view percent; below 100 letterboxes the refresh. |
| `gl_bitdepth` | `0` | Color depth request; `0` = default. |
| `vid_xpos` / `vid_ypos` | `3` / `22` | Window position (windowed). |

**Stable `gl_mode` indices (hard-coded prefix):** Modes `0`â€“`27` are fixed across machines. `EnumDisplaySettings` may append extra resolutions after that with **machine-specific** indices â€” those extras never reorder the prefix.

| Index | Resolution | Notes |
|------:|------------|--------|
| 0 | 320Ã—240 | Classic |
| 1 | 640Ã—480 | Classic |
| 2 | 800Ã—600 | Classic |
| 3 | 1024Ã—768 | Classic |
| 4 | 1152Ã—864 | Classic |
| 5 | 1280Ã—960 | Classic |
| 6 | 1280Ã—1024 | Classic |
| 7 | 1600Ã—1200 | Classic |
| 8 | 1280Ã—720 | 16:9 |
| 9 | 1366Ã—768 | 16:9 |
| 10 | 1600Ã—900 | 16:9 |
| **11** | **1920Ã—1080** | **16:9 (stable 1080p)** |
| 12 | 2560Ã—1440 | 16:9 |
| 13 | 3840Ã—2160 | 16:9 |
| 14 | 1280Ã—800 | 16:10 |
| 15 | 1440Ã—900 | 16:10 |
| 16 | 1680Ã—1050 | 16:10 |
| 17 | 1920Ã—1200 | 16:10 |
| 18 | 2560Ã—1600 | 16:10 |
| 19 | 2560Ã—1080 | Ultrawide 21:9 |
| **20** | **3440Ã—1440** | **Ultrawide 21:9 (stable)** |
| 21 | 3840Ã—1600 | Ultrawide 21:9 |
| 22 | 5120Ã—1440 | Ultrawide 21:9 |
| 23 | 1920Ã—1440 | Other |
| 24 | 2048Ã—1152 | Other |
| 25 | 2880Ã—1800 | Other |
| 26 | 3200Ã—1800 | Other |
| 27 | 5120Ã—2160 | Other |

**1080p preset:** Use stable **`gl_mode 11`** (1920Ã—1080). Shipped [`baseq2/r1q2v2_visual.cfg`](baseq2/r1q2v2_visual.cfg) sets that index and still pairs `vid_forcewidth` / `vid_forceheight` as a backup. Prefer the indexed mode over `gl_mode -1` when the size is in the table. Avoid mixing `gl_mode -1` with a saved indexed mode â€” that mismatch can stick you at 320Ã—240.

**Ultrawide (3440Ã—1440 and cousins):** Use stable **`gl_mode 20`**, or keep force dims. Working recipe for 3440Ã—1440:

```
seta vid_fullscreen "1"
seta vid_borderless "1"
seta gl_mode "20"
seta sw_mode "20"
seta vid_forcewidth "3440"
seta vid_forceheight "1440"
vid_restart
```

(`gl_mode -1` + force dims still works if you prefer custom-only.)

Notes:
- There is **no** `r_customwidth` / `r_customheight` in R1Q2v2 â€” use `vid_forcewidth` / `vid_forceheight` (with `gl_mode -1` for sizes outside the table, or alongside an indexed mode as override).
- If desktop is already 3440Ã—1440, the engine uses **borderless** automatically even with `vid_borderless 0` (skips exclusive CDS).
- If exclusive CDS fails for a non-desktop size, it falls back to borderless at the requested size (console: `falling back to borderless windowed fullscreen`) instead of the old dual-monitor `width*2` hack.
- Non-zero `vid_forcewidth` / `vid_forceheight` **override** whatever `gl_mode` you pick â€” change or zero them when leaving a preset, or the 1920Ã—1080 force dims in shipped `r1q2v2_visual.cfg` will keep you at 1080p if you `exec` that file.
- Hor+ FOV (`cl_adjustfov 1`) and wide HUD (`scr_hudwide 1`) already apply on 21:9.
- The video menu lists the hard-coded modes even when the GPU did not enumerate them; selecting may use borderless / CDS fallback.

Local install helper (Steam path only, not in repo): `scripts/_fix_video_resolution_cfg.py` patches a live Quake II tree; 1080p is now stable index **11**.

---

## Graphics quality (R1GL)

| Cvar | Default | Notes |
|------|---------|--------|
| `gl_picmip` | `0` | Texture downsample. `0` = full res; higher = blurrier / faster. |
| `gl_texturemode` | `GL_LINEAR_MIPMAP_LINEAR` | Filtering (trilinear default). |
| `gl_ext_max_anisotropy` | `16` | Anisotropic filtering â€” keeps textures sharper when viewed at steep angles (floors/walls). Levels 1 / 2 / 4 / 8 / 16; clamped to the GPU max at init. **Archived.** Video menu row. |
| `gl_anisotropy` | `16` | Same as `gl_ext_max_anisotropy` (sharper angled textures; default 16; HW-clamped). Convenience alias that copies into `gl_ext_max_anisotropy` when changed. **Archived.** |
| `gl_ext_texture_filter_anisotropic` | `1` | Master enable for the anisotropy extension. |
| `gl_msaa` | `0` | Multisample anti-aliasing â€” smooths jagged polygon edges. Values: **`0`** off (default, classic crisp), **`2` / `4` / `8`**. Cost is mainly GPU fill-rate. Maps to `gl_ext_multisample` + `gl_ext_samples`. **Requires `vid_restart`** (pixel format). **Archived.** Video menu row. |
| `gl_ext_multisample` | `0` | Legacy enable flag for WGL MSAA pixel format (`1` on). Prefer `gl_msaa`. Needs `vid_restart`. |
| `gl_ext_samples` | `2` | Legacy MSAA sample count when `gl_ext_multisample` is on. Prefer `gl_msaa`. |
| `gl_texture_lodbias` | `0` | Texture sharpen (video menu). Negative values sharpen world textures (about `0` â€¦ `-4`). |
| `gl_linear_mipmaps` | `1` | Prefer linear mipmap generation. |
| `gl_modulate` | `2` | Lightmap / world brightness (video â€œLightmapâ€ slider). |
| `intensity` | `2` | Texture intensity table (clamped â‰¥ 1). |
| `gl_shadows` | `0` | Simple entity blob shadows. |
| `gl_dynamic` | `1` | Dynamic lights from weapons / explosions. |
| `gl_flashblend` | `0` | Draw dlights as blend sprites instead of lighting surfaces. |
| `gl_dlight_falloff` | `0` | Dynamic-light attenuation curve. **`0`** = classic linear (default). **`1`** (or up to `2`) = smoother quadratic falloff on the same dlights - softer edges, no extra through-wall brightness. **Archived.** Video -> **Advanced Settings**. |
| `gl_lightmap_filter` | `1` | Lightmap texture sampling. **`1`** = linear (smoother LM; aniso-friendly when anisotropy is on). **`0`** = nearest (blockier classic LM). Does not change visibility or reveal occluded geometry. **Archived.** Video -> **Advanced Settings**. |
| `gl_light_corona` | `0` | Soft corona sprite at dynamic-light origins. Depth-tested so occluded lights do **not** show (no wallhack). **`0`** = off (default). Try `0.25`-`0.5` for a subtle glow. Menu yes/no writes `0`/`1`. Not bloom / shafts / player ESP. **Archived.** Video -> **Advanced Settings**. |
| `gl_ambient_lift` | `0` | Tiny fill added to lightmaps (and matching entity light samples) so pitch-black is less crushed. Hard-capped at **0.1** linear (~25/255). **`0`** = off (default). Keep at `0.03`-`0.05` if used - not night-vision. **Archived.** Video -> **Advanced Settings**. |
| `gl_warp_amp` | `1` | Water/slime warp amplitude scale (`1` = classic). Range about `0`-`2`. Does not change transparency / SURF_TRANS rules. **Archived.** Video -> **Advanced Settings**. |
| `gl_warp_speed` | `1` | Water/slime warp timing scale (`1` = classic). Range about `0`-`3`. **Archived.** Video -> **Advanced Settings**. |
| `gl_transwater` | `1` | Translucent turb water/slime alpha draw. Effective only when current gamedir basename is exactly `baseq2`; forced opaque for other gamedirs regardless of value. Does not change `SURF_TRANS33`/`SURF_TRANS66` glass. **Archived.** |
| `gl_subdivide` | `64` | Warp surface tessellation size at map load (`64` = classic). Lower (e.g. `32`) = finer water mesh. Needs a map reload / reconnect. Clamped ~16-128. **Archived.** Video -> **Advanced Settings** (64/48/32). |
| `gl_coloredlightmaps` | `1` | Colored lightmaps. |
| `gl_finish` | `0` | Call `glFinish` each frame (can hurt FPS). Leave off. |
| `gl_flush` | `0` | Call `glFlush` before each 3D view (can hurt FPS). Leave off. |
| `gl_defertext` | `0` | Opt-in: draw all 2D text at EndFrame (changes layering vs pics). Console/HUD chars are already batched in order; you usually do not need this. |
| `gl_polyblend` | `0` | Full-screen damage / powerup tint (default **off** in R1Q2v2). |
| `cl_blend` | `0` | Client blend effects (default **off**). |
| `gl_texture_formats` | `png jpg tga` | High-res wall try order before `.wal`. |
| `gl_pic_formats` | `png jpg tga` | High-res 2D pic try order. |
| `gl_pic_scale` | `1` | Enables stock-PCX size hints when loading hi-res pic replacements. |
| `gl_jpg_quality` | `90` | Quality for `screenshot jpg`. |
| `r_lerpmodels` | `1` | Interpolate model frames. |

Optional fair-play lighting polish (console or **Video Settings -> advanced settings...**; defaults stay classic):

```
seta gl_lightmap_filter "1"
seta gl_dlight_falloff "1"
seta gl_warp_amp "1.15"
seta gl_warp_speed "1.1"
seta gl_subdivide "32"
seta gl_light_corona "0.35"
seta gl_ambient_lift "0.04"
```

(`gl_subdivide` needs map reload. No bloom, shafts, see-through, or player glow.)

HD drop-ins: see [README.md](README.md) (`.pkz` packs, loose `textures/` / `env/` replacements).

---

## HUD, menus, console, crosshair

| Cvar | Default | Notes |
|------|---------|--------|
| `scr_menuscale` | `0` | Menu text scale. `0` = auto (~height/480, clamped 1â€“6). Try `2.5` at 1080p. |
| `scr_menupicscale` | `1.2` | Extra scale for menu PCX art (banners, plaques) on top of menu scale (clamped 1â€“2). |
| `con_scale` | `0` | Console / notify text scale. `0` = auto (~height/540, clamped 1-6; ~2.0 at 1080p â€” softer than menu auto). Explicit values work; try `1.75`-`2.0` at 1080p if auto still feels large. Shipped `r1q2v2_visual.cfg` sets `2.0`. |
| `scr_hudscale` | `1` | **Status bar only** (~0.5â€“2.5). Menus and crosshair stay unscaled. Presets must use this, not `gl_hudscale`. |
| `scr_layoutscale` | `0` | **In-game layout menus only** (`STAT_LAYOUTS` / `cl.layout`: Arena team select, help, scoreboards) plus inventory. `0` = auto (~height/480, clamped 1-6). `1` = classic tiny 8px. When scaled, `xl`/`xr` use the same centered 320-wide virtual canvas as `xv` (Arena team menus stay centered at HD). Does **not** apply `scr_hud_top` / `scr_hudwide`. |
| `scr_hud_top` | `0` | **Status bar only.** `1` draws the status bar at the top. Does not affect Arena/RA team menus, help, inventory, or other `STAT_LAYOUTS` UIs. |
| `scr_hudwide` | `1` | **Status bar only.** On screens wider than 4:3, pin health/ammo left and armor/weapon right. Off = original 320-wide centered strip. Saved. Does not affect layout menus. |
| `gl_hudscale` | `1` | Legacy whole-2D scale. Any later `seta` is redirected into `scr_hudscale` (if still `1`) and forced back to `1`. |
| `crosshair` | `0` | `crosshair N` loads `pics/chN` (TGA/PNG/PCX OK). `0` = off (does **not** draw `ch0`). `1`+ uses stock `ch1`â€¦`ch3`, or higher if the pic exists. |
| `ch1` / `ch2` / `ch3` | empty | Free-name overlay slots (Q2PRO-style). Pic name only, e.g. `ch2` or `ch3`. Drawn on top of `crosshair` when non-empty and not cleared. **Clear with `chN 0`** (client treats `string[0]=='0'` as cleared in `cl_scrn.c`). Do **not** use `ch1 ""` — Quake tokenizes away empty quotes so the set never sticks (footgun). Drawn at native size × `ch_scale` × `chN_scale` (large vignettes need a high scale or a full-res asset). |
| `ch_scale` | `1` | Scale for the stock crosshair and all `ch1`â€“`ch3` layers. Options slider (0.1â€“4). Saved. |
| `ch_red` / `ch_green` / `ch_blue` | `1` | Crosshair tint 0â€“1. Options â†’ Crosshair setup. Saved. |
| `ch_alpha` | `1` | Crosshair opacity 0.1â€“1. Saved. |
| `ch_health` | `0` | `1` = greenâ†’yellowâ†’red from current HP (overrides RGB). Saved. |
| `ch1_scale` / `ch2_scale` / `ch3_scale` | `1` | Per-layer scale on top of `ch_scale` (0.1â€“4). Final size = native Ã— `ch_scale` Ã— `chN_scale`. Saved. |
| `ch_x` / `ch_y` | `0` / `0` | Extra pixel offset, added to `scr_crosshair_x` / `y`. |
| `scr_crosshair_x` / `scr_crosshair_y` | `0` / `0` | Crosshair pixel offset. |
| `scr_alpha` | `1` | Status-bar opacity (Options â†’ R1Q2). Saved. |
| `loc_enable` | `1` | Draw nearest `.loc` name on the HUD. Saved. |
| `cl_adjustfov` | `1` | Hor+ FOV: keep 4:3 vertical FOV, widen on widescreen. Saved. |
| `scr_conspeed` | `3` | Console drop / raise speed. |
| `scr_conheight` | `0.5` | Open console height as a fraction of the screen. |
| `con_notifytime` | `3` | Seconds notify lines stay on screen. |
| `scr_showpause` | `1` | Draw â€œPausedâ€; set `0` for cleaner screenshots. |
| `scr_chathud` | `0` | Persistent chat HUD overlay. |
| `scr_chathud_lines` | `4` | Chat HUD line count. |
| `cl_drawfps` | `0` | On-screen FPS. |
| `cl_drawmaptime` | `0` | On-screen map timer. |
| `netgraph` | `0` | Network latency graph. |

**Example hold-to-zoom (crosshair overlays)**

`crosshair N` loads `pics/chN`. Overlay cvars `ch1`/`ch2`/`ch3` take any pic name under `pics/` (no path/extension). Clear overlays with `ch1 0; ch2 0; ch3 0` in your restore alias or they stick after zoom (empty `ch1 ""` does **not** clear — Quake drops empty quotes). Needs matching pics in a pak/pkz (e.g. MaxPak `ch0`â€“`ch9`, `sniper`).

Hold Mouse3 zoom pattern (from a live R1Q2v2 autoexec â€” tweak FOV/sensitivity to taste):

```
alias def_view "fov 110; sensitivity 4.15; crosshair 2; ch1 0; ch2 0; ch3 0; ch_scale 1; ch1_scale 1; ch2_scale 1; ch3_scale 1"
alias +railzoom "fov 60; sensitivity 2.6975; crosshair 9; ch1 ch0; ch2 sniper; ch_scale 1; ch1_scale 1; ch2_scale 1"
alias -railzoom "def_view"
bind MOUSE3 +railzoom
```

Notes: `crosshair 0` is off (does not draw `ch0` â€” use an overlay slot for `ch0`). If `sniper` is a small asset, raise `ch2_scale` (or ship a full-res `pics/sniper`). Options menu spinner still clamps base `crosshair` to 0â€“3; aliases may use higher indices. Optional: `m_autosens 1` instead of hard-coded sensitivity on zoom.


---

## Sound

OpenAL (when compiled in): player-local and `ATTN_NONE` sources are `AL_SOURCE_RELATIVE` at the listener so they do not drift; world sources use the Q2â†’AL axis remap and inverse-distance with rolloff 1.

| Cvar | Default | Notes |
|------|---------|--------|
| `s_volume` | `0.5` | Master SFX volume. |
| `s_khz` | `22` | Mix rate (`11` / `22` / `44` from Options). |
| `s_loadas8bit` | `0` | Load samples as 8-bit. |
| `s_mixahead` | `0.2` | Mix-ahead buffer (seconds). |
| `s_ambient` | `1` | Ambient / looping world sounds. |
| `s_focusfree` | `0` | Keep playing sound when the window is unfocused. |
| `s_initsound` | `1` | `0` silent; `1` DirectSound/DMA; **`2` OpenAL** (needs `OpenAL32.dll` beside exe). |
| `cd_nocd` | `0` | Disable CD audio. |

---

## Input / mouse

| Cvar | Default | Notes |
|------|---------|--------|
| `sensitivity` | `3` | Mouse look sensitivity. |
| `m_pitch` | `0.022` | Pitch scale; negate to invert Y. |
| `m_yaw` | `0.022` | Yaw scale. |
| `m_filter` | `0` | Average mouse deltas (smoother, less raw). Independent of DirectInput mode. |
| `m_autosens` | `0` | Scale mouse by FOV (Q2PRO zoom aliases). `1` uses 90 as the base. |
| `m_directinput` | `0` | Backend when `in_mouse` is 1: `0` = Win32 cursor recenter; `1` = buffered DirectInput; `2` = immediate DirectInput. Does **not** replace or bypass `in_mouse`. |
| `m_fixaccel` | OS-dependent | XP mouse-acceleration fix (Options â†’ R1Q2). |
| `in_mouse` | `1` | **Master mouse enable.** `0` disables mouselook entirely (startup/activate early-out). It is **not** â€œuse Win32 instead of DirectInput.â€ |
| `freelook` | `1` | Always mouselook without `+mlook`. |
| `lookspring` | `0` | Center view when mouselook ends. |
| `lookstrafe` | `0` | Strafe with mouse while looking. |

`in_mouse` is the master switch for mouse look; `m_directinput` only picks the backend while mouse is enabled. DirectInput does **not** replace or bypass `in_mouse`. Recommended crisp combo: `in_mouse 1`, `m_directinput 2`, `m_filter 0`. `m_filter 1` averages deltas for a smoother feel and is independent of DI mode.

---

## Movement, prediction, view

| Cvar | Default | Notes |
|------|---------|--------|
| `cl_async` | `1` | **`1` = stock R1Q2 async** (net/render split, default feel). **`0` = sync physics** â€” render FPS locked to `cl_maxfps` so jump timing matches classic Quake II. This is *not* Q2PRO pmove. Saved. |
| `cl_predict` | `1` | Local movement prediction. |
| `cl_smoothsteps` | `3` | Stair-step smoothing in prediction (modes 1â€“3). |
| `cl_run` | `1` | Always-run. |
| `cl_forwardspeed` / `cl_sidespeed` / `cl_upspeed` | `200` | Base move speeds when not running. |
| `fov` | `90` | Field of view (userinfo). |
| `cl_gun` | `1` | Draw the view weapon. |
| `cl_vwep` | `1` | Visible weapons on other players. |
| `cl_footsteps` | `1` | Local footstep sounds. |
| `cl_nolerp` | `0` | Disable entity interpolation. |

---

## Network / FPS

| Cvar | Default | Notes |
|------|---------|--------|
| `rate` | `15000` | Client bandwidth cap (userinfo). Online often `25000`. |
| `cl_maxfps` | `60` | Network / command FPS; also render FPS when `cl_async 0`. |
| `r_maxfps` | `250` | Render FPS cap when `cl_async 1` (Video â€œMax FPSâ€). |
| `cl_updaterate` | `0` | Requested update rate on R1Q2 protocol (`0` = server default). |
| `cl_timeout` | `120` | Seconds with no packets before disconnect. |
| `cl_instantpacket` | `1` | Send packets immediately when possible. |
| `cl_http_downloads` | `1` | Allow HTTP downloads when the server offers them (`dlserver=`). Needs a **USE_CURL** build and `libcurl.dll` beside the exe. |
| `allow_download` | `1` | Master UDP download switch (maps / models / sounds / players have siblings). |

There is no `cl_snaps` in this tree.

---

## Multiplayer / identity

| Cvar | Default | Notes |
|------|---------|--------|
| `name` | `unnamed` | Player name (userinfo). |
| `skin` | `male/grunt` | Player skin (userinfo). |
| `password` | `""` | Server password (userinfo). |
| `spectator` | `0` | Join as spectator. |
| `adr0` â€¦ `adr15` | `""` | Address Book favorites (Join Server bookmarks). |
| `rcon_password` / `rcon_address` | `""` | Remote console. |
| `cl_filterchat` | `0` | Filter chat messages. |
| `cl_demospeed` | `1` | Demo playback speed 0.1â€“8 (`demospeed`). Saved. |

---

## R1Q2 extras (Options â†’ R1Q2)

These are often **not** written to `config.cfg` â€” keep them in `autoexec.cfg` if you want them permanent.

| Cvar | Default | Notes |
|------|---------|--------|
| `m_directinput` | `0` | Mouse backend when `in_mouse` is on (see Input). |
| `m_fixaccel` | OS-dependent | OS mouse accel fix. |
| `cl_defermodels` | `1` | Map-prep: defer non-world model loads onto the **render** frame (~1 model / 16 ms) so connect/load hitches less. Mid-game `CS_MODELS` / `CS_SOUNDS` / `CS_IMAGES` / `CS_PLAYERSKINS` updates are **always** queued the same way (net parse only stores the string; Register* / stepped `CL_LoadClientinfoStep` run on the render budget). Map-prep drip **overlaps** the mid-game queue (1 map model and/or 1 queued unit per tick). `DA_PLAYERSKIN` is split (tris/skin/icon/one-vwep per tick) with `baseclientinfo` placeholders until ready. Overflow never sync-loads from parse — queue is 512 and force-drains on render only. Custom player models also drip-prefetch sexed pain/death/fall/jump. Does not touch packet send, prediction, or pmove. Timedemo still loads sync. |
| `cl_deferstats` | `0` | Developer/debug: every ~2s prints deferred-asset queue depth, overflow drop count, ms of last `CL_ProcessDeferredAsset`, force-drain flag, pending sexed-prefetch models, and map-prep index (`-1` = done). No gameplay/feel change. |
| `cl_async` | `1` | Sync physics when `0`. Saved. Not Q2PRO pmove. |
| `cl_autorecord` | `0` | Auto-record demos on map start. |
| `cl_railtrail` | `0` | Xania-style rail colors `1`â€“`5`; `0` off. |

---

## Misc

| Cvar | Default | Notes |
|------|---------|--------|
| `game` | `""` | Mod gamedir (latched; needs restart / `game` command). |
| `logfile` | `0` | `1`+ writes console output to a log. |
| `logfile_name` | `qconsole.log` | Log file name under the gamedir. |
| `cl_quietstartup` | `1` | Quieter engine startup. |
| `gl_fontscale` | `1` | Renderer-side font scale helper. |

**Commands (not cvars):** `screenshot` (PNG on Win32), `screenshot jpg`, `vid_restart`, `menu_video`, `connect`, `disconnect`, `ignore` / `unignore` / `mute` / `unmute` / `ignorelist` (persisted in `ignore.txt`), `demopause`, `demospeed`, `addloc`, `saveloc`, `writeconfig` / `writecfg`.

### Config commands

| Command | Notes |
|---------|--------|
| `writeconfig` | Flush key bindings and archived cvars (`seta` / `CVAR_ARCHIVE`) to `<gamedir>/config.cfg` **now** â€” same write path used on quit. Prints `Wrote <gamedir>/config.cfg` on success. **No filename argument in v1** (always `config.cfg`). |
| `writecfg` | Alias for `writeconfig`. |

---

## Quick presets

1920Ã—1080 video (stable `gl_mode 11`; force dims as backup):

```
seta vid_ref "r1gl"
seta vid_fullscreen "1"
seta gl_mode "11"
seta sw_mode "11"
seta vid_forcewidth "1920"
seta vid_forceheight "1080"
```

3440Ã—1440 ultrawide (stable `gl_mode 20`; force dims optional backup):

```
seta vid_ref "r1gl"
seta vid_fullscreen "1"
seta vid_borderless "1"
seta gl_mode "20"
seta sw_mode "20"
seta vid_forcewidth "3440"
seta vid_forceheight "1440"
```

Clear or override force dims after `exec r1q2v2_visual.cfg` (that preset still defaults to 1920Ã—1080).

Readable menus + HUD at 1080p:

```
seta scr_menuscale "2.5"
seta scr_menupicscale "1.2"
seta con_scale "2.0"
seta scr_hudscale "2.5"
seta scr_layoutscale "0"
seta gl_hudscale "1"
```

Shipped preset: copy [`baseq2/r1q2v2_visual.cfg`](baseq2/r1q2v2_visual.cfg) (and optionally [`baseq2/pretty_r1q2v2.cfg`](baseq2/pretty_r1q2v2.cfg)) into the Quake II install `baseq2` folder. Exec from your own `autoexec.cfg` or launch line; it must keep `gl_hudscale 1`.

Smooth high FPS, no vsync, classic sync movement:

```
seta gl_swapinterval "0"
seta r_maxfps "250"
seta cl_async "0"
seta cl_maxfps "125"
```

Sharper HD walls:

```
seta gl_ext_max_anisotropy "16"
seta gl_texture_lodbias "-1"
seta gl_linear_mipmaps "1"
seta gl_texturemode "GL_LINEAR_MIPMAP_LINEAR"
```

