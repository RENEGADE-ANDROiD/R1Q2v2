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
#include <ctype.h>
#ifdef _WIN32
#include <io.h>
#endif
#include "client.h"
#include "../client/qmenu.h"

static int	m_main_cursor;

#define NUM_CURSOR_FRAMES 15

static char *menu_in_sound		= "misc/menu1.wav";
static char *menu_move_sound	= "misc/menu2.wav";
static char *menu_out_sound		= "misc/menu3.wav";

void M_Menu_Main_f (void);
static 	void M_Menu_Game_f (void);
static 		void M_Menu_LoadGame_f (void);
static 		void M_Menu_SaveGame_f (void);
static 		void M_Menu_PlayerConfig_f (void);
static 			void M_Menu_DownloadOptions_f (void);
static 		void M_Menu_Credits_f( void );
static 	void M_Menu_Multiplayer_f( void );
static 		void M_Menu_JoinServer_f (void);
static 			void M_Menu_AddressBook_f( void );
static 		void M_Menu_StartServer_f (void);
static 			void M_Menu_DMOptions_f (void);
static 	void M_Menu_Video_f (void);
static 	void M_Menu_Options_f (void);
static 		void M_Menu_R1Q2_f (void);
static 		void M_Menu_Keys_f (void);
static 	void M_Menu_Quit_f (void);

qboolean	m_entersound;		// play after drawing a frame, so caching
								// won't disrupt the sound

void	(*m_drawfunc) (void);
const char *(*m_keyfunc) (int key);

//=============================================================================
/* Support Routines */

#define	MAX_MENU_DEPTH	8

static float ClampCvar( float min, float max, float value )
{
	if ( value < min ) return min;
	if ( value > max ) return max;
	return value;
}

typedef struct
{
	void	(*draw) (void);
	const char *(*key) (int k);
} menulayer_t;

menulayer_t	m_layers[MAX_MENU_DEPTH];
int		m_menudepth;

static void M_Banner( char *name )
{
	int w, h;
	float s = SCR_GetMenuScale();
	float ps = SCR_GetMenuPicScale();

	re.DrawGetPicSize (&w, &h, name );
	re.DrawStretchPic( (int)(viddef.width / 2 - (w * ps) / 2),
		(int)(viddef.height / 2 - 110 * s),
		(int)(w * ps), (int)(h * ps), name );
}

static void M_PushMenu ( void (*draw) (void), const char *(*key) (int k) )
{
	int		i;

	/*if (Cvar_VariableValue ("maxclients") == 1 
		&& Com_ServerState ())
		Cvar_Set ("paused", "1");*/

	// if this menu is already present, drop back to that level
	// to avoid stacking menus by hotkeys
	for (i=0 ; i<m_menudepth ; i++)
		if (m_layers[i].draw == draw &&
			m_layers[i].key == key)
		{
			m_menudepth = i;
		}

	if (i == m_menudepth)
	{
		if (m_menudepth >= MAX_MENU_DEPTH)
			Com_Error (ERR_FATAL, "M_PushMenu: MAX_MENU_DEPTH");
		m_layers[m_menudepth].draw = m_drawfunc;
		m_layers[m_menudepth].key = m_keyfunc;
		m_menudepth++;
	}

	m_drawfunc = draw;
	m_keyfunc = key;

	m_entersound = true;

	cls.key_dest = key_menu;
}

void M_ForceMenuOff (void)
{
	m_drawfunc = 0;
	m_keyfunc = 0;
	cls.key_dest = key_game;
	m_menudepth = 0;
	Key_ClearStates ();
	//Cvar_Set ("paused", "0");
}

void M_PopMenu (void)
{
	S_StartLocalSound( menu_out_sound );
	if (m_menudepth < 1)
		Com_Error (ERR_FATAL, "M_PopMenu: depth < 1");
	m_menudepth--;

	m_drawfunc = m_layers[m_menudepth].draw;
	m_keyfunc = m_layers[m_menudepth].key;

	if (!m_menudepth)
		M_ForceMenuOff ();
}


static const char *Default_MenuKey( menuframework_s *m, int key )
{
	const char *sound = NULL;
	menucommon_s *item;

	if ( m )
	{
		if ( ( item = Menu_ItemAtCursor( m ) ) != 0 )
		{
			if ( item->type == MTYPE_FIELD )
			{
				if ( Field_Key( ( menufield_s * ) item, key ) )
					return NULL;
			}
		}
	}

	switch ( key )
	{
	case K_ESCAPE:
		M_PopMenu();
		return menu_out_sound;
	case K_KP_UPARROW:
	case K_UPARROW:
		if ( m )
		{
			m->cursor--;
			Menu_AdjustCursor( m, -1 );
			sound = menu_move_sound;
		}
		break;
	case K_TAB:
		if ( m )
		{
			m->cursor++;
			Menu_AdjustCursor( m, 1 );
			sound = menu_move_sound;
		}
		break;
	case K_KP_DOWNARROW:
	case K_DOWNARROW:
		if ( m )
		{
			m->cursor++;
			Menu_AdjustCursor( m, 1 );
			sound = menu_move_sound;
		}
		break;
	case K_KP_LEFTARROW:
	case K_LEFTARROW:
		if ( m )
		{
			Menu_SlideItem( m, -1 );
			sound = menu_move_sound;
		}
		break;
	case K_KP_RIGHTARROW:
	case K_RIGHTARROW:
		if ( m )
		{
			Menu_SlideItem( m, 1 );
			sound = menu_move_sound;
		}
		break;

	case K_MOUSE1:
	case K_MOUSE2:
	case K_MOUSE3:
		IN_UpdateMenuMouse();
		/* Only activate if the click actually hit a menu item (avoids
		   double-click on Main->Game immediately firing Easy/StartGame). */
		if ( !m || !Menu_UpdateCursorFromMouse( m ) )
			return NULL;
		item = Menu_ItemAtCursor( m );
		if ( item && ( item->type == MTYPE_SPINCONTROL || item->type == MTYPE_SLIDER ) )
		{
			Menu_SlideItem( m, ( key == K_MOUSE2 ) ? -1 : 1 );
			return menu_move_sound;
		}
		/* fallthrough */
	case K_JOY1:
	case K_JOY2:
	case K_JOY3:
	case K_JOY4:
	case K_AUX1:
	case K_AUX2:
	case K_AUX3:
	case K_AUX4:
	case K_AUX5:
	case K_AUX6:
	case K_AUX7:
	case K_AUX8:
	case K_AUX9:
	case K_AUX10:
	case K_AUX11:
	case K_AUX12:
	case K_AUX13:
	case K_AUX14:
	case K_AUX15:
	case K_AUX16:
	case K_AUX17:
	case K_AUX18:
	case K_AUX19:
	case K_AUX20:
	case K_AUX21:
	case K_AUX22:
	case K_AUX23:
	case K_AUX24:
	case K_AUX25:
	case K_AUX26:
	case K_AUX27:
	case K_AUX28:
	case K_AUX29:
	case K_AUX30:
	case K_AUX31:
	case K_AUX32:
		
	case K_KP_ENTER:
	case K_ENTER:
		if ( m )
			Menu_SelectItem( m );
		sound = menu_move_sound;
		break;
	}

	return sound;
}

//=============================================================================

/*
================
M_DrawCharacter

Draws one solid graphics character
cx and cy are in 320*240 coordinates, and will be centered on
higher res screens.
================
*/
static void M_DrawCharacter (int cx, int cy, int num)
{
	float s = SCR_GetMenuScale();
	re.DrawChar(
		(int)(cx * s) + ((viddef.width - (int)(320 * s)) >> 1),
		(int)(cy * s) + ((viddef.height - (int)(240 * s)) >> 1),
		num);
}

static void M_Print (int cx, int cy, char *str)
{
	while (*str)
	{
		M_DrawCharacter (cx, cy, (*str)+128);
		str++;
		cx += 8; /* virtual 320-space; M_DrawCharacter applies scale */
	}
}

/*static void M_PrintWhite (int cx, int cy, char *str)
{
	while (*str)
	{
		M_DrawCharacter (cx, cy, *str);
		str++;
		cx += 8;
	}
}*/

/*static void M_DrawPic (int x, int y, char *pic)
{
	re.DrawPic (x + ((viddef.width - 320)>>1), y + ((viddef.height - 240)>>1), pic);
}*/


/*
=============
M_DrawCursor

Draws an animating cursor with the point at
x,y.  The pic will extend to the left of x,
and both above and below y.
=============
*/
static void M_DrawCursor( int x, int y, int f )
{
	char	cursorname[80];
	static qboolean cached;

	if ( !cached )
	{
		int i;

		for ( i = 0; i < NUM_CURSOR_FRAMES; i++ )
		{
			Com_sprintf( cursorname, sizeof( cursorname ), "m_cursor%d", i );

			re.RegisterPic( cursorname );
		}
		cached = true;
	}

	Com_sprintf( cursorname, sizeof(cursorname), "m_cursor%d", f );
	{
		int cw, ch;
		float ps = SCR_GetMenuPicScale();
		re.DrawGetPicSize( &cw, &ch, cursorname );
		re.DrawStretchPic( x, y, (int)(cw * ps), (int)(ch * ps), cursorname );
	}
}

static void M_DrawTextBox (int x, int y, int width, int lines)
{
	int		cx, cy;
	int		n;

	// draw left side
	cx = x;
	cy = y;
	M_DrawCharacter (cx, cy, 1);
	for (n = 0; n < lines; n++)
	{
		cy += 8;
		M_DrawCharacter (cx, cy, 4);
	}
	M_DrawCharacter (cx, cy+8, 7);

	// draw middle
	cx += 8;
	while (width > 0)
	{
		cy = y;
		M_DrawCharacter (cx, cy, 2);
		for (n = 0; n < lines; n++)
		{
			cy += 8;
			M_DrawCharacter (cx, cy, 5);
		}
		M_DrawCharacter (cx, cy+8, 8);
		width -= 1;
		cx += 8;
	}

	// draw right side
	cy = y;
	M_DrawCharacter (cx, cy, 3);
	for (n = 0; n < lines; n++)
	{
		cy += 8;
		M_DrawCharacter (cx, cy, 6);
	}
	M_DrawCharacter (cx, cy+8, 9);
}

		
/*
=======================================================================

MAIN MENU

=======================================================================
*/
#define	MAIN_ITEMS	5



static char *m_main_names[] =
{
	"m_main_game",
	"m_main_multiplayer",
	"m_main_options",
	"m_main_video",
	"m_main_quit",
	0
};

static void M_Main_Layout( int *ystart, int *xoffset, int *widest_out )
{
	int i, w, h, widest = -1;
	float s = SCR_GetMenuScale();
	float ps = SCR_GetMenuPicScale();

	for ( i = 0; m_main_names[i] != 0; i++ )
	{
		re.DrawGetPicSize( &w, &h, m_main_names[i] );
		if ( w > widest )
			widest = w;
	}

	*ystart = (int)( viddef.height / 2 - 110 * s );
	*xoffset = (int)( ( viddef.width - widest * ps + 70 * s ) / 2 );
	if ( widest_out )
		*widest_out = widest;
}

static void M_Main_UpdateHoverFromMouse( void )
{
	int i, w, h, ystart, xoffset;
	float s = SCR_GetMenuScale();
	float ps = SCR_GetMenuPicScale();

	if ( !menu_mouse_valid )
		return;

	M_Main_Layout( &ystart, &xoffset, NULL );
	for ( i = 0; m_main_names[i] != 0; i++ )
	{
		int iy = (int)( ystart + i * 40 * s + 13 * s );
		/* Widen X a bit beyond the pic so clicks near the cursor still hit */
		re.DrawGetPicSize( &w, &h, m_main_names[i] );
		if ( menu_mouse_y >= iy && menu_mouse_y < iy + (int)(h * ps)
			&& menu_mouse_x >= xoffset - (int)(8 * s)
			&& menu_mouse_x < xoffset + (int)(w * ps) + (int)(8 * s) )
		{
			m_main_cursor = i;
			break;
		}
	}
}


static void M_Main_Draw (void)
{
	int i;
	int w, h;
	int ystart;
	int	xoffset;
	char litname[80];
	float s = SCR_GetMenuScale();
	float ps = SCR_GetMenuPicScale();

	M_Main_Layout( &ystart, &xoffset, NULL );
	M_Main_UpdateHoverFromMouse();

	for ( i = 0; m_main_names[i] != 0; i++ )
	{
		re.DrawGetPicSize( &w, &h, m_main_names[i] );
		if ( i != m_main_cursor )
			re.DrawStretchPic( xoffset, (int)(ystart + i * 40 * s + 13 * s),
				(int)(w * ps), (int)(h * ps), m_main_names[i] );
	}
	strcpy( litname, m_main_names[m_main_cursor] );
	strcat( litname, "_sel" );
	re.DrawGetPicSize( &w, &h, litname );
	re.DrawStretchPic( xoffset, (int)(ystart + m_main_cursor * 40 * s + 13 * s),
		(int)(w * ps), (int)(h * ps), litname );

	M_DrawCursor( (int)(xoffset - 25 * s), (int)(ystart + m_main_cursor * 40 * s + 11 * s),
		(int)(cls.realtime / 100)%NUM_CURSOR_FRAMES );

	re.DrawGetPicSize( &w, &h, "m_main_plaque" );
	re.DrawStretchPic( (int)(xoffset - 30 * s - w * ps), ystart,
		(int)(w * ps), (int)(h * ps), "m_main_plaque" );

	re.DrawGetPicSize( &w, &h, "m_main_logo" );
	re.DrawStretchPic( (int)(xoffset - 30 * s - w * ps), (int)(ystart + h * ps + 5 * s),
		(int)(w * ps), (int)(h * ps), "m_main_logo" );
}


static const char *M_Main_Key (int key)
{
	const char *sound = menu_move_sound;

	switch (key)
	{
	case K_ESCAPE:
		M_PopMenu ();
		break;

	case K_KP_DOWNARROW:
	case K_DOWNARROW:
		if (++m_main_cursor >= MAIN_ITEMS)
			m_main_cursor = 0;
		return sound;

	case K_KP_UPARROW:
	case K_UPARROW:
		if (--m_main_cursor < 0)
			m_main_cursor = MAIN_ITEMS - 1;
		return sound;

	case K_MOUSE1:
		IN_UpdateMenuMouse();
		M_Main_UpdateHoverFromMouse();
		/* fallthrough */
	case K_KP_ENTER:
	case K_ENTER:
		m_entersound = true;

		switch (m_main_cursor)
		{
		case 0:
			M_Menu_Game_f ();
			break;

		case 1:
			M_Menu_Multiplayer_f();
			break;

		case 2:
			M_Menu_Options_f ();
			break;

		case 3:
			M_Menu_Video_f ();
			break;

		case 4:
			M_Menu_Quit_f ();
			break;
		}
	}

	return NULL;
}


void M_Menu_Main_f (void)
{
	M_PushMenu (M_Main_Draw, M_Main_Key);
}

/*
=======================================================================

MULTIPLAYER MENU

=======================================================================
*/
static menuframework_s	s_multiplayer_menu;
static menuaction_s		s_join_network_server_action;
static menuaction_s		s_start_network_server_action;
static menuaction_s		s_player_setup_action;

static void Multiplayer_MenuDraw (void)
{
	M_Banner( "m_banner_multiplayer" );

	Menu_AdjustCursor( &s_multiplayer_menu, 1 );
	Menu_Draw( &s_multiplayer_menu );
}

static void PlayerSetupFunc( void *unused )
{
	M_Menu_PlayerConfig_f();
}

static void JoinNetworkServerFunc( void *unused )
{
	M_Menu_JoinServer_f();
}

static void StartNetworkServerFunc( void *unused )
{
	M_Menu_StartServer_f ();
}

static void Multiplayer_MenuInit( void )
{
	s_multiplayer_menu.nitems = 0;

	s_join_network_server_action.generic.type	= MTYPE_ACTION;
	s_join_network_server_action.generic.flags  = QMF_LEFT_JUSTIFY;
	s_join_network_server_action.generic.x		= 0;
	s_join_network_server_action.generic.y		= 0;
	s_join_network_server_action.generic.name	= " join network server";
	s_join_network_server_action.generic.callback = JoinNetworkServerFunc;

	s_start_network_server_action.generic.type	= MTYPE_ACTION;
	s_start_network_server_action.generic.flags  = QMF_LEFT_JUSTIFY;
	s_start_network_server_action.generic.x		= 0;
	s_start_network_server_action.generic.y		= 10;
	s_start_network_server_action.generic.name	= " start network server";
	s_start_network_server_action.generic.callback = StartNetworkServerFunc;

	s_player_setup_action.generic.type	= MTYPE_ACTION;
	s_player_setup_action.generic.flags  = QMF_LEFT_JUSTIFY;
	s_player_setup_action.generic.x		= 0;
	s_player_setup_action.generic.y		= 20;
	s_player_setup_action.generic.name	= " player setup";
	s_player_setup_action.generic.callback = PlayerSetupFunc;

	Menu_AddItem( &s_multiplayer_menu, ( void * ) &s_join_network_server_action );
	Menu_AddItem( &s_multiplayer_menu, ( void * ) &s_start_network_server_action );
	Menu_AddItem( &s_multiplayer_menu, ( void * ) &s_player_setup_action );

	Menu_SetStatusBar( &s_multiplayer_menu, NULL );

	Menu_Center( &s_multiplayer_menu );
	/* Left-justified rows: center ~21-char labels under the banner. */
	s_multiplayer_menu.x = (int)(viddef.width * 0.50f)
		- (int)(44 * SCR_GetMenuScale());
}

static const char *Multiplayer_MenuKey( int key )
{
	return Default_MenuKey( &s_multiplayer_menu, key );
}

static void M_Menu_Multiplayer_f( void )
{
	Multiplayer_MenuInit();
	M_PushMenu( Multiplayer_MenuDraw, Multiplayer_MenuKey );
}

/*
=======================================================================

KEYS MENU

=======================================================================
*/
char *bindnames[][2] =
{
{"+attack", 		"attack"},
{"weapnext", 		"next weapon"},
{"weapprev", 		"previous weapon"},
{"+forward", 		"walk forward"},
{"+back", 			"backpedal"},
{"+left", 			"turn left"},
{"+right", 			"turn right"},
{"+speed", 			"run"},
{"+moveleft", 		"step left"},
{"+moveright", 		"step right"},
{"+strafe", 		"sidestep"},
{"+lookup", 		"look up"},
{"+lookdown", 		"look down"},
{"centerview", 		"center view"},
{"+mlook", 			"mouse look"},
{"+klook", 			"keyboard look"},
{"+moveup",			"up / jump"},
{"+movedown",		"down / crouch"},

{"inven",			"inventory"},
{"invuse",			"use item"},
{"invdrop",			"drop item"},
{"invprev",			"prev item"},
{"invnext",			"next item"},

{"cmd help", 		"help computer" }, 
{ 0, 0 }
};

int				keys_cursor;
static int		bind_grab;

static menuframework_s	s_keys_menu;
static menuaction_s		s_keys_attack_action;
static menuaction_s		s_keys_change_weapon_action;
static menuaction_s		s_keys_walk_forward_action;
static menuaction_s		s_keys_backpedal_action;
static menuaction_s		s_keys_turn_left_action;
static menuaction_s		s_keys_turn_right_action;
static menuaction_s		s_keys_run_action;
static menuaction_s		s_keys_step_left_action;
static menuaction_s		s_keys_step_right_action;
static menuaction_s		s_keys_sidestep_action;
static menuaction_s		s_keys_look_up_action;
static menuaction_s		s_keys_look_down_action;
static menuaction_s		s_keys_center_view_action;
static menuaction_s		s_keys_mouse_look_action;
static menuaction_s		s_keys_keyboard_look_action;
static menuaction_s		s_keys_move_up_action;
static menuaction_s		s_keys_move_down_action;
static menuaction_s		s_keys_inventory_action;
static menuaction_s		s_keys_inv_use_action;
static menuaction_s		s_keys_inv_drop_action;
static menuaction_s		s_keys_inv_prev_action;
static menuaction_s		s_keys_inv_next_action;

static menuaction_s		s_keys_help_computer_action;

static void M_UnbindCommand (char *command)
{
	int		j;
	int		l;
	char	*b;

	l = (int)strlen(command);

	for (j=0 ; j<256 ; j++)
	{
		b = keybindings[j];
		if (!b)
			continue;
		if (!strncmp (b, command, l) )
			Key_SetBinding (j, "");
	}
}

static void M_FindKeysForCommand (char *command, int *twokeys)
{
	int		count;
	int		j;
	int		l;
	char	*b;

	twokeys[0] = twokeys[1] = -1;
	l = (int)strlen(command);
	count = 0;

	for (j=0 ; j<256 ; j++)
	{
		b = keybindings[j];
		if (!b)
			continue;
		if (!strncmp (b, command, l) )
		{
			twokeys[count] = j;
			count++;
			if (count == 2)
				break;
		}
	}
}

static void KeyCursorDrawFunc( menuframework_s *menu )
{
	int		x, y;
	float	s = SCR_GetMenuScale();
	menucommon_s *item = (menucommon_s *)Menu_ItemAtCursor( menu );

	x = menu->x;
	if ( item )
		y = menu->y + (int)(item->y * s);
	else
		y = menu->y + (int)(menu->cursor * 10 * s);

	if ( bind_grab )
		re.DrawChar( x, y, '=' );
	else
		re.DrawChar( x, y, 12 + ( ( int ) ( Sys_Milliseconds() / 250 ) & 1 ) );
}

static void DrawKeyBindingFunc( void *self )
{
	int keys[2];
	menuaction_s *a = ( menuaction_s * ) self;
	float s = SCR_GetMenuScale();
	int x, y, lx;

	x = a->generic.parent->x + (int)(16 * s);
	y = a->generic.parent->y + (int)(a->generic.y * s);
	lx = a->generic.parent->x + (int)(-16 * s);

	if ( a->generic.name )
		Menu_DrawStringR2LDark( lx, y, a->generic.name );

	M_FindKeysForCommand( bindnames[a->generic.localdata[0]][0], keys);
		
	if (keys[0] == -1)
	{
		Menu_DrawString( x, y, "???" );
	}
	else
	{
		int w;
		const char *name;

		name = Key_KeynumToString (keys[0]);
		Menu_DrawString( x, y, name );

		w = (int)strlen(name) * (int)(8 * s);

		if (keys[1] != -1)
		{
			Menu_DrawString( x + w + (int)(8 * s), y, "or" );
			Menu_DrawString( x + w + (int)(24 * s), y, Key_KeynumToString (keys[1]) );
		}
	}
}

static void KeyBindingFunc( void *self )
{
	menuaction_s *a = ( menuaction_s * ) self;
	int keys[2];

	M_FindKeysForCommand( bindnames[a->generic.localdata[0]][0], keys );

	if (keys[1] != -1)
		M_UnbindCommand( bindnames[a->generic.localdata[0]][0]);

	bind_grab = true;

	Menu_SetStatusBar( &s_keys_menu, "press a key or button for this action" );
}

static void Keys_MenuInit( void )
{
	int y = 0;
	int i = 0;

	s_keys_menu.x = (int)(viddef.width * 0.50f);
	s_keys_menu.nitems = 0;
	s_keys_menu.cursordraw = KeyCursorDrawFunc;

	s_keys_attack_action.generic.type	= MTYPE_ACTION;
	s_keys_attack_action.generic.flags  = QMF_GRAYED;
	s_keys_attack_action.generic.x		= 0;
	s_keys_attack_action.generic.y		= y;
	s_keys_attack_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_attack_action.generic.localdata[0] = i;
	s_keys_attack_action.generic.name	= bindnames[s_keys_attack_action.generic.localdata[0]][1];

	s_keys_change_weapon_action.generic.type	= MTYPE_ACTION;
	s_keys_change_weapon_action.generic.flags  = QMF_GRAYED;
	s_keys_change_weapon_action.generic.x		= 0;
	s_keys_change_weapon_action.generic.y		= y += 10;
	s_keys_change_weapon_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_change_weapon_action.generic.localdata[0] = ++i;
	s_keys_change_weapon_action.generic.name	= bindnames[s_keys_change_weapon_action.generic.localdata[0]][1];

	s_keys_walk_forward_action.generic.type	= MTYPE_ACTION;
	s_keys_walk_forward_action.generic.flags  = QMF_GRAYED;
	s_keys_walk_forward_action.generic.x		= 0;
	s_keys_walk_forward_action.generic.y		= y += 10;
	s_keys_walk_forward_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_walk_forward_action.generic.localdata[0] = ++i;
	s_keys_walk_forward_action.generic.name	= bindnames[s_keys_walk_forward_action.generic.localdata[0]][1];

	s_keys_backpedal_action.generic.type	= MTYPE_ACTION;
	s_keys_backpedal_action.generic.flags  = QMF_GRAYED;
	s_keys_backpedal_action.generic.x		= 0;
	s_keys_backpedal_action.generic.y		= y += 10;
	s_keys_backpedal_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_backpedal_action.generic.localdata[0] = ++i;
	s_keys_backpedal_action.generic.name	= bindnames[s_keys_backpedal_action.generic.localdata[0]][1];

	s_keys_turn_left_action.generic.type	= MTYPE_ACTION;
	s_keys_turn_left_action.generic.flags  = QMF_GRAYED;
	s_keys_turn_left_action.generic.x		= 0;
	s_keys_turn_left_action.generic.y		= y += 10;
	s_keys_turn_left_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_turn_left_action.generic.localdata[0] = ++i;
	s_keys_turn_left_action.generic.name	= bindnames[s_keys_turn_left_action.generic.localdata[0]][1];

	s_keys_turn_right_action.generic.type	= MTYPE_ACTION;
	s_keys_turn_right_action.generic.flags  = QMF_GRAYED;
	s_keys_turn_right_action.generic.x		= 0;
	s_keys_turn_right_action.generic.y		= y += 10;
	s_keys_turn_right_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_turn_right_action.generic.localdata[0] = ++i;
	s_keys_turn_right_action.generic.name	= bindnames[s_keys_turn_right_action.generic.localdata[0]][1];

	s_keys_run_action.generic.type	= MTYPE_ACTION;
	s_keys_run_action.generic.flags  = QMF_GRAYED;
	s_keys_run_action.generic.x		= 0;
	s_keys_run_action.generic.y		= y += 10;
	s_keys_run_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_run_action.generic.localdata[0] = ++i;
	s_keys_run_action.generic.name	= bindnames[s_keys_run_action.generic.localdata[0]][1];

	s_keys_step_left_action.generic.type	= MTYPE_ACTION;
	s_keys_step_left_action.generic.flags  = QMF_GRAYED;
	s_keys_step_left_action.generic.x		= 0;
	s_keys_step_left_action.generic.y		= y += 10;
	s_keys_step_left_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_step_left_action.generic.localdata[0] = ++i;
	s_keys_step_left_action.generic.name	= bindnames[s_keys_step_left_action.generic.localdata[0]][1];

	s_keys_step_right_action.generic.type	= MTYPE_ACTION;
	s_keys_step_right_action.generic.flags  = QMF_GRAYED;
	s_keys_step_right_action.generic.x		= 0;
	s_keys_step_right_action.generic.y		= y += 10;
	s_keys_step_right_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_step_right_action.generic.localdata[0] = ++i;
	s_keys_step_right_action.generic.name	= bindnames[s_keys_step_right_action.generic.localdata[0]][1];

	s_keys_sidestep_action.generic.type	= MTYPE_ACTION;
	s_keys_sidestep_action.generic.flags  = QMF_GRAYED;
	s_keys_sidestep_action.generic.x		= 0;
	s_keys_sidestep_action.generic.y		= y += 10;
	s_keys_sidestep_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_sidestep_action.generic.localdata[0] = ++i;
	s_keys_sidestep_action.generic.name	= bindnames[s_keys_sidestep_action.generic.localdata[0]][1];

	s_keys_look_up_action.generic.type	= MTYPE_ACTION;
	s_keys_look_up_action.generic.flags  = QMF_GRAYED;
	s_keys_look_up_action.generic.x		= 0;
	s_keys_look_up_action.generic.y		= y += 10;
	s_keys_look_up_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_look_up_action.generic.localdata[0] = ++i;
	s_keys_look_up_action.generic.name	= bindnames[s_keys_look_up_action.generic.localdata[0]][1];

	s_keys_look_down_action.generic.type	= MTYPE_ACTION;
	s_keys_look_down_action.generic.flags  = QMF_GRAYED;
	s_keys_look_down_action.generic.x		= 0;
	s_keys_look_down_action.generic.y		= y += 10;
	s_keys_look_down_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_look_down_action.generic.localdata[0] = ++i;
	s_keys_look_down_action.generic.name	= bindnames[s_keys_look_down_action.generic.localdata[0]][1];

	s_keys_center_view_action.generic.type	= MTYPE_ACTION;
	s_keys_center_view_action.generic.flags  = QMF_GRAYED;
	s_keys_center_view_action.generic.x		= 0;
	s_keys_center_view_action.generic.y		= y += 10;
	s_keys_center_view_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_center_view_action.generic.localdata[0] = ++i;
	s_keys_center_view_action.generic.name	= bindnames[s_keys_center_view_action.generic.localdata[0]][1];

	s_keys_mouse_look_action.generic.type	= MTYPE_ACTION;
	s_keys_mouse_look_action.generic.flags  = QMF_GRAYED;
	s_keys_mouse_look_action.generic.x		= 0;
	s_keys_mouse_look_action.generic.y		= y += 10;
	s_keys_mouse_look_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_mouse_look_action.generic.localdata[0] = ++i;
	s_keys_mouse_look_action.generic.name	= bindnames[s_keys_mouse_look_action.generic.localdata[0]][1];

	s_keys_keyboard_look_action.generic.type	= MTYPE_ACTION;
	s_keys_keyboard_look_action.generic.flags  = QMF_GRAYED;
	s_keys_keyboard_look_action.generic.x		= 0;
	s_keys_keyboard_look_action.generic.y		= y += 10;
	s_keys_keyboard_look_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_keyboard_look_action.generic.localdata[0] = ++i;
	s_keys_keyboard_look_action.generic.name	= bindnames[s_keys_keyboard_look_action.generic.localdata[0]][1];

	s_keys_move_up_action.generic.type	= MTYPE_ACTION;
	s_keys_move_up_action.generic.flags  = QMF_GRAYED;
	s_keys_move_up_action.generic.x		= 0;
	s_keys_move_up_action.generic.y		= y += 10;
	s_keys_move_up_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_move_up_action.generic.localdata[0] = ++i;
	s_keys_move_up_action.generic.name	= bindnames[s_keys_move_up_action.generic.localdata[0]][1];

	s_keys_move_down_action.generic.type	= MTYPE_ACTION;
	s_keys_move_down_action.generic.flags  = QMF_GRAYED;
	s_keys_move_down_action.generic.x		= 0;
	s_keys_move_down_action.generic.y		= y += 10;
	s_keys_move_down_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_move_down_action.generic.localdata[0] = ++i;
	s_keys_move_down_action.generic.name	= bindnames[s_keys_move_down_action.generic.localdata[0]][1];

	s_keys_inventory_action.generic.type	= MTYPE_ACTION;
	s_keys_inventory_action.generic.flags  = QMF_GRAYED;
	s_keys_inventory_action.generic.x		= 0;
	s_keys_inventory_action.generic.y		= y += 10;
	s_keys_inventory_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_inventory_action.generic.localdata[0] = ++i;
	s_keys_inventory_action.generic.name	= bindnames[s_keys_inventory_action.generic.localdata[0]][1];

	s_keys_inv_use_action.generic.type	= MTYPE_ACTION;
	s_keys_inv_use_action.generic.flags  = QMF_GRAYED;
	s_keys_inv_use_action.generic.x		= 0;
	s_keys_inv_use_action.generic.y		= y += 10;
	s_keys_inv_use_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_inv_use_action.generic.localdata[0] = ++i;
	s_keys_inv_use_action.generic.name	= bindnames[s_keys_inv_use_action.generic.localdata[0]][1];

	s_keys_inv_drop_action.generic.type	= MTYPE_ACTION;
	s_keys_inv_drop_action.generic.flags  = QMF_GRAYED;
	s_keys_inv_drop_action.generic.x		= 0;
	s_keys_inv_drop_action.generic.y		= y += 10;
	s_keys_inv_drop_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_inv_drop_action.generic.localdata[0] = ++i;
	s_keys_inv_drop_action.generic.name	= bindnames[s_keys_inv_drop_action.generic.localdata[0]][1];

	s_keys_inv_prev_action.generic.type	= MTYPE_ACTION;
	s_keys_inv_prev_action.generic.flags  = QMF_GRAYED;
	s_keys_inv_prev_action.generic.x		= 0;
	s_keys_inv_prev_action.generic.y		= y += 10;
	s_keys_inv_prev_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_inv_prev_action.generic.localdata[0] = ++i;
	s_keys_inv_prev_action.generic.name	= bindnames[s_keys_inv_prev_action.generic.localdata[0]][1];

	s_keys_inv_next_action.generic.type	= MTYPE_ACTION;
	s_keys_inv_next_action.generic.flags  = QMF_GRAYED;
	s_keys_inv_next_action.generic.x		= 0;
	s_keys_inv_next_action.generic.y		= y += 10;
	s_keys_inv_next_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_inv_next_action.generic.localdata[0] = ++i;
	s_keys_inv_next_action.generic.name	= bindnames[s_keys_inv_next_action.generic.localdata[0]][1];

	s_keys_help_computer_action.generic.type	= MTYPE_ACTION;
	s_keys_help_computer_action.generic.flags  = QMF_GRAYED;
	s_keys_help_computer_action.generic.x		= 0;
	s_keys_help_computer_action.generic.y		= y += 10;
	s_keys_help_computer_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_help_computer_action.generic.localdata[0] = ++i;
	s_keys_help_computer_action.generic.name	= bindnames[s_keys_help_computer_action.generic.localdata[0]][1];

	Menu_AddItem( &s_keys_menu, ( void * ) &s_keys_attack_action );
	Menu_AddItem( &s_keys_menu, ( void * ) &s_keys_change_weapon_action );
	Menu_AddItem( &s_keys_menu, ( void * ) &s_keys_walk_forward_action );
	Menu_AddItem( &s_keys_menu, ( void * ) &s_keys_backpedal_action );
	Menu_AddItem( &s_keys_menu, ( void * ) &s_keys_turn_left_action );
	Menu_AddItem( &s_keys_menu, ( void * ) &s_keys_turn_right_action );
	Menu_AddItem( &s_keys_menu, ( void * ) &s_keys_run_action );
	Menu_AddItem( &s_keys_menu, ( void * ) &s_keys_step_left_action );
	Menu_AddItem( &s_keys_menu, ( void * ) &s_keys_step_right_action );
	Menu_AddItem( &s_keys_menu, ( void * ) &s_keys_sidestep_action );
	Menu_AddItem( &s_keys_menu, ( void * ) &s_keys_look_up_action );
	Menu_AddItem( &s_keys_menu, ( void * ) &s_keys_look_down_action );
	Menu_AddItem( &s_keys_menu, ( void * ) &s_keys_center_view_action );
	Menu_AddItem( &s_keys_menu, ( void * ) &s_keys_mouse_look_action );
	Menu_AddItem( &s_keys_menu, ( void * ) &s_keys_keyboard_look_action );
	Menu_AddItem( &s_keys_menu, ( void * ) &s_keys_move_up_action );
	Menu_AddItem( &s_keys_menu, ( void * ) &s_keys_move_down_action );

	Menu_AddItem( &s_keys_menu, ( void * ) &s_keys_inventory_action );
	Menu_AddItem( &s_keys_menu, ( void * ) &s_keys_inv_use_action );
	Menu_AddItem( &s_keys_menu, ( void * ) &s_keys_inv_drop_action );
	Menu_AddItem( &s_keys_menu, ( void * ) &s_keys_inv_prev_action );
	Menu_AddItem( &s_keys_menu, ( void * ) &s_keys_inv_next_action );

	Menu_AddItem( &s_keys_menu, ( void * ) &s_keys_help_computer_action );
	
	Menu_SetStatusBar( &s_keys_menu, "enter to change, backspace to clear" );
	Menu_Center( &s_keys_menu );
}

static void Keys_MenuDraw (void)
{
	Menu_AdjustCursor( &s_keys_menu, 1 );
	Menu_Draw( &s_keys_menu );
}

static const char *Keys_MenuKey( int key )
{
	menuaction_s *item = ( menuaction_s * ) Menu_ItemAtCursor( &s_keys_menu );

	if ( bind_grab )
	{	
		if ( key != K_ESCAPE && key != '`' )
		{
			char cmd[1024];

			Com_sprintf (cmd, sizeof(cmd), "bind \"%s\" \"%s\"\n", Key_KeynumToString(key), bindnames[item->generic.localdata[0]][0]);
			Cbuf_InsertText (cmd);
		}
		
		Menu_SetStatusBar( &s_keys_menu, "enter to change, backspace to clear" );
		bind_grab = false;
		return menu_out_sound;
	}

	switch ( key )
	{
	case K_KP_ENTER:
	case K_ENTER:
		KeyBindingFunc( item );
		return menu_in_sound;
	case K_BACKSPACE:		// delete bindings
	case K_DEL:				// delete bindings
	case K_KP_DEL:
		M_UnbindCommand( bindnames[item->generic.localdata[0]][0] );
		return menu_out_sound;
	default:
		return Default_MenuKey( &s_keys_menu, key );
	}
}

static void M_Menu_Keys_f (void)
{
	Keys_MenuInit();
	M_PushMenu( Keys_MenuDraw, Keys_MenuKey );
}


/*
=======================================================================

CONTROLS MENU

=======================================================================
*/
//static cvar_t *win_noalttab;
#ifdef JOYSTICK
extern cvar_t *in_joystick;
#endif


static menuframework_s	s_r1q2_options_menu;
static menuseparator_s	s_r1q2_warning;
static menuseparator_s	s_r1q2_warning2;

#ifdef _WIN32
static menulist_s		s_r1q2_dinput;
static menulist_s		s_r1q2_winxp;
#endif

static menulist_s		s_r1q2_defer;
static menulist_s		s_r1q2_q2promove;
static menulist_s		s_r1q2_autorecord;
static menulist_s		s_r1q2_xaniarail;

static menuframework_s	s_options_menu;
static menuaction_s		s_options_r1q2_action;
static menuaction_s		s_options_defaults_action;
static menuaction_s		s_options_customize_options_action;
static menufield_s		s_options_sensitivity_slider;
static menulist_s		s_options_freelook_box;
//static menulist_s		s_options_noalttab_box;
static menulist_s		s_options_alwaysrun_box;
static menulist_s		s_options_invertmouse_box;
static menulist_s		s_options_lookspring_box;
static menulist_s		s_options_lookstrafe_box;
static menulist_s		s_options_crosshair_box;
static menuslider_s		s_options_sfxvolume_slider;
#ifdef JOYSTICK
static menulist_s		s_options_joystick_box;
#endif
static menulist_s		s_options_cdvolume_box;
static menulist_s		s_options_quality_list;
static menulist_s		s_options_compatibility_list;
static menulist_s		s_options_console_action;

static void CrosshairFunc( void *unused )
{
	Cvar_SetValue( "crosshair", (float)s_options_crosshair_box.curvalue );
}

#ifdef JOYSTICK
static void JoystickFunc( void *unused )
{
	Cvar_SetValue( "in_joystick", (float)s_options_joystick_box.curvalue );
}
#endif

static void CustomizeControlsFunc( void *unused )
{
	M_Menu_Keys_f();
}

#ifdef _WIN32
static void DirectInputFunc (void *unused)
{
	Cvar_SetValue ("m_directinput", (float)s_r1q2_dinput.curvalue);
	IN_Restart_f();
}

static void AccelFixFunc (void *unused)
{
	Cvar_SetValue ("m_fixaccel", (float)s_r1q2_winxp.curvalue);
	IN_Restart_f();
}
#endif

static void DeferFunc (void *unused)
{
	Cvar_SetValue ("cl_defermodels", (float)s_r1q2_defer.curvalue);
}

static void Q2ProMoveFunc (void *unused)
{
	/* Enabled = sync physics (classic / Q2Pro-style jump feel).
	   Disabled = stock R1Q2 async net/render split (cl_async 1). */
	Cvar_SetValue ("cl_async", s_r1q2_q2promove.curvalue ? 0.0f : 1.0f);
}

static void AutoFunc (void *unused)
{
	Cvar_SetValue ("cl_autorecord", (float)s_r1q2_autorecord.curvalue);
}

static void RailTrailFunc (void *unused)
{
	Cvar_SetValue ("cl_railtrail", (float)s_r1q2_xaniarail.curvalue);
}


static void R1Q2_MenuInit (void)
{
	static const char *yesno_names[] =
	{
		"disabled",
		"enabled",
		0
	};

	static const char *xanianames[] = 
	{
		"disabled",
		"red",
		"green",
		"blue",
		"yellow",
		"orange",
		0
	};

#ifdef _WIN32
	static const char *dinputnames[] = 
	{
		"disabled",
		"buffered",
		"immediate",
		0
	};
#endif

	/*
	** configure controls menu and menu items
	*/
	s_r1q2_options_menu.x = viddef.width / 2;
	s_r1q2_options_menu.y = viddef.height / 2 - (int)(58 * SCR_GetMenuScale());
	s_r1q2_options_menu.nitems = 0;
	s_r1q2_options_menu.cursor = 0;

	s_r1q2_warning.generic.type = MTYPE_SEPARATOR;
	s_r1q2_warning.generic.name = "WARNING: Settings here will not be saved. Any settings";
	s_r1q2_warning.generic.x    = 160;
	s_r1q2_warning.generic.y	 = 0;

	s_r1q2_warning2.generic.type = MTYPE_SEPARATOR;
	s_r1q2_warning2.generic.name = "that you wish to keep will need adding to a config.";
	s_r1q2_warning2.generic.x    = 160;
	s_r1q2_warning2.generic.y	 = 10;

#ifdef _WIN32
	s_r1q2_dinput.generic.type = MTYPE_SPINCONTROL;
	s_r1q2_dinput.generic.x	= 0;
	s_r1q2_dinput.generic.y	= 30;
	s_r1q2_dinput.generic.name	= "directinput mouse";
	s_r1q2_dinput.generic.callback = DirectInputFunc;
	s_r1q2_dinput.itemnames = dinputnames;
	s_r1q2_dinput.curvalue = (int)ClampCvar (0, 2, Cvar_VariableValue ("m_directinput"));

	s_r1q2_winxp.generic.type = MTYPE_SPINCONTROL;
	s_r1q2_winxp.generic.x	= 0;
	s_r1q2_winxp.generic.y	= 40;
	s_r1q2_winxp.generic.name	= "xp mouse acceleration fix";
	s_r1q2_winxp.generic.callback = AccelFixFunc;
	s_r1q2_winxp.itemnames = yesno_names;
	s_r1q2_winxp.curvalue = (int)ClampCvar (0, 1, Cvar_VariableValue ("m_fixaccel"));
#endif

	s_r1q2_defer.generic.type = MTYPE_SPINCONTROL;
	s_r1q2_defer.generic.x	= 0;
	s_r1q2_defer.generic.y	= 60;
	s_r1q2_defer.generic.name	= "defer model loading";
	s_r1q2_defer.generic.callback = DeferFunc;
	s_r1q2_defer.itemnames = yesno_names;
	s_r1q2_defer.curvalue = (int)ClampCvar (0, 1, Cvar_VariableValue ("cl_defermodels"));

	s_r1q2_q2promove.generic.type = MTYPE_SPINCONTROL;
	s_r1q2_q2promove.generic.x	= 0;
	s_r1q2_q2promove.generic.y	= 70;
	s_r1q2_q2promove.generic.name	= "Q2Pro movement";
	s_r1q2_q2promove.generic.callback = Q2ProMoveFunc;
	s_r1q2_q2promove.generic.statusbar = "sync physics for classic jumps (cl_async 0); off = R1Q2 async";
	s_r1q2_q2promove.itemnames = yesno_names;
	/* cl_async 1 (default) = R1Q2 async; show Q2Pro movement as disabled */
	s_r1q2_q2promove.curvalue = Cvar_VariableValue ("cl_async") ? 0 : 1;

	s_r1q2_autorecord.generic.type = MTYPE_SPINCONTROL;
	s_r1q2_autorecord.generic.x	= 0;
	s_r1q2_autorecord.generic.y	= 80;
	s_r1q2_autorecord.generic.name	= "automatic demo record";
	s_r1q2_autorecord.generic.callback = AutoFunc;
	s_r1q2_autorecord.itemnames = yesno_names;
	s_r1q2_autorecord.curvalue = (int)ClampCvar (0, 1, Cvar_VariableValue ("cl_autorecord"));

	s_r1q2_xaniarail.generic.type = MTYPE_SPINCONTROL;
	s_r1q2_xaniarail.generic.x	= 0;
	s_r1q2_xaniarail.generic.y	= 90;
	s_r1q2_xaniarail.generic.name	= "\"xania\" railgun trail";
	s_r1q2_xaniarail.generic.callback = RailTrailFunc;
	s_r1q2_xaniarail.itemnames = xanianames;
	s_r1q2_xaniarail.curvalue = (int)ClampCvar (0, 5, Cvar_VariableValue ("cl_railtrail"));

/*
	s_options_invertmouse_box.generic.type = MTYPE_SPINCONTROL;
	s_options_invertmouse_box.generic.x	= 0;
	s_options_invertmouse_box.generic.y	= 70;
	s_options_invertmouse_box.generic.name	= "invert mouse";
	s_options_invertmouse_box.generic.callback = InvertMouseFunc;
	s_options_invertmouse_box.itemnames = yesno_names;

	s_options_lookspring_box.generic.type = MTYPE_SPINCONTROL;
	s_options_lookspring_box.generic.x	= 0;
	s_options_lookspring_box.generic.y	= 80;
	s_options_lookspring_box.generic.name	= "lookspring";
	s_options_lookspring_box.generic.callback = LookspringFunc;
	s_options_lookspring_box.itemnames = yesno_names;

	s_options_lookstrafe_box.generic.type = MTYPE_SPINCONTROL;
	s_options_lookstrafe_box.generic.x	= 0;
	s_options_lookstrafe_box.generic.y	= 90;
	s_options_lookstrafe_box.generic.name	= "lookstrafe";
	s_options_lookstrafe_box.generic.callback = LookstrafeFunc;
	s_options_lookstrafe_box.itemnames = yesno_names;

	s_options_freelook_box.generic.type = MTYPE_SPINCONTROL;
	s_options_freelook_box.generic.x	= 0;
	s_options_freelook_box.generic.y	= 100;
	s_options_freelook_box.generic.name	= "free look";
	s_options_freelook_box.generic.callback = FreeLookFunc;
	s_options_freelook_box.itemnames = yesno_names;

	s_options_crosshair_box.generic.type = MTYPE_SPINCONTROL;
	s_options_crosshair_box.generic.x	= 0;
	s_options_crosshair_box.generic.y	= 110;
	s_options_crosshair_box.generic.name	= "crosshair";
	s_options_crosshair_box.generic.callback = CrosshairFunc;
	s_options_crosshair_box.itemnames = crosshair_names;
*/

	Menu_AddItem (&s_r1q2_options_menu, (void *)&s_r1q2_warning);
	Menu_AddItem (&s_r1q2_options_menu, (void *)&s_r1q2_warning2);

#ifdef _WIN32
	Menu_AddItem( &s_r1q2_options_menu, ( void * ) &s_r1q2_dinput );
	Menu_AddItem( &s_r1q2_options_menu, ( void * ) &s_r1q2_winxp );
#endif

	Menu_AddItem( &s_r1q2_options_menu, ( void * ) &s_r1q2_defer );
	Menu_AddItem( &s_r1q2_options_menu, ( void * ) &s_r1q2_q2promove );
	Menu_AddItem( &s_r1q2_options_menu, ( void * ) &s_r1q2_autorecord );
	Menu_AddItem( &s_r1q2_options_menu, ( void * ) &s_r1q2_xaniarail );
}

static void R1Q2_MenuDraw (void)
{
	M_Banner ("m_banner_options");
	Menu_AdjustCursor( &s_r1q2_options_menu, 1 );
	Menu_Draw( &s_r1q2_options_menu );
}

static const char *R1Q2_MenuKey( int key )
{
	return Default_MenuKey( &s_r1q2_options_menu, key );
}

static void M_Menu_R1Q2_f (void)
{
	R1Q2_MenuInit();
	M_PushMenu ( R1Q2_MenuDraw, R1Q2_MenuKey);
}

static void R1Q2OptionsMenu ( void *unused )
{
	M_Menu_R1Q2_f();
}

static void AlwaysRunFunc( void *unused )
{
	Cvar_SetValue( "cl_run", (float)s_options_alwaysrun_box.curvalue );
}

static void FreeLookFunc( void *unused )
{
	Cvar_SetValue( "freelook", (float)s_options_freelook_box.curvalue );
}

static void MouseSpeedFunc( void *unused )
{
	float	value;

	value = (float)atof(s_options_sensitivity_slider.buffer);

	if (value < 2)
		value = 2;

	Cvar_SetValue( "sensitivity", value);
}

/*static void NoAltTabFunc( void *unused )
{
	Cvar_SetValue( "win_noalttab", s_options_noalttab_box.curvalue );
}*/


static void ControlsSetMenuItemValues( void )
{
	s_options_sfxvolume_slider.curvalue		= Cvar_VariableValue( "s_volume" ) * 10;
	s_options_cdvolume_box.curvalue 		= !Cvar_IntValue("cd_nocd");
	//s_options_quality_list.curvalue			= !Cvar_VariableValue( "s_loadas8bit" );
	//strncpy (s_options_sensitivity_slider.buffer, sensitivity->string, sizeof(s_options_sensitivity_slider.buffer)-1);
	snprintf (s_options_sensitivity_slider.buffer, sizeof(s_options_sensitivity_slider.buffer)-1, "%.3g", sensitivity->value);
	//s_options_sensitivity_slider.curvalue	= ( sensitivity->value;

	Cvar_SetValue( "cl_run", ClampCvar( 0, 1, cl_run->value ) );
	s_options_alwaysrun_box.curvalue		= cl_run->intvalue;

	s_options_invertmouse_box.curvalue		= m_pitch->value < 0;

	Cvar_SetValue( "lookspring", ClampCvar( 0, 1, lookspring->value ) );
	s_options_lookspring_box.curvalue		= lookspring->intvalue;

	Cvar_SetValue( "lookstrafe", ClampCvar( 0, 1, lookstrafe->value ) );
	s_options_lookstrafe_box.curvalue		= lookstrafe->intvalue;

	Cvar_SetValue( "freelook", ClampCvar( 0, 1, freelook->value ) );
	s_options_freelook_box.curvalue			= freelook->intvalue;

	Cvar_SetValue( "crosshair", ClampCvar( 0, 3, crosshair->value ) );
	s_options_crosshair_box.curvalue		= crosshair->intvalue;

#ifdef JOYSTICK
	Cvar_SetValue( "in_joystick", ClampCvar( 0, 1, in_joystick->value ) );
	s_options_joystick_box.curvalue		= in_joystick->intvalue;
#endif

	//s_options_noalttab_box.curvalue			= win_noalttab->value;
}

static void ControlsResetDefaultsFunc( void *unused )
{
	Cbuf_AddText ("exec default.cfg\n");
	Cbuf_Execute();

	ControlsSetMenuItemValues();
}

static void InvertMouseFunc( void *unused )
{
	Cvar_SetValue( "m_pitch", -m_pitch->value );
}

static void LookspringFunc( void *unused )
{
	Cvar_SetValue( "lookspring", (float)!lookspring->value );
}

static void LookstrafeFunc( void *unused )
{
	Cvar_SetValue( "lookstrafe", (float)!lookstrafe->value );
}

static void UpdateVolumeFunc( void *unused )
{
	Cvar_SetValue( "s_volume", s_options_sfxvolume_slider.curvalue / 10.0f );
}

static void UpdateCDVolumeFunc( void *unused )
{
	Cvar_SetValue( "cd_nocd", (float)!s_options_cdvolume_box.curvalue );
}

extern void Key_ClearTyping( void );
static void ConsoleFunc( void *unused )
{
	/*
	** the proper way to do this is probably to have ToggleConsole_f accept a parameter
	*/

	if ( cl.attractloop )
	{
		Cbuf_AddText ("killserver\n");
		return;
	}

	Key_ClearTyping ();
	Con_ClearNotify ();

	M_ForceMenuOff ();
	cls.key_dest = key_console;
}

static void UpdateSoundQualityFunc( void *unused )
{
	if ( s_options_quality_list.curvalue == 2)
	{
		Cvar_Set ( "s_khz", "44" );
		Cvar_Set ( "s_loadas8bit", "0" );
	}
	else if ( s_options_quality_list.curvalue == 1)
	{
		Cvar_Set ( "s_khz", "22" );
		Cvar_Set ( "s_loadas8bit", "0" );
	}
	else
	{
		Cvar_Set ( "s_khz", "11" );
		Cvar_Set ( "s_loadas8bit", "1" );
	}
	
	Cvar_SetValue( "s_primary", (float)s_options_compatibility_list.curvalue );

	M_DrawTextBox( 8, 120 - 48, 36, 3 );
	M_Print( 16 + 16, 120 - 48 + 8,  "Restarting the sound system. This" );
	M_Print( 16 + 16, 120 - 48 + 16, "could take up to a minute, so" );
	M_Print( 16 + 16, 120 - 48 + 24, "please be patient." );

	// the text box won't show up unless we do a buffer swap
	re.EndFrame();

	CL_Snd_Restart_f();
}

static void Options_MenuInit( void )
{
	int squality;

	static const char *cd_music_items[] =
	{
		"disabled",
		"enabled",
		0
	};
	static const char *quality_items[] =
	{
		"11 KHz", "22 KHz", "44 KHz", 0
	};

	static const char *compatibility_items[] =
	{
		"normal writing", "direct writing", 0
	};

	static const char *yesno_names[] =
	{
		"no",
		"yes",
		0
	};

	static const char *crosshair_names[] =
	{
		"none",
		"cross",
		"dot",
		"angle",
		0
	};

	//win_noalttab = Cvar_Get( "win_noalttab", "0", CVAR_ARCHIVE );

	squality = Cvar_IntValue ("s_khz");

	switch (squality)	{
		case 11:
			squality = 0;
			break;
		case 22:
			squality = 1;
			break;
		case 44:
			squality = 2;
			break;
		default:
			squality = 1;
			break;
	}

	/*
	** configure controls menu and menu items
	*/
	s_options_menu.x = viddef.width / 2;
	s_options_menu.y = viddef.height / 2 - 58;
	s_options_menu.nitems = 0;

	s_options_sfxvolume_slider.generic.type	= MTYPE_SLIDER;
	s_options_sfxvolume_slider.generic.x	= 0;
	s_options_sfxvolume_slider.generic.y	= 0;
	s_options_sfxvolume_slider.generic.name	= "effects volume";
	s_options_sfxvolume_slider.generic.callback	= UpdateVolumeFunc;
	s_options_sfxvolume_slider.minvalue		= 0;
	s_options_sfxvolume_slider.maxvalue		= 10;
	s_options_sfxvolume_slider.curvalue		= Cvar_VariableValue( "s_volume" ) * 10;

	s_options_cdvolume_box.generic.type	= MTYPE_SPINCONTROL;
	s_options_cdvolume_box.generic.x		= 0;
	s_options_cdvolume_box.generic.y		= 10;
	s_options_cdvolume_box.generic.name	= "CD music";
	s_options_cdvolume_box.generic.callback	= UpdateCDVolumeFunc;
	s_options_cdvolume_box.itemnames		= cd_music_items;
	s_options_cdvolume_box.curvalue 		= !Cvar_IntValue("cd_nocd");

	s_options_quality_list.generic.type	= MTYPE_SPINCONTROL;
	s_options_quality_list.generic.x		= 0;
	s_options_quality_list.generic.y		= 20;
	s_options_quality_list.generic.name		= "sound quality";
	s_options_quality_list.generic.callback = UpdateSoundQualityFunc;
	s_options_quality_list.itemnames		= quality_items;
	s_options_quality_list.curvalue			= squality;

	s_options_compatibility_list.generic.type	= MTYPE_SPINCONTROL;
	s_options_compatibility_list.generic.x		= 0;
	s_options_compatibility_list.generic.y		= 30;
	s_options_compatibility_list.generic.name	= "sound compatibility";
	s_options_compatibility_list.generic.callback = UpdateSoundQualityFunc;
	s_options_compatibility_list.itemnames		= compatibility_items;
	s_options_compatibility_list.curvalue		= Cvar_IntValue( "s_primary" );

	s_options_sensitivity_slider.generic.type	= MTYPE_FIELD;
	s_options_sensitivity_slider.generic.flags = QMF_NUMBERSONLY;
	s_options_sensitivity_slider.length = 3;
	s_options_sensitivity_slider.visible_length = 3;
	s_options_sensitivity_slider.generic.x		= 0;
	s_options_sensitivity_slider.generic.y		= 50;
	s_options_sensitivity_slider.generic.name	= "mouse speed";
	//s_options_sensitivity_slider.generic.callback = MouseSpeedFunc;
	//s_options_sensitivity_slider.minvalue		= 2;
	//s_options_sensitivity_slider.maxvalue		= 22;

	s_options_alwaysrun_box.generic.type = MTYPE_SPINCONTROL;
	s_options_alwaysrun_box.generic.x	= 0;
	s_options_alwaysrun_box.generic.y	= 60;
	s_options_alwaysrun_box.generic.name	= "always run";
	s_options_alwaysrun_box.generic.callback = AlwaysRunFunc;
	s_options_alwaysrun_box.itemnames = yesno_names;

	s_options_invertmouse_box.generic.type = MTYPE_SPINCONTROL;
	s_options_invertmouse_box.generic.x	= 0;
	s_options_invertmouse_box.generic.y	= 70;
	s_options_invertmouse_box.generic.name	= "invert mouse";
	s_options_invertmouse_box.generic.callback = InvertMouseFunc;
	s_options_invertmouse_box.itemnames = yesno_names;

	s_options_lookspring_box.generic.type = MTYPE_SPINCONTROL;
	s_options_lookspring_box.generic.x	= 0;
	s_options_lookspring_box.generic.y	= 80;
	s_options_lookspring_box.generic.name	= "lookspring";
	s_options_lookspring_box.generic.callback = LookspringFunc;
	s_options_lookspring_box.itemnames = yesno_names;

	s_options_lookstrafe_box.generic.type = MTYPE_SPINCONTROL;
	s_options_lookstrafe_box.generic.x	= 0;
	s_options_lookstrafe_box.generic.y	= 90;
	s_options_lookstrafe_box.generic.name	= "lookstrafe";
	s_options_lookstrafe_box.generic.callback = LookstrafeFunc;
	s_options_lookstrafe_box.itemnames = yesno_names;

	s_options_freelook_box.generic.type = MTYPE_SPINCONTROL;
	s_options_freelook_box.generic.x	= 0;
	s_options_freelook_box.generic.y	= 100;
	s_options_freelook_box.generic.name	= "free look";
	s_options_freelook_box.generic.callback = FreeLookFunc;
	s_options_freelook_box.itemnames = yesno_names;

	s_options_crosshair_box.generic.type = MTYPE_SPINCONTROL;
	s_options_crosshair_box.generic.x	= 0;
	s_options_crosshair_box.generic.y	= 110;
	s_options_crosshair_box.generic.name	= "crosshair";
	s_options_crosshair_box.generic.callback = CrosshairFunc;
	s_options_crosshair_box.itemnames = crosshair_names;
/*
	s_options_noalttab_box.generic.type = MTYPE_SPINCONTROL;
	s_options_noalttab_box.generic.x	= 0;
	s_options_noalttab_box.generic.y	= 110;
	s_options_noalttab_box.generic.name	= "disable alt-tab";
	s_options_noalttab_box.generic.callback = NoAltTabFunc;
	s_options_noalttab_box.itemnames = yesno_names;
*/
#ifdef JOYSTICK
	s_options_joystick_box.generic.type = MTYPE_SPINCONTROL;
	s_options_joystick_box.generic.x	= 0;
	s_options_joystick_box.generic.y	= 120;
	s_options_joystick_box.generic.name	= "use joystick";
	s_options_joystick_box.generic.callback = JoystickFunc;
	s_options_joystick_box.itemnames = yesno_names;
#endif

	s_options_r1q2_action.generic.type = MTYPE_ACTION;
	s_options_r1q2_action.generic.x		= 0;
	s_options_r1q2_action.generic.y		= 140;
	s_options_r1q2_action.generic.name	= PRODUCTNAMELOWER " options";
	s_options_r1q2_action.generic.callback = R1Q2OptionsMenu;

	s_options_customize_options_action.generic.type	= MTYPE_ACTION;
	s_options_customize_options_action.generic.x		= 0;
	s_options_customize_options_action.generic.y		= 160;
	s_options_customize_options_action.generic.name	= "customize controls";
	s_options_customize_options_action.generic.callback = CustomizeControlsFunc;

	s_options_defaults_action.generic.type	= MTYPE_ACTION;
	s_options_defaults_action.generic.x		= 0;
	s_options_defaults_action.generic.y		= 170;
	s_options_defaults_action.generic.name	= "reset defaults";
	s_options_defaults_action.generic.callback = ControlsResetDefaultsFunc;

	s_options_console_action.generic.type	= MTYPE_ACTION;
	s_options_console_action.generic.x		= 0;
	s_options_console_action.generic.y		= 180;
	s_options_console_action.generic.name	= "go to console";
	s_options_console_action.generic.callback = ConsoleFunc;

	ControlsSetMenuItemValues();

	Menu_AddItem( &s_options_menu, ( void * ) &s_options_sfxvolume_slider );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_cdvolume_box );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_quality_list );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_compatibility_list );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_sensitivity_slider );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_alwaysrun_box );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_invertmouse_box );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_lookspring_box );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_lookstrafe_box );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_freelook_box );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_crosshair_box );
#ifdef JOYSTICK
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_joystick_box );
#endif
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_r1q2_action );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_customize_options_action );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_defaults_action );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_console_action );
}

static void Options_MenuDraw (void)
{
	M_Banner( "m_banner_options" );
	Menu_AdjustCursor( &s_options_menu, 1 );
	Menu_Draw( &s_options_menu );
}

static const char *Options_MenuKey( int key )
{
	if (key == K_ESCAPE)
		MouseSpeedFunc (NULL);

	return Default_MenuKey( &s_options_menu, key );
}

static void M_Menu_Options_f (void)
{
	Options_MenuInit();
	M_PushMenu ( Options_MenuDraw, Options_MenuKey );
}

/*
=======================================================================

VIDEO MENU

=======================================================================
*/

static void M_Menu_Video_f (void)
{
	VID_MenuInit();
	M_PushMenu( VID_MenuDraw, VID_MenuKey );
}

/*
=============================================================================

END GAME MENU

=============================================================================
*/
static int credits_start_time;
static const char **credits;
static char *creditsIndex[256];
static char *creditsBuffer;
static const char *idcredits[] =
{
	"+QUAKE II BY ID SOFTWARE",
	"",
	"+PROGRAMMING",
	"John Carmack",
	"John Cash",
	"Brian Hook",
	"",
	"+ART",
	"Adrian Carmack",
	"Kevin Cloud",
	"Paul Steed",
	"",
	"+LEVEL DESIGN",
	"Tim Willits",
	"American McGee",
	"Christian Antkow",
	"Paul Jaquays",
	"Brandon James",
	"",
	"+BIZ",
	"Todd Hollenshead",
	"Barrett (Bear) Alexander",
	"Donna Jackson",
	"",
	"",
	"+SPECIAL THANKS",
	"Ben Donges for beta testing",
	"",
	"",
	"",
	"",
	"",
	"",
	"+ADDITIONAL SUPPORT",
	"",
	"+LINUX PORT AND CTF",
	"Dave \"Zoid\" Kirsch",
	"",
	"+CINEMATIC SEQUENCES",
	"Ending Cinematic by Blur Studio - ",
	"Venice, CA",
	"",
	"Environment models for Introduction",
	"Cinematic by Karl Dolgener",
	"",
	"Assistance with environment design",
	"by Cliff Iwai",
	"",
	"+SOUND EFFECTS AND MUSIC",
	"Sound Design by Soundelux Media Labs.",
	"Music Composed and Produced by",
	"Soundelux Media Labs.  Special thanks",
	"to Bill Brown, Tom Ozanich, Brian",
	"Celano, Jeff Eisner, and The Soundelux",
	"Players.",
	"",
	"\"Level Music\" by Sonic Mayhem",
	"www.sonicmayhem.com",
	"",
	"\"Quake II Theme Song\"",
	"(C) 1997 Rob Zombie. All Rights",
	"Reserved.",
	"",
	"Track 10 (\"Climb\") by Jer Sypult",
	"",
	"Voice of computers by",
	"Carly Staehlin-Taylor",
	"",
	"+THANKS TO ACTIVISION",
	"+IN PARTICULAR:",
	"",
	"John Tam",
	"Steve Rosenthal",
	"Marty Stratton",
	"Henk Hartong",
	"",
	"Quake II(tm) (C)1997 Id Software, Inc.",
	"All Rights Reserved.  Distributed by",
	"Activision, Inc. under license.",
	"Quake II(tm), the Id Software name,",
	"the \"Q II\"(tm) logo and id(tm)",
	"logo are trademarks of Id Software,",
	"Inc. Activision(R) is a registered",
	"trademark of Activision, Inc. All",
	"other trademarks and trade names are",
	"properties of their respective owners.",
	0
};

static const char *xatcredits[] =
{
	"+QUAKE II MISSION PACK: THE RECKONING",
	"+BY",
	"+XATRIX ENTERTAINMENT, INC.",
	"",
	"+DESIGN AND DIRECTION",
	"Drew Markham",
	"",
	"+PRODUCED BY",
	"Greg Goodrich",
	"",
	"+PROGRAMMING",
	"Rafael Paiz",
	"",
	"+LEVEL DESIGN / ADDITIONAL GAME DESIGN",
	"Alex Mayberry",
	"",
	"+LEVEL DESIGN",
	"Mal Blackwell",
	"Dan Koppel",
	"",
	"+ART DIRECTION",
	"Michael \"Maxx\" Kaufman",
	"",
	"+COMPUTER GRAPHICS SUPERVISOR AND",
	"+CHARACTER ANIMATION DIRECTION",
	"Barry Dempsey",
	"",
	"+SENIOR ANIMATOR AND MODELER",
	"Jason Hoover",
	"",
	"+CHARACTER ANIMATION AND",
	"+MOTION CAPTURE SPECIALIST",
	"Amit Doron",
	"",
	"+ART",
	"Claire Praderie-Markham",
	"Viktor Antonov",
	"Corky Lehmkuhl",
	"",
	"+INTRODUCTION ANIMATION",
	"Dominique Drozdz",
	"",
	"+ADDITIONAL LEVEL DESIGN",
	"Aaron Barber",
	"Rhett Baldwin",
	"",
	"+3D CHARACTER ANIMATION TOOLS",
	"Gerry Tyra, SA Technology",
	"",
	"+ADDITIONAL EDITOR TOOL PROGRAMMING",
	"Robert Duffy",
	"",
	"+ADDITIONAL PROGRAMMING",
	"Ryan Feltrin",
	"",
	"+PRODUCTION COORDINATOR",
	"Victoria Sylvester",
	"",
	"+SOUND DESIGN",
	"Gary Bradfield",
	"",
	"+MUSIC BY",
	"Sonic Mayhem",
	"",
	"",
	"",
	"+SPECIAL THANKS",
	"+TO",
	"+OUR FRIENDS AT ID SOFTWARE",
	"",
	"John Carmack",
	"John Cash",
	"Brian Hook",
	"Adrian Carmack",
	"Kevin Cloud",
	"Paul Steed",
	"Tim Willits",
	"Christian Antkow",
	"Paul Jaquays",
	"Brandon James",
	"Todd Hollenshead",
	"Barrett (Bear) Alexander",
	"Dave \"Zoid\" Kirsch",
	"Donna Jackson",
	"",
	"",
	"",
	"+THANKS TO ACTIVISION",
	"+IN PARTICULAR:",
	"",
	"Marty Stratton",
	"Henk \"The Original Ripper\" Hartong",
	"Kevin Kraff",
	"Jamey Gottlieb",
	"Chris Hepburn",
	"",
	"+AND THE GAME TESTERS",
	"",
	"Tim Vanlaw",
	"Doug Jacobs",
	"Steven Rosenthal",
	"David Baker",
	"Chris Campbell",
	"Aaron Casillas",
	"Steve Elwell",
	"Derek Johnstone",
	"Igor Krinitskiy",
	"Samantha Lee",
	"Michael Spann",
	"Chris Toft",
	"Juan Valdes",
	"",
	"+THANKS TO INTERGRAPH COMPUTER SYTEMS",
	"+IN PARTICULAR:",
	"",
	"Michael T. Nicolaou",
	"",
	"",
	"Quake II Mission Pack: The Reckoning",
	"(tm) (C)1998 Id Software, Inc. All",
	"Rights Reserved. Developed by Xatrix",
	"Entertainment, Inc. for Id Software,",
	"Inc. Distributed by Activision Inc.",
	"under license. Quake(R) is a",
	"registered trademark of Id Software,",
	"Inc. Quake II Mission Pack: The",
	"Reckoning(tm), Quake II(tm), the Id",
	"Software name, the \"Q II\"(tm) logo",
	"and id(tm) logo are trademarks of Id",
	"Software, Inc. Activision(R) is a",
	"registered trademark of Activision,",
	"Inc. Xatrix(R) is a registered",
	"trademark of Xatrix Entertainment,",
	"Inc. All other trademarks and trade",
	"names are properties of their",
	"respective owners.",
	0
};

static const char *roguecredits[] =
{
	"+QUAKE II MISSION PACK 2: GROUND ZERO",
	"+BY",
	"+ROGUE ENTERTAINMENT, INC.",
	"",
	"+PRODUCED BY",
	"Jim Molinets",
	"",
	"+PROGRAMMING",
	"Peter Mack",
	"Patrick Magruder",
	"",
	"+LEVEL DESIGN",
	"Jim Molinets",
	"Cameron Lamprecht",
	"Berenger Fish",
	"Robert Selitto",
	"Steve Tietze",
	"Steve Thoms",
	"",
	"+ART DIRECTION",
	"Rich Fleider",
	"",
	"+ART",
	"Rich Fleider",
	"Steve Maines",
	"Won Choi",
	"",
	"+ANIMATION SEQUENCES",
	"Creat Studios",
	"Steve Maines",
	"",
	"+ADDITIONAL LEVEL DESIGN",
	"Rich Fleider",
	"Steve Maines",
	"Peter Mack",
	"",
	"+SOUND",
	"James Grunke",
	"",
	"+GROUND ZERO THEME",
	"+AND",
	"+MUSIC BY",
	"Sonic Mayhem",
	"",
	"+VWEP MODELS",
	"Brent \"Hentai\" Dill",
	"",
	"",
	"",
	"+SPECIAL THANKS",
	"+TO",
	"+OUR FRIENDS AT ID SOFTWARE",
	"",
	"John Carmack",
	"John Cash",
	"Brian Hook",
	"Adrian Carmack",
	"Kevin Cloud",
	"Paul Steed",
	"Tim Willits",
	"Christian Antkow",
	"Paul Jaquays",
	"Brandon James",
	"Todd Hollenshead",
	"Barrett (Bear) Alexander",
	"Katherine Anna Kang",
	"Donna Jackson",
	"Dave \"Zoid\" Kirsch",
	"",
	"",
	"",
	"+THANKS TO ACTIVISION",
	"+IN PARTICULAR:",
	"",
	"Marty Stratton",
	"Henk Hartong",
	"Mitch Lasky",
	"Steve Rosenthal",
	"Steve Elwell",
	"",
	"+AND THE GAME TESTERS",
	"",
	"The Ranger Clan",
	"Dave \"Zoid\" Kirsch",
	"Nihilistic Software",
	"Robert Duffy",
	"",
	"And Countless Others",
	"",
	"",
	"",
	"Quake II Mission Pack 2: Ground Zero",
	"(tm) (C)1998 Id Software, Inc. All",
	"Rights Reserved. Developed by Rogue",
	"Entertainment, Inc. for Id Software,",
	"Inc. Distributed by Activision Inc.",
	"under license. Quake(R) is a",
	"registered trademark of Id Software,",
	"Inc. Quake II Mission Pack 2: Ground",
	"Zero(tm), Quake II(tm), the Id",
	"Software name, the \"Q II\"(tm) logo",
	"and id(tm) logo are trademarks of Id",
	"Software, Inc. Activision(R) is a",
	"registered trademark of Activision,",
	"Inc. Rogue(R) is a registered",
	"trademark of Rogue Entertainment,",
	"Inc. All other trademarks and trade",
	"names are properties of their",
	"respective owners.",
	0
};


static void M_Credits_MenuDraw( void )
{
	int i, y;

	/*
	** draw the credits
	*/
	for ( i = 0, y = (int)(viddef.height - ( ( cls.realtime - credits_start_time ) / 40.0F )); credits[i] && y < viddef.height; y += 10, i++ )
	{
		int j, stringoffset = 0;
		int bold = false;

		if ( y <= -8 )
			continue;

		if ( credits[i][0] == '+' )
		{
			bold = true;
			stringoffset = 1;
		}
		else
		{
			bold = false;
			stringoffset = 0;
		}

		for ( j = 0; credits[i][j+stringoffset]; j++ )
		{
			int x;

			x = ( viddef.width - (int)strlen( credits[i] ) * 8 - stringoffset * 8 ) / 2 + ( j + stringoffset ) * 8;

			if ( bold )
				re.DrawChar( x, y, credits[i][j+stringoffset] + 128 );
			else
				re.DrawChar( x, y, credits[i][j+stringoffset] );
		}
	}

	if ( y < 0 )
		credits_start_time = cls.realtime;
}

static const char *M_Credits_Key( int key )
{
	switch (key)
	{
	case K_ESCAPE:
		if (creditsBuffer)
			FS_FreeFile (creditsBuffer);
		M_PopMenu ();
		break;
	}

	return menu_out_sound;

}

extern int Developer_searchpath (void);

static void M_Menu_Credits_f( void )
{
	int		n;
	int		count;
	char	*p;
	int		isdeveloper = 0;

	creditsBuffer = NULL;
	count = FS_LoadFile ("credits", (void **)&creditsBuffer);
	if (count != -1)
	{
		p = creditsBuffer;
		for (n = 0; n < 255; n++)
		{
			creditsIndex[n] = p;
			while (*p != '\r' && *p != '\n')
			{
				p++;
				if (--count == 0)
					break;
			}
			if (*p == '\r')
			{
				*p++ = 0;
				if (--count == 0)
					break;
			}
			*p++ = 0;
			if (--count == 0)
				break;
		}
		creditsIndex[++n] = 0;
		credits = (const char **)creditsIndex;
	}
	else
	{
		isdeveloper = Developer_searchpath ();
		
		if (isdeveloper == 1)			// xatrix
			credits = xatcredits;
		else if (isdeveloper == 2)		// ROGUE
			credits = roguecredits;
		else
		{
			credits = idcredits;	
		}

	}

	credits_start_time = cls.realtime;
	M_PushMenu( M_Credits_MenuDraw, M_Credits_Key);
}

/*
=============================================================================

GAME MENU

=============================================================================
*/

//static int		m_game_cursor;

static menuframework_s	s_game_menu;
static menuaction_s		s_easy_game_action;
static menuaction_s		s_medium_game_action;
static menuaction_s		s_hard_game_action;
static menuaction_s		s_load_game_action;
static menuaction_s		s_save_game_action;
static menuaction_s		s_credits_action;
static menuseparator_s	s_blankline;

static void StartGame( void )
{
	if (!CM_MapWillLoad ("maps/base1.bsp"))
	{
		Com_Printf ("ERROR: Your Quake II installation is missing the single player data, you cannot start a single player game.\n", LOG_GENERAL);
		M_PopMenu ();
		return;
	}

	// disable updates and start the cinematic going
	cl.servercount = -1;
	M_ForceMenuOff ();
	Cvar_SetValue( "deathmatch", 0 );
	Cvar_SetValue( "coop", 0 );

	//Cvar_SetValue( "gamerules", 0 );		//PGM

	Cbuf_AddText ("loading ; killserver ; wait ; newgame\n");
	cls.key_dest = key_game;
}

static void EasyGameFunc( void *data )
{
	Cvar_ForceSet( "skill", "0" );
	StartGame();
}

static void MediumGameFunc( void *data )
{
	Cvar_ForceSet( "skill", "1" );
	StartGame();
}

static void HardGameFunc( void *data )
{
	Cvar_ForceSet( "skill", "2" );
	StartGame();
}

static void LoadGameFunc( void *unused )
{
	M_Menu_LoadGame_f ();
}

static void SaveGameFunc( void *unused )
{
	M_Menu_SaveGame_f();
}

static void CreditsFunc( void *unused )
{
	M_Menu_Credits_f();
}

static void Game_MenuInit( void )
{
	/*static const char *difficulty_names[] =
	{
		"easy",
		"medium",
		"hard",
		0
	};*/

	s_game_menu.nitems = 0;
	s_game_menu.cursor = 0;

	s_easy_game_action.generic.type	= MTYPE_ACTION;
	s_easy_game_action.generic.flags  = QMF_LEFT_JUSTIFY;
	s_easy_game_action.generic.x		= 0;
	s_easy_game_action.generic.y		= 0;
	s_easy_game_action.generic.name	= "easy";
	s_easy_game_action.generic.callback = EasyGameFunc;

	s_medium_game_action.generic.type	= MTYPE_ACTION;
	s_medium_game_action.generic.flags  = QMF_LEFT_JUSTIFY;
	s_medium_game_action.generic.x		= 0;
	s_medium_game_action.generic.y		= 10;
	s_medium_game_action.generic.name	= "medium";
	s_medium_game_action.generic.callback = MediumGameFunc;

	s_hard_game_action.generic.type	= MTYPE_ACTION;
	s_hard_game_action.generic.flags  = QMF_LEFT_JUSTIFY;
	s_hard_game_action.generic.x		= 0;
	s_hard_game_action.generic.y		= 20;
	s_hard_game_action.generic.name	= "hard";
	s_hard_game_action.generic.callback = HardGameFunc;

	s_blankline.generic.type = MTYPE_SEPARATOR;

	s_load_game_action.generic.type	= MTYPE_ACTION;
	s_load_game_action.generic.flags  = QMF_LEFT_JUSTIFY;
	s_load_game_action.generic.x		= 0;
	s_load_game_action.generic.y		= 40;
	s_load_game_action.generic.name	= "load game";
	s_load_game_action.generic.callback = LoadGameFunc;

	s_save_game_action.generic.type	= MTYPE_ACTION;
	s_save_game_action.generic.flags  = QMF_LEFT_JUSTIFY;
	s_save_game_action.generic.x		= 0;
	s_save_game_action.generic.y		= 50;
	s_save_game_action.generic.name	= "save game";
	s_save_game_action.generic.callback = SaveGameFunc;

	s_credits_action.generic.type	= MTYPE_ACTION;
	s_credits_action.generic.flags  = QMF_LEFT_JUSTIFY;
	s_credits_action.generic.x		= 0;
	s_credits_action.generic.y		= 60;
	s_credits_action.generic.name	= "credits";
	s_credits_action.generic.callback = CreditsFunc;

	Menu_AddItem( &s_game_menu, ( void * ) &s_easy_game_action );
	Menu_AddItem( &s_game_menu, ( void * ) &s_medium_game_action );
	Menu_AddItem( &s_game_menu, ( void * ) &s_hard_game_action );
	Menu_AddItem( &s_game_menu, ( void * ) &s_blankline );
	Menu_AddItem( &s_game_menu, ( void * ) &s_load_game_action );
	Menu_AddItem( &s_game_menu, ( void * ) &s_save_game_action );
	Menu_AddItem( &s_game_menu, ( void * ) &s_blankline );
	Menu_AddItem( &s_game_menu, ( void * ) &s_credits_action );

	Menu_Center( &s_game_menu );
	/* Left-justified difficulty/load rows under the banner. */
	s_game_menu.x = (int)(viddef.width * 0.50f)
		- (int)(8 * SCR_GetMenuScale());
}

static void Game_MenuDraw( void )
{
	M_Banner( "m_banner_game" );
	Menu_AdjustCursor( &s_game_menu, 1 );
	Menu_Draw( &s_game_menu );
}

static const char *Game_MenuKey( int key )
{
	return Default_MenuKey( &s_game_menu, key );
}

static void M_Menu_Game_f (void)
{
	Game_MenuInit();
	M_PushMenu( Game_MenuDraw, Game_MenuKey );
	//m_game_cursor = 1;
}

/*
=============================================================================

LOADGAME MENU

=============================================================================
*/

#define	MAX_SAVEGAMES	15

static menuframework_s	s_savegame_menu;

static menuframework_s	s_loadgame_menu;
static menuaction_s		s_loadgame_actions[MAX_SAVEGAMES];

char		m_savestrings[MAX_SAVEGAMES][32];
qboolean	m_savevalid[MAX_SAVEGAMES];

static void Create_Savestrings (void)
{
	int		i;
	FILE	*f;
	char	name[MAX_OSPATH];

	for (i=0 ; i<MAX_SAVEGAMES ; i++)
	{
		Com_sprintf (name, sizeof(name), "%s/save/save%i/server.ssv", FS_Gamedir(), i);
		f = fopen (name, "rb");
		if (!f)
		{
			strcpy (m_savestrings[i], "<EMPTY>");
			m_savevalid[i] = false;
		}
		else
		{
			FS_Read (m_savestrings[i], sizeof(m_savestrings[i]), f);
			fclose (f);
			m_savevalid[i] = true;
		}
	}
}

static void LoadGameCallback( void *self )
{
	menuaction_s *a = ( menuaction_s * ) self;

	if ( m_savevalid[ a->generic.localdata[0] ] )
		Cbuf_AddText (va("load save%i\n",  a->generic.localdata[0] ) );
	M_ForceMenuOff ();
}

static void LoadGame_MenuInit( void )
{
	int i;

	s_loadgame_menu.x = viddef.width / 2 - 120;
	s_loadgame_menu.y = viddef.height / 2 - 58;
	s_loadgame_menu.nitems = 0;

	Create_Savestrings();

	for ( i = 0; i < MAX_SAVEGAMES; i++ )
	{
		s_loadgame_actions[i].generic.name			= m_savestrings[i];
		s_loadgame_actions[i].generic.flags			= QMF_LEFT_JUSTIFY;
		s_loadgame_actions[i].generic.localdata[0]	= i;
		s_loadgame_actions[i].generic.callback		= LoadGameCallback;

		s_loadgame_actions[i].generic.x = 0;
		s_loadgame_actions[i].generic.y = ( i ) * 10;
		if (i>0)	// separate from autosave
			s_loadgame_actions[i].generic.y += 10;

		s_loadgame_actions[i].generic.type = MTYPE_ACTION;

		Menu_AddItem( &s_loadgame_menu, &s_loadgame_actions[i] );
	}
}

static void LoadGame_MenuDraw( void )
{
	M_Banner( "m_banner_load_game" );
//	Menu_AdjustCursor( &s_loadgame_menu, 1 );
	Menu_Draw( &s_loadgame_menu );
}

static const char *LoadGame_MenuKey( int key )
{
	if ( key == K_ESCAPE || key == K_ENTER )
	{
		s_savegame_menu.cursor = s_loadgame_menu.cursor - 1;
		if ( s_savegame_menu.cursor < 0 )
			s_savegame_menu.cursor = 0;
	}
	return Default_MenuKey( &s_loadgame_menu, key );
}

static void M_Menu_LoadGame_f (void)
{
	LoadGame_MenuInit();
	M_PushMenu( LoadGame_MenuDraw, LoadGame_MenuKey );
}


/*
=============================================================================

SAVEGAME MENU

=============================================================================
*/
static menuframework_s	s_savegame_menu;
static menuaction_s		s_savegame_actions[MAX_SAVEGAMES];

static void SaveGameCallback( void *self )
{
	menuaction_s *a = ( menuaction_s * ) self;

	Cbuf_AddText (va("save save%i\n", a->generic.localdata[0] ));
	M_ForceMenuOff ();
}

static void SaveGame_MenuDraw( void )
{
	M_Banner( "m_banner_save_game" );
	Menu_AdjustCursor( &s_savegame_menu, 1 );
	Menu_Draw( &s_savegame_menu );
}

static void SaveGame_MenuInit( void )
{
	int i;

	s_savegame_menu.x = viddef.width / 2 - 120;
	s_savegame_menu.y = viddef.height / 2 - 58;
	s_savegame_menu.nitems = 0;

	Create_Savestrings();

	// don't include the autosave slot
	for ( i = 0; i < MAX_SAVEGAMES-1; i++ )
	{
		s_savegame_actions[i].generic.name = m_savestrings[i+1];
		s_savegame_actions[i].generic.localdata[0] = i+1;
		s_savegame_actions[i].generic.flags = QMF_LEFT_JUSTIFY;
		s_savegame_actions[i].generic.callback = SaveGameCallback;

		s_savegame_actions[i].generic.x = 0;
		s_savegame_actions[i].generic.y = ( i ) * 10;

		s_savegame_actions[i].generic.type = MTYPE_ACTION;

		Menu_AddItem( &s_savegame_menu, &s_savegame_actions[i] );
	}
}

static const char *SaveGame_MenuKey( int key )
{
	if ( key == K_ENTER || key == K_ESCAPE )
	{
		s_loadgame_menu.cursor = s_savegame_menu.cursor - 1;
		if ( s_loadgame_menu.cursor < 0 )
			s_loadgame_menu.cursor = 0;
	}
	return Default_MenuKey( &s_savegame_menu, key );
}

static void M_Menu_SaveGame_f (void)
{
	if (!Com_ServerState())
		return;		// not playing a game

	SaveGame_MenuInit();
	M_PushMenu( SaveGame_MenuDraw, SaveGame_MenuKey );
	Create_Savestrings ();
}


/*
=============================================================================

JOIN SERVER MENU (LAN + internet master; address book is separate)

=============================================================================
*/
#define MAX_LOCAL_SERVERS		512
#define JOIN_ADDRESSBOOK_SLOTS	16
#define SERVER_SLOTS_VISIBLE	12

static menuframework_s	s_joinserver_menu;
static menuseparator_s	s_joinserver_server_title;
static menuaction_s		s_joinserver_search_action;
static menuaction_s		s_joinserver_address_book_action;
static menuaction_s		s_joinserver_server_actions[SERVER_SLOTS_VISIBLE];

int		m_num_servers;

static char local_server_names[MAX_LOCAL_SERVERS][80];
static char local_server_empty[SERVER_SLOTS_VISIBLE][80];

static netadr_t local_server_netadr[MAX_LOCAL_SERVERS];
static int		local_server_ping[MAX_LOCAL_SERVERS];
static qboolean	local_server_isfav[MAX_LOCAL_SERVERS];
static int		m_serverlist_start_time;
static int		m_server_page;
static int		m_slots_visible = SERVER_SLOTS_VISIBLE;
static qboolean	m_searching;
static int		m_search_idle_time;

static void JoinServer_RowDraw (void *self)
{
	menuaction_s	*a = (menuaction_s *)self;
	float			s = SCR_GetMenuScale();
	int				x, y;
	int				idx;
	const char		*text;

	x = a->generic.parent->x + (int)(a->generic.x * s);
	y = a->generic.parent->y + (int)(a->generic.y * s);
	idx = a->generic.localdata[0];
	if (idx >= 0 && idx < m_num_servers)
		text = local_server_names[idx];
	else
		text = a->generic.name;
	if (!text || !text[0])
		return;
	Menu_DrawString (x, y, text);
}

static void JoinServer_HeaderDraw (void *self)
{
	menuseparator_s	*sep = (menuseparator_s *)self;
	float			s = SCR_GetMenuScale();
	int				x, y;

	x = sep->generic.parent->x + (int)(sep->generic.x * s);
	y = sep->generic.parent->y + (int)(sep->generic.y * s);
	Menu_DrawStringDark (x, y, " #  hostname           map      pl   ms");
}

static int		m_visible_index[SERVER_SLOTS_VISIBLE];

static void JoinServer_RebuildVisible (void);
static qboolean JoinServer_AdrIsFavorite (netadr_t *adr);
static qboolean JoinServer_AddFavoriteAdr (const char *addr);
static qboolean JoinServer_RemoveFavoriteAdr (const char *addr);

static qboolean JoinServer_AdrIsFavorite (netadr_t *adr)
{
	int			i;
	netadr_t	tmp;
	char		name[16];
	const char	*s;

	for (i = 0; i < JOIN_ADDRESSBOOK_SLOTS; i++)
	{
		Com_sprintf (name, sizeof(name), "adr%i", i);
		s = Cvar_VariableString (name);
		if (!s || !s[0])
			continue;
		if (!NET_StringToAdr (s, &tmp))
			continue;
		if (!tmp.port)
			tmp.port = ShortSwap (PORT_SERVER);
		if (NET_CompareAdr (&tmp, adr))
			return true;
	}
	return false;
}

static qboolean JoinServer_AddFavoriteAdr (const char *addr)
{
	int			i;
	netadr_t	adr;
	char		name[16];
	char		*canon;
	const char	*s;

	if (!addr || !addr[0])
		return false;
	if (!NET_StringToAdr (addr, &adr))
	{
		Com_Printf ("Bad favorite address: %s\n", LOG_CLIENT, addr);
		return false;
	}
	if (!adr.port)
		adr.port = ShortSwap (PORT_SERVER);

	canon = NET_AdrToString (&adr);
	for (i = 0; i < JOIN_ADDRESSBOOK_SLOTS; i++)
	{
		Com_sprintf (name, sizeof(name), "adr%i", i);
		s = Cvar_VariableString (name);
		if (s && s[0])
		{
			netadr_t	have;
			if (NET_StringToAdr (s, &have))
			{
				if (!have.port)
					have.port = ShortSwap (PORT_SERVER);
				if (NET_CompareAdr (&have, &adr))
					return false;
			}
			continue;
		}
		Cvar_FullSet (name, canon, CVAR_ARCHIVE);
		return true;
	}
	Com_Printf ("Address book full (%d). Open Address Book to edit.\n", LOG_CLIENT, JOIN_ADDRESSBOOK_SLOTS);
	return false;
}

static qboolean JoinServer_RemoveFavoriteAdr (const char *addr)
{
	int			i;
	netadr_t	want, have;
	char		name[16];
	const char	*s;

	if (!addr || !addr[0])
		return false;
	if (!NET_StringToAdr (addr, &want))
		return false;
	if (!want.port)
		want.port = ShortSwap (PORT_SERVER);

	for (i = 0; i < JOIN_ADDRESSBOOK_SLOTS; i++)
	{
		Com_sprintf (name, sizeof(name), "adr%i", i);
		s = Cvar_VariableString (name);
		if (!s || !s[0])
			continue;
		if (!NET_StringToAdr (s, &have))
			continue;
		if (!have.port)
			have.port = ShortSwap (PORT_SERVER);
		if (!NET_CompareAdr (&want, &have))
			continue;
		Cvar_Set (name, "");
		return true;
	}
	return false;
}

static void JoinServer_RefreshName (int i)
{
	char	*src;

	if (i < 0 || i >= m_num_servers)
		return;

	src = local_server_names[i];
	/* formatted as "%2d.%c ..." - flip favorite marker */
	if (src[0] && src[2] == '.' && (src[3] == ' ' || src[3] == '*'))
		src[3] = local_server_isfav[i] ? '*' : ' ';
}

void M_AddToServerList (netadr_t adr, char *info)
{
	int		i;
	char	hostname[32];
	char	mapname[16];
	char	players[16];
	char	*p;
	int		ping;
	int		nmap;
	int		npl;

	if (m_num_servers == MAX_LOCAL_SERVERS)
		return;

	while (info && *info == ' ')
		info++;
	if (!info || !info[0])
		return;

	for (i = 0; i < m_num_servers; i++)
	{
		if (NET_CompareAdr (&adr, &local_server_netadr[i]))
			return;
	}

	hostname[0] = mapname[0] = players[0] = 0;

	/* Classic SVC_Info: "%20s %8s %2i/%2i\n" */
	if (strlen(info) >= 20)
	{
		memcpy (hostname, info, 20);
		hostname[20] = 0;
		p = hostname;
		while (*p == ' ')
			p++;
		memmove (hostname, p, strlen(p) + 1);
		p = hostname + strlen(hostname);
		while (p > hostname && (p[-1] == ' ' || p[-1] == '\n' || p[-1] == '\r'))
		{
			p--;
			*p = 0;
		}

		p = info + 20;
		while (*p == ' ')
			p++;
		nmap = 0;
		while (*p && *p != ' ' && nmap < (int)sizeof(mapname) - 1)
			mapname[nmap++] = *p++;
		mapname[nmap] = 0;

		while (*p == ' ')
			p++;
		npl = 0;
		while (*p && *p != '\n' && *p != '\r' && npl < (int)sizeof(players) - 1)
			players[npl++] = *p++;
		players[npl] = 0;
	}

	if (!hostname[0])
	{
		Q_strncpy (hostname, info, sizeof(hostname)-1);
		p = strchr (hostname, '\n');
		if (p)
			*p = 0;
	}
	if (!mapname[0])
		strcpy (mapname, "???");
	if (!players[0])
		strcpy (players, "?/?");

	ping = cls.realtime - m_serverlist_start_time;
	if (ping < 0)
		ping = 0;
	if (ping > 999)
		ping = 999;

	local_server_netadr[m_num_servers] = adr;
	local_server_ping[m_num_servers] = ping;
	local_server_isfav[m_num_servers] = JoinServer_AdrIsFavorite (&adr);

	Com_sprintf (local_server_names[m_num_servers], sizeof(local_server_names[0]),
		"%2d.%c %-18.18s %-8.8s %5s %4d",
		m_num_servers + 1,
		local_server_isfav[m_num_servers] ? '*' : ' ',
		hostname,
		mapname,
		players,
		ping);

	m_num_servers++;
	JoinServer_RebuildVisible ();
}

static void JoinServer_RebuildVisible (void)
{
	int		i;
	int		idx;
	int		matched;
	int		skip;
	int		start;

	/* count matching servers for paging */
	matched = 0;
	for (i = 0; i < m_num_servers; i++)
		matched++;

	start = m_server_page * m_slots_visible;
	if (start >= matched && m_server_page > 0)
	{
		m_server_page = (matched > 0) ? (matched - 1) / m_slots_visible : 0;
		start = m_server_page * m_slots_visible;
	}

	skip = 0;
	idx = 0;
	for (i = 0; i < SERVER_SLOTS_VISIBLE; i++)
		m_visible_index[i] = -1;

	for (i = 0; i < m_num_servers && idx < m_slots_visible; i++)
	{
		if (skip < start)
		{
			skip++;
			continue;
		}
		m_visible_index[idx] = i;
		s_joinserver_server_actions[idx].generic.name = local_server_names[i];
		s_joinserver_server_actions[idx].generic.localdata[0] = i;
		idx++;
	}

	for (; idx < m_slots_visible; idx++)
	{
		local_server_empty[idx][0] = 0;
		s_joinserver_server_actions[idx].generic.name = local_server_empty[idx];
		s_joinserver_server_actions[idx].generic.localdata[0] = -1;
		m_visible_index[idx] = -1;
	}
}

static void JoinServerFunc( void *self )
{
	char			buffer[128];
	int				index;
	menuaction_s	*a = (menuaction_s *)self;

	index = a->generic.localdata[0];
	if (index < 0 || index >= m_num_servers)
		return;

	Com_sprintf (buffer, sizeof(buffer), "connect %s\n", NET_AdrToString (&local_server_netadr[index]));
	Cbuf_AddText (buffer);
	M_ForceMenuOff ();
}

static void AddressBookFunc( void *self )
{
	M_Menu_AddressBook_f();
}

static void SearchLocalGames( void )
{
	int			i;

	m_num_servers = 0;
	m_server_page = 0;
	m_searching = true;
	m_search_idle_time = 0;
	m_serverlist_start_time = cls.realtime;
	for (i = 0; i < MAX_LOCAL_SERVERS; i++)
	{
		local_server_ping[i] = 0;
		local_server_isfav[i] = false;
		local_server_names[i][0] = 0;
	}

	CL_PingServers_f();

	m_searching = true;
	JoinServer_RebuildVisible ();
}

static void SearchLocalGamesFunc( void *self )
{
	SearchLocalGames();
}

static void JoinServer_MenuInit( void )
{
	int		i;
	int		bw, bh;
	float	s;
	int		slots;
	int		y;

	s = SCR_GetMenuScale();
	re.DrawGetPicSize (&bw, &bh, "m_banner_join_server");

	/* Sit the list under the banner. Do NOT Menu_Center — that was stacking
	 * the old plaque coords on top of the new rows.
	 * Left-justify rows are ~43 chars wide; put the block on screen center. */
	s_joinserver_menu.x = (int)(viddef.width * 0.50f) - (int)((43 * 8 / 2) * s);
	s_joinserver_menu.y = (int)(viddef.height * 0.50f - 110 * s) + (int)(bh * SCR_GetMenuPicScale() + 8 * s);
	s_joinserver_menu.nitems = 0;
	s_joinserver_menu.cursor = 0;

	slots = (int)((viddef.height - s_joinserver_menu.y - 24 * s) / (10 * s)) - 5;
	if (slots < 6)
		slots = 6;
	if (slots > SERVER_SLOTS_VISIBLE)
		slots = SERVER_SLOTS_VISIBLE;
	m_slots_visible = slots;

	s_joinserver_address_book_action.generic.type	= MTYPE_ACTION;
	s_joinserver_address_book_action.generic.name	= "address book";
	s_joinserver_address_book_action.generic.flags	= QMF_LEFT_JUSTIFY;
	s_joinserver_address_book_action.generic.x		= 0;
	s_joinserver_address_book_action.generic.y		= 0;
	s_joinserver_address_book_action.generic.callback = AddressBookFunc;
	s_joinserver_address_book_action.generic.statusbar = "favorite servers (adr0-adr15)";

	s_joinserver_search_action.generic.type = MTYPE_ACTION;
	s_joinserver_search_action.generic.name	= "refresh list";
	s_joinserver_search_action.generic.flags	= QMF_LEFT_JUSTIFY;
	s_joinserver_search_action.generic.x	= 0;
	s_joinserver_search_action.generic.y	= 12;
	s_joinserver_search_action.generic.callback = SearchLocalGamesFunc;
	s_joinserver_search_action.generic.statusbar = "LAN + internet master list";

	s_joinserver_server_title.generic.type = MTYPE_SEPARATOR;
	s_joinserver_server_title.generic.name = " #  hostname           map      pl   ms";
	s_joinserver_server_title.generic.x    = 0;
	s_joinserver_server_title.generic.y	   = 28;
	s_joinserver_server_title.generic.ownerdraw = JoinServer_HeaderDraw;

	y = 40;
	for ( i = 0; i < SERVER_SLOTS_VISIBLE; i++ )
	{
		local_server_empty[i][0] = 0;
		s_joinserver_server_actions[i].generic.type	= MTYPE_ACTION;
		s_joinserver_server_actions[i].generic.name	= local_server_empty[i];
		s_joinserver_server_actions[i].generic.flags	= QMF_LEFT_JUSTIFY;
		s_joinserver_server_actions[i].generic.x		= 0;
		s_joinserver_server_actions[i].generic.y		= y;
		s_joinserver_server_actions[i].generic.callback = JoinServerFunc;
		s_joinserver_server_actions[i].generic.ownerdraw = JoinServer_RowDraw;
		s_joinserver_server_actions[i].generic.statusbar = "ENTER connect  F address book  [/] page  SPACE refresh";
		s_joinserver_server_actions[i].generic.localdata[0] = -1;
		m_visible_index[i] = -1;
		if (i < m_slots_visible)
			y += 10;
	}

	Menu_AddItem( &s_joinserver_menu, &s_joinserver_address_book_action );
	Menu_AddItem( &s_joinserver_menu, &s_joinserver_search_action );
	Menu_AddItem( &s_joinserver_menu, &s_joinserver_server_title );

	for ( i = 0; i < m_slots_visible; i++ )
		Menu_AddItem( &s_joinserver_menu, &s_joinserver_server_actions[i] );

	SearchLocalGames();
}

static void JoinServer_MenuDraw(void)
{
	char	pagebuf[64];
	int		matched;
	int		maxpage;
	int		charh;
	int		x;
	int		y;
	float	s;

	M_Banner( "m_banner_join_server" );
	Menu_Draw( &s_joinserver_menu );

	matched = m_num_servers;
	maxpage = (matched + m_slots_visible - 1) / m_slots_visible;
	if (maxpage < 1)
		maxpage = 1;

	if (CL_ServerPingBusy ())
	{
		m_searching = true;
		m_search_idle_time = 0;
	}
	else if (m_searching)
	{
		if (!m_search_idle_time)
			m_search_idle_time = cls.realtime;
		else if (cls.realtime - m_search_idle_time >= 2000)
			m_searching = false;
	}

	s = SCR_GetMenuScale();
	if (m_searching)
		Com_sprintf (pagebuf, sizeof(pagebuf), "searching...  page %d/%d  %d listed",
			m_server_page + 1, maxpage, matched);
	else
		Com_sprintf (pagebuf, sizeof(pagebuf), "page %d/%d  %d listed",
			m_server_page + 1, maxpage, matched);

	charh = (int)(8 * s);
	y = (int)(viddef.height - 20 * s);
	if (y + charh > viddef.height - 1)
		y = viddef.height - 1 - charh;
	if (y < 0)
		y = 0;
	x = (int)(viddef.width * 0.50f) - (int)((int)strlen(pagebuf) * 4 * s);
	Menu_DrawString (x, y, pagebuf);
}

static void JoinServer_ToggleFavoriteSelected (void)
{
	int		cursor;
	int		index;
	char	*addr;

	cursor = s_joinserver_menu.cursor;
	/* 0 book, 1 refresh, 2 title, 3+ servers */
	if (cursor < 3)
		return;
	index = s_joinserver_server_actions[cursor - 3].generic.localdata[0];
	if (index < 0 || index >= m_num_servers)
		return;

	addr = NET_AdrToString (&local_server_netadr[index]);
	if (local_server_isfav[index])
	{
		JoinServer_RemoveFavoriteAdr (addr);
		local_server_isfav[index] = false;
		Com_Printf ("Removed from address book %s\n", LOG_CLIENT, addr);
	}
	else
	{
		if (JoinServer_AddFavoriteAdr (addr))
		{
			local_server_isfav[index] = true;
			Com_Printf ("Added to address book %s\n", LOG_CLIENT, addr);
		}
	}

	JoinServer_RefreshName (index);
	JoinServer_RebuildVisible ();
}

static const char *JoinServer_MenuKey( int key )
{
	int maxpage;
	int matched;

	if (key >= '1' && key <= '9')
	{
		int slot = key - '1';
		if (slot < m_slots_visible)
		{
			s_joinserver_menu.cursor = 3 + slot;
			Menu_AdjustCursor (&s_joinserver_menu, 1);
			Menu_SelectItem (&s_joinserver_menu);
		}
		return NULL;
	}

	if (key == 'f' || key == 'F' || key == K_INS)
	{
		JoinServer_ToggleFavoriteSelected ();
		return menu_move_sound;
	}

	if (key == K_SPACE)
	{
		SearchLocalGames ();
		return menu_move_sound;
	}

	matched = m_num_servers;
	maxpage = (matched + m_slots_visible - 1) / m_slots_visible;
	if (maxpage < 1)
		maxpage = 1;

	if (key == ']' || key == K_PGDN)
	{
		if (m_server_page + 1 < maxpage)
		{
			m_server_page++;
			JoinServer_RebuildVisible ();
		}
		return menu_move_sound;
	}

	if (key == '[' || key == K_PGUP)
	{
		if (m_server_page > 0)
		{
			m_server_page--;
			JoinServer_RebuildVisible ();
		}
		return menu_move_sound;
	}

	return Default_MenuKey( &s_joinserver_menu, key );
}

static void M_Menu_JoinServer_f (void)
{
	JoinServer_MenuInit();
	M_PushMenu( JoinServer_MenuDraw, JoinServer_MenuKey );
}
/*
=============================================================================

START SERVER MENU

=============================================================================
*/
static menuframework_s s_startserver_menu;
static char **mapnames;
static int	  nummaps;

static menuaction_s	s_startserver_start_action;
static menuaction_s	s_startserver_dmoptions_action;

static menufield_s	s_timelimit_field;
static menufield_s	s_fraglimit_field;
static menufield_s	s_maxclients_field;
static menufield_s	s_hostname_field;
static menulist_s	s_startmap_list;
static menulist_s	s_rules_box;

static void DMOptionsFunc( void *self )
{
	if (s_rules_box.curvalue == 1)
		return;
	M_Menu_DMOptions_f();
}

static void RulesChangeFunc ( void *self )
{
	// DM
	if (s_rules_box.curvalue == 0)
	{
		s_maxclients_field.generic.statusbar = NULL;
		s_startserver_dmoptions_action.generic.statusbar = NULL;
	}
	else if(s_rules_box.curvalue == 1)		// coop				// PGM
	{
		s_maxclients_field.generic.statusbar = "4 maximum for cooperative";
		if (atoi(s_maxclients_field.buffer) > 4)
			strcpy( s_maxclients_field.buffer, "4" );
		s_startserver_dmoptions_action.generic.statusbar = "N/A for cooperative";
	}
//=====
//PGM
	// ROGUE GAMES
	else if(Developer_searchpath() == 2)
	{
		if (s_rules_box.curvalue == 2)			// tag	
		{
			s_maxclients_field.generic.statusbar = NULL;
			s_startserver_dmoptions_action.generic.statusbar = NULL;
		}
/*
		else if(s_rules_box.curvalue == 3)		// deathball
		{
			s_maxclients_field.generic.statusbar = NULL;
			s_startserver_dmoptions_action.generic.statusbar = NULL;
		}
*/
	}
//PGM
//=====
}

static void StartServerActionFunc( void *self )
{
	char	startmap[1024];
	float	timelimit;
	float	fraglimit;
	float	maxclients;
	char	*spot;

	strcpy( startmap, strchr( mapnames[s_startmap_list.curvalue], '\n' ) + 1 );

	maxclients  = (float)atof( s_maxclients_field.buffer );
	timelimit	= (float)atof( s_timelimit_field.buffer );
	fraglimit	= (float)atof( s_fraglimit_field.buffer );

	Cvar_SetValue( "maxclients", ClampCvar( 0, maxclients, maxclients ) );
	Cvar_SetValue ("timelimit", ClampCvar( 0, timelimit, timelimit ) );
	Cvar_SetValue ("fraglimit", ClampCvar( 0, fraglimit, fraglimit ) );
	Cvar_Set("hostname", s_hostname_field.buffer );
//	Cvar_SetValue ("deathmatch", !s_rules_box.curvalue );
//	Cvar_SetValue ("coop", s_rules_box.curvalue );

//PGM
	if((s_rules_box.curvalue < 2) || (Developer_searchpath() != 2))
	{
		Cvar_SetValue ("deathmatch", (float)!s_rules_box.curvalue );
		Cvar_SetValue ("coop", (float)s_rules_box.curvalue );
		//Cvar_Set ("gamerules", "0" );
	}
	else
	{
		Cvar_Set ("deathmatch", "1" );	// deathmatch is always true for rogue games, right?
		Cvar_Set ("coop", "0" );			// FIXME - this might need to depend on which game we're running
		//Cvar_SetValue ("gamerules", (float)s_rules_box.curvalue );
	}
//PGM

	spot = NULL;
	if (s_rules_box.curvalue == 1)		// PGM
	{
 		if(Q_stricmp(startmap, "bunk1") == 0)
  			spot = "start";
 		else if(Q_stricmp(startmap, "mintro") == 0)
  			spot = "start";
 		else if(Q_stricmp(startmap, "fact1") == 0)
  			spot = "start";
 		else if(Q_stricmp(startmap, "power1") == 0)
  			spot = "pstart";
 		else if(Q_stricmp(startmap, "biggun") == 0)
  			spot = "bstart";
 		else if(Q_stricmp(startmap, "hangar1") == 0)
  			spot = "unitstart";
 		else if(Q_stricmp(startmap, "city1") == 0)
  			spot = "unitstart";
 		else if(Q_stricmp(startmap, "boss1") == 0)
			spot = "bosstart";
	}

	if (spot)
	{
		if (Com_ServerState())
			Cbuf_AddText ("disconnect\n");
		Cbuf_AddText (va("gamemap \"*%s$%s\"\n", startmap, spot));
	}
	else
	{
		Cbuf_AddText (va("killserver\nmap %s\n", startmap));
	}

	M_ForceMenuOff ();
}

static void StartServer_MenuInit( void )
{
	static const char *dm_coop_names[] =
	{
		"deathmatch",
		"cooperative",
		0
	};
//=======
//PGM
	static const char *dm_coop_names_rogue[] =
	{
		"deathmatch",
		"cooperative",
		"tag",
//		"deathball",
		0
	};
//PGM
//=======
	char *buffer;
	char  mapsname[1024];
	char *s;
	int length;
	int i;
	FILE *fp;

	/*
	** load the list of map names
	*/
	{
	qboolean maps_from_fs = false;

	Com_sprintf( mapsname, sizeof( mapsname ), "%s/maps.lst", FS_Gamedir() );
	if ( ( fp = fopen( mapsname, "rb" ) ) == 0 )
	{
		if ( ( length = FS_LoadFile( "maps.lst", ( void ** ) &buffer ) ) == -1 )
			Com_Error( ERR_DROP, "couldn't find maps.lst\n" );
		maps_from_fs = true;
	}
	else
	{
#ifdef _WIN32
		length = filelength( fileno( fp  ) );
#else
		fseek(fp, 0, SEEK_END);
		length = ftell(fp);
		fseek(fp, 0, SEEK_SET);
#endif
		buffer = malloc( length );
		if (!buffer)
		{
			fclose (fp);
			Com_Error( ERR_DROP, "maps.lst: out of memory\n" );
		}
		if (length > 0 && fread( buffer, length, 1, fp ) != 1)
		{
			free (buffer);
			fclose (fp);
			Com_Error( ERR_DROP, "maps.lst: short read\n" );
		}
		fclose (fp);
		fp = NULL;
	}

	s = buffer;

	i = 0;
	while ( i < length )
	{
		if ( s[i] == '\r' || s[i] == '\n' )
		{
			nummaps++;
			if (s[i] == '\r' && i + 1 < length && s[i+1] == '\n')
				i++;
		}
		i++;
	}

	if ( nummaps == 0 )
		Com_Error( ERR_DROP, "no maps in maps.lst\n" );

	mapnames = malloc( sizeof( char * ) * ( nummaps + 1 ) );
	memset( mapnames, 0, sizeof( char * ) * ( nummaps + 1 ) );

	s = buffer;

	for ( i = 0; i < nummaps; i++ )
	{
    char  shortname[MAX_TOKEN_CHARS];
    char  longname[MAX_TOKEN_CHARS];
		char  scratch[200];
		int		j, l;

		strcpy( shortname, COM_Parse( &s ) );
		l = (int)strlen(shortname);
		for (j=0 ; j<l ; j++)
			shortname[j] = toupper(shortname[j]);
		strcpy( longname, COM_Parse( &s ) );
		Com_sprintf( scratch, sizeof( scratch ), "%s\n%s", longname, shortname );

		mapnames[i] = malloc( strlen( scratch ) + 1 );
		strcpy( mapnames[i], scratch );
	}
	mapnames[nummaps] = 0;

	if ( maps_from_fs )
		FS_FreeFile( buffer );
	else
		free( buffer );
	}

	/*
	** initialize the menu stuff
	*/
	s_startserver_menu.x = (int)(viddef.width * 0.50f);
	s_startserver_menu.nitems = 0;

	s_startmap_list.generic.type = MTYPE_SPINCONTROL;
	s_startmap_list.generic.x	= 0;
	s_startmap_list.generic.y	= 0;
	s_startmap_list.generic.name	= "initial map";
	s_startmap_list.itemnames = (const char **)mapnames;
	s_startmap_list.curvalue = 0;

	s_rules_box.generic.type = MTYPE_SPINCONTROL;
	s_rules_box.generic.x	= 0;
	s_rules_box.generic.y	= 20;
	s_rules_box.generic.name	= "rules";
	
//PGM - rogue games only available with rogue DLL.
	if(Developer_searchpath() == 2)
		s_rules_box.itemnames = dm_coop_names_rogue;
	else
		s_rules_box.itemnames = dm_coop_names;
//PGM

	if (Cvar_IntValue("coop"))
		s_rules_box.curvalue = 1;
	else
		s_rules_box.curvalue = 0;
	s_rules_box.generic.callback = RulesChangeFunc;

	s_timelimit_field.generic.type = MTYPE_FIELD;
	s_timelimit_field.generic.name = "time limit";
	s_timelimit_field.generic.flags = QMF_NUMBERSONLY;
	s_timelimit_field.generic.x	= 0;
	s_timelimit_field.generic.y	= 36;
	s_timelimit_field.generic.statusbar = "0 = no limit";
	s_timelimit_field.length = 3;
	s_timelimit_field.visible_length = 3;
	strncpy( s_timelimit_field.buffer, Cvar_VariableString("timelimit"), sizeof(s_timelimit_field.buffer)-1);

	s_fraglimit_field.generic.type = MTYPE_FIELD;
	s_fraglimit_field.generic.name = "frag limit";
	s_fraglimit_field.generic.flags = QMF_NUMBERSONLY;
	s_fraglimit_field.generic.x	= 0;
	s_fraglimit_field.generic.y	= 54;
	s_fraglimit_field.generic.statusbar = "0 = no limit";
	s_fraglimit_field.length = 3;
	s_fraglimit_field.visible_length = 3;
	strncpy( s_fraglimit_field.buffer, Cvar_VariableString("fraglimit"), sizeof(s_fraglimit_field.buffer)-1);

	/*
	** maxclients determines the maximum number of players that can join
	** the game.  If maxclients is only "1" then we should default the menu
	** option to 8 players, otherwise use whatever its current value is. 
	** Clamping will be done when the server is actually started.
	*/
	s_maxclients_field.generic.type = MTYPE_FIELD;
	s_maxclients_field.generic.name = "max players";
	s_maxclients_field.generic.flags = QMF_NUMBERSONLY;
	s_maxclients_field.generic.x	= 0;
	s_maxclients_field.generic.y	= 72;
	s_maxclients_field.generic.statusbar = NULL;
	s_maxclients_field.length = 3;
	s_maxclients_field.visible_length = 3;
	if ( Cvar_IntValue( "maxclients" ) == 1)
		strcpy( s_maxclients_field.buffer, "8");
	else 
		strncpy( s_maxclients_field.buffer, Cvar_VariableString("maxclients"), sizeof(s_maxclients_field.buffer)-1);

	s_hostname_field.generic.type = MTYPE_FIELD;
	s_hostname_field.generic.name = "hostname";
	s_hostname_field.generic.flags = 0;
	s_hostname_field.generic.x	= 0;
	s_hostname_field.generic.y	= 90;
	s_hostname_field.generic.statusbar = NULL;
	s_hostname_field.length = 12;
	s_hostname_field.visible_length = 12;
	strncpy( s_hostname_field.buffer, Cvar_VariableString("hostname"), sizeof(s_hostname_field.buffer));

	s_startserver_dmoptions_action.generic.type = MTYPE_ACTION;
	s_startserver_dmoptions_action.generic.name	= " deathmatch flags";
	s_startserver_dmoptions_action.generic.flags= QMF_LEFT_JUSTIFY;
	s_startserver_dmoptions_action.generic.x	= 24;
	s_startserver_dmoptions_action.generic.y	= 108;
	s_startserver_dmoptions_action.generic.statusbar = NULL;
	s_startserver_dmoptions_action.generic.callback = DMOptionsFunc;

	s_startserver_start_action.generic.type = MTYPE_ACTION;
	s_startserver_start_action.generic.name	= " begin";
	s_startserver_start_action.generic.flags= QMF_LEFT_JUSTIFY;
	s_startserver_start_action.generic.x	= 24;
	s_startserver_start_action.generic.y	= 128;
	s_startserver_start_action.generic.callback = StartServerActionFunc;

	Menu_AddItem( &s_startserver_menu, &s_startmap_list );
	Menu_AddItem( &s_startserver_menu, &s_rules_box );
	Menu_AddItem( &s_startserver_menu, &s_timelimit_field );
	Menu_AddItem( &s_startserver_menu, &s_fraglimit_field );
	Menu_AddItem( &s_startserver_menu, &s_maxclients_field );
	Menu_AddItem( &s_startserver_menu, &s_hostname_field );
	Menu_AddItem( &s_startserver_menu, &s_startserver_dmoptions_action );
	Menu_AddItem( &s_startserver_menu, &s_startserver_start_action );

	Menu_Center( &s_startserver_menu );

	// call this now to set proper inital state
	RulesChangeFunc ( NULL );
}

static void StartServer_MenuDraw(void)
{
	Menu_Draw( &s_startserver_menu );
}

static const char *StartServer_MenuKey( int key )
{
	if ( key == K_ESCAPE )
	{
		if ( mapnames )
		{
			int i;

			for ( i = 0; i < nummaps; i++ )
				free( mapnames[i] );
			free( mapnames );
		}
		mapnames = 0;
		nummaps = 0;
	}

	return Default_MenuKey( &s_startserver_menu, key );
}

static void M_Menu_StartServer_f (void)
{
	StartServer_MenuInit();
	M_PushMenu( StartServer_MenuDraw, StartServer_MenuKey );
}

/*
=============================================================================

DMOPTIONS BOOK MENU

=============================================================================
*/
static char dmoptions_statusbar[128];

static menuframework_s s_dmoptions_menu;

static menulist_s	s_friendlyfire_box;
static menulist_s	s_falls_box;
static menulist_s	s_weapons_stay_box;
static menulist_s	s_instant_powerups_box;
static menulist_s	s_powerups_box;
static menulist_s	s_health_box;
static menulist_s	s_spawn_farthest_box;
static menulist_s	s_teamplay_box;
static menulist_s	s_samelevel_box;
static menulist_s	s_force_respawn_box;
static menulist_s	s_armor_box;
static menulist_s	s_allow_exit_box;
static menulist_s	s_infinite_ammo_box;
static menulist_s	s_fixed_fov_box;
static menulist_s	s_quad_drop_box;

//ROGUE
static menulist_s	s_no_mines_box;
static menulist_s	s_no_nukes_box;
static menulist_s	s_stack_double_box;
static menulist_s	s_no_spheres_box;
//ROGUE

static void DMFlagCallback( void *self )
{
	menulist_s *f = ( menulist_s * ) self;
	int flags;
	int bit = 0;

	flags = Cvar_IntValue( "dmflags" );

	if ( f == &s_friendlyfire_box )
	{
		if ( f->curvalue )
			flags &= ~DF_NO_FRIENDLY_FIRE;
		else
			flags |= DF_NO_FRIENDLY_FIRE;
		goto setvalue;
	}
	else if ( f == &s_falls_box )
	{
		if ( f->curvalue )
			flags &= ~DF_NO_FALLING;
		else
			flags |= DF_NO_FALLING;
		goto setvalue;
	}
	else if ( f == &s_weapons_stay_box ) 
	{
		bit = DF_WEAPONS_STAY;
	}
	else if ( f == &s_instant_powerups_box )
	{
		bit = DF_INSTANT_ITEMS;
	}
	else if ( f == &s_allow_exit_box )
	{
		bit = DF_ALLOW_EXIT;
	}
	else if ( f == &s_powerups_box )
	{
		if ( f->curvalue )
			flags &= ~DF_NO_ITEMS;
		else
			flags |= DF_NO_ITEMS;
		goto setvalue;
	}
	else if ( f == &s_health_box )
	{
		if ( f->curvalue )
			flags &= ~DF_NO_HEALTH;
		else
			flags |= DF_NO_HEALTH;
		goto setvalue;
	}
	else if ( f == &s_spawn_farthest_box )
	{
		bit = DF_SPAWN_FARTHEST;
	}
	else if ( f == &s_teamplay_box )
	{
		if ( f->curvalue == 1 )
		{
			flags |=  DF_SKINTEAMS;
			flags &= ~DF_MODELTEAMS;
		}
		else if ( f->curvalue == 2 )
		{
			flags |=  DF_MODELTEAMS;
			flags &= ~DF_SKINTEAMS;
		}
		else
		{
			flags &= ~( DF_MODELTEAMS | DF_SKINTEAMS );
		}

		goto setvalue;
	}
	else if ( f == &s_samelevel_box )
	{
		bit = DF_SAME_LEVEL;
	}
	else if ( f == &s_force_respawn_box )
	{
		bit = DF_FORCE_RESPAWN;
	}
	else if ( f == &s_armor_box )
	{
		if ( f->curvalue )
			flags &= ~DF_NO_ARMOR;
		else
			flags |= DF_NO_ARMOR;
		goto setvalue;
	}
	else if ( f == &s_infinite_ammo_box )
	{
		bit = DF_INFINITE_AMMO;
	}
	else if ( f == &s_fixed_fov_box )
	{
		bit = DF_FIXED_FOV;
	}
	else if ( f == &s_quad_drop_box )
	{
		bit = DF_QUAD_DROP;
	}

//=======
//ROGUE
	else if (Developer_searchpath() == 2)
	{
		if ( f == &s_no_mines_box)
		{
			bit = DF_NO_MINES;
		}
		else if ( f == &s_no_nukes_box)
		{
			bit = DF_NO_NUKES;
		}
		else if ( f == &s_stack_double_box)
		{
			bit = DF_NO_STACK_DOUBLE;
		}
		else if ( f == &s_no_spheres_box)
		{
			bit = DF_NO_SPHERES;
		}
	}
//ROGUE
//=======

	if ( f )
	{
		if ( f->curvalue == 0 )
			flags &= ~bit;
		else
			flags |= bit;
	}

setvalue:
	Cvar_SetValue ("dmflags", (float)flags);

	Com_sprintf( dmoptions_statusbar, sizeof( dmoptions_statusbar ), "dmflags = %d", flags );

}

static void DMOptions_MenuInit( void )
{
	static const char *yes_no_names[] =
	{
		"no", "yes", 0
	};
	static const char *teamplay_names[] = 
	{
		"disabled", "by skin", "by model", 0
	};
	int dmflags = Cvar_IntValue( "dmflags" );
	int y = 0;

	s_dmoptions_menu.x = (int)(viddef.width * 0.50f);
	s_dmoptions_menu.nitems = 0;

	s_falls_box.generic.type = MTYPE_SPINCONTROL;
	s_falls_box.generic.x	= 0;
	s_falls_box.generic.y	= y;
	s_falls_box.generic.name	= "falling damage";
	s_falls_box.generic.callback = DMFlagCallback;
	s_falls_box.itemnames = yes_no_names;
	s_falls_box.curvalue = ( dmflags & DF_NO_FALLING ) == 0;

	s_weapons_stay_box.generic.type = MTYPE_SPINCONTROL;
	s_weapons_stay_box.generic.x	= 0;
	s_weapons_stay_box.generic.y	= y += 10;
	s_weapons_stay_box.generic.name	= "weapons stay";
	s_weapons_stay_box.generic.callback = DMFlagCallback;
	s_weapons_stay_box.itemnames = yes_no_names;
	s_weapons_stay_box.curvalue = ( dmflags & DF_WEAPONS_STAY ) != 0;

	s_instant_powerups_box.generic.type = MTYPE_SPINCONTROL;
	s_instant_powerups_box.generic.x	= 0;
	s_instant_powerups_box.generic.y	= y += 10;
	s_instant_powerups_box.generic.name	= "instant powerups";
	s_instant_powerups_box.generic.callback = DMFlagCallback;
	s_instant_powerups_box.itemnames = yes_no_names;
	s_instant_powerups_box.curvalue = ( dmflags & DF_INSTANT_ITEMS ) != 0;

	s_powerups_box.generic.type = MTYPE_SPINCONTROL;
	s_powerups_box.generic.x	= 0;
	s_powerups_box.generic.y	= y += 10;
	s_powerups_box.generic.name	= "allow powerups";
	s_powerups_box.generic.callback = DMFlagCallback;
	s_powerups_box.itemnames = yes_no_names;
	s_powerups_box.curvalue = ( dmflags & DF_NO_ITEMS ) == 0;

	s_health_box.generic.type = MTYPE_SPINCONTROL;
	s_health_box.generic.x	= 0;
	s_health_box.generic.y	= y += 10;
	s_health_box.generic.callback = DMFlagCallback;
	s_health_box.generic.name	= "allow health";
	s_health_box.itemnames = yes_no_names;
	s_health_box.curvalue = ( dmflags & DF_NO_HEALTH ) == 0;

	s_armor_box.generic.type = MTYPE_SPINCONTROL;
	s_armor_box.generic.x	= 0;
	s_armor_box.generic.y	= y += 10;
	s_armor_box.generic.name	= "allow armor";
	s_armor_box.generic.callback = DMFlagCallback;
	s_armor_box.itemnames = yes_no_names;
	s_armor_box.curvalue = ( dmflags & DF_NO_ARMOR ) == 0;

	s_spawn_farthest_box.generic.type = MTYPE_SPINCONTROL;
	s_spawn_farthest_box.generic.x	= 0;
	s_spawn_farthest_box.generic.y	= y += 10;
	s_spawn_farthest_box.generic.name	= "spawn farthest";
	s_spawn_farthest_box.generic.callback = DMFlagCallback;
	s_spawn_farthest_box.itemnames = yes_no_names;
	s_spawn_farthest_box.curvalue = ( dmflags & DF_SPAWN_FARTHEST ) != 0;

	s_samelevel_box.generic.type = MTYPE_SPINCONTROL;
	s_samelevel_box.generic.x	= 0;
	s_samelevel_box.generic.y	= y += 10;
	s_samelevel_box.generic.name	= "same map";
	s_samelevel_box.generic.callback = DMFlagCallback;
	s_samelevel_box.itemnames = yes_no_names;
	s_samelevel_box.curvalue = ( dmflags & DF_SAME_LEVEL ) != 0;

	s_force_respawn_box.generic.type = MTYPE_SPINCONTROL;
	s_force_respawn_box.generic.x	= 0;
	s_force_respawn_box.generic.y	= y += 10;
	s_force_respawn_box.generic.name	= "force respawn";
	s_force_respawn_box.generic.callback = DMFlagCallback;
	s_force_respawn_box.itemnames = yes_no_names;
	s_force_respawn_box.curvalue = ( dmflags & DF_FORCE_RESPAWN ) != 0;

	s_teamplay_box.generic.type = MTYPE_SPINCONTROL;
	s_teamplay_box.generic.x	= 0;
	s_teamplay_box.generic.y	= y += 10;
	s_teamplay_box.generic.name	= "teamplay";
	s_teamplay_box.generic.callback = DMFlagCallback;
	s_teamplay_box.itemnames = teamplay_names;

	s_allow_exit_box.generic.type = MTYPE_SPINCONTROL;
	s_allow_exit_box.generic.x	= 0;
	s_allow_exit_box.generic.y	= y += 10;
	s_allow_exit_box.generic.name	= "allow exit";
	s_allow_exit_box.generic.callback = DMFlagCallback;
	s_allow_exit_box.itemnames = yes_no_names;
	s_allow_exit_box.curvalue = ( dmflags & DF_ALLOW_EXIT ) != 0;

	s_infinite_ammo_box.generic.type = MTYPE_SPINCONTROL;
	s_infinite_ammo_box.generic.x	= 0;
	s_infinite_ammo_box.generic.y	= y += 10;
	s_infinite_ammo_box.generic.name	= "infinite ammo";
	s_infinite_ammo_box.generic.callback = DMFlagCallback;
	s_infinite_ammo_box.itemnames = yes_no_names;
	s_infinite_ammo_box.curvalue = ( dmflags & DF_INFINITE_AMMO ) != 0;

	s_fixed_fov_box.generic.type = MTYPE_SPINCONTROL;
	s_fixed_fov_box.generic.x	= 0;
	s_fixed_fov_box.generic.y	= y += 10;
	s_fixed_fov_box.generic.name	= "fixed FOV";
	s_fixed_fov_box.generic.callback = DMFlagCallback;
	s_fixed_fov_box.itemnames = yes_no_names;
	s_fixed_fov_box.curvalue = ( dmflags & DF_FIXED_FOV ) != 0;

	s_quad_drop_box.generic.type = MTYPE_SPINCONTROL;
	s_quad_drop_box.generic.x	= 0;
	s_quad_drop_box.generic.y	= y += 10;
	s_quad_drop_box.generic.name	= "quad drop";
	s_quad_drop_box.generic.callback = DMFlagCallback;
	s_quad_drop_box.itemnames = yes_no_names;
	s_quad_drop_box.curvalue = ( dmflags & DF_QUAD_DROP ) != 0;

	s_friendlyfire_box.generic.type = MTYPE_SPINCONTROL;
	s_friendlyfire_box.generic.x	= 0;
	s_friendlyfire_box.generic.y	= y += 10;
	s_friendlyfire_box.generic.name	= "friendly fire";
	s_friendlyfire_box.generic.callback = DMFlagCallback;
	s_friendlyfire_box.itemnames = yes_no_names;
	s_friendlyfire_box.curvalue = ( dmflags & DF_NO_FRIENDLY_FIRE ) == 0;

//============
//ROGUE
	if(Developer_searchpath() == 2)
	{
		s_no_mines_box.generic.type = MTYPE_SPINCONTROL;
		s_no_mines_box.generic.x	= 0;
		s_no_mines_box.generic.y	= y += 10;
		s_no_mines_box.generic.name	= "remove mines";
		s_no_mines_box.generic.callback = DMFlagCallback;
		s_no_mines_box.itemnames = yes_no_names;
		s_no_mines_box.curvalue = ( dmflags & DF_NO_MINES ) != 0;

		s_no_nukes_box.generic.type = MTYPE_SPINCONTROL;
		s_no_nukes_box.generic.x	= 0;
		s_no_nukes_box.generic.y	= y += 10;
		s_no_nukes_box.generic.name	= "remove nukes";
		s_no_nukes_box.generic.callback = DMFlagCallback;
		s_no_nukes_box.itemnames = yes_no_names;
		s_no_nukes_box.curvalue = ( dmflags & DF_NO_NUKES ) != 0;

		s_stack_double_box.generic.type = MTYPE_SPINCONTROL;
		s_stack_double_box.generic.x	= 0;
		s_stack_double_box.generic.y	= y += 10;
		s_stack_double_box.generic.name	= "2x/4x stacking off";
		s_stack_double_box.generic.callback = DMFlagCallback;
		s_stack_double_box.itemnames = yes_no_names;
		s_stack_double_box.curvalue = ( dmflags & DF_NO_STACK_DOUBLE ) != 0;

		s_no_spheres_box.generic.type = MTYPE_SPINCONTROL;
		s_no_spheres_box.generic.x	= 0;
		s_no_spheres_box.generic.y	= y += 10;
		s_no_spheres_box.generic.name	= "remove spheres";
		s_no_spheres_box.generic.callback = DMFlagCallback;
		s_no_spheres_box.itemnames = yes_no_names;
		s_no_spheres_box.curvalue = ( dmflags & DF_NO_SPHERES ) != 0;

	}
//ROGUE
//============

	Menu_AddItem( &s_dmoptions_menu, &s_falls_box );
	Menu_AddItem( &s_dmoptions_menu, &s_weapons_stay_box );
	Menu_AddItem( &s_dmoptions_menu, &s_instant_powerups_box );
	Menu_AddItem( &s_dmoptions_menu, &s_powerups_box );
	Menu_AddItem( &s_dmoptions_menu, &s_health_box );
	Menu_AddItem( &s_dmoptions_menu, &s_armor_box );
	Menu_AddItem( &s_dmoptions_menu, &s_spawn_farthest_box );
	Menu_AddItem( &s_dmoptions_menu, &s_samelevel_box );
	Menu_AddItem( &s_dmoptions_menu, &s_force_respawn_box );
	Menu_AddItem( &s_dmoptions_menu, &s_teamplay_box );
	Menu_AddItem( &s_dmoptions_menu, &s_allow_exit_box );
	Menu_AddItem( &s_dmoptions_menu, &s_infinite_ammo_box );
	Menu_AddItem( &s_dmoptions_menu, &s_fixed_fov_box );
	Menu_AddItem( &s_dmoptions_menu, &s_quad_drop_box );
	Menu_AddItem( &s_dmoptions_menu, &s_friendlyfire_box );

//=======
//ROGUE
	if(Developer_searchpath() == 2)
	{
		Menu_AddItem( &s_dmoptions_menu, &s_no_mines_box );
		Menu_AddItem( &s_dmoptions_menu, &s_no_nukes_box );
		Menu_AddItem( &s_dmoptions_menu, &s_stack_double_box );
		Menu_AddItem( &s_dmoptions_menu, &s_no_spheres_box );
	}
//ROGUE
//=======

	Menu_Center( &s_dmoptions_menu );

	// set the original dmflags statusbar
	DMFlagCallback( 0 );
	Menu_SetStatusBar( &s_dmoptions_menu, dmoptions_statusbar );
}

static void DMOptions_MenuDraw(void)
{
	Menu_Draw( &s_dmoptions_menu );
}

const char *DMOptions_MenuKey( int key )
{
	return Default_MenuKey( &s_dmoptions_menu, key );
}

static void M_Menu_DMOptions_f (void)
{
	DMOptions_MenuInit();
	M_PushMenu( DMOptions_MenuDraw, DMOptions_MenuKey );
}

/*
=============================================================================

DOWNLOADOPTIONS BOOK MENU

=============================================================================
*/
static menuframework_s s_downloadoptions_menu;

static menuseparator_s	s_download_title;
static menulist_s	s_allow_download_box;
static menulist_s	s_allow_download_maps_box;
static menulist_s	s_allow_download_models_box;
static menulist_s	s_allow_download_players_box;
static menulist_s	s_allow_download_sounds_box;

static void DownloadCallback( void *self )
{
	menulist_s *f = ( menulist_s * ) self;

	if (f == &s_allow_download_box)
	{
		Cvar_SetValue("allow_download", (float)f->curvalue);
	}

	else if (f == &s_allow_download_maps_box)
	{
		Cvar_SetValue("allow_download_maps", (float)f->curvalue);
	}

	else if (f == &s_allow_download_models_box)
	{
		Cvar_SetValue("allow_download_models", (float)f->curvalue);
	}

	else if (f == &s_allow_download_players_box)
	{
		Cvar_SetValue("allow_download_players", (float)f->curvalue);
	}

	else if (f == &s_allow_download_sounds_box)
	{
		Cvar_SetValue("allow_download_sounds", (float)f->curvalue);
	}
}

static void DownloadOptions_MenuInit( void )
{
	static const char *yes_no_names[] =
	{
		"no", "yes", 0
	};
	int y = 0;

	s_downloadoptions_menu.x = (int)(viddef.width * 0.50f);
	s_downloadoptions_menu.nitems = 0;

	s_download_title.generic.type = MTYPE_SEPARATOR;
	s_download_title.generic.name = "Download Options";
	s_download_title.generic.x    = 48;
	s_download_title.generic.y	 = y;

	s_allow_download_box.generic.type = MTYPE_SPINCONTROL;
	s_allow_download_box.generic.x	= 0;
	s_allow_download_box.generic.y	= y += 20;
	s_allow_download_box.generic.name	= "allow downloading";
	s_allow_download_box.generic.callback = DownloadCallback;
	s_allow_download_box.itemnames = yes_no_names;
	s_allow_download_box.curvalue = (Cvar_IntValue("allow_download") != 0);

	s_allow_download_maps_box.generic.type = MTYPE_SPINCONTROL;
	s_allow_download_maps_box.generic.x	= 0;
	s_allow_download_maps_box.generic.y	= y += 20;
	s_allow_download_maps_box.generic.name	= "maps";
	s_allow_download_maps_box.generic.callback = DownloadCallback;
	s_allow_download_maps_box.itemnames = yes_no_names;
	s_allow_download_maps_box.curvalue = (Cvar_IntValue("allow_download_maps") != 0);

	s_allow_download_players_box.generic.type = MTYPE_SPINCONTROL;
	s_allow_download_players_box.generic.x	= 0;
	s_allow_download_players_box.generic.y	= y += 10;
	s_allow_download_players_box.generic.name	= "player models/skins";
	s_allow_download_players_box.generic.callback = DownloadCallback;
	s_allow_download_players_box.itemnames = yes_no_names;
	s_allow_download_players_box.curvalue = (Cvar_IntValue("allow_download_players") != 0);

	s_allow_download_models_box.generic.type = MTYPE_SPINCONTROL;
	s_allow_download_models_box.generic.x	= 0;
	s_allow_download_models_box.generic.y	= y += 10;
	s_allow_download_models_box.generic.name	= "models";
	s_allow_download_models_box.generic.callback = DownloadCallback;
	s_allow_download_models_box.itemnames = yes_no_names;
	s_allow_download_models_box.curvalue = (Cvar_IntValue("allow_download_models") != 0);

	s_allow_download_sounds_box.generic.type = MTYPE_SPINCONTROL;
	s_allow_download_sounds_box.generic.x	= 0;
	s_allow_download_sounds_box.generic.y	= y += 10;
	s_allow_download_sounds_box.generic.name	= "sounds";
	s_allow_download_sounds_box.generic.callback = DownloadCallback;
	s_allow_download_sounds_box.itemnames = yes_no_names;
	s_allow_download_sounds_box.curvalue = (Cvar_IntValue("allow_download_sounds") != 0);

	Menu_AddItem( &s_downloadoptions_menu, &s_download_title );
	Menu_AddItem( &s_downloadoptions_menu, &s_allow_download_box );
	Menu_AddItem( &s_downloadoptions_menu, &s_allow_download_maps_box );
	Menu_AddItem( &s_downloadoptions_menu, &s_allow_download_players_box );
	Menu_AddItem( &s_downloadoptions_menu, &s_allow_download_models_box );
	Menu_AddItem( &s_downloadoptions_menu, &s_allow_download_sounds_box );

	Menu_Center( &s_downloadoptions_menu );

	// skip over title
	if (s_downloadoptions_menu.cursor == 0)
		s_downloadoptions_menu.cursor = 1;
}

static void DownloadOptions_MenuDraw(void)
{
	Menu_Draw( &s_downloadoptions_menu );
}

static const char *DownloadOptions_MenuKey( int key )
{
	return Default_MenuKey( &s_downloadoptions_menu, key );
}

static void M_Menu_DownloadOptions_f (void)
{
	DownloadOptions_MenuInit();
	M_PushMenu( DownloadOptions_MenuDraw, DownloadOptions_MenuKey );
}
/*
=============================================================================

ADDRESS BOOK MENU

=============================================================================
*/
#define NUM_ADDRESSBOOK_ENTRIES JOIN_ADDRESSBOOK_SLOTS

static menuframework_s	s_addressbook_menu;
static menufield_s		s_addressbook_fields[NUM_ADDRESSBOOK_ENTRIES];

static void AddressBook_MenuInit( void )
{
	int i;

	s_addressbook_menu.x = viddef.width / 2 - 142;
	s_addressbook_menu.y = viddef.height / 2 - 110;
	s_addressbook_menu.nitems = 0;

	for ( i = 0; i < NUM_ADDRESSBOOK_ENTRIES; i++ )
	{
		cvar_t *adr;
		char buffer[16];

		Com_sprintf( buffer, sizeof( buffer ), "adr%d", i );

		adr = Cvar_Get( buffer, "", CVAR_ARCHIVE );

		s_addressbook_fields[i].generic.type = MTYPE_FIELD;
		s_addressbook_fields[i].generic.name = 0;
		s_addressbook_fields[i].generic.callback = 0;
		s_addressbook_fields[i].generic.x		= 0;
		s_addressbook_fields[i].generic.y		= i * 12 + 0;
		s_addressbook_fields[i].generic.localdata[0] = i;
		s_addressbook_fields[i].cursor			= 0;
		s_addressbook_fields[i].length			= 60;
		s_addressbook_fields[i].visible_length	= 30;

		strncpy( s_addressbook_fields[i].buffer, adr->string, sizeof (s_addressbook_fields[i].buffer));

		Menu_AddItem( &s_addressbook_menu, &s_addressbook_fields[i] );
	}
}

const char *AddressBook_MenuKey( int key )
{
	if ( key == K_ESCAPE )
	{
		int index;
		char buffer[16];

		for ( index = 0; index < NUM_ADDRESSBOOK_ENTRIES; index++ )
		{
			Com_sprintf( buffer, sizeof( buffer ), "adr%d", index );
			Cvar_Set( buffer, s_addressbook_fields[index].buffer );
		}
	}
	return Default_MenuKey( &s_addressbook_menu, key );
}

static void AddressBook_MenuDraw(void)
{
	M_Banner( "m_banner_addressbook" );
	Menu_Draw( &s_addressbook_menu );
}

static void M_Menu_AddressBook_f(void)
{
	AddressBook_MenuInit();
	M_PushMenu( AddressBook_MenuDraw, AddressBook_MenuKey );
}

/*
=============================================================================

PLAYER CONFIG MENU

=============================================================================
*/
static menuframework_s	s_player_config_menu;
static menufield_s		s_player_name_field;
static menulist_s		s_player_model_box;
static menulist_s		s_player_skin_box;
static menulist_s		s_player_handedness_box;
static menulist_s		s_player_rate_box;
static menuseparator_s	s_player_skin_title;
static menuseparator_s	s_player_model_title;
static menuseparator_s	s_player_hand_title;
static menuseparator_s	s_player_rate_title;
static menuaction_s		s_player_download_action;

#define MAX_DISPLAYNAME 16
#define MAX_PLAYERMODELS 1024

typedef struct
{
	int		nskins;
	char	**skindisplaynames;
	char	displayname[MAX_DISPLAYNAME];
	char	directory[MAX_QPATH];
} playermodelinfo_s;

static playermodelinfo_s s_pmi[MAX_PLAYERMODELS];
static char *s_pmnames[MAX_PLAYERMODELS];
static int s_numplayermodels;

static int rate_tbl[] = { 2500, 3200, 4300, 5000, 15000, 0 };
static const char *rate_names[] = { "28.8 Modem", "33.6 Modem", "56.6 Modem", "ISDN",
	"Cable/DSL/LAN", "User defined", 0 };

static void DownloadOptionsFunc( void *self )
{
	M_Menu_DownloadOptions_f();
}

static void HandednessCallback( void *unused )
{
	Cvar_SetValue( "hand", (float)s_player_handedness_box.curvalue );
}

static void RateCallback( void *unused )
{
	if (s_player_rate_box.curvalue != sizeof(rate_tbl) / sizeof(*rate_tbl) - 1)
		Cvar_SetValue( "rate", (float)rate_tbl[s_player_rate_box.curvalue] );
}

static void ModelCallback( void *unused )
{
	s_player_skin_box.itemnames = (const char **)s_pmi[s_player_model_box.curvalue].skindisplaynames;
	s_player_skin_box.curvalue = 0;
}

static void FreeFileList( char **list, int n )
{
	int i;

	for ( i = 0; i < n; i++ )
	{
		if ( list[i] )
		{
			free( list[i] );
			list[i] = 0;
		}
	}
	free( list );
}

static qboolean IconOfSkinExists( char *skin, char **pcxfiles, int npcxfiles )
{
	int i;
	char scratch[1024];

	Q_strncpy( scratch, skin, sizeof(scratch)-7);
	*strrchr( scratch, '.' ) = 0;
	strcat( scratch, "_i.pcx" );

	for ( i = 0; i < npcxfiles; i++ )
	{
		if ( strcmp( pcxfiles[i], scratch ) == 0 )
			return true;
	}

	return false;
}

extern char **FS_ListFiles( char *, int *, unsigned, unsigned );
static qboolean PlayerConfig_ScanDirectories( void )
{
	char findname[1024];
	char scratch[1024];
	int ndirs = 0, npms = 0;
	char **dirnames;
	char *path = NULL;
	int i;

	s_numplayermodels = 0;

	/*
	** get a list of directories
	*/
	do 
	{
		path = FS_NextPath( path );
		Com_sprintf( findname, sizeof(findname), "%s/players/*.*", path );

		if ( ( dirnames = FS_ListFiles( findname, &ndirs, SFF_SUBDIR, 0 ) ) != 0 )
			break;
	} while ( path );

	if ( !dirnames )
		return false;

	/*
	** go through the subdirectories
	*/
	npms = ndirs;
	if ( npms > MAX_PLAYERMODELS )
		npms = MAX_PLAYERMODELS;

	//jesus christ this is fucking ugly
	for ( i = 0; i < npms; i++ )
	{
		int k, s;
		char *a, *b, *c;
		char **pcxnames;
		char **skinnames;
		int npcxfiles;
		int nskins = 0;

		if ( dirnames[i] == 0 )
			continue;

		// verify the existence of tris.md2
		Q_strncpy( scratch, dirnames[i], sizeof(scratch)-10);
		strcat( scratch, "/tris.md2" );

		if ( !Sys_FindFirst( scratch, 0, SFF_SUBDIR | SFF_HIDDEN | SFF_SYSTEM ) )
		{
			free( dirnames[i] );
			dirnames[i] = 0;
			Sys_FindClose();
			continue;
		}
		Sys_FindClose();

		// verify the existence of at least one pcx skin
		Q_strncpy( scratch, dirnames[i], sizeof(scratch)-10);
		strcat( scratch, "/*.pcx" );
		pcxnames = FS_ListFiles( scratch, &npcxfiles, 0, SFF_SUBDIR | SFF_HIDDEN | SFF_SYSTEM );

		if ( !pcxnames )
		{
			free( dirnames[i] );
			dirnames[i] = 0;
			continue;
		}

		// count valid skins, which consist of a skin with a matching "_i" icon
		for ( k = 0; k < npcxfiles-1; k++ )
		{
			if ( !strstr( pcxnames[k], "_i.pcx" ) )
			{
				if ( IconOfSkinExists( pcxnames[k], pcxnames, npcxfiles - 1 ) )
				{
					nskins++;
				}
			}
		}

		if ( !nskins )
			continue;

		skinnames = malloc( sizeof( char * ) * ( nskins + 1 ) );
		memset( skinnames, 0, sizeof( char * ) * ( nskins + 1 ) );

		// copy the valid skins
		for ( s = 0, k = 0; k < npcxfiles-1; k++ )
		{
			char *a, *b, *c;

			if ( !strstr( pcxnames[k], "_i.pcx" ) )
			{
				if ( IconOfSkinExists( pcxnames[k], pcxnames, npcxfiles - 1 ) )
				{
					a = strrchr( pcxnames[k], '/' );
					b = strrchr( pcxnames[k], '\\' );

					if ( a > b )
						c = a;
					else
						c = b;

					Q_strncpy( scratch, c + 1, sizeof(scratch)-1);

					if ( strrchr( scratch, '.' ) )
						*strrchr( scratch, '.' ) = 0;

					skinnames[s] = strdup( scratch );
					s++;
				}
			}
		}

		// at this point we have a valid player model
		s_pmi[s_numplayermodels].nskins = nskins;
		s_pmi[s_numplayermodels].skindisplaynames = skinnames;

		// make short name for the model
		a = strrchr( dirnames[i], '/' );
		b = strrchr( dirnames[i], '\\' );

		if ( a > b )
			c = a;
		else
			c = b;

		strncpy( s_pmi[s_numplayermodels].displayname, c + 1, MAX_DISPLAYNAME-1);
		strncpy( s_pmi[s_numplayermodels].directory, c + 1, MAX_QPATH-1);

		FreeFileList( pcxnames, npcxfiles );

		if (++s_numplayermodels == MAX_PLAYERMODELS)
			break;
	}
	if ( dirnames )
		FreeFileList( dirnames, ndirs );

	//shut up
	return false;
}

static int EXPORT pmicmpfnc( const void *_a, const void *_b )
{
	const playermodelinfo_s *a = ( const playermodelinfo_s * ) _a;
	const playermodelinfo_s *b = ( const playermodelinfo_s * ) _b;

	/*
	** sort by male, female, then alphabetical
	*/
	if ( strcmp( a->directory, "male" ) == 0 )
		return -1;
	else if ( strcmp( b->directory, "male" ) == 0 )
		return 1;

	if ( strcmp( a->directory, "female" ) == 0 )
		return -1;
	else if ( strcmp( b->directory, "female" ) == 0 )
		return 1;

	return strcmp( a->directory, b->directory );
}


static qboolean PlayerConfig_MenuInit( void )
{
	//extern cvar_t *name;
	//extern cvar_t *skin;

	char currentdirectory[1024];
	char currentskin[1024];
	int i = 0;
	int	hand;

	int currentdirectoryindex = 0;
	int currentskinindex = 0;

	//cvar_t *hand = Cvar_Get( "hand", "0", CVAR_USERINFO | CVAR_ARCHIVE );

	static const char *handedness[] = { "right", "left", "center", 0 };

	PlayerConfig_ScanDirectories();

	if (s_numplayermodels == 0)
		return false;

	hand = Cvar_IntValue ("hand");

	if ( hand < 0 || hand > 2)
		Cvar_Set( "hand", "0" );

	//r1: BUFFER OVERFLOW WAS HERE
	Q_strncpy( currentdirectory, Cvar_VariableString ("skin"), sizeof(currentdirectory)-1);

	if ( strchr( currentdirectory, '/' ) )
	{
		strcpy( currentskin, strchr( currentdirectory, '/' ) + 1 );
		*strchr( currentdirectory, '/' ) = 0;
	}
	else if ( strchr( currentdirectory, '\\' ) )
	{
		strcpy( currentskin, strchr( currentdirectory, '\\' ) + 1 );
		*strchr( currentdirectory, '\\' ) = 0;
	}
	else
	{
		strcpy( currentdirectory, "male" );
		strcpy( currentskin, "grunt" );
	}

	qsort( s_pmi, s_numplayermodels, sizeof( s_pmi[0] ), (int (EXPORT *)(const void *, const void *))pmicmpfnc );

	memset( s_pmnames, 0, sizeof( s_pmnames ) );
	for ( i = 0; i < s_numplayermodels; i++ )
	{
		s_pmnames[i] = s_pmi[i].displayname;
		if ( Q_stricmp( s_pmi[i].directory, currentdirectory ) == 0 )
		{
			int j;

			currentdirectoryindex = i;

			for ( j = 0; j < s_pmi[i].nskins; j++ )
			{
				if ( Q_stricmp( s_pmi[i].skindisplaynames[j], currentskin ) == 0 )
				{
					currentskinindex = j;
					break;
				}
			}
		}
	}

	{
		float scale = SCR_GetMenuScale();
		s_player_config_menu.x = viddef.width / 2 - (int)(95 * scale);
		s_player_config_menu.y = viddef.height / 2 - (int)(97 * scale);
	}
	s_player_config_menu.nitems = 0;

	s_player_name_field.generic.type = MTYPE_FIELD;
	s_player_name_field.generic.name = "name";
	s_player_name_field.generic.callback = 0;
	s_player_name_field.generic.x		= 0;
	s_player_name_field.generic.y		= 0;
	s_player_name_field.length	= 20;
	s_player_name_field.visible_length = 20;
	strncpy( s_player_name_field.buffer, Cvar_VariableString ("name"), sizeof(s_player_name_field.buffer)-1);
	s_player_name_field.cursor = (int)strlen( s_player_name_field.buffer );

	s_player_model_title.generic.type = MTYPE_SEPARATOR;
	s_player_model_title.generic.name = "model";
	s_player_model_title.generic.x    = -8;
	s_player_model_title.generic.y	 = 60;

	s_player_model_box.generic.type = MTYPE_SPINCONTROL;
	s_player_model_box.generic.x	= -56;
	s_player_model_box.generic.y	= 70;
	s_player_model_box.generic.callback = ModelCallback;
	s_player_model_box.generic.cursor_offset = -48;
	s_player_model_box.curvalue = currentdirectoryindex;
	s_player_model_box.itemnames = (const char **)s_pmnames;

	s_player_skin_title.generic.type = MTYPE_SEPARATOR;
	s_player_skin_title.generic.name = "skin";
	s_player_skin_title.generic.x    = -16;
	s_player_skin_title.generic.y	 = 84;

	s_player_skin_box.generic.type = MTYPE_SPINCONTROL;
	s_player_skin_box.generic.x	= -56;
	s_player_skin_box.generic.y	= 94;
	s_player_skin_box.generic.name	= 0;
	s_player_skin_box.generic.callback = 0;
	s_player_skin_box.generic.cursor_offset = -48;
	s_player_skin_box.curvalue = currentskinindex;
	s_player_skin_box.itemnames = (const char **)s_pmi[currentdirectoryindex].skindisplaynames;

	s_player_hand_title.generic.type = MTYPE_SEPARATOR;
	s_player_hand_title.generic.name = "handedness";
	s_player_hand_title.generic.x    = 32;
	s_player_hand_title.generic.y	 = 108;

	s_player_handedness_box.generic.type = MTYPE_SPINCONTROL;
	s_player_handedness_box.generic.x	= -56;
	s_player_handedness_box.generic.y	= 118;
	s_player_handedness_box.generic.name	= 0;
	s_player_handedness_box.generic.cursor_offset = -48;
	s_player_handedness_box.generic.callback = HandednessCallback;
	s_player_handedness_box.curvalue = Cvar_IntValue( "hand" );
	s_player_handedness_box.itemnames = handedness;

	for (i = 0; i < sizeof(rate_tbl) / sizeof(*rate_tbl) - 1; i++)
		if (Cvar_IntValue("rate") == rate_tbl[i])
			break;

	s_player_rate_title.generic.type = MTYPE_SEPARATOR;
	s_player_rate_title.generic.name = "connect speed";
	s_player_rate_title.generic.x    = 56;
	s_player_rate_title.generic.y	 = 156;

	s_player_rate_box.generic.type = MTYPE_SPINCONTROL;
	s_player_rate_box.generic.x	= -56;
	s_player_rate_box.generic.y	= 166;
	s_player_rate_box.generic.name	= 0;
	s_player_rate_box.generic.cursor_offset = -48;
	s_player_rate_box.generic.callback = RateCallback;
	s_player_rate_box.curvalue = i;
	s_player_rate_box.itemnames = rate_names;

	s_player_download_action.generic.type = MTYPE_ACTION;
	s_player_download_action.generic.name	= "download options";
	s_player_download_action.generic.flags= QMF_LEFT_JUSTIFY;
	s_player_download_action.generic.x	= -24;
	s_player_download_action.generic.y	= 186;
	s_player_download_action.generic.statusbar = NULL;
	s_player_download_action.generic.callback = DownloadOptionsFunc;

	Menu_AddItem( &s_player_config_menu, &s_player_name_field );
	Menu_AddItem( &s_player_config_menu, &s_player_model_title );
	Menu_AddItem( &s_player_config_menu, &s_player_model_box );
	if ( s_player_skin_box.itemnames )
	{
		Menu_AddItem( &s_player_config_menu, &s_player_skin_title );
		Menu_AddItem( &s_player_config_menu, &s_player_skin_box );
	}
	Menu_AddItem( &s_player_config_menu, &s_player_hand_title );
	Menu_AddItem( &s_player_config_menu, &s_player_handedness_box );
	Menu_AddItem( &s_player_config_menu, &s_player_rate_title );
	Menu_AddItem( &s_player_config_menu, &s_player_rate_box );
	Menu_AddItem( &s_player_config_menu, &s_player_download_action );

	return true;
}

extern float CalcFov( float fov_x, int w, int h );
static void PlayerConfig_MenuDraw( void )
{
	refdef_t refdef;
	char scratch[MAX_QPATH];

	float scale = SCR_GetMenuScale();

	memset( &refdef, 0, sizeof( refdef ) );

	/* Scale preview viewport with menu scale so it stays clear of the name field */
	refdef.x = viddef.width / 2;
	refdef.y = viddef.height / 2 - (int)(72 * scale);
	refdef.width = (int)(144 * scale);
	refdef.height = (int)(168 * scale);
	refdef.fov_x = 40;
	refdef.fov_y = CalcFov( refdef.fov_x, refdef.width, refdef.height );
	refdef.time = cls.realtime*0.001f;

	if ( s_pmi[s_player_model_box.curvalue].skindisplaynames )
	{
		static int yaw;
		//int maxframe = 29;
		entity_t entity;

		memset( &entity, 0, sizeof( entity ) );

		Com_sprintf( scratch, sizeof( scratch ), "players/%s/tris.md2", s_pmi[s_player_model_box.curvalue].directory );
		entity.model = re.RegisterModel( scratch );
		Com_sprintf( scratch, sizeof( scratch ), "players/%s/%s.pcx", s_pmi[s_player_model_box.curvalue].directory, s_pmi[s_player_model_box.curvalue].skindisplaynames[s_player_skin_box.curvalue] );
		entity.skin = re.RegisterSkin( scratch );
		entity.flags = RF_FULLBRIGHT;
		entity.origin[0] = 80;
		entity.origin[1] = 0;
		entity.origin[2] = 0;
		FastVectorCopy( entity.origin, entity.oldorigin );
		entity.frame = 0;
		entity.oldframe = 0;
		entity.backlerp = 0.0;
		entity.angles[1] = (float)yaw;
		yaw++;
		if ( ++yaw > 360 )
			yaw -= 360;

		refdef.areabits = 0;
		refdef.num_entities = 1;
		refdef.entities = &entity;
		refdef.lightstyles = 0;
		refdef.rdflags = RDF_NOWORLDMODEL;

		Menu_Draw( &s_player_config_menu );

		/* Virtual 320 coords + M_DrawCharacter scale must match scaled refdef pixels */
		M_DrawTextBox( (int)(( refdef.x ) * ( 320.0F / viddef.width ) - 8),
			(int)(( viddef.height / 2 ) * ( 240.0F / viddef.height) - 77),
			(int)(refdef.width / (8 * scale)), (int)(refdef.height / (8 * scale)) );
		refdef.height += 4;

		re.RenderFrame( &refdef );

		Com_sprintf( scratch, sizeof( scratch ), "/players/%s/%s_i.pcx", 
			s_pmi[s_player_model_box.curvalue].directory,
			s_pmi[s_player_model_box.curvalue].skindisplaynames[s_player_skin_box.curvalue] );
		{
			int iw, ih;
			re.DrawGetPicSize( &iw, &ih, scratch );
			re.DrawStretchPic( s_player_config_menu.x - (int)(40 * scale), refdef.y,
				(int)(iw * scale), (int)(ih * scale), scratch );
		}
	}
}

static const char *PlayerConfig_MenuKey (int key)
{
	int i;

	if ( key == K_ESCAPE )
	{
		char scratch[1024];

		Cvar_Set( "name", s_player_name_field.buffer );

		Com_sprintf( scratch, sizeof( scratch ), "%s/%s", 
			s_pmi[s_player_model_box.curvalue].directory, 
			s_pmi[s_player_model_box.curvalue].skindisplaynames[s_player_skin_box.curvalue] );

		Cvar_Set( "skin", scratch );

		for ( i = 0; i < s_numplayermodels; i++ )
		{
			int j;

			for ( j = 0; j < s_pmi[i].nskins; j++ )
			{
				if ( s_pmi[i].skindisplaynames[j] )
					free( s_pmi[i].skindisplaynames[j] );
				s_pmi[i].skindisplaynames[j] = 0;
			}
			free( s_pmi[i].skindisplaynames );
			s_pmi[i].skindisplaynames = 0;
			s_pmi[i].nskins = 0;
		}
	}
	return Default_MenuKey( &s_player_config_menu, key );
}


static void M_Menu_PlayerConfig_f (void)
{
	if (!PlayerConfig_MenuInit())
	{
		Menu_SetStatusBar( &s_multiplayer_menu, "No valid player models found" );
		return;
	}
	Menu_SetStatusBar( &s_multiplayer_menu, NULL );
	M_PushMenu( PlayerConfig_MenuDraw, PlayerConfig_MenuKey );
}


/*
=======================================================================

GALLERY MENU

=======================================================================
*/

static void M_Menu_Quit_f (void)
{
	CL_Quit_f ();
}



//=============================================================================
/* Menu Subsystem */


/*
=================
M_Init
=================
*/
void M_Init (void)
{
	Cmd_AddCommand ("menu_main", M_Menu_Main_f);
	Cmd_AddCommand ("menu_game", M_Menu_Game_f);
		Cmd_AddCommand ("menu_loadgame", M_Menu_LoadGame_f);
		Cmd_AddCommand ("menu_savegame", M_Menu_SaveGame_f);
		Cmd_AddCommand ("menu_joinserver", M_Menu_JoinServer_f);
			Cmd_AddCommand ("menu_addressbook", M_Menu_AddressBook_f);
		Cmd_AddCommand ("menu_startserver", M_Menu_StartServer_f);
			Cmd_AddCommand ("menu_dmoptions", M_Menu_DMOptions_f);
		Cmd_AddCommand ("menu_playerconfig", M_Menu_PlayerConfig_f);
			Cmd_AddCommand ("menu_downloadoptions", M_Menu_DownloadOptions_f);
		Cmd_AddCommand ("menu_credits", M_Menu_Credits_f );
	Cmd_AddCommand ("menu_multiplayer", M_Menu_Multiplayer_f );
	Cmd_AddCommand ("menu_video", M_Menu_Video_f);
	Cmd_AddCommand ("menu_options", M_Menu_Options_f);
		Cmd_AddCommand ("menu_r1q2", M_Menu_R1Q2_f);
		Cmd_AddCommand ("menu_keys", M_Menu_Keys_f);
	Cmd_AddCommand ("menu_quit", M_Menu_Quit_f);
}


/*
=================
M_Draw
=================
*/
void M_Draw (void)
{
	float scale;

	if (cls.key_dest != key_menu)
		return;

	// repaint everything next frame
	SCR_DirtyScreen ();

	// dim everything behind it down
#ifdef CINEMATICS
	if (cl.cinematictime > 0)
		re.DrawFill (0,0,viddef.width, viddef.height, 0);
	else
#endif
		re.DrawFadeScreen ();

	scale = SCR_GetMenuScale();
	Cvar_SetValue( "gl_fontscale", scale );

	/* Fresh absolute cursor before hover hit-tests in draw funcs */
	IN_UpdateMenuMouse();

	m_drawfunc ();

	Cvar_SetValue( "gl_fontscale", 1 );

	// delay playing the enter sound until after the
	// menu has been drawn, to avoid delay while
	// caching images
	if (m_entersound)
	{
		S_StartLocalSound( menu_in_sound );
		m_entersound = false;
	}
}


/*
=================
M_Keydown
=================
*/
void M_Keydown (int key)
{
	const char *s;

	if (m_keyfunc)
		if ( ( s = m_keyfunc( key ) ) != 0 )
			S_StartLocalSound( ( char * ) s );
}


