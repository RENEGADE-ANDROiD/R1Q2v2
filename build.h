#define BUILD "8059"
/* BUILD 8059: defensive cold-path validation for network strings, files,
 * cinematics, demos, clipboard and renderer diagnostics; correct the stock
 * top-HUD armor/ammo grouping without changing multiplayer hot paths. */
/* BUILD 8058: keep Arena team/enemy classification stable through round
 * countdown skin flashes; defer initial player assets without blocking net;
 * player-only enemy brightmaps through 100%; classic packet cadence default. */
/* BUILD 8057: validate incoming packets, downloads, JPEG/WAV/MD3 assets;
 * version the extended renderer interface. Input/send timing is unchanged. */
/* BUILD 8056:
 * Stock-only HUD top/scale/wide; custom CS_STATUSBAR as original R1Q2.
 * MP force colors (r2red/r2blue), tint opacity, enemy brightmaps (33.3/66.6/88).
 * Team countdown: unknown skins are not painted as enemies.
 * RGB PNG player skins (Arena 568x390) no longer crash in GL_MipMap.
 * BUILD 8055: Video spinner is R1GL only; vid_ref soft/gl/ncgl remap to r1gl.
 * BUILD 8054: real window size to Vid_NewWindow; spincontrol clamp.
 */
