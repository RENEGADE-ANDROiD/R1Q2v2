**Binaries:** Download `R1Q2v2-8060-win32.zip` (BUILD 8060).

**R1Q2v2** is a modern Windows build of **r1ch's R1Q2**. Classic look, menus, console, movement, prediction, and packet-send timing are preserved.

**Install**

You need a Quake II install (Steam is fine). Fully quit the old client, then extract the archive into your Quake II folder. Console should show **R1Q2v2 (Build: 8060)**.

Replace `R1Q2v2.exe`, `ref_r1gl.dll`, and `ref_gl.dll` together. Retail / Steam game data is not included.

---

**What's new in Build 8060**

Player-facing config, alt-tab, and crosshair setup. Multiplayer movement, prediction, and the default packet cadence are unchanged.

- **Config load:** Startup `exec`s a loose `config.cfg` from the current gamedir, then `baseq2`, then next to the exe. A `.pak` / `.pkz` cannot shadow it. If that file is missing, `Q2config.cfg` is used (Q2PRO / Yamagi). The console prints `execing config.cfg` so a miss is visible. Hidden `autoexec.cfg` is no longer skipped.
- **Alt-tab:** Unfocused `cmd.msec` stays near `1000/cl_maxfps` instead of a flat 50 or the stock `250 → 100` remap that q2admin HT_MSEC treats as a timing bot. Desktop mouse movement still counts as look without capturing the cursor. Windows background EcoQoS throttling is disabled so alt-tab does not stall the loop.
- **Crosshair position:** Options → Crosshair setup has **x offset** / **y offset** sliders (`ch_x` / `ch_y`, −64…+64 px). Separate saved slots for right, left, and center `hand` (`ch_x_right` / `ch_x_left` / `ch_x_center` and matching `ch_y_*`). Switching handedness loads that hand’s slot.

**Main changes carried forward since Build 8020**

- **8059:** Hardened cold-path network, file, cinematic, demo, clipboard, and renderer diagnostics; corrected stock top-HUD armor/ammo grouping
- **8058:** Prevented Arena countdown freezes by deferring connected-player models, skins, icons, and weapon models in small steps
- **8058:** Restored classic `cl_maxpackets 0` packet cadence as the default while retaining the limiter as an explicit option
- **8058:** Added 100% player-only Enemy Brightmaps without changing map lighting, monsters, items, teammates, or the first-person weapon
- **8058:** Stabilized Arena red/blue classification across countdown skin flashes, using the status-bar face for the local team and delayed acceptance of genuine remote team changes
- **8058:** Added BSP/MD2 model validation for truncated headers, offsets, frames, triangles, skins, indices, and GL command streams
- **8057:** Hardened incoming packet, download, JPEG, WAV, and MD3 processing; versioned the extended renderer interface
- **8057:** Fixed `scr_hud_top` placement so stock HUD elements retain their normal order and the deathmatch frag counter does not clip
- **8057:** Restored mapper-authored `TRANS33`/`TRANS66` warp-water opacity online while continuing to ignore the global `gl_wateralpha` override in multiplayer
- **8057:** Fixed short Game-menu mouse hitboxes and standard-menu mouse-wheel navigation
- **8057:** Added basic, height-faded, and soft projected model shadows plus subtle liquid-colored underwater distance fog
- **8056:** Limited HUD top/scale/wide handling to the stock status bar so custom multiplayer HUDs retain their original coordinates
- **8056:** Added optional multiplayer enemy/team colors, tint opacity, enemy brightmaps, and team-countdown handling
- **8056:** Fixed crashes when Arena player skins use large RGB PNG images
- **8055:** Simplified video-driver selection to R1GL and remapped old `soft`, `gl`, and `ncgl` selections to it
- **8054:** Passed actual window dimensions through video-mode changes and clamped stale menu spinner values
- **8053:** Limited the global translucent-water override to single-player
- **8052:** Improved menu hover, sliders, and mouse-wheel handling; hid unsupported stock OpenGL choices and guarded invalid video sizes during shutdown or Alt-Tab
- **8051:** Added the optional `cl_maxpackets` limiter with two-command recovery and immediate attack/use/jump sends; moved synchronous HTTP work after packet read/send
- **8050:** Ignored failed ICMP receives instead of reparsing a previous packet, disabled Windows UDP connection-reset errors, and expanded `CMD_BACKUP`
- **8049:** Rendered Arena and TastySpleen MOTD layouts on a centered 320×240 widescreen canvas
- **8047:** Improved mouse reacquisition after Alt-Tab and corrected unfocused frame timing
- **8045–8046:** Preserved classic opaque multiplayer water until the later mapper-authored translucency refinement
- **8040–8044:** Reduced multiplayer first-use hitches with deferred assets, larger queues, sound prefetching, missing-image caching, disguise prefetching, deferred stats, and reloads after downloads
- **8039:** Added paging and reliable click targets to the full Join Server list, retained `q2rpg2` servers, and pinned the Quaketown hub
- **8038:** Made R1GL the default renderer; added deferred render assets, click-to-rebind controls, configuration writing, centered 4:3 console/menu backdrops, and expanded advanced video settings
- **8038:** Added optional MSAA, anisotropic filtering, adaptive sync, lightmap filtering, dynamic-light falloff, restrained ambient/corona controls, and configurable warp tessellation with classic defaults
- **8020:** Centered scaled Arena layouts on the same 320-wide canvas and softened automatic console scaling for modern resolutions

**Validation**

- Clean Windows x86 rebuild of the client, dedicated server, and R1GL renderer (BUILD 8060 in the version string)
- 8059 cinematic, WAV, HTTP, packet, and MD3 harness coverage is unchanged; this build does not alter those loaders

Build 8057 security details: [SECURITY-FIXES-8057.md](https://github.com/RENEGADE-ANDROiD/R1Q2v2/blob/master/SECURITY-FIXES-8057.md)

**Credits**

r1ch (original R1Q2), h0s3r (this update). Ideas from Q2PRO — not a port of it.
