**Binaries:** Download `R1Q2v2-8058-win32.zip` (BUILD 8058).

**R1Q2v2** is a modern Windows build of **r1ch's R1Q2**. Classic look, menus, console, movement, prediction, and packet-send timing are preserved.

**Install**

You need a Quake II install (Steam is fine). Fully quit the old client, then extract the archive into your Quake II folder. Console should show **R1Q2v2 (Build: 8058)**.

Replace `R1Q2v2.exe`, `ref_r1gl.dll`, and `ref_gl.dll` together. Retail / Steam game data is not included.

---

**What's new in Build 8058**

- Prevents Arena round-countdown network freezes by loading the placeholder player immediately and deferring connected-player models, skins, icons, and weapon models in small steps
- Reports any unusually slow deferred asset step in the console to make future hitch diagnosis easier
- Restores classic R1Q2 packet cadence as the default: `cl_maxpackets 0`; the packet limiter remains available as an explicit option
- Adds a 100% Enemy Brightmaps setting while keeping the effect limited to enemy player bodies and their attached weapons
- Keeps enemy brightmaps independent of map light, darkness, monsters, items, teammates, the local player, and the first-person weapon
- Stabilizes Arena red/blue classification across countdown skin flashes, using the documented status-bar face for the local team and accepting genuine remote team changes after they remain stable
- Adds overflow-safe bounds and index validation for BSP and MD2 model data, including truncated headers, frames, triangles, skins, and GL command streams
- Retains the responsive Game-menu mouse fix, top-HUD placement fix, mapper-authored multiplayer water translucency, improved projected shadows, and subtle underwater fog from the refreshed Build 8057 work

The experimental per-entity shadow light resampling that caused worse gameplay lag was removed before this release. Build 8058 does not alter movement, prediction, input sampling, or the classic default packet schedule.

**Main changes carried forward since Build 8020**

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

- Clean Windows x86 build: all 150 client, dedicated-server, game, and renderer targets completed
- 17 WAV regression checks
- 10 HTTP file-list checks
- 6 incoming-packet checks
- 7 MD3 checks
- Local Arena testing confirmed the countdown freeze and gameplay-lag regression were resolved before publication; the final team-color stabilization was built and verified immediately afterward

Build 8057 security details: [SECURITY-FIXES-8057.md](https://github.com/RENEGADE-ANDROiD/R1Q2v2/blob/b8058/SECURITY-FIXES-8057.md)

**Credits**

r1ch (original R1Q2), h0s3r (this update). Ideas from Q2PRO — not a port of it.
