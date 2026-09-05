# Building r1q2 on Windows (CMake + VS2022 + vcpkg)

Modern MSVC scaffold for this R1Q2 fork. Classic VS6 projects (`quake2.dsp` / `game.dsp`) remain for reference.

**Fork marker:** `build.h` defines `BUILD` (currently `"8014"`). Console shows `R1Q2v2 (Build: <BUILD>)` — bump `BUILD` for each release.

## Prerequisites

- Visual Studio 2022 **Build Tools** (or full VS) with MSVC C++ tools
- CMake 3.20+ (VS ships one under `Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin`)
- Ninja (same CMake extension folder: `...\CMake\Ninja\ninja.exe`)
- vcpkg at `C:\vcpkg` (or set `CMAKE_TOOLCHAIN_FILE`)
- **zlib**, **libpng**, **libjpeg-turbo** via vcpkg (see below)

### Architecture note (important)

Prefer **Win32 (x86)** for now:

- R1Q2 uses MSVC inline `__asm` / `__declspec(naked)` (FPU helpers, `Q_ftol`, etc.) which **x64 MSVC does not support**
- Game DLL savegame field packing assumes 32-bit pointers
- Stock Steam `ref_*.dll` / `gamex86.dll` are Win32; an x64 client hits Windows error **193** loading them

x64 will need asm replacements before it can be a primary target.

Client builds against Windows SDK `dinput.h` / `dsound.h` (no separate DirectX SDK required on current Win10/11 SDKs). If those headers are missing, configure with `-DR1Q2_BUILD_CLIENT=OFF`.

## Install deps (x86)

```powershell
cd C:\vcpkg
.\vcpkg.exe install zlib libpng libjpeg-turbo --triplet x86-windows
```

A `vcpkg.json` manifest is included; CMake+vcpkg will also restore these in manifest mode.

## Configure + build (recommended: Ninja + vcvars x86)

From a **Developer** environment (or call `vcvarsall.bat x86` first). Do **not** set the `CL` environment variable to a path â€” MSVC treats `%CL%` as extra compiler flags.

```powershell
$vs = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools"
cmd /c "`"$vs\VC\Auxiliary\Build\vcvarsall.bat`" x86 && `"$vs\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe`" -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x86-windows -DCMAKE_MAKE_PROGRAM=`"$vs\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe`" && `"$vs\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe`" --build build --parallel"
```

Outputs in `build\bin\`:

| Target     | Output                          |
|------------|---------------------------------|
| `game`     | `gamex86.dll`                   |
| `r1q2ded`  | `r1q2ded.exe`                   |
| `r1q2`     | `R1Q2v2.exe`                                          |
| `ref_r1gl` | `ref_r1gl.dll` (+ `ref_gl.dll`) |

Also copy/use `z.dll` from `build\bin` (vcpkg shared zlib), and libpng/jpeg DLLs if they appear beside the build outputs.

### Visual Studio generator

`-G "Visual Studio 17 2022" -A Win32` may fail on some Build Tools installs (`VCTargetsPath` probe). Prefer Ninja as above.

## CMake options

| Option | Default | Meaning |
|--------|---------|---------|
| `R1Q2_BUILD_CLIENT` | ON | Build Win32 client |
| `R1Q2_BUILD_REF_GL` | ON | Build `ref_r1gl.dll` / `ref_gl.dll` |
| `R1Q2_ANTICHEAT` | OFF | Define `ANTICHEAT` + compile `sv_anticheat.c` |

## Source inventory

- From `quake2.dsp` (dedicated excludes client/input/sound/vid), `game.dsp`, and `binaries/*/Makefile`
- `qcommon/net_common.c` is `#include`d by `win32/net_wins.c` (not a separate TU)
- `qcommon/unzip.c` + `ioapi.c` compiled (Linux Makefiles; needed by `files.c`)
- `cl_http.c` / `USE_CURL` not enabled
- `ref_r1gl` built from `ref_gl/ref_gl.dsp` sources (see `RENDERER.md`)

## Runtime layout

Deploy into your Quake II folder (example: `D:\SteamLibrary\steamapps\common\Quake 2`):

- `R1Q2v2.exe`, `r1q2ded.exe`, `gamex86.dll`, `z.dll`
- `ref_r1gl.dll` (required) and `ref_gl.dll` (same binary; default `vid_ref`)
- Shared libpng / jpeg DLLs from `build\bin` or `build\vcpkg_installed\x86-windows\bin` when present

`gamex86.dll` also belongs where `Sys_GetGameAPI` searches (often `baseq2\` as well as the exe root for this install). Launch `R1Q2v2.exe` from that folder (create your own shortcut if you want).

More renderer detail: `RENDERER.md`.
