#define BUILD "8045"
/* BUILD 8045 (local tracking):
 * Remove translucent turb water entirely: delete gl_transwater cvar and
 * R_IsExactBaseq2 / R_TurbSurfaceAlpha / R_TurbWantsAlpha. Water/slime
 * SURF_WARP always uses classic opaque turb path for all gamedirs
 * (including baseq2). SURF_TRANS33/66 glass unchanged.
 */
