#define BUILD "8047"
/* BUILD 8047:
 * Alt-tab / multi-monitor focus: reacquire mouse on WA_ACTIVE, drop one
 * mouse sample, 16ms unfocused yield instead of 100ms, cap unfocused
 * packet_delta so cmd.msec is not clamp-to-100. Not Q2PRO pmove.
 */
