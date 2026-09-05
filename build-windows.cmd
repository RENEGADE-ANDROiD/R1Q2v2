@echo off
REM One-shot Win32 Release build for R1Q2v2.
REM Requires: VS2022 Build Tools (or full VS) + CMake + Ninja + vcpkg at C:\vcpkg
REM Output: build\bin\R1Q2v2.exe  ref_r1gl.dll  ref_gl.dll  gamex86.dll  r1q2ded.exe

setlocal
cd /d "%~dp0"

if not defined VCPKG_ROOT set "VCPKG_ROOT=C:\vcpkg"
if not exist "%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" (
  echo ERROR: vcpkg not found at %VCPKG_ROOT%
  echo Install vcpkg and run from repo root: vcpkg install --triplet x86-windows
  exit /b 1
)

set "VSROOT="
if exist "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" (
  set "VSROOT=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools"
)
if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" (
  set "VSROOT=C:\Program Files\Microsoft Visual Studio\2022\Community"
)
if exist "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat" (
  set "VSROOT=C:\Program Files\Microsoft Visual Studio\2022\Professional"
)
if exist "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvarsall.bat" (
  set "VSROOT=C:\Program Files\Microsoft Visual Studio\2022\Enterprise"
)
if not defined VSROOT (
  echo ERROR: VS2022 vcvarsall.bat not found.
  exit /b 1
)

set "CMAKEEXE=%VSROOT%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
set "NINJAEXE=%VSROOT%\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe"
if not exist "%CMAKEEXE%" set "CMAKEEXE=cmake.exe"
if not exist "%NINJAEXE%" set "NINJAEXE=ninja.exe"

echo Using VS: %VSROOT%
echo Using vcpkg: %VCPKG_ROOT%

call "%VSROOT%\VC\Auxiliary\Build\vcvarsall.bat" x86
if errorlevel 1 exit /b 1

REM Manifest mode (vcpkg.json): install without package names.
"%VCPKG_ROOT%\vcpkg.exe" install --triplet x86-windows
if errorlevel 1 exit /b 1

"%CMAKEEXE%" -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake" -DVCPKG_TARGET_TRIPLET=x86-windows -DVCPKG_MANIFEST_INSTALL=ON -DCMAKE_MAKE_PROGRAM="%NINJAEXE%"
if errorlevel 1 exit /b 1

"%CMAKEEXE%" --build build --parallel
if errorlevel 1 exit /b 1

echo.
echo Built:
dir /b build\bin\*.exe build\bin\*.dll 2>nul
echo.
echo Copy build\bin\R1Q2v2.exe, ref_r1gl.dll, ref_gl.dll, gamex86.dll, OpenAL32.dll,
echo libcurl.dll, and the zlib/png/jpeg DLLs into your Quake 2 folder and launch R1Q2v2.exe
exit /b 0
