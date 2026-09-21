# R1Q2v2 Build 8062

**Binaries:** Download `R1Q2v2-8062-win32.zip` (BUILD 8062).

**R1Q2v2** is a modern Windows build of **r1ch's R1Q2**. Classic look, menus, console, movement, prediction, and packet-send timing are preserved.

**Install**

You need a Quake II install (Steam is fine). Fully quit the old client, then extract the archive into your Quake II folder. Console should show **R1Q2v2 (Build: 8062)**.

Replace `R1Q2v2.exe`, `ref_r1gl.dll`, and `ref_gl.dll` together. Retail / Steam game data is not included.

---

**What's new in Build 8062**

Auto **client-side** multiplayer visual lean. No net, prediction, or pmove feel changes.

- **MP auto lean:** With `r_mp_visual_lean 1` (default), when connected to multiplayer (`CS_MAXCLIENTS` > 1) the renderer turns expensive 8061 FX **effective-off** without destroying your archived SP `seta` values. Console prints when lean engages / clears.
- **SP / baseq2 / disconnected:** Full `r1q2v2_visual.cfg` look stays — bloom, godrays, soft particles, coronas, shafts, pixel dlights, normal maps, etc.
- **Leaned when MP:** `gl_bloom`, `gl_godrays`, `gl_softparticles`, `gl_world_corona`, `gl_light_shafts`, `gl_normalmaps`, `gl_dlight_shader`, `gl_dynamic`, `gl_shadows`, `gl_light_corona`, `gl_ambient_lift`, `gl_waterfog`.
- **MSAA:** Lean does not auto-`vid_restart`. If you use `gl_msaa`, lower it and restart video when you want MSAA off in MP.
- **Pixel format:** Bad `gl_colorbits 32` / `gl_depthbits 32` are clamped to **24/24** when choosing a pixel format.
- **Opt-out:** `seta r_mp_visual_lean 0` keeps full FX in MP (Quake2Bot `mp_perf.cfg` lean remains fine and redundant).

**Still classic / not in this build**

- No SSAO, shadow maps, path tracing, or player glow / ESP.
- Fair-play: leaning bloom/shafts/world corona in MP is intentional.

**Main changes carried forward since Build 8020**

- **8061:** Fair-play lighting FX (LOS coronas/shafts, capped bloom, godrays, soft particles, pixel dlights, normal maps); new-player Advanced Settings defaults
- **8060:** Loose `config.cfg` load with `Q2config.cfg` fallback; alt-tab msec/mouse; per-hand crosshair offsets
- **8059:** Hardened cold-path diagnostics; corrected stock top-HUD armor/ammo grouping
- **8058:** Arena countdown asset defer; classic `cl_maxpackets 0` default
