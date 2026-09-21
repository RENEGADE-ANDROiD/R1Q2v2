# R1Q2v2 Build 8063

**Binaries:** Build from this tree (not pushed/released yet). Console should show **R1Q2v2 (Build: 8063)**.

**R1Q2v2** is a modern Windows build of **r1ch's R1Q2**. Classic look, menus, console, movement, prediction, and packet-send timing are preserved.

---

**What's new in Build 8063**

Fixes Arena-style **asset-load freezes** that could follow Build 8062's MP visual lean when `r1q2v2_visual.cfg` still seta expensive FX on.

- **Lean stays on across map changes:** `CL_ClearState` no longer forces `r_water_alpha_ok` to 0. Mid-match map changes used to briefly disable lean before `CS_MAXCLIENTS` arrived, which could sync-probe `_norm` / `_n` / `_bump` images for every visible wall while FX setas were still 1. Disconnect still clears to 0 so SP/menu keep full visuals.
- **Frame-latched lean:** `R_CvarEff` / `R_MPVisualLean` use a once-per-`R_BeginFrame` latch (no mid-draw flip; cheaper hot paths).
- **No normalmap FS while lean:** `R_WorldShaderBegin` returns immediately under lean before any image probes.
- **No wasted MarkLights while lean:** `R_PushDlights` skips when effective `gl_dynamic` is 0.
- **Playerskin male fallback:** missing non-male skin no longer `RegisterModel` + `RegisterSkin` in the same defer tick.

No net, prediction, or pmove feel changes.

**Smoke checklist**

- **Arena MP:** connect, confirm console once prints `r_mp_visual_lean: multiplayer — expensive FX effective-off`. Change map mid-match — should **not** print singleplayer/menu lean clear, and should not hitch-storm on walls.
- **Arena MP:** watch a round with several custom player models joining — defer drip only; no multi-second freezes on skin fallback.
- **SP / baseq2:** disconnect or local maxclients 1 — full bloom/godrays/normals from visual.cfg still apply.
- **Opt-out:** `seta r_mp_visual_lean 0` still keeps full FX in MP.

**Carried forward**

- **8062:** Auto MP visual lean; `gl_colorbits`/`gl_depthbits` 32→24 clamp
- **8061:** Fair-play lighting FX; new-player Advanced Settings defaults
