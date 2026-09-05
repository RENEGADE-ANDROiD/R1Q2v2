cmake_minimum_required(VERSION 3.20)
project(r1q2 VERSION 8014 LANGUAGES C RC)

# Prefer vcpkg toolchain:
#   cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake

option(R1Q2_BUILD_CLIENT "Build Win32 client (needs DirectInput/DirectSound headers)" ON)
option(R1Q2_USE_OPENAL "Compile client with OpenAL (s_initsound 2)" ON)
option(R1Q2_USE_CURL "Compile client HTTP downloads (libcurl / TastySpleen dlserver)" ON)
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
find_package(PNG REQUIRED)
find_package(JPEG REQUIRED)
if(R1Q2_USE_OPENAL)
  find_package(OpenAL CONFIG REQUIRED)
endif()
if(R1Q2_USE_CURL)
  find_package(CURL REQUIRED)
endif()

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
  if(R1Q2_USE_OPENAL)
    target_compile_definitions(r1q2 PRIVATE USE_OPENAL)
  endif()
  if(R1Q2_USE_CURL)
    target_compile_definitions(r1q2 PRIVATE USE_CURL)
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
  if(R1Q2_USE_OPENAL)
    target_link_libraries(r1q2 PRIVATE OpenAL::OpenAL)
  endif()
  if(R1Q2_USE_CURL)
    target_link_libraries(r1q2 PRIVATE CURL::libcurl)
  endif()
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
    wininet
  )
  if(R1Q2_USE_OPENAL)
    # OpenAL Soft from vcpkg also needs fmt.dll next to OpenAL32.dll (LoadLibrary).
    add_custom_command(TARGET r1q2 POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:OpenAL::OpenAL>
        $<TARGET_FILE_DIR:r1q2>/OpenAL32.dll
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/bin/fmt.dll"
        $<TARGET_FILE_DIR:r1q2>/fmt.dll
      COMMENT "Copy OpenAL32.dll + fmt.dll beside R1Q2v2.exe"
    )
  endif()
  if(R1Q2_USE_CURL)
    add_custom_command(TARGET r1q2 POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:CURL::libcurl>
        $<TARGET_FILE_DIR:r1q2>
      COMMENT "Copy libcurl beside R1Q2v2.exe"
    )
    # Schannel curl still pulls zlib; copy zlib1.dll if vcpkg installed it.
    if(DEFINED VCPKG_TARGET_TRIPLET)
      set(_r1q2_zlib1 "${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/bin/zlib1.dll")
      if(EXISTS "${_r1q2_zlib1}")
        add_custom_command(TARGET r1q2 POST_BUILD
          COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${_r1q2_zlib1}"
            $<TARGET_FILE_DIR:r1q2>/zlib1.dll
          COMMENT "Copy zlib1.dll (libcurl) beside R1Q2v2.exe"
        )
      endif()
    endif()
  endif()
  set_target_properties(r1q2 PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    OUTPUT_NAME "R1Q2v2"
  )
endif()


# ---------------------------------------------------------------------------
# ref_gl / R1GL renderer DLL (Win32 OpenGL)
# ---------------------------------------------------------------------------
option(R1Q2_BUILD_REF_GL "Build R1 ref_gl renderer (ref_r1gl.dll)" ON)
if(R1Q2_BUILD_REF_GL)
  add_library(ref_r1gl SHARED ${R1Q2_REF_GL_SOURCES} ref_gl/ref_gl.def)
  target_compile_definitions(ref_r1gl PRIVATE REF_GL)
  target_include_directories(ref_r1gl PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/ref_gl
    ${CMAKE_CURRENT_SOURCE_DIR}/client
    ${CMAKE_CURRENT_SOURCE_DIR}/game
    ${CMAKE_CURRENT_SOURCE_DIR}/qcommon
    ${CMAKE_CURRENT_SOURCE_DIR}/win32
    ${ZLIB_INCLUDE_DIRS}
    ${PNG_INCLUDE_DIRS}
    ${JPEG_INCLUDE_DIR}
  )
  target_link_libraries(ref_r1gl PRIVATE
    ZLIB::ZLIB
    PNG::PNG
    JPEG::JPEG
    opengl32
    glu32
    gdi32
    user32
    winmm
  )
  set_target_properties(ref_r1gl PROPERTIES
    OUTPUT_NAME "ref_r1gl"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    PREFIX ""
  )
  # Optional alias copy name used when vid_ref is "gl"
  add_custom_command(TARGET ref_r1gl POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      $<TARGET_FILE:ref_r1gl>
      $<TARGET_FILE_DIR:ref_r1gl>/ref_gl.dll
    COMMENT "Also emit ref_gl.dll alongside ref_r1gl.dll"
  )
endif()

if(R1Q2_BUILD_CLIENT)
  if(R1Q2_BUILD_REF_GL)
    message(STATUS "r1q2 targets: game, r1q2ded, r1q2, ref_r1gl (client ON)")
  else()
    message(STATUS "r1q2 targets: game, r1q2ded, r1q2 (client ON, ref_gl OFF)")
  endif()
else()
  if(R1Q2_BUILD_REF_GL)
    message(STATUS "r1q2 targets: game, r1q2ded, ref_r1gl (client OFF)")
  else()
    message(STATUS "r1q2 targets: game, r1q2ded (client OFF)")
  endif()
endif()
