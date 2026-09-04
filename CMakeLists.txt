cmake_minimum_required(VERSION 3.20)
project(r1q2 VERSION 8012 LANGUAGES C RC)

# Prefer vcpkg toolchain:
#   cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake

option(R1Q2_BUILD_CLIENT "Build Win32 client (needs DirectInput/DirectSound headers)" ON)
option(R1Q2_ANTICHEAT "Compile server anticheat hooks (ANTICHEAT)" OFF)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(Sources)

# R1Q2 still relies on MSVC 32-bit inline asm (Q_ftol, FPU control, etc.).
if(MSVC AND CMAKE_SIZEOF_VOID_P EQUAL 8)
  message(WARNING "x64 MSVC is not supported yet (inline __asm). Prefer Win32 / x86-windows triplet.")
endif()

find_package(ZLIB REQUIRED)

if(MSVC)
  add_compile_definitions(
    _CRT_SECURE_NO_WARNINGS
    _CRT_NONSTDC_NO_WARNINGS
    WIN32
    _WINDOWS
  )
  add_compile_options(/W3 /MP)
endif()

# ---------------------------------------------------------------------------
# game DLL (baseq2)
# ---------------------------------------------------------------------------
add_library(game SHARED ${R1Q2_GAME_SOURCES} game/game.def)
target_compile_definitions(game PRIVATE GAME_DLL)
target_include_directories(game PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/game
  ${ZLIB_INCLUDE_DIRS}
)
target_link_libraries(game PRIVATE winmm)
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set_target_properties(game PROPERTIES OUTPUT_NAME "gamex86_64")
else()
  set_target_properties(game PROPERTIES OUTPUT_NAME "gamex86")
endif()
set_target_properties(game PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
  ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
  PREFIX ""
)

# ---------------------------------------------------------------------------
# dedicated server
# ---------------------------------------------------------------------------
set(R1Q2DED_SOURCES
  ${R1Q2_QCOMMON_SOURCES}
  ${R1Q2_SERVER_SOURCES}
  ${R1Q2_WIN32_COMMON_SOURCES}
  game/q_shared.c
)
if(R1Q2_ANTICHEAT)
  list(APPEND R1Q2DED_SOURCES server/sv_anticheat.c)
endif()

add_executable(r1q2ded WIN32 ${R1Q2DED_SOURCES})
target_compile_definitions(r1q2ded PRIVATE DEDICATED_ONLY)
if(R1Q2_ANTICHEAT)
  target_compile_definitions(r1q2ded PRIVATE ANTICHEAT)
endif()
target_include_directories(r1q2ded PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/qcommon
  ${CMAKE_CURRENT_SOURCE_DIR}/server
  ${CMAKE_CURRENT_SOURCE_DIR}/game
  ${CMAKE_CURRENT_SOURCE_DIR}/win32
  ${ZLIB_INCLUDE_DIRS}
)
target_link_libraries(r1q2ded PRIVATE
  ZLIB::ZLIB
  winmm
  ws2_32
  advapi32
  user32
  gdi32
  shell32
  ole32
)
set_target_properties(r1q2ded PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
  OUTPUT_NAME "r1q2ded"
)

# ---------------------------------------------------------------------------
# client (optional)
# ---------------------------------------------------------------------------
if(R1Q2_BUILD_CLIENT)
  set(R1Q2_CLIENT_EXE_SOURCES
    ${R1Q2_QCOMMON_SOURCES}
    ${R1Q2_SERVER_SOURCES}
    ${R1Q2_WIN32_COMMON_SOURCES}
    ${R1Q2_CLIENT_SOURCES}
    game/q_shared.c
  )
  if(R1Q2_ANTICHEAT)
    list(APPEND R1Q2_CLIENT_EXE_SOURCES server/sv_anticheat.c)
  endif()

  add_executable(r1q2 WIN32 ${R1Q2_CLIENT_EXE_SOURCES})
  if(R1Q2_ANTICHEAT)
    target_compile_definitions(r1q2 PRIVATE ANTICHEAT)
  endif()
  target_include_directories(r1q2 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/client
    ${CMAKE_CURRENT_SOURCE_DIR}/qcommon
    ${CMAKE_CURRENT_SOURCE_DIR}/server
    ${CMAKE_CURRENT_SOURCE_DIR}/game
    ${CMAKE_CURRENT_SOURCE_DIR}/win32
    ${ZLIB_INCLUDE_DIRS}
  )
  target_link_libraries(r1q2 PRIVATE
    ZLIB::ZLIB
    winmm
    ws2_32
    advapi32
    user32
    gdi32
    shell32
    ole32
    dxguid
    dinput8
  )
  set_target_properties(r1q2 PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    OUTPUT_NAME "r1q2"
  )
endif()

if(R1Q2_BUILD_CLIENT)
  message(STATUS "r1q2 targets: game, r1q2ded, r1q2 (client ON)")
else()
  message(STATUS "r1q2 targets: game, r1q2ded (client OFF)")
endif()
