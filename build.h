#define BUILD "8064"
/* BUILD 8064: keep desktop mouse input out of the game; preserve command
 * angles on zero-time input samples; use elapsed background command time.
 * Fixes client-generated angle reversals seen by legacy Arena bot checks. */
/* BUILD 8063: keep MP visual lean across map ClearState (avoids normalmap FS
 * hitch storms when visual.cfg has FX on); frame-latch R_CvarEff; skip world
 * shader/normalmap probes and MarkLights when lean; playerskin male fallback
 * one Register* per defer tick. No net/prediction/pmove changes. */
/* BUILD 8062: auto client-side MP visual lean (r_mp_visual_lean) — expensive
 * 8061 FX effective-off when CS_MAXCLIENTS > 1 without overwriting archived
 * SP seta; baseq2 SP / disconnected keep full visuals. Clamp bad gl_colorbits
 * / gl_depthbits 32 -> 24 at pixel-format choose. MSAA lean deferred to next
 * vid_restart (documented). No net/prediction/pmove changes. */
/* BUILD 8061: R1GL world-light coronas, LOS-tested shafts, overbright menu;
 * FBO bloom / god rays / soft particles; per-pixel world dlights + optional
 * normal maps; dlight coronas no longer show explosions through walls.
 * New-player Advanced Settings defaults match the 8061 playtest look. */
/* BUILD 8060: load loose config.cfg (Q2config.cfg fallback); unfocused
 * msec/mouse so alt-tab is not flagged as a timing bot; per-hand ch_x/ch_y. */
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
