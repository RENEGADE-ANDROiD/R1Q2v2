**Binaries:** Download `R1Q2v2-8061-win32.zip` (BUILD 8061).

**R1Q2v2** is a modern Windows build of **r1ch's R1Q2**. Classic look, menus, console, movement, prediction, and packet-send timing are preserved.

**Install**

You need a Quake II install (Steam is fine). Fully quit the old client, then extract the archive into your Quake II folder. Console should show **R1Q2v2 (Build: 8061)**.

Replace `R1Q2v2.exe`, `ref_r1gl.dll`, and `ref_gl.dll` together. Retail / Steam game data is not included.

New installs get the Advanced Settings look below. An existing `config.cfg` keeps its archived values; `exec r1q2v2_visual.cfg` (or toggle Video → Advanced Settings) applies this look.

---

**What's new in Build 8061**

Fair-play R1GL lighting. Multiplayer movement, prediction, and the default packet cadence are unchanged.

- **Explosion wallhack fix:** Dynamic-light coronas (`gl_light_corona`) require a BSP line-of-sight from the camera to the light origin. Occluded explosions draw nothing.
- **World coronas / light shafts:** Soft sprites on map lamps and a short streak on **visible** lights only. Sprite only — no extra dynamic lights.
- **Bloom:** On/off only (no slider). Glows **already visible** bright pixels from the 3D buffer at a hardcoded low add. HUD is not bloomed. Intensity cannot be raised to leak through walls.
- **God rays / soft particles:** Radial shafts toward a LOS-visible on-screen light; sprites fade against a copy of scene depth.
- **Pixel dlights / normal maps:** Per-pixel world lights instead of lightmap squares. Occluded rooms stay dark. Optional `_norm` / `_n` / `_bump` if the pack ships png/jpg/tga (not `.wal`). Missing normals are ignored once — they no longer freeze the client on map change.
- **Overbright lightmaps:** 2× combine is on Advanced Settings and actually toggles without a renderer restart.
- **New-player defaults:** Advanced Settings start on this look (linear lightmaps, falloff 1.4, warp 1.5/1.5, subdivide 32, coronas/shafts on, overbright on, bloom on, god rays 0.35, soft particles, pixel dlights, normal maps, ambient 0.04, underwater fog 0.35).
- **New-game map load:** Hunk reservation no longer fatal-errors `VirtualAlloc` on Outer Base / `*base1`.
- **Local cheat files:** Optional client check can refuse remote multiplayer if known cheat files or wallhack wrappers are present (`cl_cheatcheck`, default on). Single-player and localhost are not blocked.

**Still classic / not in this build**

- With `gl_dlight_shader 0`, surface dynamic lights can still stain a floor through a thin wall (stock Quake II `R_MarkLights`).
- Window MSAA does not anti-alias the 3D view while bloom / god rays / soft particles are on.
- No SSAO, shadow maps, path tracing, or player glow / ESP.

**Main changes carried forward since Build 8020**

- **8060:** Loose `config.cfg` load with `Q2config.cfg` fallback; alt-tab msec/mouse so q2admin HT_MSEC is not tripped; per-hand crosshair offsets
- **8059:** Hardened cold-path network, file, cinematic, demo, clipboard, and renderer diagnostics; corrected stock top-HUD armor/ammo grouping
- **8058:** Prevented Arena countdown freezes by deferring connected-player models, skins, icons, and weapon models in small steps
- **8058:** Restored classic `cl_maxpackets 0` packet cadence as the default while retaining the limiter as an explicit option
- **8058:** Added 100% player-only Enemy Brightmaps without changing map lighting, monsters, items, teammates, or the first-person weapon
- **8057:** Restored mapper-authored `TRANS33`/`TRANS66` warp-water opacity online while continuing to ignore the global `gl_wateralpha` override in multiplayer
- **8057:** Added basic, height-faded, and soft projected model shadows plus subtle liquid-colored underwater distance fog

**Credits**

r1ch (original R1Q2), h0s3r (this update). Ideas from Q2PRO — not a port of it.
