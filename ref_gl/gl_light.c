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
// r_light.c

#include "gl_local.h"
#include <stdlib.h>

int	r_dlightframecount;

#define	DLIGHT_CUTOFF	64

/*
=============================================================================

DYNAMIC LIGHTS BLEND RENDERING

=============================================================================
*/

void R_RenderDlight (dlight_t *light)
{
	int		i, j;
	float	a;
	vec3_t	v;
	float	rad;

	rad = light->intensity * 0.35f;

	VectorSubtract (light->origin, r_origin, v);
#if 0
	// FIXME?
	if (VectorLength (v) < rad)
	{	// view is inside the dlight
		V_AddBlend (light->color[0], light->color[1], light->color[2], light->intensity * 0.0003, v_blend);
		return;
	}
#endif

	qglBegin (GL_TRIANGLE_FAN);
	qglColor3f (light->color[0]*0.2f, light->color[1]*0.2f, light->color[2]*0.2f);
	for (i=0 ; i<3 ; i++)
		v[i] = light->origin[i] - vpn[i]*rad;
	qglVertex3fv (v);
	qglColor3f (0,0,0);
	for (i=16 ; i>=0 ; i--)
	{
		a = i/16.0f * M_PI*2;
		for (j=0 ; j<3 ; j++)
			v[j] = light->origin[j] + vright[j]*(float)cos(a)*rad
				+ vup[j]*(float)sin(a)*rad;
		qglVertex3fv (v);
	}
	qglEnd ();
}

/*
=============
R_RenderDlights
=============
*/
void R_RenderDlights (void)
{
	int		i;
	dlight_t	*l;

	if (FLOAT_EQ_ZERO(gl_flashblend->value))
		return;

	r_dlightframecount = r_framecount + 1;	// because the count hasn't
											//  advanced yet for this frame
	qglDepthMask (0);
	qglDisable (GL_TEXTURE_2D);
	qglShadeModel (GL_SMOOTH);
	qglEnable (GL_BLEND);
	qglBlendFunc (GL_ONE, GL_ONE);

	l = r_newrefdef.dlights;
	for (i=0 ; i<r_newrefdef.num_dlights ; i++, l++)
		R_RenderDlight (l);

	qglColor3f (1,1,1);
	qglDisable (GL_BLEND);
	qglEnable (GL_TEXTURE_2D);
	qglBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	qglDepthMask (1);
}


/*
=============
World-light coronas / shafts (fair-play)

BSP `classname light` sprites plus short additive streaks. A light is drawn
only if a BSP trace from the camera reaches it (solid leaves block; a short
slop at the light end allows ceiling/wall mounts). Large depth-tested quads
alone are not enough — they poke through thin walls.
=============
*/

#define MAX_RWORLD_LIGHTS	768
#define LIGHT_VIS_SLOP		24.0f

typedef struct
{
	vec3_t	origin;
	vec3_t	color;
	float	intensity;
	int		style;
	int		cluster;
	int		area;
} rworldlight_t;

static rworldlight_t	r_worldlights[MAX_RWORLD_LIGHTS];
static int				r_numworldlights;

static qboolean R_RecursiveLightVis (mnode_t *node, vec3_t p1, vec3_t p2, vec3_t light)
{
	cplane_t	*plane;
	float		t1, t2, frac;
	int			side;
	vec3_t		mid;

	if (!node)
		return false;

	if (node->contents != -1)
	{
		if (node->contents & CONTENTS_SOLID)
		{
			vec3_t	d;
			/* Solid only counts as a blocker if it is not the light's mount. */
			VectorSubtract (light, p1, d);
			return (DotProduct (d, d) <= LIGHT_VIS_SLOP * LIGHT_VIS_SLOP);
		}
		return true;
	}

	plane = node->plane;
	t1 = DotProduct (p1, plane->normal) - plane->dist;
	t2 = DotProduct (p2, plane->normal) - plane->dist;

	if (t1 >= 0.0f && t2 >= 0.0f)
		return R_RecursiveLightVis (node->children[0], p1, p2, light);
	if (t1 < 0.0f && t2 < 0.0f)
		return R_RecursiveLightVis (node->children[1], p1, p2, light);

	side = (t1 < 0.0f);
	frac = t1 / (t1 - t2);
	if (frac < 0.0f)
		frac = 0.0f;
	if (frac > 1.0f)
		frac = 1.0f;
	mid[0] = p1[0] + frac * (p2[0] - p1[0]);
	mid[1] = p1[1] + frac * (p2[1] - p1[1]);
	mid[2] = p1[2] + frac * (p2[2] - p1[2]);

	if (!R_RecursiveLightVis (node->children[side], p1, mid, light))
		return false;
	return R_RecursiveLightVis (node->children[!side], mid, p2, light);
}

qboolean R_LightOriginVisible (vec3_t origin)
{
	if (!r_worldmodel || !r_worldmodel->nodes)
		return false;
	return R_RecursiveLightVis (r_worldmodel->nodes, r_origin, origin, origin);
}

/* First solid-leaf entry along p1->p2. hit is the crossing point. */
static qboolean R_RayHitSolid (mnode_t *node, vec3_t p1, vec3_t p2, vec3_t hit)
{
	cplane_t	*plane;
	float		t1, t2, frac;
	int			side;
	vec3_t		mid;

	if (!node)
		return false;

	if (node->contents != -1)
	{
		if (node->contents & CONTENTS_SOLID)
		{
			FastVectorCopy (p1, hit);
			return true;
		}
		return false;
	}

	plane = node->plane;
	t1 = DotProduct (p1, plane->normal) - plane->dist;
	t2 = DotProduct (p2, plane->normal) - plane->dist;

	if (t1 >= 0.0f && t2 >= 0.0f)
		return R_RayHitSolid (node->children[0], p1, p2, hit);
	if (t1 < 0.0f && t2 < 0.0f)
		return R_RayHitSolid (node->children[1], p1, p2, hit);

	side = (t1 < 0.0f);
	frac = t1 / (t1 - t2);
	if (frac < 0.0f)
		frac = 0.0f;
	if (frac > 1.0f)
		frac = 1.0f;
	mid[0] = p1[0] + frac * (p2[0] - p1[0]);
	mid[1] = p1[1] + frac * (p2[1] - p1[1]);
	mid[2] = p1[2] + frac * (p2[2] - p1[2]);

	if (R_RayHitSolid (node->children[side], p1, mid, hit))
		return true;
	return R_RayHitSolid (node->children[!side], mid, p2, hit);
}

#define WORLD_LIGHT_MOUNT	56.0f

/* Snap fill-lights away; keep sprites on ceiling/wall mounts only. */
static qboolean R_MountWorldLight (model_t *world, vec3_t origin)
{
	static const float ax[6][3] = {
		{ 0, 0, 1 }, { 0, 0, -1 },
		{ 1, 0, 0 }, { -1, 0, 0 },
		{ 0, 1, 0 }, { 0, -1, 0 }
	};
	vec3_t	end, hit, inward, best_hit, best_from;
	float	best, dist;
	int		d;
	qboolean found = false;

	if (!world || !world->nodes)
		return false;

	best = WORLD_LIGHT_MOUNT;
	for (d = 0; d < 6; d++)
	{
		end[0] = origin[0] + ax[d][0] * WORLD_LIGHT_MOUNT;
		end[1] = origin[1] + ax[d][1] * WORLD_LIGHT_MOUNT;
		end[2] = origin[2] + ax[d][2] * WORLD_LIGHT_MOUNT;
		if (!R_RayHitSolid (world->nodes, origin, end, hit))
			continue;
		VectorSubtract (hit, origin, inward);
		dist = VectorLength (inward);
		if (dist < 2.0f || dist >= best)
			continue;
		best = dist;
		FastVectorCopy (hit, best_hit);
		FastVectorCopy (origin, best_from);
		found = true;
	}

	if (!found)
		return false;

	VectorSubtract (best_from, best_hit, inward);
	if (VectorNormalize (inward) < 0.01f)
		return false;
	origin[0] = best_hit[0] + inward[0] * 6.0f;
	origin[1] = best_hit[1] + inward[1] * 6.0f;
	origin[2] = best_hit[2] + inward[2] * 6.0f;
	return true;
}

int R_NumWorldLights (void)
{
	return r_numworldlights;
}

void R_WorldLightOrigin (int i, vec3_t origin, float *intensity)
{
	if (i < 0 || i >= r_numworldlights)
	{
		VectorClear (origin);
		if (intensity)
			*intensity = 0.0f;
		return;
	}
	FastVectorCopy (r_worldlights[i].origin, origin);
	if (intensity)
		*intensity = r_worldlights[i].intensity;
}

void R_LoadWorldLights (model_t *world, byte *base, lump_t *l)
{
	char		*copy;
	char		*data;
	const char	*token;
	vec3_t		origin, color;
	float		intensity;
	int			style, spawnflags;
	qboolean	inentity, islight, have_origin;
	mleaf_t		*leaf;

	r_numworldlights = 0;
	if (!world || !base || !l || l->filelen <= 0)
		return;
	if (l->fileofs < 0 || l->filelen > 2 * 1024 * 1024)
		return;

	copy = (char *)malloc ((size_t)l->filelen + 1);
	if (!copy)
		return;
	memcpy (copy, base + l->fileofs, (size_t)l->filelen);
	copy[l->filelen] = 0;

	data = copy;
	inentity = false;
	islight = false;
	have_origin = false;
	intensity = 300.0f;
	style = 0;
	spawnflags = 0;
	VectorSet (origin, 0, 0, 0);
	VectorSet (color, 1.0f, 1.0f, 1.0f);

	while (1)
	{
		token = COM_Parse (&data);
		if (!token || !token[0])
			break;

		if (token[0] == '{')
		{
			inentity = true;
			islight = false;
			have_origin = false;
			intensity = 300.0f;
			style = 0;
			spawnflags = 0;
			VectorSet (origin, 0, 0, 0);
			VectorSet (color, 1.0f, 1.0f, 1.0f);
			continue;
		}

		if (token[0] == '}')
		{
			if (inentity && islight && have_origin && !(spawnflags & 1)
				&& r_numworldlights < MAX_RWORLD_LIGHTS)
			{
				rworldlight_t	*wl;

				if (color[0] + color[1] + color[2] < 0.05f)
					VectorSet (color, 1.0f, 1.0f, 1.0f);
				if (intensity < 40.0f)
					intensity = 40.0f;

				/* Q2 `light` ents are often fill points in empty space.
				 * Only keep those mounted on a nearby ceiling/wall. */
				if (!R_MountWorldLight (world, origin))
				{
					inentity = false;
					continue;
				}

				wl = &r_worldlights[r_numworldlights];
				FastVectorCopy (origin, wl->origin);
				FastVectorCopy (color, wl->color);
				wl->intensity = intensity;
				wl->style = style;
				wl->cluster = -1;
				wl->area = 0;
				leaf = Mod_PointInLeaf (wl->origin, world);
				if (leaf)
				{
					wl->cluster = leaf->cluster;
					wl->area = leaf->area;
				}
				r_numworldlights++;
			}
			inentity = false;
			continue;
		}

		if (!inentity)
			continue;

		if (!Q_stricmp (token, "classname"))
		{
			token = COM_Parse (&data);
			if (token && !Q_stricmp (token, "light"))
				islight = true;
		}
		else if (!Q_stricmp (token, "origin"))
		{
			token = COM_Parse (&data);
			if (token && sscanf (token, "%f %f %f", &origin[0], &origin[1], &origin[2]) == 3)
				have_origin = true;
		}
		else if (!Q_stricmp (token, "light"))
		{
			token = COM_Parse (&data);
			if (token)
				intensity = (float)atof (token);
		}
		else if (!Q_stricmp (token, "_color") || !Q_stricmp (token, "color"))
		{
			token = COM_Parse (&data);
			if (token && sscanf (token, "%f %f %f", &color[0], &color[1], &color[2]) == 3)
			{
				if (color[0] > 1.0f || color[1] > 1.0f || color[2] > 1.0f)
				{
					color[0] /= 255.0f;
					color[1] /= 255.0f;
					color[2] /= 255.0f;
				}
			}
		}
		else if (!Q_stricmp (token, "style"))
		{
			token = COM_Parse (&data);
			if (token)
				style = atoi (token);
		}
		else if (!Q_stricmp (token, "spawnflags"))
		{
			token = COM_Parse (&data);
			if (token)
				spawnflags = atoi (token);
		}
		else
		{
			COM_Parse (&data); /* skip unknown value */
		}
	}

	free (copy);
	if (r_numworldlights)
		ri.Con_Printf (PRINT_DEVELOPER, "R1GL: %d world lights for coronas\n", r_numworldlights);
}

static void R_EmitCoronaQuad (vec3_t origin, vec3_t color, float scale, float alpha)
{
	vec3_t	up, right;

	VectorScale (vup, scale, up);
	VectorScale (vright, scale, right);

	qglColor4f (color[0], color[1], color[2], alpha);
	qglBegin (GL_QUADS);
	qglTexCoord2f (0.0f, 0.0f);
	qglVertex3f (origin[0] + up[0] - right[0], origin[1] + up[1] - right[1], origin[2] + up[2] - right[2]);
	qglTexCoord2f (1.0f, 0.0f);
	qglVertex3f (origin[0] + up[0] + right[0], origin[1] + up[1] + right[1], origin[2] + up[2] + right[2]);
	qglTexCoord2f (1.0f, 1.0f);
	qglVertex3f (origin[0] - up[0] + right[0], origin[1] - up[1] + right[1], origin[2] - up[2] + right[2]);
	qglTexCoord2f (0.0f, 1.0f);
	qglVertex3f (origin[0] - up[0] - right[0], origin[1] - up[1] - right[1], origin[2] - up[2] - right[2]);
	qglEnd ();
}

static void R_EmitShaftQuad (vec3_t origin, vec3_t color, float intensity, float strength, float dist)
{
	vec3_t	to_cam, side, tip;
	float	len, width, tipw, alpha;

	VectorSubtract (r_origin, origin, to_cam);
	if (VectorNormalize (to_cam) < 32.0f)
		return;

	len = intensity * 0.12f * strength;
	if (len < 8.0f)
		len = 8.0f;
	if (len > 96.0f)
		len = 96.0f;
	if (len > dist * 0.35f)
		len = dist * 0.35f;

	width = intensity * 0.018f * strength;
	if (width < 1.5f)
		width = 1.5f;
	if (width > 10.0f)
		width = 10.0f;
	tipw = width * 0.25f;

	CrossProduct (to_cam, vup, side);
	if (VectorNormalize (side) < 0.1f)
	{
		CrossProduct (to_cam, vright, side);
		if (VectorNormalize (side) < 0.1f)
			return;
	}

	tip[0] = origin[0] + to_cam[0] * len;
	tip[1] = origin[1] + to_cam[1] * len;
	tip[2] = origin[2] + to_cam[2] * len;

	alpha = 0.11f * strength;
	if (alpha > 0.26f)
		alpha = 0.26f;

	qglColor4f (color[0], color[1], color[2], alpha);
	qglBegin (GL_QUADS);
	qglTexCoord2f (0.0f, 0.5f);
	qglVertex3f (origin[0] - side[0] * width, origin[1] - side[1] * width, origin[2] - side[2] * width);
	qglTexCoord2f (1.0f, 0.5f);
	qglVertex3f (origin[0] + side[0] * width, origin[1] + side[1] * width, origin[2] + side[2] * width);
	qglTexCoord2f (1.0f, 0.0f);
	qglVertex3f (tip[0] + side[0] * tipw, tip[1] + side[1] * tipw, tip[2] + side[2] * tipw);
	qglTexCoord2f (0.0f, 0.0f);
	qglVertex3f (tip[0] - side[0] * tipw, tip[1] - side[1] * tipw, tip[2] - side[2] * tipw);
	qglEnd ();
}

static void R_DrawOneLightSprite (vec3_t origin, vec3_t color, float intensity,
	float corona_strength, float shaft_strength, qboolean world_lamp)
{
	float	dist, scale, alpha;
	vec3_t	lit;

	dist = (origin[0] - r_origin[0]) * vpn[0]
		+ (origin[1] - r_origin[1]) * vpn[1]
		+ (origin[2] - r_origin[2]) * vpn[2];
	if (dist < 16.0f)
		return;

	if (!R_LightOriginVisible (origin))
		return;

	FastVectorCopy (color, lit);

	if (FLOAT_NE_ZERO (corona_strength))
	{
		if (world_lamp)
		{
			scale = 14.0f + intensity * 0.05f * corona_strength;
			if (scale < 16.0f)
				scale = 16.0f;
			if (scale > 42.0f)
				scale = 42.0f;
			alpha = 0.28f * corona_strength;
			if (alpha > 0.50f)
				alpha = 0.50f;
		}
		else
		{
			scale = intensity * 0.035f * corona_strength;
			if (scale < 2.0f)
				scale = 2.0f;
			if (scale > 28.0f)
				scale = 28.0f;
			alpha = 0.22f * corona_strength;
			if (alpha > 0.45f)
				alpha = 0.45f;
		}
		R_EmitCoronaQuad (origin, lit, scale, alpha);
	}

	if (FLOAT_NE_ZERO (shaft_strength) && dist > 40.0f)
		R_EmitShaftQuad (origin, lit, intensity, shaft_strength, dist);
}

/*
=============
R_DrawDlightCoronas

Soft additive corona / optional shaft at light origins.
LOS-tested so occluded lights (explosions behind walls) do not show.
gl_light_corona / gl_world_corona / gl_light_shafts: 0=off.
=============
*/
void R_DrawDlightCoronas (void)
{
	int			i;
	dlight_t	*l;
	float		corona_s, shaft_s, world_s;
	byte		*vis;
	qboolean	want_dlight, want_world;

	if (!r_coronatexture)
		return;
	if (r_newrefdef.rdflags & RDF_NOWORLDMODEL)
		return;

	corona_s = R_CvarEff(gl_light_corona);
	world_s = R_CvarEff(gl_world_corona);
	shaft_s = R_CvarEff(gl_light_shafts);
	if (corona_s < 0.0f) corona_s = 0.0f;
	if (corona_s > 2.0f) corona_s = 2.0f;
	if (world_s < 0.0f) world_s = 0.0f;
	if (world_s > 2.0f) world_s = 2.0f;
	if (shaft_s < 0.0f) shaft_s = 0.0f;
	if (shaft_s > 2.0f) shaft_s = 2.0f;

	want_dlight = FLOAT_NE_ZERO (corona_s) || FLOAT_NE_ZERO (shaft_s);
	want_world = FLOAT_NE_ZERO (world_s);
	if (!want_dlight && !want_world)
		return;

	GL_Bind (r_coronatexture->texnum);
	qglEnable (GL_DEPTH_TEST);
	qglDepthMask (GL_FALSE);
	qglEnable (GL_BLEND);
	qglBlendFunc (GL_SRC_ALPHA, GL_ONE);
	GL_TexEnv (GL_MODULATE);

	if (want_dlight)
	{
		l = r_newrefdef.dlights;
		for (i = 0; i < r_newrefdef.num_dlights; i++, l++)
			R_DrawOneLightSprite (l->origin, l->color, l->intensity, corona_s, shaft_s, false);
	}

	if (want_world && r_numworldlights && r_worldmodel)
	{
		float	world_shaft = FLOAT_NE_ZERO (world_s) ? shaft_s : 0.0f;
		vis = NULL;
		if (r_viewcluster != -1 && r_worldmodel->vis)
			vis = Mod_ClusterPVS (r_viewcluster, r_worldmodel);

		for (i = 0; i < r_numworldlights; i++)
		{
			rworldlight_t	*wl = &r_worldlights[i];
			vec3_t			delta, lit;
			float			intensity;

			VectorSubtract (wl->origin, r_origin, delta);
			if (DotProduct (delta, delta) > (2048.0f * 2048.0f))
				continue;

			if (vis && wl->cluster >= 0)
			{
				if (!(vis[wl->cluster >> 3] & (1 << (wl->cluster & 7))))
					continue;
			}

			if (r_newrefdef.areabits && wl->area >= 0)
			{
				if (!(r_newrefdef.areabits[wl->area >> 3] & (1 << (wl->area & 7))))
					continue;
			}

			FastVectorCopy (wl->color, lit);
			intensity = wl->intensity;
			if (wl->style >= 0 && wl->style < MAX_LIGHTSTYLES)
			{
				lit[0] *= r_newrefdef.lightstyles[wl->style].rgb[0];
				lit[1] *= r_newrefdef.lightstyles[wl->style].rgb[1];
				lit[2] *= r_newrefdef.lightstyles[wl->style].rgb[2];
				if (lit[0] + lit[1] + lit[2] < 0.08f)
					continue;
			}

			R_DrawOneLightSprite (wl->origin, lit, intensity, world_s, world_shaft, true);
		}
	}

	qglColor4fv (colorWhite);
	qglBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	qglDisable (GL_BLEND);
	qglDepthMask (GL_TRUE);
	GL_TexEnv (GL_REPLACE);
}



/*
=============================================================================

DYNAMIC LIGHTS

=============================================================================
*/

/*
=============
R_MarkLights
=============
*/
static qboolean R_DlightOccluded (dlight_t *light, msurface_t *surf)
{
	vec3_t	dest, hit, d;
	float	slop;

	if (!surf->polys || !r_worldmodel || !r_worldmodel->nodes)
		return false;

	FastVectorCopy (surf->polys->verts[0], dest);
	if (!R_RayHitSolid (r_worldmodel->nodes, light->origin, dest, hit))
		return false;

	VectorSubtract (hit, dest, d);
	slop = 8.0f;
	return (DotProduct (d, d) > slop * slop);
}

void R_MarkLights (dlight_t *light, int bit, mnode_t *node)
{
	cplane_t	*splitplane;
	float		dist;
	msurface_t	*surf;
	int			i;
	qboolean	shader_dl;
	
	if (node->contents != -1)
		return;

	splitplane = node->plane;
	dist = DotProduct (light->origin, splitplane->normal) - splitplane->dist;
	
	if (dist > light->intensity-DLIGHT_CUTOFF)
	{
		R_MarkLights (light, bit, node->children[0]);
		return;
	}
	if (dist < -light->intensity+DLIGHT_CUTOFF)
	{
		R_MarkLights (light, bit, node->children[1]);
		return;
	}

	shader_dl = R_WorldShaderDlights ();
		
// mark the polygons
	surf = r_worldmodel->surfaces + node->firstsurface;
	for (i=0 ; i<node->numsurfaces ; i++, surf++)
	{
		if (shader_dl)
		{
			int	sidebit;

			dist = DotProduct (light->origin, surf->plane->normal) - surf->plane->dist;
			sidebit = (dist >= 0) ? 0 : SURF_PLANEBACK;
			if ( (surf->flags & SURF_PLANEBACK) != sidebit )
				continue;
			if (R_DlightOccluded (light, surf))
				continue;
		}

		if (surf->dlightframe != r_dlightframecount)
		{
			surf->dlightbits = 0;
			surf->dlightframe = r_dlightframecount;
		}
		surf->dlightbits |= bit;
	}

	R_MarkLights (light, bit, node->children[0]);
	R_MarkLights (light, bit, node->children[1]);
}


/*
=============
R_PushDlights
=============
*/
void R_PushDlights (void)
{
	int		i;
	dlight_t	*l;

	if (FLOAT_NE_ZERO(gl_flashblend->value))
		return;
	/* MP lean / gl_dynamic 0: MarkLights would be ignored at draw — skip BSP walk. */
	if (R_CvarEff(gl_dynamic) == 0.0f)
		return;

	r_dlightframecount = r_framecount + 1;	// because the count hasn't
											//  advanced yet for this frame
	l = r_newrefdef.dlights;
	for (i=0 ; i<r_newrefdef.num_dlights ; i++, l++)
		R_MarkLights ( l, 1<<i, r_worldmodel->nodes );
}


/*
=============================================================================

LIGHT SAMPLING

=============================================================================
*/

vec3_t			pointcolor;
cplane_t		*lightplane;		// used as shadow plane
vec3_t			lightspot;

int RecursiveLightPoint (mnode_t *node, vec3_t start, vec3_t end)
{
	float		front, back, frac;
	int			side;
	cplane_t	*plane;
	vec3_t		mid;
	msurface_t	*surf;
	int			s, t, ds, dt;
	int			i;
	mtexinfo_t	*tex;
	byte		*lightmap;
	int			maps;
	int			r;

	if (node->contents != -1)
		return -1;		// didn't hit anything
	
// calculate mid point

// FIXME: optimize for axial
	plane = node->plane;
	front = DotProduct (start, plane->normal) - plane->dist;
	back = DotProduct (end, plane->normal) - plane->dist;
	side = FLOAT_LT_ZERO(front);
	
	if ((FLOAT_LT_ZERO (back)) == side)
		return RecursiveLightPoint (node->children[side], start, end);
	
	frac = front / (front-back);
	mid[0] = start[0] + (end[0] - start[0])*frac;
	mid[1] = start[1] + (end[1] - start[1])*frac;
	mid[2] = start[2] + (end[2] - start[2])*frac;
	
// go down front side	
	r = RecursiveLightPoint (node->children[side], start, mid);
	if (r >= 0)
		return r;		// hit something
		
	if ((FLOAT_LT_ZERO (back)) == side )
		return -1;		// didn't hit anuthing
		
// check for impact on this node
	FastVectorCopy (mid, lightspot);
	lightplane = plane;

	surf = r_worldmodel->surfaces + node->firstsurface;
	for (i=0 ; i<node->numsurfaces ; i++, surf++)
	{
		if (surf->flags&(SURF_DRAWTURB|SURF_DRAWSKY)) 
			continue;	// no lightmaps

		tex = surf->texinfo;
		
		s = (int)(DotProduct (mid, tex->vecs[0]) + tex->vecs[0][3]);
		t = (int)(DotProduct (mid, tex->vecs[1]) + tex->vecs[1][3]);

		if (s < surf->texturemins[0] ||
		t < surf->texturemins[1])
			continue;
		
		ds = s - surf->texturemins[0];
		dt = t - surf->texturemins[1];
		
		if ( ds > surf->extents[0] || dt > surf->extents[1] )
			continue;

		if (!surf->samples)
			return 0;

		ds >>= 4;
		dt >>= 4;

		lightmap = surf->samples;
		VectorClear (pointcolor);

		if (lightmap)
		{
			vec3_t scale;

			lightmap += 3*(dt * ((surf->extents[0]>>4)+1) + ds);

			for (maps = 0 ; maps < MAXLIGHTMAPS && surf->styles[maps] != 255 ;
					maps++)
			{
				scale[0] = gl_modulate->value*r_newrefdef.lightstyles[surf->styles[maps]].rgb[0];
				scale[1] = gl_modulate->value*r_newrefdef.lightstyles[surf->styles[maps]].rgb[1];
				scale[2] = gl_modulate->value*r_newrefdef.lightstyles[surf->styles[maps]].rgb[2];

				pointcolor[0] += lightmap[0] * scale[0] * 0.003921568627450980392156862745098f;
				pointcolor[1] += lightmap[1] * scale[1] * 0.003921568627450980392156862745098f;
				pointcolor[2] += lightmap[2] * scale[2] * 0.003921568627450980392156862745098f;
				lightmap += 3*((surf->extents[0]>>4)+1) *
						((surf->extents[1]>>4)+1);
			}
		}
		
		return 1;
	}

// go down back side
	return RecursiveLightPoint (node->children[!side], mid, end);
}

/*
===============
R_LightPoint
===============
*/
void R_LightPoint (vec3_t p, vec3_t color)
{
	vec3_t		end;
	int			r;
	int			lnum;
	dlight_t	*dl;
	//float		light;
	vec3_t		dist;
	float		add;
	
	if (!r_worldmodel->lightdata)
	{
		color[0] = color[1] = color[2] = 1.0f;
		return;
	}
	
	end[0] = p[0];
	end[1] = p[1];
	end[2] = p[2] - 2048;
	
	r = RecursiveLightPoint (r_worldmodel->nodes, p, end);
	
	if (r == -1)
	{
		VectorClear (color);
	}
	else
	{
		FastVectorCopy (pointcolor, *color);
	}

	/* Match lightmap ambient lift for entities (hard-capped). */
	if ((R_CvarEff(gl_ambient_lift) != 0.0f))
	{
		float lift = R_CvarEff(gl_ambient_lift);
		if (lift < 0.0f)
			lift = 0.0f;
		if (lift > 0.1f)
			lift = 0.1f;
		color[0] += lift;
		color[1] += lift;
		color[2] += lift;
	}

	//
	// add dynamic lights
	//
	//light = 0;
	if ((R_CvarEff(gl_dynamic) != 0.0f))
	{
		dl = r_newrefdef.dlights;
		for (lnum=0 ; lnum<r_newrefdef.num_dlights ; lnum++, dl++)
		{
			VectorSubtract (currententity->origin,
							dl->origin,
							dist);
			add = dl->intensity - VectorLength(dist);
			add *= (1.0f/256);
			if (FLOAT_GT_ZERO(add))
			{
				VectorMA (color, add, dl->color, color);
			}
		}
	}

	if (FLOAT_NE_ZERO(gl_doublelight_entities->value))
		VectorScale (color, gl_modulate->value, color);

	if (usingmodifiedlightmaps)
	{
		float		max, r, g, b;

		r = color[0];
		g = color[1];
		b = color[2];

		max = r + g + b;
		max /= 3;
		if (FLOAT_EQ_ZERO (gl_coloredlightmaps->value))
		{
			color[0] = color[1] = color[2] = max;
		}
		else
		{
			color[0] = max + (r - max) * gl_coloredlightmaps->value;
			color[1] = max + (g - max) * gl_coloredlightmaps->value;
			color[2] = max + (b - max) * gl_coloredlightmaps->value;
		}
	}
}


//===================================================================

#define BLOCKLIGHT_SIZE 3

#ifdef WIN32
__declspec(align(16)) static float s_blocklights[34*34*BLOCKLIGHT_SIZE];
#else
static float s_blocklights[34*34*BLOCKLIGHT_SIZE];
#endif

#define INTEGER_DLIGHTS		1

/*
===============
R_AddDynamicLights
===============
*/
void R_AddDynamicLights (msurface_t *surf)
{
	int			lnum;
	int			sd, td;

#ifdef INTEGER_DLIGHTS
	int			fdist, frad, fminlight;
	int			fsacc, ftacc;
	int			local[3];
#else
	float		fdist, frad, fminlight;
	float		fsacc, ftacc;
	vec3_t		local;
#endif

	vec3_t		impact;

	int			s, t;
	int			i;
	int			smax, tmax;
	mtexinfo_t	*tex;
	dlight_t	*dl;
	//float		*pfBL;

	smax = (surf->extents[0]>>4)+1;
	tmax = (surf->extents[1]>>4)+1;
	tex = surf->texinfo;

	for (lnum=0 ; lnum<r_newrefdef.num_dlights ; lnum++)
	{
		if ( !(surf->dlightbits & (1<<lnum) ) )
			continue;		// not lit by this light

		dl = &r_newrefdef.dlights[lnum];

#ifdef INTEGER_DLIGHTS
		frad = Q_ftol(dl->intensity);
#else
		frad = dl->intensity;
#endif

		fdist = (int)(DotProduct (dl->origin, surf->plane->normal) -
				surf->plane->dist);

#ifdef INTEGER_DLIGHTS
		frad -= abs(fdist);
#else
		frad -= fabs(fdist);
#endif
		// rad is now the highest intensity on the plane

		fminlight = DLIGHT_CUTOFF;	// FIXME: make configurable?

		if (frad < fminlight)
			continue;

		fminlight = frad - fminlight;

		//for (i=0 ; i<3 ; i++)
		impact[0] = dl->origin[0] - surf->plane->normal[0]*fdist;
		impact[1] = dl->origin[1] - surf->plane->normal[1]*fdist;
		impact[2] = dl->origin[2] - surf->plane->normal[2]*fdist;

		local[0] = (int)(DotProduct (impact, tex->vecs[0]) + tex->vecs[0][3] - surf->texturemins[0]);
		local[1] = (int)(DotProduct (impact, tex->vecs[1]) + tex->vecs[1][3] - surf->texturemins[1]);

		//pfBL = s_blocklights;
		i = 0;
		for (t = 0, ftacc = 0 ; t<tmax ; ftacc += 16, t++)
		{
			td = abs(local[1] - ftacc);
			//if ( td < 0 )
			//	td = -td;
			//td = abs(td);

			//for ( s=0, fsacc = 0 ; s<smax ; fsacc += 16, pfBL += 3, s++)
			s = 0;
			fsacc = 0;
			for (;;)
			{
				if (s++ == smax)
					break;
#ifdef INTEGER_DLIGHTS
				sd = abs(local[0] - fsacc);
#else
				sd = Q_ftol (local[0] - fsacc);
#endif

				//if ( sd < 0 )
				//	sd = -sd;
				//sd = abs(sd);

				if (sd > td)
					fdist = sd + (td>>1);
				else
					fdist = td + (sd>>1);

				if ( fdist < fminlight)
				{
					if (FLOAT_EQ_ZERO (gl_dlight_falloff->value))
					{
						s_blocklights[i++] += ( frad - fdist ) * dl->color[0];
						s_blocklights[i++] += ( frad - fdist ) * dl->color[1];
						s_blocklights[i++] += ( frad - fdist ) * dl->color[2];
					}
					else
					{
						/* Quadratic soft falloff: same dlights, smoother edge, no wall bleed. */
						float u = 1.0f - (float)fdist / (float)fminlight;
						float atten = u * u;
						float strength = gl_dlight_falloff->value;
						float scale;
						if (strength < 0.0f)
							strength = 0.0f;
						if (strength > 2.0f)
							strength = 2.0f;
						scale = atten * (float)frad * strength;
						s_blocklights[i++] += scale * dl->color[0];
						s_blocklights[i++] += scale * dl->color[1];
						s_blocklights[i++] += scale * dl->color[2];
					}
#if BLOCKLIGHT_SIZE == 4
					i ++;
#endif
				}
				else
				{
					i += BLOCKLIGHT_SIZE;
				}

				fsacc += 16;
			}
		}
	}
}


/*
** R_SetCacheState
*/
void R_SetCacheState( msurface_t *surf )
{
	int maps;

	for (maps = 0 ; maps < MAXLIGHTMAPS && surf->styles[maps] != 255 ;
		 maps++)
	{
		surf->cached_light[maps] = r_newrefdef.lightstyles[surf->styles[maps]].white;
	}
}

/*
===============
R_BuildLightMap

Combine and scale multiple lightmaps into the floating format in blocklights
===============
*/
void R_BuildLightMap (msurface_t *surf, byte *dest, int stride)
{
	int			smax, tmax;
	//int			r, g, b, a, max;
	int			max;
	int			colors[4];
	int			i, j, size;
	byte		*lightmap;
	float		scale[4];
	int			nummaps;
	float		*bl;

	if ( surf->texinfo->flags & (SURF_SKY|SURF_TRANS33|SURF_TRANS66|SURF_WARP) )
		ri.Sys_Error (ERR_DROP, "R_BuildLightMap called for non-lit surface");

	smax = (surf->extents[0]>>4)+1;
	tmax = (surf->extents[1]>>4)+1;

	size = smax*tmax;

	if (size > (sizeof(s_blocklights)>>4) )
		ri.Sys_Error (ERR_DROP, "R_BuildLightMap: Bad s_blocklights size %d", size);

// set to full bright if no light data
	if (!surf->samples)
	{
//		int maps;

		for (i=0 ; i<size*BLOCKLIGHT_SIZE ; i++)
			s_blocklights[i] = 255;
		/*for (maps = 0 ; maps < MAXLIGHTMAPS && surf->styles[maps] != 255 ;
			 maps++)
		{
			//style = &r_newrefdef.lightstyles[surf->styles[maps]];
		}*/
		goto store;
	}

	// count the # of maps
	for ( nummaps = 0 ; nummaps < MAXLIGHTMAPS && surf->styles[nummaps] != 255 ;
		 nummaps++)
		;

	lightmap = surf->samples;

	// add all the lightmaps
	if ( nummaps == 1 )
	{
		int maps;

		for (maps = 0 ; maps < MAXLIGHTMAPS && surf->styles[maps] != 255 ;
			 maps++)
		{
			bl = s_blocklights;

			scale[0] = gl_modulate->value*r_newrefdef.lightstyles[surf->styles[maps]].rgb[0];
			scale[1] = gl_modulate->value*r_newrefdef.lightstyles[surf->styles[maps]].rgb[1];
			scale[2] = gl_modulate->value*r_newrefdef.lightstyles[surf->styles[maps]].rgb[2];

			if ( scale[0] == 1.0F &&
				 scale[1] == 1.0F &&
				 scale[2] == 1.0F )
			{
				for (i=0 ; i<size; i++, bl+=3)
				{
					bl[0] = lightmap[i*3+0];
					bl[1] = lightmap[i*3+1];
					bl[2] = lightmap[i*3+2];
				}
			}
			else
			{
				for (i=0 ; i<size; i++, bl+=3)
				{
					bl[0] = lightmap[i*3+0] * scale[0];
					bl[1] = lightmap[i*3+1] * scale[1];
					bl[2] = lightmap[i*3+2] * scale[2];
				}
			}
			lightmap += size*3;		// skip to next lightmap
		}
	}
	else
	{
		int maps;

		memset( s_blocklights, 0, sizeof( s_blocklights[0] ) * size * BLOCKLIGHT_SIZE );

		for (maps = 0 ; maps < MAXLIGHTMAPS && surf->styles[maps] != 255 ;
			 maps++)
		{
			bl = s_blocklights;

			scale[0] = gl_modulate->value*r_newrefdef.lightstyles[surf->styles[maps]].rgb[0];
			scale[1] = gl_modulate->value*r_newrefdef.lightstyles[surf->styles[maps]].rgb[1];
			scale[2] = gl_modulate->value*r_newrefdef.lightstyles[surf->styles[maps]].rgb[2];

			if ( scale[0] == 1.0F &&
				 scale[1] == 1.0F &&
				 scale[2] == 1.0F )
			{
				for (i=0 ; i<size ; i++, bl+=3 )
				{
					bl[0] += lightmap[i*3+0];
					bl[1] += lightmap[i*3+1];
					bl[2] += lightmap[i*3+2];
				}
			}
			else
			{
				for (i=0 ; i<size ; i++, bl+=3)
				{
					bl[0] += lightmap[i*3+0] * scale[0];
					bl[1] += lightmap[i*3+1] * scale[1];
					bl[2] += lightmap[i*3+2] * scale[2];
				}
			}
			lightmap += size*3;		// skip to next lightmap
		}
	}

// add all the dynamic lights (shader path lights the fragment instead)
	if (surf->dlightframe == r_framecount && !R_WorldShaderDlights ())
		R_AddDynamicLights (surf);

// put into texture format
store:
	stride -= (smax<<2);
	bl = s_blocklights;

	//monolightmap = gl_monolightmap->string[0];

	for (i=0 ; i<tmax ; i++, dest += stride)
	{
		for (j=0 ; j<smax ; j++)
		{
			Q_fastfloats (bl, colors);

			// catch negative lights
			if (colors[0] < 0)
				colors[0] = 0;

			if (colors[1] < 0)
				colors[1] = 0;

			if (colors[2] < 0)
				colors[2] = 0;

			/* Tiny ambient lift with hard cap (gl_ambient_lift 0..0.1). Not night-vision. */
			if ((R_CvarEff(gl_ambient_lift) != 0.0f))
			{
				float lift = R_CvarEff(gl_ambient_lift);
				int add;
				if (lift < 0.0f)
					lift = 0.0f;
				if (lift > 0.1f)
					lift = 0.1f;
				add = (int)(lift * 255.0f + 0.5f);
				if (add > 0)
				{
					colors[0] += add;
					colors[1] += add;
					colors[2] += add;
					if (colors[0] > 255) colors[0] = 255;
					if (colors[1] > 255) colors[1] = 255;
					if (colors[2] > 255) colors[2] = 255;
				}
			}

			/*
			** determine the brightest of the three color components
			*/
			if (colors[0] > colors[1])
				max = colors[0];
			else
				max = colors[1];
			if (colors[2] > max)
				max = colors[2];

			/*
			** alpha is ONLY used for the mono lightmap case.  For this reason
			** we set it to the brightest of the color components so that 
			** things don't get too dim.
			*/
			//a = max;
			colors[3] = max;

			/*
			** rescale all the color components if the intensity of the greatest
			** channel exceeds 1.0
			*/
			if (max > 255)
			{
				float t = 255.0F / max;

				colors[0] = Q_ftol(colors[0]*t);
				colors[1] = Q_ftol(colors[1]*t);
				colors[2] = Q_ftol(colors[2]*t);
				colors[3] = Q_ftol(colors[3]*t);
			}

			if (!usingmodifiedlightmaps)
			{
				dest[0] = colors[0];
				dest[1] = colors[1];
				dest[2] = colors[2];
			}
			else
			{
				//max = colors[0] + colors[1] + colors[2];
				//max /= 3;
				if (FLOAT_NE_ZERO (gl_r1gl_test->value))
					max = (int)(0.289f * colors[0] + 0.587f * colors[1] + 0.114f * colors[2]);
				else
					max = (colors[0] + colors[1] + colors[2]) / 3;
				if (FLOAT_EQ_ZERO (gl_coloredlightmaps->value))
				{
					dest[0] = dest[1] = dest[2] = max;
				}
				else
				{
					dest[0] = (byte)Q_ftol(max + (colors[0] - max) * gl_coloredlightmaps->value);
					dest[1] = (byte)Q_ftol(max + (colors[1] - max) * gl_coloredlightmaps->value);
					dest[2] = (byte)Q_ftol(max + (colors[2] - max) * gl_coloredlightmaps->value);
				}
			}

			dest[3] = colors[3];

			bl += BLOCKLIGHT_SIZE;
			dest += 4;
		}
	}
}
