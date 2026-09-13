**Binaries:** Download `R1Q2v2-8059-win32.zip` (BUILD 8059).

**R1Q2v2** is a modern Windows build of **r1ch's R1Q2**. Classic look, menus, console, movement, prediction, and packet-send timing are preserved.

**Install**

You need a Quake II install (Steam is fine). Fully quit the old client, then extract the archive into your Quake II folder. Console should show **R1Q2v2 (Build: 8059)**.

Replace `R1Q2v2.exe`, `ref_r1gl.dll`, and `ref_gl.dll` together. Retail / Steam game data is not included.

---

**What's new in Build 8059**

This release focuses on defensive correctness in cold-path and event-driven code without changing multiplayer rendering cadence, input sampling, movement, prediction, or packet scheduling:

- Prevents oversized network strings and lines from leaving unread bytes behind and desynchronizing subsequent packet commands
- Safely handles empty print messages and maximum-length filtered chat without reading before or writing past the message buffer
- Corrects the upper entity-index boundary for server-started sounds
- Uses bounded copies for multi-line menu spin-control labels
- Validates PAK entry offsets with overflow-safe arithmetic
- Prevents invalid zero or negative `cl_maxfps` and `r_maxfps` values from causing division errors while leaving valid configured values unchanged
- Rejects truncated collision BSP headers and validates collision lumps with overflow-safe bounds checks
- Fixes map `.override` handling so an override without a replacement-map flag retains the original map name; rejects an explicitly empty replacement name
- Guarantees collision map names and savegame menu descriptions are terminated
- Fixes undefined pointer arithmetic when command/alias argument expansion contains no further `$` marker
- Hardens static cinematic PCX loading against truncated headers, invalid palettes, data exhaustion, and row-crossing run lengths
- Validates cinematic dimensions, audio rate/width/channels, frame commands, compressed frame sizes, decoded pixel counts, audio-frame sizes, and Huffman input bounds
- Makes in-memory PNG reads resistant to size arithmetic overflow
- Replaces unbounded renderer diagnostic formatting with bounded formatting
- Handles empty location files safely and preserves the final character when a `.loc` file has no trailing newline
- Rejects invalid negative demo-message lengths before attempting to read frame data
- Validates clipboard text and allocation results before pasting into menu or console fields
- Bounds the accumulated renderer-failure report during video initialization
- Corrects the stock top HUD grouping so armor stays beside health on the left and ammo stays beside the selected weapon on the right

These safeguards run while parsing a message, opening a menu, loading a map/file, or playing a cinematic. They add no per-entity lighting work and do not modify the multiplayer movement or rendering hot paths.

**Main changes carried forward since Build 8020**

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

- Clean Windows x86 build: all 150 client, dedicated-server, game, and renderer targets completed
- 17 WAV regression checks
- 10 HTTP file-list checks
- 10 incoming-packet checks
- 7 MD3 checks
- All 35 loose installed cinematics passed the new header compatibility validation
- Local multiplayer and top-HUD placement verified before release

Build 8057 security details: [SECURITY-FIXES-8057.md](https://github.com/RENEGADE-ANDROiD/R1Q2v2/blob/master/SECURITY-FIXES-8057.md)

**Credits**

r1ch (original R1Q2), h0s3r (this update). Ideas from Q2PRO — not a port of it.
