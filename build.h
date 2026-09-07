#define BUILD "8046"
/* BUILD 8046 (local tracking):
 * Opaque turb for real: baseq2 water WALs often carry SURF_WARP|SURF_TRANS33
 * (water4/8, bluwter, etc.). Do not put WARP/DRAWTURB on the alpha chain --
 * draw classic opaque modulate with depth write and no blend. Glass
 * TRANS33/66 without WARP stays translucent.
 */
