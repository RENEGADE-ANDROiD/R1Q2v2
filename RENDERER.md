# Renderer notes (R1GL / ref_r1gl)

## Build output

CMake target `ref_r1gl` produces:

- `build/bin/ref_r1gl.dll` — primary R1GL module (`vid_ref r1gl`)
- `build/bin/ref_gl.dll` — copy of the same DLL for default `vid_ref gl`

Sources match classic `ref_gl/ref_gl.dsp` (ref_gl + win32 GL + `game/q_shared.c` + `ref_gl.def`). Compile with `REF_GL`. Links OpenGL, GDI, winmm, zlib, libpng, and libjpeg-turbo (vcpkg).

Prefer **Win32 / x86** so the DLL matches `r1q2.exe` and Steam’s 32-bit game folder. An x64 renderer will fail to load into a Win32 client (Windows error 193).

## First-pass modernization (fair + classic)

Kept classic fonts, console, and menus. No bloom wallhacks, player wireframes/ESP, weather, or heavy post-FX.

Landed in this pass:

- **MD3 / IDP3 models** — ref_gl/md3.h + load in gl_model.c + draw in gl_mesh.c. Detects magic IDP3 regardless of extension (fixes w_*.md2 vweps that are really MD3). Multi-mesh + frame lerp; tags ignored; skin names resolved as Q2 images.

- **Modern libpng API** — `png_get_IHDR` / getters instead of direct `png_info` field access; `png_sig_cmp`; `png_set_expand_gray_1_2_4_to_8`; allocation failure check.
- **JPEG robustness** — custom memory source renamed to avoid clash with libjpeg-turbo’s `jpeg_mem_src`; accept any SOI (`FF D8`) JPEG, not only JFIF APP0.
- **Anisotropic filtering** — default `gl_ext_texture_filter_anisotropic 1` and `gl_ext_max_anisotropy 16` (archived; HW-clamped), still using the existing extension path in R1GL.

## Frame path / hitch (Build 8014)

Safe client-only smoothness work. **No net send, prediction, pmove, or `cl_async` changes.** No new feel cvars; stock defaults unchanged.

- **2D text:** `Draw_Char` always batches into one bind + one `glBegin(GL_QUADS)` and flushes before pics/fills (classic layering). `gl_defertext 1` still waits until EndFrame.
- **2D state:** skip redundant blend/alpha/TexEnv when consecutive HUD pics share the same mode; `Draw_FindPic` keeps a last-name cache.
- **Present:** `R_EndFrame` flushes text then swaps (Linux previously never drained deferred chars). `gl_finish` / `gl_flush` / `gl_swapinterval` defaults stay `0`.
- **MSAA:** `gl_msaa` 0/2/4/8 (default 0). Pixel-format path; changing it forces `vid_restart`. Legacy: `gl_ext_multisample` + `gl_ext_samples`.
- **Adaptive sync:** `gl_swapinterval -1` when `WGL_EXT_swap_control_tear` exists; falls back to `1` otherwise. Menu exposes off/on/adaptive.
- **GL thrash:** `GL_SelectTexture` no-ops when already on that TMU; LOD-bias env is applied on change/init, not every `R_SetupGL`.
- **Modelview:** CPU matrix matching the classic rotate/translate sequence + `glLoadMatrixf` — no per-view `glGetFloatv` stall.
- **Load hitch:** deferred models run on the render frame (not the send frame), at most one every ~16 ms. Map-load clientinfo no longer `SCR_UpdateScreen`s once per player.

## Fair-play lighting sprites (Build 8061)

Still fixed-function R1GL. Build 8061 new-player defaults match Advanced Settings playtest (coronas/shafts/bloom/god rays/soft particles/pixel dlights on).

- **LOS test:** BSP walk from the camera to the light origin. `CONTENTS_SOLID` blocks; a 24-unit slop at the light end allows ceiling/wall mounts. Used for dlight coronas, world coronas, and shafts so sprites cannot poke through thin walls (depth test alone was not enough).
- **`gl_world_corona`:** `classname light` entities from `LUMP_ENTITIES`. Sprite only — no extra dlights.
- **`gl_light_shafts`:** short additive streak on visible lights. Not volumetric god rays.
- **`gl_overbrights`:** existing 2× lightmap combine, now on Advanced Settings and archived. TexEnv follows the cvar immediately (the old `modified` short-circuit left it stuck on modulate).

No FBO, bloom, or player glow in slice 1. Surface dlight marking is unchanged (classic Q2 can still light a floor through a thin wall).

## FBO post-FX (Build 8061 slice 2)

Still R1GL. Compatibility-context `GL_FRAMEBUFFER` + GLSL 2.0 entry points in `ref_gl/gl_fbo.c`. HUD is drawn after the blit.

- 3D view renders into a color+depth FBO (`vid.width` × `vid.height`). Window MSAA does not apply to that 3D buffer while any of these are on.
- **`gl_bloom`:** on/off only. Downsample + high threshold (~0.80) + **one** separable blur of **on-screen** bright texels, composited at a hardcoded ~0.20 add. The cvar magnitude is ignored so bloom cannot be cranked into a through-wall glow. Occluded lights are black in the scene.
- **`gl_godrays`:** radial blur of that bright buffer toward one LOS-visible projected dlight/world light. No center if the light is behind a wall.
- **`gl_softparticles`:** blit scene depth to a copy, then a particle shader fades alpha against linearised depth. Forced onto the triangle particle path.

If FBO or GLSL is missing, the console prints that post-FX is unavailable and the cvars do nothing.

## World-pass shaders (Build 8061 slice 3)

Still R1GL compatibility-context GLSL 1.10. No shadow maps. Pixel dlights default on.

- **`gl_dlight_shader`:** Per-pixel dlights on lightmapped world/bmodel surfaces. CPU `R_AddDynamicLights` is skipped so lightmaps stay baked-only. A BSP trace from the light to the surface centroid plus a back-face test stops stains through thin walls.
- **`gl_normalmaps`:** If the pack ships `textures/<wal>_norm` / `_n` / `_bump` (png/jpg/tga/wal), that map perturbs the geometric normal. Missing files are searched once and ignored.

Console prints `using GLSL world dlights` at renderer init when the program links. Needs `gl_dynamic 1`.

## Next steps (roadmap)

1. Texture upload path: clamp/NPOT handling consistency, safer `glTexImage2D` error paths (extensions already partly present).
2. Optional higher default anisotropy / quality cvars documented in-game.
3. Larger Q2PRO-inspired renderer work only in small, reviewable chunks (state hygiene, extension init) — not a full q2pro port in one shot.
4. Keep cheat-adjacent visuals out of tree.

See `BUILD-WINDOWS.md` for configure/build/deploy commands.

## Multiplayer visual lean (BUILD 8062)

`r_mp_visual_lean` (default `1`) makes the renderer treat expensive FX as off when the client is in multiplayer (`r_water_alpha_ok == 2` from `CS_MAXCLIENTS` > 1). Archived SP `seta` values are not rewritten. Singleplayer and the disconnected menu keep the full 8061 look. MSAA is not auto-restarted. **BUILD 8063:** `r_water_alpha_ok` is preserved across map `ClearState` so lean does not drop between levels (avoids a burst of normalmap filesystem probes when `r1q2v2_visual.cfg` still has FX seta on). Lean is latched once per frame; world-shader begin and `R_PushDlights` skip work when lean is active.
