#define BUILD "8040"
/* BUILD 8040 (local tracking - not a GitHub release ship):
 * Prefetch sexed player sounds (male/female/cyborg pain/death/fall/jump)
 * in CL_RegisterTEntSounds via #players/... paths matching S_RegisterSexedSound.
 * Reduces first-use MP audio hitches. Not a complete hitch-fix release.
 */
