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

- **Modern libpng API** — `png_get_IHDR` / getters instead of direct `png_info` field access; `png_sig_cmp`; `png_set_expand_gray_1_2_4_to_8`; allocation failure check.
- **JPEG robustness** — custom memory source renamed to avoid clash with libjpeg-turbo’s `jpeg_mem_src`; accept any SOI (`FF D8`) JPEG, not only JFIF APP0.
- **Anisotropic filtering** — default `gl_ext_texture_filter_anisotropic 1` and `gl_ext_max_anisotropy 8` (archived), still using the existing extension path in R1GL.

## Next steps (roadmap)

1. Texture upload path: clamp/NPOT handling consistency, safer `glTexImage2D` error paths (extensions already partly present).
2. Optional higher default anisotropy / quality cvars documented in-game.
3. Larger Q2PRO-inspired renderer work only in small, reviewable chunks (state hygiene, extension init) — not a full q2pro port in one shot.
4. Keep cheat-adjacent visuals out of tree.

See `BUILD-WINDOWS.md` for configure/build/deploy commands.
