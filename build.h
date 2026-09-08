#define BUILD "8054"
/* BUILD 8054:
 * Vid_NewWindow gets the real window size (no hudscale divide / 0x0 viddef).
 * SpinControl clamps curvalue instead of ERR_DROP after hiding stock GL.
 * BUILD 8053: SP-only TRANS water. BUILD 8052: menu/VID_NewWindow guard.
 */
