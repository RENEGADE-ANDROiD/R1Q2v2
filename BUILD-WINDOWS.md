# Building r1q2 on Windows (CMake + VS2022 + vcpkg)

Modern MSVC scaffold for this R1Q2 fork. Classic VS6 projects (`quake2.dsp` / `game.dsp`) remain for reference.

**Fork marker:** `build.h` defines `BUILD` as `"8012-RENEGADE"` (per `r1q2.txt`).

## Prerequisites

- Visual Studio 2022 **Build Tools** (or full VS) with MSVC C++ tools
- CMake 3.20+ (VS ships one under `Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin`)
- Ninja (same CMake extension folder: `...\CMake\Ninja\ninja.exe`)
- vcpkg at `C:\vcpkg` (or set `CMAKE_TOOLCHAIN_FILE`)
- **zlib** via vcpkg

### Architecture note (important)

Prefer **Win32 (x86)** for now:

- R1Q2 uses MSVC inline `__asm` / `__declspec(naked)` (FPU helpers, `Q_ftol`, etc.) which **x64 MSVC does not support**
- Game DLL savegame field packing assumes 32-bit pointers

x64 will need asm replacements before it can be a primary target.

Client builds against Windows SDK `dinput.h` / `dsound.h` (no separate DirectX SDK required on current Win10/11 SDKs). If those headers are missing, configure with `-DR1Q2_BUILD_CLIENT=OFF`.

## Install zlib (x86)

```powershell
cd C:\vcpkg
.\vcpkg.exe install zlib --triplet x86-windows
```

A `vcpkg.json` manifest is included; CMake+vcpkg will also restore zlib in manifest mode.

## Configure + build (recommended: Ninja + vcvars x86)

From a **Developer** environment (or call `vcvarsall.bat x86` first). Do **not** set the `CL` environment variable to a path ? MSVC treats `%CL%` as extra compiler flags.

```powershell
$vs = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools"
cmd /c "`"$vs\VC\Auxiliary\Build\vcvarsall.bat`" x86 && `"$vs\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe`" -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x86-windows -DCMAKE_MAKE_PROGRAM=`"$vs\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe`" && `"$vs\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe`" --build build --parallel"
```

Outputs in `build\bin\`:

| Target    | Output         |
|-----------|----------------|
| `game`    | `gamex86.dll`  |
| `r1q2ded` | `r1q2ded.exe`  |
| `r1q2`    | `r1q2.exe`     |

Also copy/use `z.dll` from `build\bin` (vcpkg shared zlib).

### Visual Studio generator

`-G "Visual Studio 17 2022" -A Win32` may fail on some Build Tools installs (`VCTargetsPath` probe). Prefer Ninja as above.

## CMake options

| Option | Default | Meaning |
|--------|---------|---------|
| `R1Q2_BUILD_CLIENT` | ON | Build Win32 client |
| `R1Q2_ANTICHEAT` | OFF | Define `ANTICHEAT` + compile `sv_anticheat.c` |

## Source inventory

- From `quake2.dsp` (dedicated excludes client/input/sound/vid), `game.dsp`, and `binaries/*/Makefile`
- `qcommon/net_common.c` is `#include`d by `win32/net_wins.c` (not a separate TU)
- `qcommon/unzip.c` + `ioapi.c` compiled (Linux Makefiles; needed by `files.c`)
- `cl_http.c` / `USE_CURL` not enabled
- `ref_gl` renderer DLL is **not** in this scaffold yet (client loads it at runtime)

## Runtime layout

Place `gamex86.dll` where `Sys_GetGameAPI` searches (typically `baseq2\`). Ensure a compatible `ref_gl` / renderer DLL is available for the client.
