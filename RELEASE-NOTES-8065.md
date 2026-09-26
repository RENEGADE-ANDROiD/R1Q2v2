# R1Q2v2 Build 8065

Eases Quake II's sodium-yellow baked lighting. The new look is on by default.

Download `R1Q2v2-8065-win32.zip`. Quit Quake II, back up your existing binaries, and extract the archive into your Quake II directory. Launch `R1Q2v2.exe` and check for **Build 8065**. The package includes the client, dedicated server, game DLL, both R1GL renderer filenames, and required audio/network/image runtime DLLs. Your configurations and game data are not replaced. Retail / Steam game data is not included.

## Changes

- **Less harsh yellow lighting** is the first row under Video → Advanced Settings. Default is **yes**.
- `gl_less_yellow 1` lifts sodium-yellow lightmap texels toward warm white. Red, green, blue, and orange accent lights stay. Models and map-lamp glows use the same rule.
- `gl_less_yellow 0` restores the classic yellow cast. The cvar is archived.
- Toggling reuploads baked lightmaps in place. No map reload.
- Lamp textures themselves are unchanged. Only the light they cast is eased.

## Validation

The Windows x86 Release build of the client and `ref_r1gl.dll` completed. `ref_gl.dll` is that same renderer. This session did not play through a map.

Credits: r1ch (original R1Q2), h0s3r (R1Q2v2).
