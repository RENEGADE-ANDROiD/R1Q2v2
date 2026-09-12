# Build 8057

This update fixes confirmed issues in incoming packet and download validation, HTTP filelist buffering, JPEG/WAV/MD3 loading, and renderer compatibility checks.

- Reject invalid entity indexes, oversized area visibility data, truncated downloads, and oversized compressed packet payloads before using them.
- Validate download names and decompressed download lengths; read downloads from the active message buffer.
- Bound HTTP filelists to 16 MiB and preserve their decoded write position during progress updates. Normal asset downloads retain their existing size behavior.
- Reject truncated, oversized, and unsupported CMYK/YCCK JPEGs safely. RGB and grayscale JPEGs remain supported.
- Validate WAV chunk bounds, PCM formats, loop metadata, and resampling allocation sizes through both sound backends.
- Validate MD3 offsets and skin counts without overflowing size calculations.
- Require matching renderer API version 8057, preventing older DLLs from interpreting the extended entity structure incorrectly.

Input handling, packet-send timing, movement prediction, and multiplayer setting defaults are unchanged. This work does not establish the cause of the reported PacketFlinger RA2 incident, and a live arena session has not been tested.

## Install the client update

Close the game. Extract `R1Q2v2.exe`, `ref_r1gl.dll`, and `ref_gl.dll` together into your existing R1Q2v2 installation, replacing those three files. Keep the existing runtime dependencies, game data, configurations, and arena files. This is an update package, not a complete installation.

## Validation

The Windows x86 client and renderer build passed. Focused regression harnesses exercise actual source functions with isolated callbacks: 17 WAV checks, 10 HTTP checks, 6 network checks, 7 MD3 checks, and 6 JPEG cases with guarded output allocations. Network decompression is stubbed in its harness; JPEG tests use the real libjpeg library. These checks are not a complete security audit or gameplay integration test.

Run `scripts/test-client-safety.ps1` for WAV, HTTP, packet, and MD3 regressions. `scripts/security-audit-jpeg.ps1` generates the separate JPEG harness in `build/security-audit/jpeg-audit.c`; compile it with the configured x86 MSVC headers and libjpeg library, and run it with `build/bin` on PATH.
