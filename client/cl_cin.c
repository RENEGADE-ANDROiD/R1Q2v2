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
#ifdef CINEMATICS
#include "client.h"

typedef struct
{
	byte	*data;
	int		count;
} cblock_t;

typedef struct
{
	qboolean	restart_sound;
	int		s_rate;
	int		s_width;
	int		s_channels;

	int		width;
	int		height;
	byte	*pic;
	byte	*pic_pending;

	// order 1 huffman stuff
	int		*hnodes1;	// [256][256][2];
	int		numhnodes1[256];

	int		h_used[512];
	int		h_count[512];
} cinematics_t;

cinematics_t	cin;

/*
=================================================================

PCX LOADING

=================================================================
*/


/*
==============
SCR_LoadPCX
==============
*/
void SCR_LoadPCX (char *filename, byte **pic, byte **palette, int *width, int *height)
{
	byte	*raw, *filebuf, *data_end;
	pcx_t	*pcx;
	int		x, y;
	int		len;
	int		dataByte, runLength;
	byte	*out, *pix;

	*pic = NULL;
	if (palette)
		*palette = NULL;

	//
	// load the file
	//
	len = FS_LoadFile (filename, (void **)&filebuf);
	if (!filebuf)
		return;
	if (len < (int)offsetof(pcx_t, data) + 1)
	{
		Com_Printf ("Bad pcx file %s (truncated header)\n", LOG_CLIENT, filename);
		FS_FreeFile (filebuf);
		return;
	}

	//
	// parse the PCX file
	//
	pcx = (pcx_t *)filebuf;
	raw = &pcx->data;
	data_end = filebuf + len;

	if (pcx->manufacturer != 0x0a
		|| pcx->version != 5
		|| pcx->encoding != 1
		|| pcx->bits_per_pixel != 8
		|| pcx->xmin != 0
		|| pcx->ymin != 0
		|| pcx->xmax >= 640
		|| pcx->ymax >= 480
		|| (palette && (len < 769 || filebuf[len-769] != 0x0c)))
	{
		Com_Printf ("Bad pcx file %s\n", LOG_CLIENT, filename);
		FS_FreeFile (filebuf);
		return;
	}

	out = Z_TagMalloc ( (pcx->ymax+1) * (pcx->xmax+1), TAGMALLOC_CLIENT_LOADPCX);

	*pic = out;

	pix = out;

	if (palette)
	{
		*palette = Z_TagMalloc(768, TAGMALLOC_CLIENT_LOADPCX);
		memcpy (*palette, filebuf + len - 768, 768);
		data_end = filebuf + len - 769;
	}

	if (width)
		*width = pcx->xmax+1;
	if (height)
		*height = pcx->ymax+1;

	for (y=0 ; y<=pcx->ymax ; y++, pix += pcx->xmax+1)
	{
		for (x=0 ; x<=pcx->xmax ; )
		{
			if (raw >= data_end)
				goto malformed;
			dataByte = *raw++;

			if((dataByte & 0xC0) == 0xC0)
			{
				runLength = dataByte & 0x3F;
				if (raw >= data_end)
					goto malformed;
				dataByte = *raw++;
			}
			else
				runLength = 1;

			if (runLength > pcx->xmax + 1 - x)
				goto malformed;
			while(runLength-- > 0)
				pix[x++] = dataByte;
		}

	}

	FS_FreeFile (filebuf);
	return;

malformed:
	Com_Printf ("PCX file %s was malformed\n", LOG_CLIENT, filename);
	if (*pic)
	{
		Z_Free (*pic);
		*pic = NULL;
	}
	if (palette && *palette)
	{
		Z_Free (*palette);
		*palette = NULL;
	}
	FS_FreeFile (filebuf);
}

//=============================================================

/*
==================
SCR_StopCinematic
==================
*/
void SCR_StopCinematic (void)
{
	cl.cinematictime = 0;	// done
	if (cin.pic)
	{
		Z_Free (cin.pic);
		cin.pic = NULL;
	}
	if (cin.pic_pending)
	{
		Z_Free (cin.pic_pending);
		cin.pic_pending = NULL;
	}
	if (cl.cinematicpalette_active)
	{
		re.CinematicSetPalette(NULL);
		cl.cinematicpalette_active = false;
	}
	if (cl.cinematic_file)
	{
		fclose (cl.cinematic_file);
		cl.cinematic_file = NULL;
	}
	if (cin.hnodes1)
	{
		Z_Free (cin.hnodes1);
		cin.hnodes1 = NULL;
	}

	// switch back down to 11 khz sound if necessary
	if (cin.restart_sound)
	{
		cin.restart_sound = false;
		CL_Snd_Restart_f ();
	}

}

/*
====================
SCR_FinishCinematic

Called when either the cinematic completes, or it is aborted
====================
*/
void SCR_FinishCinematic (void)
{
	// tell the server to advance to the next map / cinematic
	MSG_WriteByte (&cls.netchan.message, clc_stringcmd);
	SZ_Print (&cls.netchan.message, va("nextserver %i\n", cl.servercount));
}

//==========================================================================

/*
==================
SmallestNode1
==================
*/
int	SmallestNode1 (int numhnodes)
{
	int		i;
	int		best, bestnode;

	best = 99999999;
	bestnode = -1;
	for (i=0 ; i<numhnodes ; i++)
	{
		if (cin.h_used[i])
			continue;
		if (!cin.h_count[i])
			continue;
		if (cin.h_count[i] < best)
		{
			best = cin.h_count[i];
			bestnode = i;
		}
	}

	if (bestnode == -1)
		return -1;

	cin.h_used[bestnode] = true;
	return bestnode;
}


/*
==================
Huff1TableInit

Reads the 64k counts table and initializes the node trees
==================
*/
void Huff1TableInit (void)
{
	int		prev;
	int		j;
	int		*node, *nodebase;
	byte	counts[256];
	int		numhnodes;

	cin.hnodes1 = Z_TagMalloc (256*256*2*4, TAGMALLOC_CLIENT_CINEMA);
	memset (cin.hnodes1, 0, 256*256*2*4);

	for (prev=0 ; prev<256 ; prev++)
	{
		memset (cin.h_count,0,sizeof(cin.h_count));
		memset (cin.h_used,0,sizeof(cin.h_used));

		// read a row of counts
		FS_Read (counts, sizeof(counts), cl.cinematic_file);
		for (j=0 ; j<256 ; j++)
			cin.h_count[j] = counts[j];

		// build the nodes
		numhnodes = 256;
		nodebase = cin.hnodes1 + prev*256*2;

		while (numhnodes != 511)
		{
			node = nodebase + (numhnodes-256)*2;

			// pick two lowest counts
			node[0] = SmallestNode1 (numhnodes);
			if (node[0] == -1)
				break;	// no more

			node[1] = SmallestNode1 (numhnodes);
			if (node[1] == -1)
				break;

			cin.h_count[numhnodes] = cin.h_count[node[0]] + cin.h_count[node[1]];
			numhnodes++;
		}

		cin.numhnodes1[prev] = numhnodes-1;
	}
}

/*
==================
Huff1Decompress
==================
*/
cblock_t Huff1Decompress (cblock_t in)
{
	byte		*input;
	byte		*input_end;
	byte		*out_p;
	int			nodenum;
	int			count;
	cblock_t	out;
	int			inbyte;
	int			*hnodes, *hnodesbase;
//int		i;

	if (!in.data || in.count < 4)
		Com_Error (ERR_DROP, "Truncated cinematic frame");

	// get decompressed count
	count = (int)((uint32)in.data[0] | ((uint32)in.data[1] << 8) |
		((uint32)in.data[2] << 16) | ((uint32)in.data[3] << 24));
	if (count <= 0 || cin.width <= 0 || cin.height <= 0 ||
		cin.width > INT_MAX / cin.height || count != cin.width * cin.height)
		Com_Error (ERR_DROP, "Bad cinematic frame dimensions");
	input = in.data + 4;
	input_end = in.data + in.count;
	out_p = out.data = Z_TagMalloc (count, TAGMALLOC_CLIENT_CINEMA);

	// read bits

	hnodesbase = cin.hnodes1 - 256*2;	// nodes 0-255 aren't stored

	hnodes = hnodesbase;
	nodenum = cin.numhnodes1[0];
	while (count)
	{
		if (input >= input_end)
		{
			Z_Free (out.data);
			Com_Error (ERR_DROP, "Truncated cinematic Huffman data");
		}
		inbyte = *input++;
		//-----------
		if (nodenum < 256)
		{
			hnodes = hnodesbase + (nodenum<<9);
			*out_p++ = nodenum;
			if (!--count)
				break;
			nodenum = cin.numhnodes1[nodenum];
		}
		nodenum = hnodes[nodenum*2 + (inbyte&1)];
		inbyte >>=1;
		//-----------
		if (nodenum < 256)
		{
			hnodes = hnodesbase + (nodenum<<9);
			*out_p++ = nodenum;
			if (!--count)
				break;
			nodenum = cin.numhnodes1[nodenum];
		}
		nodenum = hnodes[nodenum*2 + (inbyte&1)];
		inbyte >>=1;
		//-----------
		if (nodenum < 256)
		{
			hnodes = hnodesbase + (nodenum<<9);
			*out_p++ = nodenum;
			if (!--count)
				break;
			nodenum = cin.numhnodes1[nodenum];
		}
		nodenum = hnodes[nodenum*2 + (inbyte&1)];
		inbyte >>=1;
		//-----------
		if (nodenum < 256)
		{
			hnodes = hnodesbase + (nodenum<<9);
			*out_p++ = nodenum;
			if (!--count)
				break;
			nodenum = cin.numhnodes1[nodenum];
		}
		nodenum = hnodes[nodenum*2 + (inbyte&1)];
		inbyte >>=1;
		//-----------
		if (nodenum < 256)
		{
			hnodes = hnodesbase + (nodenum<<9);
			*out_p++ = nodenum;
			if (!--count)
				break;
			nodenum = cin.numhnodes1[nodenum];
		}
		nodenum = hnodes[nodenum*2 + (inbyte&1)];
		inbyte >>=1;
		//-----------
		if (nodenum < 256)
		{
			hnodes = hnodesbase + (nodenum<<9);
			*out_p++ = nodenum;
			if (!--count)
				break;
			nodenum = cin.numhnodes1[nodenum];
		}
		nodenum = hnodes[nodenum*2 + (inbyte&1)];
		inbyte >>=1;
		//-----------
		if (nodenum < 256)
		{
			hnodes = hnodesbase + (nodenum<<9);
			*out_p++ = nodenum;
			if (!--count)
				break;
			nodenum = cin.numhnodes1[nodenum];
		}
		nodenum = hnodes[nodenum*2 + (inbyte&1)];
		inbyte >>=1;
		//-----------
		if (nodenum < 256)
		{
			hnodes = hnodesbase + (nodenum<<9);
			*out_p++ = nodenum;
			if (!--count)
				break;
			nodenum = cin.numhnodes1[nodenum];
		}
		nodenum = hnodes[nodenum*2 + (inbyte&1)];
		inbyte >>=1;
	}

	if (input - in.data != in.count && input - in.data != in.count+1)
	{
		Com_Printf ("Decompression overread by %i", LOG_CLIENT, (input - in.data) - in.count);
	}
	out.count = out_p - out.data;

	return out;
}

/*
==================
SCR_ReadNextFrame
==================
*/
byte *SCR_ReadNextFrame (void)
{
	int		r;
	int		command;
	byte	samples[22050/14*4];
	byte	compressed[0x20000];
	int		size;
	byte	*pic;
	cblock_t	in, huf1;
	int		start, end, count;

	// read the next frame
	r = fread (&command, 4, 1, cl.cinematic_file);
	if (r == 0)		// we'll give it one more chance
		r = fread (&command, 4, 1, cl.cinematic_file);

	if (r != 1)
		return NULL;
	command = LittleLong(command);
	if (command == 2)
		return NULL;	// last frame marker
	if (command != 0 && command != 1)
		Com_Error (ERR_DROP, "Bad cinematic frame command %d", command);

	if (command == 1)
	{	// read palette
		FS_Read (cl.cinematicpalette, sizeof(cl.cinematicpalette), cl.cinematic_file);
		cl.cinematicpalette_active=0;	// dubious....  exposes an edge case
	}

	// decompress the next frame
	FS_Read (&size, 4, cl.cinematic_file);
	size = LittleLong(size);
	if (size > sizeof(compressed) || size < 4)
		Com_Error (ERR_DROP, "Bad compressed frame size");
	FS_Read (compressed, size, cl.cinematic_file);

	// read sound
	start = cl.cinematicframe*cin.s_rate/14;
	end = (cl.cinematicframe+1)*cin.s_rate/14;
	count = end - start;
	if (count < 0 || count > (int)sizeof(samples) / (cin.s_width * cin.s_channels))
		Com_Error (ERR_DROP, "Bad cinematic audio frame size");

	FS_Read (samples, count*cin.s_width*cin.s_channels, cl.cinematic_file);

	S_RawSamples (count, cin.s_rate, cin.s_width, cin.s_channels, samples);

	in.data = compressed;
	in.count = size;

	huf1 = Huff1Decompress (in);

	pic = huf1.data;

	cl.cinematicframe++;

	return pic;
}


/*
==================
SCR_RunCinematic

==================
*/
void SCR_RunCinematic (void)
{
	int		frame;

	if (cl.cinematictime <= 0)
	{
		SCR_StopCinematic ();
		return;
	}

	if (cl.cinematicframe == -1)
		return;		// static image

	if (cls.key_dest != key_game)
	{	// pause if menu or console is up
		cl.cinematictime = cls.realtime - cl.cinematicframe*1000/14;
		return;
	}

	frame = (cls.realtime - cl.cinematictime)*14.0/1000;
	if (frame <= cl.cinematicframe)
		return;
	if (frame > cl.cinematicframe+1)
	{
		Com_Printf ("Dropped frame: %i > %i\n", LOG_CLIENT, frame, cl.cinematicframe+1);
		cl.cinematictime = cls.realtime - cl.cinematicframe*1000/14;
	}
	if (cin.pic)
		Z_Free (cin.pic);
	cin.pic = cin.pic_pending;
	cin.pic_pending = NULL;
	cin.pic_pending = SCR_ReadNextFrame ();
	if (!cin.pic_pending)
	{
		SCR_StopCinematic ();
		SCR_FinishCinematic ();
		cl.cinematictime = 1;	// hack to get the black screen behind loading
		SCR_BeginLoadingPlaque ();
		cl.cinematictime = 0;
		return;
	}
}

/*
==================
SCR_DrawCinematic

Returns true if a cinematic is active, meaning the view rendering
should be skipped
==================
*/
qboolean SCR_DrawCinematic (void)
{
	if (cl.cinematictime <= 0)
	{
		return false;
	}

	if (cls.key_dest == key_menu)
	{	// blank screen and pause if menu is up
		re.CinematicSetPalette(NULL);
		cl.cinematicpalette_active = false;
		return true;
	}

	if (!cl.cinematicpalette_active)
	{
		re.CinematicSetPalette(cl.cinematicpalette);
		cl.cinematicpalette_active = true;
	}

	if (!cin.pic)
		return true;

	re.DrawStretchRaw (0, 0, viddef.width, viddef.height,
		cin.width, cin.height, cin.pic);

	return true;
}

/*
==================
SCR_PlayCinematic

==================
*/
void SCR_PlayCinematic (char *arg)
{
	int			width, height;
	byte		*palette;
	char		name[MAX_OSPATH], *dot;
	int			old_khz;
	qboolean	dummy;

	// make sure CD isn't playing music
#ifdef CD_AUDIO
	CDAudio_Stop();
#endif

	cl.cinematicframe = 0;
	dot = strchr (arg, '.');
	if (dot && !strcmp (dot, ".pcx"))
	{	// static pcx image
		Com_sprintf (name, sizeof(name), "pics/%s", arg);
		SCR_LoadPCX (name, &cin.pic, &palette, &cin.width, &cin.height);
		cl.cinematicframe = -1;
		cl.cinematictime = 1;
		SCR_EndLoadingPlaque ();
		cls.state = ca_active;
		if (!cin.pic)
		{
			Com_Printf ("%s not found.\n", LOG_CLIENT, name);
			cl.cinematictime = 0;
		}
		else
		{
			memcpy (cl.cinematicpalette, palette, sizeof(cl.cinematicpalette));
			Z_Free (palette);
		}
		return;
	}

	Com_sprintf (name, sizeof(name), "video/%s", arg);
	FS_FOpenFile (name, &cl.cinematic_file, HANDLE_DUPE, &dummy);
	if (!cl.cinematic_file)
	{
//		Com_Error (ERR_DROP, "Cinematic %s not found.\n", name);
		SCR_FinishCinematic ();
		cl.cinematictime = 0;	// done
		return;
	}

	SCR_EndLoadingPlaque ();

	cls.state = ca_active;

	FS_Read (&width, 4, cl.cinematic_file);
	FS_Read (&height, 4, cl.cinematic_file);
	cin.width = LittleLong(width);
	cin.height = LittleLong(height);

	FS_Read (&cin.s_rate, 4, cl.cinematic_file);
	cin.s_rate = LittleLong(cin.s_rate);
	FS_Read (&cin.s_width, 4, cl.cinematic_file);
	cin.s_width = LittleLong(cin.s_width);
	FS_Read (&cin.s_channels, 4, cl.cinematic_file);
	cin.s_channels = LittleLong(cin.s_channels);
	if (cin.width <= 0 || cin.height <= 0 || cin.width > 4096 || cin.height > 4096 ||
		cin.width > INT_MAX / cin.height || cin.s_rate <= 0 || cin.s_rate > 22050 ||
		(cin.s_width != 1 && cin.s_width != 2) ||
		(cin.s_channels != 1 && cin.s_channels != 2))
	{
		Com_Printf ("Bad cinematic header: %dx%d, %d Hz, %d-byte, %d channel(s)\n",
			LOG_CLIENT|LOG_WARNING, cin.width, cin.height, cin.s_rate, cin.s_width, cin.s_channels);
		SCR_StopCinematic ();
		return;
	}

	Huff1TableInit ();

	// switch up to 22 khz sound if necessary
	old_khz = Cvar_IntValue ("s_khz");
	if (old_khz != cin.s_rate/1000)
	{
		cin.restart_sound = true;
		Cvar_SetValue ("s_khz", cin.s_rate/1000);
		CL_Snd_Restart_f ();
		Cvar_SetValue ("s_khz", old_khz);
	}

	cl.cinematicframe = 0;
	cin.pic = SCR_ReadNextFrame ();
	cl.cinematictime = Sys_Milliseconds ();
}
#endif
