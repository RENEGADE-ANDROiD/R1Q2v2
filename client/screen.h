/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// screen.h

void	SCR_Init (void);

void	SCR_UpdateScreen (void);
void	SCR_AddChatMessage (const char *chat);

void	SCR_SizeUp (void);
void	SCR_SizeDown (void);
void	SCR_CenterPrint (char *str);
void	SCR_BeginLoadingPlaque (void);
void	SCR_EndLoadingPlaque (void);

void	EXPORT SCR_DebugGraph (float value, int color);

void	SCR_TouchPics (void);

void	SCR_RunConsole (void);

float	SCR_GetMenuScale (void);
float	SCR_GetMenuPicScale (void);
float	SCR_GetConsoleScale (void);
float	SCR_GetLayoutScale (void);
extern	cvar_t	*scr_menuscale;
extern	cvar_t	*scr_layoutscale;
extern	cvar_t	*scr_menupicscale;
extern	cvar_t	*con_scale;

/* Absolute mouse in client coords while menu/console has the cursor */
extern	int		menu_mouse_x, menu_mouse_y;
extern	qboolean menu_mouse_valid;

extern	float		scr_con_current;
extern	float		scr_conlines;		// lines of console to display

extern	int			sb_lines;

extern	cvar_t		*scr_viewsize;
extern	cvar_t		*crosshair;
extern	cvar_t		*ch1;
extern	cvar_t		*ch2;
extern	cvar_t		*ch3;
extern	cvar_t		*ch_scale;
extern	cvar_t		*ch_x;
extern	cvar_t		*ch_y;

extern	vrect_t		scr_vrect;		// position of render window

extern	char		crosshair_pic[MAX_QPATH];
extern	int			crosshair_width, crosshair_height;
#define	MAX_CH_LAYERS	3
extern	char		ch_layer_pic[MAX_CH_LAYERS][MAX_QPATH];
extern	int			ch_layer_width[MAX_CH_LAYERS];
extern	int			ch_layer_height[MAX_CH_LAYERS];

void SCR_AddDirtyPoint (int x, int y);
void SCR_DirtyScreen (void);

//
// scr_cin.c
//

#ifdef CINEMATICS
void SCR_PlayCinematic (char *name);
qboolean SCR_DrawCinematic (void);
void SCR_RunCinematic (void);
void SCR_StopCinematic (void);
void SCR_FinishCinematic (void);
#endif
