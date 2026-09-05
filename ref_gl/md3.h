/*
==============================================================================
MD3 / IDP3 triangle model format (Quake III) ? disk + in-memory for R1GL.

Used for player vweps that ship MD3 bytes under .md2 names (magic IDP3).
Tags and Q3 shaders are not used; skins are resolved as Q2 image paths.
==============================================================================
*/

#ifndef REF_GL_MD3_H
#define REF_GL_MD3_H

#define IDMD3HEADER		(('3'<<24)+('P'<<16)+('D'<<8)+'I')	/* "IDP3" LE */
#define MD3_VERSION		15

#define MD3_MAX_LODS		3
#define MD3_MAX_TRIANGLES	8192
#define MD3_MAX_VERTS		4096
#define MD3_MAX_SKINS		256
#define MD3_MAX_FRAMES		1024
#define MD3_MAX_MESHES		32
#define MD3_MAX_TAGS		16
#define MD3_MAX_PATH		64

#define MD3_XYZ_SCALE		(1.0f/64.0f)

/* ---- on-disk (little-endian) ---- */

typedef struct {
	float	st[2];
} dmd3coord_t;

typedef struct {
	short	point[3];
	byte	norm[2];	/* lat, lng */
} dmd3vertex_t;

typedef struct {
	float	mins[3];
	float	maxs[3];
	float	translate[3];
	float	radius;
	char	creator[16];
} dmd3frame_t;

typedef struct {
	char	name[MD3_MAX_PATH];
	float	origin[3];
	float	axis[3][3];
} dmd3tag_t;

typedef struct {
	char	name[MD3_MAX_PATH];
	int		unused;		/* shader index in Q3; ignored */
} dmd3skin_t;

typedef struct {
	int		ident;
	char	name[MD3_MAX_PATH];
	int		flags;
	int		num_frames;
	int		num_skins;
	int		num_verts;
	int		num_tris;
	int		ofs_indexes;
	int		ofs_skins;
	int		ofs_tcs;
	int		ofs_verts;
	int		meshsize;
} dmd3mesh_t;

typedef struct {
	int		ident;
	int		version;
	char	filename[MD3_MAX_PATH];
	int		flags;
	int		num_frames;
	int		num_tags;
	int		num_meshes;
	int		num_skins;
	int		ofs_frames;
	int		ofs_tags;
	int		ofs_meshes;
	int		ofs_end;
} dmd3header_t;

/* ---- in-memory (hunk) ---- */

typedef struct {
	vec3_t	translate;
	float	radius;
	vec3_t	mins, maxs;
} md3frameinfo_t;

typedef struct {
	short	xyz[3];
	byte	norm[2];
} md3vert_t;

typedef struct {
	char		name[MD3_MAX_PATH];
	int			num_verts;
	int			num_tris;
	int			num_skins;
	float		*st;			/* num_verts * 2 */
	unsigned	*indexes;		/* num_tris * 3 */
	md3vert_t	*verts;			/* num_frames * num_verts */
	char		skinnames[32][MAX_SKINNAME];	/* capped like MD2 */
	image_t		*skins[32];
} md3mesh_mem_t;

typedef struct {
	int				ident;		/* IDMD3HEADER */
	int				version;
	int				num_frames;
	int				num_meshes;
	int				num_tris;	/* total tris (stats) */
	md3frameinfo_t	*frames;
	md3mesh_mem_t	*meshes;
} md3model_t;

void Mod_LoadMD3Model (model_t *mod, void *buffer, int filesize);
void R_DrawAliasMD3Model (entity_t *e);
qboolean R_CullAliasMD3Model (vec3_t bbox[8], entity_t *e);

#endif /* REF_GL_MD3_H */
