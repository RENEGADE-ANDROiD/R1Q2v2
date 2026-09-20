/*
Copyright (C) 2026 R1Q2v2

Local cheat signatures. Client-side: refuse remote multiplayer if a known
cheat EXE/DLL is sitting next to this process or already loaded. Server-side:
tables used to install default cvarbans / command bans.

Cheat Raper (cr_gl.dll / the CR quake2.exe) is an old anticheat, not a cheat;
it is not on these lists. FreakQuake (frkq2.exe) and OpenGL WH wrappers are.
*/

#include "qcommon.h"
#include "cheatcheck.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>
#else
#include <unistd.h>
#endif

static const char *cheat_files[] = {
	"frkq2.exe",
	"wh.dll",
	"wallhack.dll",
	"q2ace.dll",
	"q2ace.exe",
	NULL
};

static const char *cheat_cvars[] = {
	"frkq2_bot",
	"frkq2_aim",
	"frkq2_aimbot",
	"frkq2_wall",
	"frkq2_wallhack",
	"frkq2_wh",
	"frkq2_esp",
	"frkq2_radar",
	NULL
};

static const char *blocked_drivers[] = {
	"wh.dll",
	"wh",
	"wallhack.dll",
	"q2ace.dll",
	NULL
};

/* Kick if the stuffed cvar exists (*) or has a banned value. */
static const cheatcheck_cvarban_t server_cvarbans[] = {
	{ "frkq2_bot",		"*",		"Cheat clients are not allowed on this server." },
	{ "frkq2_aim",		"*",		"Cheat clients are not allowed on this server." },
	{ "frkq2_aimbot",	"*",		"Cheat clients are not allowed on this server." },
	{ "frkq2_wall",		"*",		"Cheat clients are not allowed on this server." },
	{ "frkq2_wallhack",	"*",		"Cheat clients are not allowed on this server." },
	{ "frkq2_wh",		"*",		"Cheat clients are not allowed on this server." },
	{ "frkq2_esp",		"*",		"Cheat clients are not allowed on this server." },
	{ "frkq2_radar",	"*",		"Cheat clients are not allowed on this server." },
	{ "version",		"~FreakQuake",	"FreakQuake clients are not allowed on this server." },
	{ "version",		"~freakquake",	"FreakQuake clients are not allowed on this server." },
	{ "version",		"~frkq2",	"FreakQuake clients are not allowed on this server." },
	{ "version",		"~FRKQ2",	"FreakQuake clients are not allowed on this server." },
	{ "version",		"~FreakQ2",	"FreakQuake clients are not allowed on this server." },
	{ "gl_driver",		"~wh.dll",	"OpenGL wallhack wrappers are not allowed on this server." },
	{ "gl_driver",		"#wh",		"OpenGL wallhack wrappers are not allowed on this server." },
	{ "gl_driver",		"~wallhack",	"OpenGL wallhack wrappers are not allowed on this server." },
	{ "timescale",		"!=1",		"timescale is cheat protected." },
	{ "timedemo",		"!=0",		"timedemo is cheat protected." },
	{ "r_drawworld",	"!=1",		"r_drawworld is cheat protected." },
	{ "r_fullbright",	"!=0",		"r_fullbright is cheat protected." },
	{ "r_drawflat",		"!=0",		"r_drawflat is cheat protected." },
	{ "gl_lightmap",	"!=0",		"gl_lightmap is cheat protected." },
	{ "gl_lockpvs",		"!=0",		"gl_lockpvs is cheat protected." },
	{ "sw_lockpvs",		"!=0",		"sw_lockpvs is cheat protected." },
	{ "gl_showtris",	"!=0",		"gl_showtris is cheat protected." },
	{ "cl_testlights",	"!=0",		"cl_testlights is cheat protected." },
	{ "fixedtime",		"!=0",		"fixedtime is cheat protected." },
	{ NULL, NULL, NULL }
};

static const char *server_cmdbans[] = {
	"frkq2cmd",
	NULL
};

static void CheatCheck_LowerCopy (char *dst, int dstSize, const char *src)
{
	int	i;

	if (dstSize <= 0)
		return;

	for (i = 0; src[i] && i < dstSize - 1; i++)
		dst[i] = (char)tolower ((unsigned char)src[i]);
	dst[i] = 0;
}

static const char *CheatCheck_BaseName (const char *path)
{
	const char	*base;
	int			i;

	if (!path || !path[0])
		return path;

	base = path;
	for (i = 0; path[i]; i++)
	{
		if (path[i] == '/' || path[i] == '\\')
			base = path + i + 1;
	}
	return base;
}

qboolean CheatCheck_IsBlockedDriver (const char *dllname)
{
	char		lower[MAX_QPATH];
	int			i;

	if (!dllname || !dllname[0])
		return false;

	CheatCheck_LowerCopy (lower, sizeof(lower), CheatCheck_BaseName (dllname));

	for (i = 0; blocked_drivers[i]; i++)
	{
		if (!strcmp (lower, blocked_drivers[i]))
			return true;
	}
	return false;
}

const cheatcheck_cvarban_t *CheatCheck_ServerCvarBan (int index)
{
	if (index < 0 || !server_cvarbans[index].varname)
		return NULL;
	return &server_cvarbans[index];
}

const char *CheatCheck_ServerCommandBan (int index)
{
	if (index < 0 || !server_cmdbans[index])
		return NULL;
	return server_cmdbans[index];
}

#ifndef DEDICATED_ONLY

static cvar_t	*cl_cheatcheck;

static qboolean CheatCheck_GetExeDir (char *out, int outSize)
{
#ifdef _WIN32
	char	*slash;
	DWORD	n;

	n = GetModuleFileNameA (NULL, out, (DWORD)outSize);
	if (!n || n >= (DWORD)outSize)
		return false;
	slash = strrchr (out, '\\');
	if (!slash)
		slash = strrchr (out, '/');
	if (!slash)
		return false;
	slash[0] = 0;
	return true;
#else
	char	*slash;
	ssize_t	n;

	n = readlink ("/proc/self/exe", out, outSize - 1);
	if (n <= 0)
		return false;
	out[n] = 0;
	slash = strrchr (out, '/');
	if (!slash)
		return false;
	slash[0] = 0;
	return true;
#endif
}

static const char *CheatCheck_ScanExeDir (void)
{
	char		dir[260];
	char		path[260];
	int			i;
	FILE		*f;

	if (!CheatCheck_GetExeDir (dir, sizeof(dir)))
		return NULL;

	for (i = 0; cheat_files[i]; i++)
	{
		Com_sprintf (path, sizeof(path), "%s/%s", dir, cheat_files[i]);
		f = fopen (path, "rb");
		if (f)
		{
			fclose (f);
			return cheat_files[i];
		}
	}
	return NULL;
}

static const char *CheatCheck_ScanModules (void)
{
#ifdef _WIN32
	HANDLE			snap;
	MODULEENTRY32	me;
	const char		*base;

	snap = CreateToolhelp32Snapshot (TH32CS_SNAPMODULE, GetCurrentProcessId ());
	if (snap == INVALID_HANDLE_VALUE)
		return NULL;

	me.dwSize = sizeof(me);
	if (Module32First (snap, &me))
	{
		do
		{
			int	i;

			base = CheatCheck_BaseName (me.szModule);
			for (i = 0; cheat_files[i]; i++)
			{
				if (!Q_stricmp (base, cheat_files[i]))
				{
					CloseHandle (snap);
					return cheat_files[i];
				}
			}
		} while (Module32Next (snap, &me));
	}
	CloseHandle (snap);
#else
	FILE	*f;
	char	line[512];
	char	*path;
	const char	*base;
	int		i;

	f = fopen ("/proc/self/maps", "r");
	if (!f)
		return NULL;
	while (fgets (line, sizeof(line), f))
	{
		path = strrchr (line, ' ');
		if (!path)
			continue;
		path++;
		/* strip newline */
		{
			char	*nl = strchr (path, '\n');
			if (nl)
				nl[0] = 0;
		}
		base = CheatCheck_BaseName (path);
		for (i = 0; cheat_files[i]; i++)
		{
			if (!Q_stricmp (base, cheat_files[i]))
			{
				fclose (f);
				return cheat_files[i];
			}
		}
	}
	fclose (f);
#endif
	return NULL;
}

static const char *CheatCheck_ScanCvars (void)
{
	cvar_t	*var;
	int		i;

	for (var = cvar_vars; var; var = var->next)
	{
		for (i = 0; cheat_cvars[i]; i++)
		{
			if (!Q_stricmp (var->name, cheat_cvars[i]))
				return cheat_cvars[i];
		}
	}
	return NULL;
}

static const char *CheatCheck_FindViolation (void)
{
	const char	*hit;
	const char	*driver;

	hit = CheatCheck_ScanModules ();
	if (hit)
		return hit;

	hit = CheatCheck_ScanExeDir ();
	if (hit)
		return hit;

	hit = CheatCheck_ScanCvars ();
	if (hit)
		return hit;

	driver = Cvar_VariableString ("gl_driver");
	if (CheatCheck_IsBlockedDriver (driver))
		return driver[0] ? driver : "gl_driver";

	if (Cvar_VariableValue ("vid_localgl") != 0)
		return "local opengl32.dll wrapper";

	return NULL;
}

qboolean CheatCheck_AllowRemoteConnect (char *reason, int reasonSize)
{
	const char	*hit;

	if (!cl_cheatcheck)
		cl_cheatcheck = Cvar_Get ("cl_cheatcheck", "1", 0);

	if (!cl_cheatcheck->intvalue)
		return true;

	hit = CheatCheck_FindViolation ();
	if (!hit)
		return true;

	if (reason && reasonSize > 0)
		Com_sprintf (reason, reasonSize,
			"Multiplayer blocked: detected '%s'. Remove cheat files/wrappers and restart this client.",
			hit);
	return false;
}

static void CheatCheck_f (void)
{
	const char	*hit;

	hit = CheatCheck_FindViolation ();
	if (!hit)
		Com_Printf ("cheatcheck: no known cheat files, modules, or drivers.\n", LOG_GENERAL);
	else
		Com_Printf ("cheatcheck: detected '%s' (remote multiplayer will be refused).\n", LOG_GENERAL, hit);
}

void CheatCheck_Init (void)
{
	cl_cheatcheck = Cvar_Get ("cl_cheatcheck", "1", 0);
	cl_cheatcheck->help = "Refuse remote multiplayer if this client finds known cheat files, loaded cheat modules, or an OpenGL wallhack wrapper. Default 1. Single-player and localhost are not blocked.\n";
	Cmd_AddCommand ("cheatcheck", CheatCheck_f);
}

#endif /* !DEDICATED_ONLY */
