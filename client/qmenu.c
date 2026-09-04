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
#include "client.h"
#include "qmenu.h"

static void	 Action_DoEnter( menuaction_s *a );
static void	 Action_Draw( menuaction_s *a );
static void  Menu_DrawStatusBar( const char *string );
//static void	 Menulist_DoEnter( menulist_s *l );
static void	 MenuList_Draw( menulist_s *l );
static void	 Separator_Draw( menuseparator_s *s );
static void	 Slider_DoSlide( menuslider_s *s, int dir );
static void	 Slider_Draw( menuslider_s *s );
//static void	 SpinControl_DoEnter( menulist_s *s );
static void	 SpinControl_Draw( menulist_s *s );
static void	 SpinControl_DoSlide( menulist_s *s, int dir );

#define RCOLUMN_OFFSET  16
#define LCOLUMN_OFFSET -16

extern refexport_t re;
extern viddef_t viddef;

#define VID_WIDTH viddef.width
#define VID_HEIGHT viddef.height

#define Draw_Char re.DrawChar
#define Draw_Fill re.DrawFill

static float Menu_Scale (void)
{
	return SCR_GetMenuScale();
}

static void Menu_ScaledXY (const menucommon_s *item, int *ox, int *oy)
{
	float s = Menu_Scale();
	*ox = item->parent->x + (int)(item->x * s);
	*oy = item->parent->y + (int)(item->y * s);
}

void Action_DoEnter( menuaction_s *a )
{
	if ( a->generic.callback )
		a->generic.callback( a );
}

void Action_Draw( menuaction_s *a )
{
	int x, y;
	float s = Menu_Scale();
	Menu_ScaledXY( &a->generic, &x, &y );

	if ( a->generic.flags & QMF_LEFT_JUSTIFY )
	{
		if ( a->generic.flags & QMF_GRAYED )
			Menu_DrawStringDark( x + (int)(LCOLUMN_OFFSET * s), y, a->generic.name );
		else
			Menu_DrawString( x + (int)(LCOLUMN_OFFSET * s), y, a->generic.name );
	}
	else
	{
		if ( a->generic.flags & QMF_GRAYED )
			Menu_DrawStringR2LDark( x + (int)(LCOLUMN_OFFSET * s), y, a->generic.name );
		else
			Menu_DrawStringR2L( x + (int)(LCOLUMN_OFFSET * s), y, a->generic.name );
	}
	if ( a->generic.ownerdraw )
		a->generic.ownerdraw( a );
}

qboolean Field_DoEnter( menufield_s *f )
{
	if ( f->generic.callback )
	{
		f->generic.callback( f );
		return true;
	}
	return false;
}

void Field_Draw( menufield_s *f )
{
	int i, x, y, step;
	char tempbuffer[128]="";
	float s = Menu_Scale();

	Menu_ScaledXY( &f->generic, &x, &y );
	step = (int)(8 * s);

	if ( f->generic.name )
		Menu_DrawStringR2LDark( x + (int)(LCOLUMN_OFFSET * s), y, f->generic.name );

	strncpy( tempbuffer, f->buffer + f->visible_offset, f->visible_length );

	Draw_Char( x + 2 * step, y - (int)(4 * s), 18 );
	Draw_Char( x + 2 * step, y + (int)(4 * s), 24 );

	Draw_Char( x + 3 * step + f->visible_length * step, y - (int)(4 * s), 20 );
	Draw_Char( x + 3 * step + f->visible_length * step, y + (int)(4 * s), 26 );

	for ( i = 0; i < f->visible_length; i++ )
	{
		Draw_Char( x + 3 * step + i * step, y - (int)(4 * s), 19 );
		Draw_Char( x + 3 * step + i * step, y + (int)(4 * s), 25 );
	}

	Menu_DrawString( x + 3 * step, y, tempbuffer );

	if ( Menu_ItemAtCursor( f->generic.parent ) == f )
	{
		int offset;

		if ( f->visible_offset )
			offset = f->visible_length;
		else
			offset = f->cursor;

		if ( ( ( int ) ( Sys_Milliseconds() / 250 ) ) & 1 )
		{
			Draw_Char( x + ( offset + 3 ) * step,
					   y,
					   11 );
		}
		else
		{
			Draw_Char( x + ( offset + 3 ) * step,
					   y,
					   ' ' );
		}
	}
}

extern int keydown[];

qboolean Field_Key( menufield_s *f, int key )
{
	switch ( key )
	{
	case K_KP_SLASH:
		key = '/';
		break;
	case K_KP_MINUS:
		key = '-';
		break;
	case K_KP_PLUS:
		key = '+';
		break;
	case K_KP_HOME:
		key = '7';
		break;
	case K_KP_UPARROW:
		key = '8';
		break;
	case K_KP_PGUP:
		key = '9';
		break;
	case K_KP_LEFTARROW:
		key = '4';
		break;
	case K_KP_5:
		key = '5';
		break;
	case K_KP_RIGHTARROW:
		key = '6';
		break;
	case K_KP_END:
		key = '1';
		break;
	case K_KP_DOWNARROW:
		key = '2';
		break;
	case K_KP_PGDN:
		key = '3';
		break;
	case K_KP_INS:
		key = '0';
		break;
	case K_KP_DEL:
		key = '.';
		break;
	}

	if ( key > 127 )
	{
		switch ( key )
		{
		case K_DEL:
		default:
			return false;
		}
	}

	/*
	** support pasting from the clipboard
	*/
	if ( ( toupper( key ) == 'V' && keydown[K_CTRL] ) ||
		 ( ( ( key == K_INS ) || ( key == K_KP_INS ) ) && keydown[K_SHIFT] ) )
	{
		char *cbd;
		
		if ( ( cbd = Sys_GetClipboardData() ) != 0 )
		{
			strtok( cbd, "\n\r\b" );

			Q_strncpy (f->buffer, cbd, sizeof(f->buffer)-1);
			f->cursor = (int)strlen( f->buffer );
			f->visible_offset = f->cursor - f->visible_length;
			if ( f->visible_offset < 0 )
				f->visible_offset = 0;

			free( cbd );
		}
		return true;
	}

	switch ( key )
	{
	case K_KP_LEFTARROW:
	case K_LEFTARROW:
	case K_BACKSPACE:
		if ( f->cursor > 0 )
		{
			memmove( &f->buffer[f->cursor-1], &f->buffer[f->cursor], strlen( &f->buffer[f->cursor] ) + 1 );
			f->cursor--;

			if ( f->visible_offset )
			{
				f->visible_offset--;
			}
		}
		break;

	case K_KP_DEL:
	case K_DEL:
		memmove( &f->buffer[f->cursor], &f->buffer[f->cursor+1], strlen( &f->buffer[f->cursor+1] ) + 1 );
		break;

	case K_KP_ENTER:
	case K_ENTER:
	case K_ESCAPE:
	case K_TAB:
		return false;

	case K_SPACE:
	default:
		if (( f->generic.flags & QMF_NUMBERSONLY ) && !isdigit( key ) && key != '.' )
			return false;

		if ( f->cursor < f->length )
		{
			f->buffer[f->cursor++] = key;
			f->buffer[f->cursor] = 0;

			if ( f->cursor > f->visible_length )
			{
				f->visible_offset++;
			}
		}
	}

	return true;
}

void Menu_AddItem( menuframework_s *menu, void *item )
{
	if ( menu->nitems == 0 )
		menu->nslots = 0;

	if ( menu->nitems < MAXMENUITEMS )
	{
		menu->items[menu->nitems] = item;
		( ( menucommon_s * ) menu->items[menu->nitems] )->parent = menu;
		menu->nitems++;
	}

	menu->nslots = Menu_TallySlots( menu );
}

/*
** Menu_AdjustCursor
**
** This function takes the given menu, the direction, and attempts
** to adjust the menu's cursor so that it's at the next available
** slot.
*/
void Menu_AdjustCursor( menuframework_s *m, int dir )
{
	menucommon_s *citem;

	/*
	** see if it's in a valid spot
	*/
	if ( m->cursor >= 0 && m->cursor < m->nitems )
	{
		if ( ( citem = Menu_ItemAtCursor( m ) ) != 0 )
		{
			if ( citem->type != MTYPE_SEPARATOR )
				return;
		}
	}

	/*
	** it's not in a valid spot, so crawl in the direction indicated until we
	** find a valid spot
	*/
	if ( dir == 1 )
	{
		for (;;)
		{
			citem = Menu_ItemAtCursor( m );
			if ( citem )
				if ( citem->type != MTYPE_SEPARATOR )
					break;
			m->cursor += dir;
			if ( m->cursor >= m->nitems )
				m->cursor = 0;
		}
	}
	else
	{
		for (;;)
		{
			citem = Menu_ItemAtCursor( m );
			if ( citem )
				if ( citem->type != MTYPE_SEPARATOR )
					break;
			m->cursor += dir;
			if ( m->cursor < 0 )
				m->cursor = m->nitems - 1;
		}
	}
}

void Menu_Center( menuframework_s *menu )
{
	int height;
	float s = Menu_Scale();

	height = ( ( menucommon_s * ) menu->items[menu->nitems-1])->y;
	height += 10;

	menu->y = ( VID_HEIGHT - (int)(height * s) ) / 2;
}

void Menu_UpdateCursorFromMouse( menuframework_s *menu )
{
	int i;
	menucommon_s *item;
	float s = Menu_Scale();
	int x0, x1;

	if ( !menu || !menu_mouse_valid )
		return;

	/* Parent menu X span (classic 320-wide UI centered on menu->x) */
	x0 = menu->x - (int)(160 * s);
	x1 = menu->x + (int)(160 * s);

	for ( i = 0; i < menu->nitems; i++ )
	{
		int iy, dummyx;
		item = ( menucommon_s * ) menu->items[i];
		if ( !item || item->type == MTYPE_SEPARATOR )
			continue;
		Menu_ScaledXY( item, &dummyx, &iy );
		if ( menu_mouse_y >= iy && menu_mouse_y < iy + (int)(10 * s)
			&& menu_mouse_x >= x0 && menu_mouse_x < x1 )
		{
			menu->cursor = i;
			break;
		}
	}
}

void Menu_Draw( menuframework_s *menu )
{
	int i;
	menucommon_s *item;
	float s = Menu_Scale();

	Menu_UpdateCursorFromMouse( menu );

	/*
	** draw contents
	*/
	for ( i = 0; i < menu->nitems; i++ )
	{
		switch ( ( ( menucommon_s * ) menu->items[i] )->type )
		{
		case MTYPE_FIELD:
			Field_Draw( ( menufield_s * ) menu->items[i] );
			break;
		case MTYPE_SLIDER:
			Slider_Draw( ( menuslider_s * ) menu->items[i] );
			break;
		case MTYPE_LIST:
			MenuList_Draw( ( menulist_s * ) menu->items[i] );
			break;
		case MTYPE_SPINCONTROL:
			SpinControl_Draw( ( menulist_s * ) menu->items[i] );
			break;
		case MTYPE_ACTION:
			Action_Draw( ( menuaction_s * ) menu->items[i] );
			break;
		case MTYPE_SEPARATOR:
			Separator_Draw( ( menuseparator_s * ) menu->items[i] );
			break;
		}
	}

	item = Menu_ItemAtCursor( menu );

	if ( item && item->cursordraw )
	{
		item->cursordraw( item );
	}
	else if ( menu->cursordraw )
	{
		menu->cursordraw( menu );
	}
	else if ( item && item->type != MTYPE_FIELD )
	{
		{
			int cx, cy;
			Menu_ScaledXY( item, &cx, &cy );
			if ( item->flags & QMF_LEFT_JUSTIFY )
				Draw_Char( cx - (int)(24 * s) + (int)(item->cursor_offset * s), cy, 12 + ( ( int ) ( Sys_Milliseconds()/250 ) & 1 ) );
			else
				Draw_Char( menu->x + (int)(item->cursor_offset * s), cy, 12 + ( ( int ) ( Sys_Milliseconds()/250 ) & 1 ) );
		}
	}

	if ( item )
	{
		if ( item->statusbarfunc )
			item->statusbarfunc( ( void * ) item );
		else if ( item->statusbar )
			Menu_DrawStatusBar( item->statusbar );
		else
			Menu_DrawStatusBar( menu->statusbar );

	}
	else
	{
		Menu_DrawStatusBar( menu->statusbar );
	}
}

void Menu_DrawStatusBar( const char *string )
{
	if ( string )
	{
		int l = (int)strlen( string );
		//int maxrow = VID_HEIGHT / 8;
		int maxcol = VID_WIDTH / 8;
		int col = maxcol / 2 - l / 2;

		{
			float sb = Menu_Scale();
			int bh = (int)(8 * sb);
			Draw_Fill( 0, VID_HEIGHT - bh, VID_WIDTH, bh, 4 );
			Menu_DrawString( (int)(col * 8 * sb), VID_HEIGHT - bh, string );
		}
	}
	else
	{
		float sb = Menu_Scale();
		int bh = (int)(8 * sb);
		Draw_Fill( 0, VID_HEIGHT - bh, VID_WIDTH, bh, 0 );
	}
}

void Menu_DrawString( int x, int y, const char *string )
{
	uint32 i;
	float s = Menu_Scale();
	int step = (int)(8 * s);

	for ( i = 0; i < strlen( string ); i++ )
	{
		Draw_Char( x + (int)i * step, y, string[i] );
	}
}

void Menu_DrawStringDark( int x, int y, const char *string )
{
	uint32 i;
	float s = Menu_Scale();
	int step = (int)(8 * s);

	for ( i = 0; i < strlen( string ); i++ )
	{
		Draw_Char( x + (int)i * step, y, string[i] + 128 );
	}
}

void Menu_DrawStringR2L( int x, int y, const char *string )
{
	uint32 i;
	float s = Menu_Scale();
	int step = (int)(8 * s);

	for ( i = 0; i < strlen( string ); i++ )
	{
		Draw_Char( x - (int)i * step, y, string[strlen(string)-i-1] );
	}
}

void Menu_DrawStringR2LDark( int x, int y, const char *string )
{
	uint32 i;
	float s = Menu_Scale();
	int step = (int)(8 * s);

	for ( i = 0; i < strlen( string ); i++ )
	{
		Draw_Char( x - (int)i * step, y, string[strlen(string)-i-1]+128 );
	}
}

void *Menu_ItemAtCursor( menuframework_s *m )
{
	if ( m->cursor < 0 || m->cursor >= m->nitems )
		return 0;

	return m->items[m->cursor];
}

qboolean Menu_SelectItem( menuframework_s *s )
{
	menucommon_s *item = ( menucommon_s * ) Menu_ItemAtCursor( s );

	if ( item )
	{
		switch ( item->type )
		{
		case MTYPE_FIELD:
			return Field_DoEnter( ( menufield_s * ) item ) ;
		case MTYPE_ACTION:
			Action_DoEnter( ( menuaction_s * ) item );
			return true;
		case MTYPE_LIST:
//			Menulist_DoEnter( ( menulist_s * ) item );
			return false;
		case MTYPE_SPINCONTROL:
//			SpinControl_DoEnter( ( menulist_s * ) item );
			return false;
		}
	}
	return false;
}

void Menu_SetStatusBar( menuframework_s *m, const char *string )
{
	m->statusbar = string;
}

void Menu_SlideItem( menuframework_s *s, int dir )
{
	menucommon_s *item = ( menucommon_s * ) Menu_ItemAtCursor( s );

	if ( item )
	{
		switch ( item->type )
		{
		case MTYPE_SLIDER:
			Slider_DoSlide( ( menuslider_s * ) item, dir );
			break;
		case MTYPE_SPINCONTROL:
			SpinControl_DoSlide( ( menulist_s * ) item, dir );
			break;
		}
	}
}

int Menu_TallySlots( menuframework_s *menu )
{
	int i;
	int total = 0;

	for ( i = 0; i < menu->nitems; i++ )
	{
		if ( ( ( menucommon_s * ) menu->items[i] )->type == MTYPE_LIST )
		{
			int nitems = 0;
			const char **n = ( ( menulist_s * ) menu->items[i] )->itemnames;

			while (*n)
				nitems++, n++;

			total += nitems;
		}
		else
		{
			total++;
		}
	}

	return total;
}

/*void Menulist_DoEnter( menulist_s *l )
{
	int start;

	start = l->generic.y / 10 + 1;

	l->curvalue = l->generic.parent->cursor - start;

	if ( l->generic.callback )
		l->generic.callback( l );
}*/

void MenuList_Draw( menulist_s *l )
{
	const char **n;
	int y = 0;
	int x, basey, row;
	float s = Menu_Scale();

	Menu_ScaledXY( &l->generic, &x, &basey );
	row = (int)(10 * s);

	Menu_DrawStringR2LDark( x + (int)(LCOLUMN_OFFSET * s), basey, l->generic.name );

	n = l->itemnames;

	Draw_Fill( x - (int)(112 * s), basey + l->curvalue * row + row, (int)(128 * s), row, 16 );
	while ( *n )
	{
		Menu_DrawStringR2LDark( x + (int)(LCOLUMN_OFFSET * s), basey + y + row, *n );

		n++;
		y += row;
	}
}

void Separator_Draw( menuseparator_s *s )
{
	int x, y;
	Menu_ScaledXY( &s->generic, &x, &y );
	Menu_DrawStringR2LDark( x, y, s->generic.name );
}

void Slider_DoSlide( menuslider_s *s, int dir )
{
	s->curvalue += dir;

	if ( s->curvalue > s->maxvalue )
		s->curvalue = s->maxvalue;
	else if ( s->curvalue < s->minvalue )
		s->curvalue = s->minvalue;

	if ( s->generic.callback )
		s->generic.callback( s );
}

#define SLIDER_RANGE 10

void Slider_Draw( menuslider_s *s )
{
	int	i, x, y, step;
	float sc = Menu_Scale();

	Menu_ScaledXY( &s->generic, &x, &y );
	step = (int)(8 * sc);

	Menu_DrawStringR2LDark( x + (int)(LCOLUMN_OFFSET * sc), y, s->generic.name );

	s->range = ( s->curvalue - s->minvalue ) / ( float ) ( s->maxvalue - s->minvalue );

	if (FLOAT_LT_ZERO(s->range))
		s->range = 0;
	if ( s->range > 1)
		s->range = 1;
	Draw_Char( x + (int)(RCOLUMN_OFFSET * sc), y, 128);
	for ( i = 0; i < SLIDER_RANGE; i++ )
		Draw_Char( x + (int)(RCOLUMN_OFFSET * sc) + i * step + step, y, 129);
	Draw_Char( x + (int)(RCOLUMN_OFFSET * sc) + i * step + step, y, 130);
	Draw_Char( x + (int)(RCOLUMN_OFFSET * sc) + step + (int)((SLIDER_RANGE-1) * step * s->range), y, 131);
}

/*void SpinControl_DoEnter( menulist_s *s )
{
	s->curvalue++;
	if ( s->itemnames[s->curvalue] == 0 )
		s->curvalue = 0;

	if ( s->generic.callback )
		s->generic.callback( s );
}*/

void SpinControl_DoSlide( menulist_s *s, int dir )
{
	s->curvalue += dir;

	if ( s->curvalue < 0 )
		s->curvalue = 0;
	else if ( s->itemnames[s->curvalue] == 0 )
		s->curvalue--;

	if ( s->generic.callback )
		s->generic.callback( s );
}

void SpinControl_Draw( menulist_s *s )
{
	char buffer[100];
	int x, y;
	float sc = Menu_Scale();

	Menu_ScaledXY( &s->generic, &x, &y );

	if ( s->generic.name )
	{
		Menu_DrawStringR2LDark( x + (int)(LCOLUMN_OFFSET * sc), y, s->generic.name );
	}

	if (!s->itemnames[s->curvalue])
		Com_Error (ERR_DROP, "SpinControl_Draw: %s: NULL current value %d", s->generic.name, s->curvalue);

	if ( !strchr( s->itemnames[s->curvalue], '\n' ) )
	{
		Menu_DrawString( x + (int)(RCOLUMN_OFFSET * sc), y, s->itemnames[s->curvalue] );
	}
	else
	{
		strcpy( buffer, s->itemnames[s->curvalue] );
		*strchr( buffer, '\n' ) = 0;
		Menu_DrawString( x + (int)(RCOLUMN_OFFSET * sc), y, buffer );
		strcpy( buffer, strchr( s->itemnames[s->curvalue], '\n' ) + 1 );
		Menu_DrawString( x + (int)(RCOLUMN_OFFSET * sc), y + (int)(10 * sc), buffer );
	}
}

