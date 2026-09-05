# R1Q2v2 console variables

Useful client settings for **R1Q2v2 (Build 8012+)**. Defaults are engine defaults unless noted. This is not every cvar in the tree — only ones players typically want.

## How to use

- Console: `` ` `` then `cvar value` (example: `scr_menuscale 2.5`)
- `seta name value` — same, and marks the cvar for `config.cfg` when archived
- Put keepers in `baseq2/autoexec.cfg` so they survive menu / `config.cfg` rewrites
- Many Video Options rows already map to the cvars below

**Not in this client:** Q2PRO-only names like `cl_movement_feel_mode`, `cl_mouse_behavior_mode`, `scr_scale` (HUD uses `scr_hudscale`). For classic jump feel use `cl_async 0`.

---

## Video / display

| Cvar | Default | Notes |
|------|---------|--------|
| `vid_ref` | `r1gl` | Renderer module. Prefer `r1gl` (`ref_r1gl.dll`). |
| `vid_fullscreen` | `0` | `1` fullscreen, `0` windowed. |
| `gl_mode` | desktop index | Mode list index. `-1` = custom size via force width/height. |
| `vid_forcewidth` | `0` | Custom width when `gl_mode -1` (must be &gt; 0). |
| `vid_forceheight` | `0` | Custom height when `gl_mode -1` (must be &gt; 0). |
| `gl_swapinterval` | `0` | Vsync. `0` off (default), `1` on. |
| `vid_gamma` | `1.0` | Display gamma / brightness path used by the video menu. |
| `viewsize` | `100` | 3D view percent; below 100 letterboxes the refresh. |
| `gl_bitdepth` | `0` | Color depth request; `0` = default. |
| `vid_xpos` / `vid_ypos` | `3` / `22` | Window position (windowed). |

---

## Graphics quality (R1GL)

| Cvar | Default | Notes |
|------|---------|--------|
| `gl_picmip` | `0` | Texture downsample. `0` = full res; higher = blurrier / faster. |
| `gl_texturemode` | `GL_LINEAR_MIPMAP_LINEAR` | Filtering (trilinear default). |
| `gl_ext_max_anisotropy` | `16` | Anisotropic filtering (1 / 2 / 4 / 8 / 16). Real cvar — not `gl_anisotropy` alone. |
| `gl_anisotropy` | `16` | Convenience alias; nonzero values copy into `gl_ext_max_anisotropy`. |
| `gl_texture_lodbias` | `0` | Texture sharpen (video menu). Negative values sharpen world textures (about `0` … `-4`). |
| `gl_linear_mipmaps` | `1` | Prefer linear mipmap generation. |
| `gl_modulate` | `2` | Lightmap / world brightness (video “Lightmap” slider). |
| `intensity` | `2` | Texture intensity table (clamped ≥ 1). |
| `gl_shadows` | `0` | Simple entity blob shadows. |
| `gl_dynamic` | `1` | Dynamic lights from weapons / explosions. |
| `gl_flashblend` | `0` | Draw dlights as blend sprites instead of lighting surfaces. |
| `gl_coloredlightmaps` | `1` | Colored lightmaps. |
| `gl_finish` | `0` | Call `glFinish` each frame (can hurt FPS). |
| `gl_polyblend` | `0` | Full-screen damage / powerup tint (default **off** in R1Q2v2). |
| `cl_blend` | `0` | Client blend effects (default **off**). |
| `gl_texture_formats` | `png jpg tga` | High-res wall try order before `.wal`. |
| `gl_pic_formats` | `png jpg tga` | High-res 2D pic try order. |
| `gl_pic_scale` | `1` | Enables stock-PCX size hints when loading hi-res pic replacements. |
| `gl_jpg_quality` | `90` | Quality for `screenshot jpg`. |
| `r_lerpmodels` | `1` | Interpolate model frames. |

HD drop-ins: see [README.md](README.md) (`.pkz` packs, loose `textures/` / `env/` replacements).

---

## HUD, menus, console, crosshair

| Cvar | Default | Notes |
|------|---------|--------|
| `scr_menuscale` | `0` | Menu text scale. `0` = auto (~height/480, clamped 1–6). Try `2.5` at 1080p. |
| `scr_menupicscale` | `1.2` | Extra scale for menu PCX art (banners, plaques) on top of menu scale (clamped 1–2). |
| `con_scale` | `0` | Console / notify text scale. `0` = auto like menus. |
| `scr_hudscale` | `1` | **Status bar only** (~0.5–2.5). Menus and crosshair stay unscaled. Presets must use this, not `gl_hudscale`. |
| `scr_hud_top` | `0` | `1` draws the HUD at the top. |
| `gl_hudscale` | `1` | Legacy whole-2D scale. Any later `seta` is redirected into `scr_hudscale` (if still `1`) and forced back to `1`. |
| `crosshair` | `0` | `0` none; `1`–`3` use `pics/ch1`…`ch3`. |
| `scr_crosshair_x` / `scr_crosshair_y` | `0` / `0` | Crosshair pixel offset. |
| `scr_conspeed` | `3` | Console drop / raise speed. |
| `scr_conheight` | `0.5` | Open console height as a fraction of the screen. |
| `con_notifytime` | `3` | Seconds notify lines stay on screen. |
| `scr_showpause` | `1` | Draw “Paused”; set `0` for cleaner screenshots. |
| `scr_chathud` | `0` | Persistent chat HUD overlay. |
| `scr_chathud_lines` | `4` | Chat HUD line count. |
| `cl_drawfps` | `0` | On-screen FPS. |
| `cl_drawmaptime` | `0` | On-screen map timer. |
| `netgraph` | `0` | Network latency graph. |

---

## Sound

| Cvar | Default | Notes |
|------|---------|--------|
| `s_volume` | `0.5` | Master SFX volume. |
| `s_khz` | `22` | Mix rate (`11` / `22` / `44` from Options). |
| `s_loadas8bit` | `0` | Load samples as 8-bit. |
| `s_mixahead` | `0.2` | Mix-ahead buffer (seconds). |
| `s_ambient` | `1` | Ambient / looping world sounds. |
| `s_focusfree` | `0` | Keep playing sound when the window is unfocused. |
| `s_initsound` | `1` | `0` silent; `1` normal DMA/DirectSound. |
| `cd_nocd` | `0` | Disable CD audio. |

---

## Input / mouse

| Cvar | Default | Notes |
|------|---------|--------|
| `sensitivity` | `3` | Mouse look sensitivity. |
| `m_pitch` | `0.022` | Pitch scale; negate to invert Y. |
| `m_yaw` | `0.022` | Yaw scale. |
| `m_filter` | `0` | Average mouse deltas (smoother, less raw). |
| `m_directinput` | `0` | `0` off; `1` buffered DirectInput; `2` immediate. |
| `m_fixaccel` | OS-dependent | XP mouse-acceleration fix (Options → R1Q2). |
| `in_mouse` | `1` | Enable mouse input. |
| `freelook` | `1` | Always mouselook without `+mlook`. |
| `lookspring` | `0` | Center view when mouselook ends. |
| `lookstrafe` | `0` | Strafe with mouse while looking. |

---

## Movement, prediction, view

| Cvar | Default | Notes |
|------|---------|--------|
| `cl_async` | `1` | `1` = R1Q2 net/render split; **`0` = synced physics** (classic / Q2PRO-like jump feel). |
| `cl_predict` | `1` | Local movement prediction. |
| `cl_smoothsteps` | `3` | Stair-step smoothing in prediction (modes 1–3). |
| `cl_run` | `1` | Always-run. |
| `cl_forwardspeed` / `cl_sidespeed` / `cl_upspeed` | `200` | Base move speeds when not running. |
| `fov` | `90` | Field of view (userinfo). |
| `hand` | `0` | Weapon hand: `0` right, `1` left, `2` center. |
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
| `r_maxfps` | `250` | Render FPS cap when `cl_async 1` (Video “Max FPS”). |
| `cl_updaterate` | `0` | Requested update rate on R1Q2 protocol (`0` = server default). |
| `cl_timeout` | `120` | Seconds with no packets before disconnect. |
| `cl_instantpacket` | `1` | Send packets immediately when possible. |
| `cl_http_downloads` | `1` | Allow HTTP downloads when the server offers them. |
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
| `adr0` … `adr15` | `""` | Address Book favorites (Join Server bookmarks). |
| `rcon_password` / `rcon_address` | `""` | Remote console. |
| `cl_filterchat` | `0` | Filter chat messages. |

---

## R1Q2 extras (Options → R1Q2)

These are often **not** written to `config.cfg` — keep them in `autoexec.cfg` if you want them permanent.

| Cvar | Default | Notes |
|------|---------|--------|
| `m_directinput` | `0` | DirectInput mouse (see Input). |
| `m_fixaccel` | OS-dependent | OS mouse accel fix. |
| `cl_defermodels` | `1` | Defer model loads to reduce hitching. |
| `cl_async` | `1` | Q2Pro-style sync when set to `0`. |
| `cl_autorecord` | `0` | Auto-record demos on map start. |
| `cl_railtrail` | `0` | Xania-style rail colors `1`–`5`; `0` off. |

---

## Misc

| Cvar | Default | Notes |
|------|---------|--------|
| `game` | `""` | Mod gamedir (latched; needs restart / `game` command). |
| `logfile` | `0` | `1`+ writes console output to a log. |
| `logfile_name` | `qconsole.log` | Log file name under the gamedir. |
| `cl_quietstartup` | `1` | Quieter engine startup. |
| `gl_fontscale` | `1` | Renderer-side font scale helper. |

**Commands (not cvars):** `screenshot` (PNG on Win32), `screenshot jpg`, `vid_restart`, `menu_video`, `connect`, `disconnect`.

---

## Quick presets

Readable menus + HUD at 1080p:

```
seta scr_menuscale "2.5"
seta scr_menupicscale "1.2"
seta con_scale "0"
seta scr_hudscale "2.5"
seta gl_hudscale "1"
```

Shipped preset: copy [`baseq2/r1q2v2_visual.cfg`](baseq2/r1q2v2_visual.cfg) (and optionally [`baseq2/pretty_r1q2v2.cfg`](baseq2/pretty_r1q2v2.cfg)) into the Quake II install `baseq2` folder. Mod autoexecs `exec` that file; it must keep `gl_hudscale 1`.

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
