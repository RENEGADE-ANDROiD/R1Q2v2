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
- **Anisotropic filtering** — default `gl_ext_texture_filter_anisotropic 1` and `gl_ext_max_anisotropy 8` (archived), still using the existing extension path in R1GL.

## Frame path / hitch (Build 8014)

Safe client-only smoothness work. **No net send, prediction, pmove, or `cl_async` changes.** No new feel cvars; stock defaults unchanged.

- **2D text:** `Draw_Char` always batches into one bind + one `glBegin(GL_QUADS)` and flushes before pics/fills (classic layering). `gl_defertext 1` still waits until EndFrame.
- **2D state:** skip redundant blend/alpha/TexEnv when consecutive HUD pics share the same mode; `Draw_FindPic` keeps a last-name cache.
- **Present:** `R_EndFrame` flushes text then swaps (Linux previously never drained deferred chars). `gl_finish` / `gl_flush` / `gl_swapinterval` defaults stay `0`.
- **GL thrash:** `GL_SelectTexture` no-ops when already on that TMU; LOD-bias env is applied on change/init, not every `R_SetupGL`.
- **Modelview:** CPU matrix matching the classic rotate/translate sequence + `glLoadMatrixf` — no per-view `glGetFloatv` stall.
- **Load hitch:** deferred models run on the render frame (not the send frame), at most one every ~16 ms. Map-load clientinfo no longer `SCR_UpdateScreen`s once per player.

## Next steps (roadmap)

1. Texture upload path: clamp/NPOT handling consistency, safer `glTexImage2D` error paths (extensions already partly present).
2. Optional higher default anisotropy / quality cvars documented in-game.
3. Larger Q2PRO-inspired renderer work only in small, reviewable chunks (state hygiene, extension init) — not a full q2pro port in one shot.
4. Keep cheat-adjacent visuals out of tree.

See `BUILD-WINDOWS.md` for configure/build/deploy commands.
