#define BUILD "8043"
/* BUILD 8043 (local tracking):
 * Priority B MP hitch fixes: static MZ weapon/muzzle sfx table
 * (CL_RegisterMuzzleFlashSounds + cached pointers in ParseMuzzleFlash/2);
 * prefetch misc/bigtele.wav; GL_FindImageExt stem negative cache so
 * repeated missing-skin ext walks do not FS-storm.
 */
