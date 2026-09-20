/*
Copyright (C) 2026 R1Q2v2

Known-cheat signatures for client join gates and server cvar/command bans.
Not a replacement for r1ch anticheat.dll; these are local filename, module,
cvar, and version checks from public cheat clients (FreakQuake, WH.dll, etc.).
*/

#ifndef _CHEATCHECK_H
#define _CHEATCHECK_H

typedef struct cheatcheck_cvarban_s
{
	const char	*varname;
	const char	*match;
	const char	*message;
} cheatcheck_cvarban_t;

/* True if gl_driver / OpenGL DLL basename is a known wallhack wrapper. */
qboolean CheatCheck_IsBlockedDriver (const char *dllname);

/* Server default bans (cvar existence / bad values / version needles). */
const cheatcheck_cvarban_t *CheatCheck_ServerCvarBan (int index);
const char *CheatCheck_ServerCommandBan (int index);

#ifndef DEDICATED_ONLY
/*
 * Return false to abort a remote multiplayer connect. reason is optional.
 * Loopback / single-player is the caller's job to skip.
 */
qboolean CheatCheck_AllowRemoteConnect (char *reason, int reasonSize);
void CheatCheck_Init (void);
#endif

#endif
