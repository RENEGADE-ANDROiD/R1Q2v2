/*
R1GL off-screen 3D + fair-play post-FX (bloom, radial god rays, soft particles).

Only the 3D color/depth buffers are used. HUD is drawn after the blit.
Occluded lights are already black in the scene, so bloom/rays cannot show
through walls. Soft particles fade against a *copy* of scene depth.

Bloom is on/off only. Intensity is a hard cap (not a slider) so it cannot
be cranked into a through-wall glow from the bright-extract blur.
*/
#include "gl_local.h"

#ifndef GL_FRAMEBUFFER
#define GL_FRAMEBUFFER                 0x8D40
#define GL_RENDERBUFFER                0x8D41
#define GL_COLOR_ATTACHMENT0           0x8CE0
#define GL_DEPTH_ATTACHMENT            0x8D00
#define GL_FRAMEBUFFER_COMPLETE        0x8CD5
#define GL_READ_FRAMEBUFFER            0x8CA8
#define GL_DRAW_FRAMEBUFFER            0x8CA9
#endif
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE               0x812F
#endif
#ifndef GL_DEPTH_COMPONENT24
#define GL_DEPTH_COMPONENT24           0x81A6
#endif
#ifndef GL_RGBA8
#define GL_RGBA8                       0x8058
#endif
#ifndef GL_FRAGMENT_SHADER
#define GL_VERTEX_SHADER               0x8B31
#define GL_FRAGMENT_SHADER             0x8B30
#define GL_COMPILE_STATUS              0x8B81
#define GL_LINK_STATUS                 0x8B82
#define GL_INFO_LOG_LENGTH             0x8B84
#endif
#ifndef GL_TEXTURE2
#define GL_TEXTURE2                    0x84C2
#endif

/* Fair-play bloom: any gl_bloom > 0 turns it on at this add, never more. */
#define R_BLOOM_ADD        0.20f
#define R_BLOOM_THRESHOLD  0.80f

cvar_t	*gl_bloom;
cvar_t	*gl_godrays;
cvar_t	*gl_softparticles;
cvar_t	*gl_dlight_shader;
cvar_t	*gl_normalmaps;

int		r_viewport[4];
float	r_projection_matrix[16];

static qboolean	pf_ok;
static qboolean	pf_bound;
static qboolean	pf_have_blit;
static int		pf_w, pf_h, pf_qw, pf_qh;

static GLuint	fbo_scene, fbo_depthcopy, tex_color, tex_depth, tex_depthcopy;
static GLuint	fbo_ping[2], tex_ping[2];

static GLuint	prog_extract, prog_blur, prog_rays, prog_composite, prog_particle;
static GLuint	prog_world;
static qboolean	world_shader_bound;

typedef void (APIENTRY *PFNbindFBO)(GLenum, GLuint);
typedef void (APIENTRY *PFNgenFBO)(GLsizei, GLuint *);
typedef void (APIENTRY *PFNdelFBO)(GLsizei, const GLuint *);
typedef void (APIENTRY *PFNfboTex)(GLenum, GLenum, GLenum, GLuint, GLint);
typedef GLenum (APIENTRY *PFNcheckFBO)(GLenum);
typedef void (APIENTRY *PFNblitFBO)(GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLbitfield, GLenum);

typedef GLuint (APIENTRY *PFNcreateSh)(GLenum);
typedef void (APIENTRY *PFNshaderSrc)(GLuint, GLsizei, const char **, const GLint *);
typedef void (APIENTRY *PFNcompileSh)(GLuint);
typedef void (APIENTRY *PFNgetShiv)(GLuint, GLenum, GLint *);
typedef void (APIENTRY *PFNgetShLog)(GLuint, GLsizei, GLsizei *, char *);
typedef GLuint (APIENTRY *PFNcreateProg)(void);
typedef void (APIENTRY *PFNattachSh)(GLuint, GLuint);
typedef void (APIENTRY *PFNlinkProg)(GLuint);
typedef void (APIENTRY *PFNuseProg)(GLuint);
typedef void (APIENTRY *PFNgetPiv)(GLuint, GLenum, GLint *);
typedef void (APIENTRY *PFNgetPLog)(GLuint, GLsizei, GLsizei *, char *);
typedef GLint (APIENTRY *PFNuniLoc)(GLuint, const char *);
typedef void (APIENTRY *PFNuni1i)(GLint, GLint);
typedef void (APIENTRY *PFNuni1f)(GLint, GLfloat);
typedef void (APIENTRY *PFNuni2f)(GLint, GLfloat, GLfloat);
typedef void (APIENTRY *PFNuni3f)(GLint, GLfloat, GLfloat, GLfloat);
typedef void (APIENTRY *PFNuni4f)(GLint, GLfloat, GLfloat, GLfloat, GLfloat);
typedef void (APIENTRY *PFNdelSh)(GLuint);
typedef void (APIENTRY *PFNdelProg)(GLuint);

static PFNbindFBO	p_BindFramebuffer;
static PFNgenFBO	p_GenFramebuffers;
static PFNdelFBO	p_DeleteFramebuffers;
static PFNfboTex	p_FramebufferTexture2D;
static PFNcheckFBO	p_CheckFramebufferStatus;
static PFNblitFBO	p_BlitFramebuffer;

static PFNcreateSh	p_CreateShader;
static PFNshaderSrc	p_ShaderSource;
static PFNcompileSh	p_CompileShader;
static PFNgetShiv	p_GetShaderiv;
static PFNgetShLog	p_GetShaderInfoLog;
static PFNcreateProg	p_CreateProgram;
static PFNattachSh	p_AttachShader;
static PFNlinkProg	p_LinkProgram;
static PFNuseProg	p_UseProgram;
static PFNgetPiv	p_GetProgramiv;
static PFNgetPLog	p_GetProgramInfoLog;
static PFNuniLoc	p_GetUniformLocation;
static PFNuni1i		p_Uniform1i;
static PFNuni1f		p_Uniform1f;
static PFNuni2f		p_Uniform2f;
static PFNuni3f		p_Uniform3f;
static PFNuni4f		p_Uniform4f;
static PFNdelSh		p_DeleteShader;
static PFNdelProg	p_DeleteProgram;

static void *R_GLProc (const char *a, const char *b, const char *c)
{
	void	*p = NULL;
#ifdef _WIN32
	if (a) p = (void *)qwglGetProcAddress (a);
	if (!p && b) p = (void *)qwglGetProcAddress (b);
	if (!p && c) p = (void *)qwglGetProcAddress (c);
#else
	if (a) p = qwglGetProcAddress (a);
	if (!p && b) p = qwglGetProcAddress (b);
	if (!p && c) p = qwglGetProcAddress (c);
#endif
	return p;
}

static const char *vs_fx =
	"void main(){\n"
	"  gl_TexCoord[0] = gl_MultiTexCoord0;\n"
	"  gl_Position = gl_Vertex;\n"
	"}\n";

static const char *fs_extract =
	"uniform sampler2D u_tex;\n"
	"uniform float u_threshold;\n"
	"void main(){\n"
	"  vec3 c = texture2D(u_tex, gl_TexCoord[0].xy).rgb;\n"
	"  float l = dot(c, vec3(0.30, 0.59, 0.11));\n"
	"  float t = smoothstep(u_threshold, u_threshold + 0.22, l);\n"
	"  gl_FragColor = vec4(c * t, 1.0);\n"
	"}\n";

static const char *fs_blur =
	"uniform sampler2D u_tex;\n"
	"uniform vec2 u_texel;\n"
	"void main(){\n"
	"  vec2 uv = gl_TexCoord[0].xy;\n"
	"  vec3 c = texture2D(u_tex, uv).rgb * 0.227027027;\n"
	"  c += texture2D(u_tex, uv + u_texel).rgb * 0.1945945946;\n"
	"  c += texture2D(u_tex, uv - u_texel).rgb * 0.1945945946;\n"
	"  c += texture2D(u_tex, uv + u_texel * 2.0).rgb * 0.1216216216;\n"
	"  c += texture2D(u_tex, uv - u_texel * 2.0).rgb * 0.1216216216;\n"
	"  c += texture2D(u_tex, uv + u_texel * 3.0).rgb * 0.054054054;\n"
	"  c += texture2D(u_tex, uv - u_texel * 3.0).rgb * 0.054054054;\n"
	"  gl_FragColor = vec4(c, 1.0);\n"
	"}\n";

static const char *fs_rays =
	"uniform sampler2D u_tex;\n"
	"uniform vec2 u_light;\n"
	"uniform float u_weight;\n"
	"void main(){\n"
	"  vec2 uv = gl_TexCoord[0].xy;\n"
	"  vec2 d = (u_light - uv) * 0.0555556;\n"
	"  vec3 acc = vec3(0.0);\n"
	"  float illum = 1.0;\n"
	"  vec2 p = uv;\n"
	"  int i;\n"
	"  for (i = 0; i < 18; i++) {\n"
	"    vec3 s = texture2D(u_tex, p).rgb;\n"
	"    acc += s * illum;\n"
	"    illum *= 0.90;\n"
	"    p += d;\n"
	"  }\n"
	"  gl_FragColor = vec4(acc * u_weight, 1.0);\n"
	"}\n";

static const char *fs_composite =
	"uniform sampler2D u_scene;\n"
	"uniform sampler2D u_addtex;\n"
	"uniform float u_add;\n"
	"uniform vec2 u_uvscale;\n"
	"uniform vec2 u_uvbias;\n"
	"void main(){\n"
	"  vec2 uv = gl_TexCoord[0].xy * u_uvscale + u_uvbias;\n"
	"  vec3 a = texture2D(u_scene, uv).rgb;\n"
	"  vec3 b = texture2D(u_addtex, uv).rgb;\n"
	"  gl_FragColor = vec4(a + b * u_add, 1.0);\n"
	"}\n";

static const char *vs_particle =
	"varying vec4 v_color;\n"
	"varying vec2 v_uv;\n"
	"void main(){\n"
	"  v_color = gl_Color;\n"
	"  v_uv = gl_MultiTexCoord0.xy;\n"
	"  gl_Position = ftransform();\n"
	"}\n";

static const char *fs_particle =
	"uniform sampler2D u_tex;\n"
	"uniform sampler2D u_depth;\n"
	"uniform vec2 u_screensize;\n"
	"uniform float u_znear;\n"
	"uniform float u_zfar;\n"
	"uniform float u_soft;\n"
	"varying vec4 v_color;\n"
	"varying vec2 v_uv;\n"
	"float linearize(float z){\n"
	"  float n = u_znear, f = u_zfar;\n"
	"  return (2.0 * n) / (f + n - z * (f - n));\n"
	"}\n"
	"void main(){\n"
	"  vec4 c = texture2D(u_tex, v_uv) * v_color;\n"
	"  vec2 uv = gl_FragCoord.xy / u_screensize;\n"
	"  float sceneZ = linearize(texture2D(u_depth, uv).r);\n"
	"  float z = linearize(gl_FragCoord.z);\n"
	"  float fade = clamp((sceneZ - z) * u_soft, 0.0, 1.0);\n"
	"  c.a *= fade;\n"
	"  gl_FragColor = c;\n"
	"}\n";

#define WORLD_SHADER_LIGHTS	8

static const char *vs_world =
	"varying vec3 v_pos;\n"
	"varying vec3 v_n;\n"
	"varying vec2 v_uv0;\n"
	"varying vec2 v_uv1;\n"
	"void main(){\n"
	"  v_pos = gl_Vertex.xyz;\n"
	"  v_n = gl_Normal;\n"
	"  v_uv0 = gl_MultiTexCoord0.xy;\n"
	"  v_uv1 = gl_MultiTexCoord1.xy;\n"
	"  gl_Position = ftransform();\n"
	"}\n";

static const char *fs_world =
	"uniform sampler2D u_diff;\n"
	"uniform sampler2D u_lm;\n"
	"uniform sampler2D u_norm;\n"
	"uniform int u_nlights;\n"
	"uniform int u_hasnorm;\n"
	"uniform float u_over;\n"
	"uniform float u_falloff;\n"
	"uniform vec4 u_lpos[8];\n"
	"uniform vec3 u_lcol[8];\n"
	"varying vec3 v_pos;\n"
	"varying vec3 v_n;\n"
	"varying vec2 v_uv0;\n"
	"varying vec2 v_uv1;\n"
	"void main(){\n"
	"  vec3 albedo = texture2D(u_diff, v_uv0).rgb;\n"
	"  vec3 lm = texture2D(u_lm, v_uv1).rgb * u_over;\n"
	"  vec3 n = normalize(v_n);\n"
	"  if (u_hasnorm != 0) {\n"
	"    vec3 t = texture2D(u_norm, v_uv0).xyz * 2.0 - 1.0;\n"
	"    n = normalize(n + t * 0.45);\n"
	"  }\n"
	"  vec3 add = vec3(0.0);\n"
	"  int i;\n"
	"  for (i = 0; i < 8; i++) {\n"
	"    float on = step(float(i) + 0.5, float(u_nlights));\n"
	"    vec3 L = u_lpos[i].xyz - v_pos;\n"
	"    float dist = length(L);\n"
	"    float rad = u_lpos[i].w;\n"
	"    float att = 1.0 - dist / max(rad, 1.0);\n"
	"    att = max(att, 0.0);\n"
	"    L = L / max(dist, 0.001);\n"
	"    float ndotl = max(dot(n, L), 0.0);\n"
	"    if (u_falloff > 0.0)\n"
	"      att = att * att * u_falloff;\n"
	"    add += u_lcol[i] * (att * ndotl * (rad / 255.0) * on);\n"
	"  }\n"
	"  gl_FragColor = vec4(albedo * (lm + add), 1.0);\n"
	"}\n";

static void R_LogShader (GLuint obj, qboolean prog, const char *name)
{
	char	buf[1024];
	GLint	ok = 0, len = 0;

	buf[0] = 0;
	if (prog)
	{
		p_GetProgramiv (obj, GL_LINK_STATUS, &ok);
		if (ok)
			return;
		p_GetProgramiv (obj, GL_INFO_LOG_LENGTH, &len);
		if (len > 1 && p_GetProgramInfoLog)
			p_GetProgramInfoLog (obj, sizeof(buf) - 1, NULL, buf);
	}
	else
	{
		p_GetShaderiv (obj, GL_COMPILE_STATUS, &ok);
		if (ok)
			return;
		p_GetShaderiv (obj, GL_INFO_LOG_LENGTH, &len);
		if (len > 1 && p_GetShaderInfoLog)
			p_GetShaderInfoLog (obj, sizeof(buf) - 1, NULL, buf);
	}
	buf[sizeof(buf) - 1] = 0;
	ri.Con_Printf (PRINT_ALL, "R1GL post-FX %s failed: %s\n", name, buf[0] ? buf : "unknown");
}

static GLuint R_MakeProgram (const char *vs, const char *fs, const char *name)
{
	GLuint	v, f, p;
	GLint	ok = 0;
	const char	*vsrc = vs;
	const char	*fsrc = fs;

	v = p_CreateShader (GL_VERTEX_SHADER);
	p_ShaderSource (v, 1, &vsrc, NULL);
	p_CompileShader (v);
	p_GetShaderiv (v, GL_COMPILE_STATUS, &ok);
	if (!ok)
	{
		R_LogShader (v, false, name);
		p_DeleteShader (v);
		return 0;
	}

	f = p_CreateShader (GL_FRAGMENT_SHADER);
	p_ShaderSource (f, 1, &fsrc, NULL);
	p_CompileShader (f);
	p_GetShaderiv (f, GL_COMPILE_STATUS, &ok);
	if (!ok)
	{
		R_LogShader (f, false, name);
		p_DeleteShader (v);
		p_DeleteShader (f);
		return 0;
	}

	p = p_CreateProgram ();
	p_AttachShader (p, v);
	p_AttachShader (p, f);
	p_LinkProgram (p);
	p_GetProgramiv (p, GL_LINK_STATUS, &ok);
	p_DeleteShader (v);
	p_DeleteShader (f);
	if (!ok)
	{
		R_LogShader (p, true, name);
		p_DeleteProgram (p);
		return 0;
	}
	return p;
}

static void R_DeleteTex (GLuint *t)
{
	if (*t)
	{
		qglDeleteTextures (1, t);
		*t = 0;
	}
}

static void R_DestroyTargets (void)
{
	if (p_DeleteFramebuffers)
	{
		if (fbo_scene) p_DeleteFramebuffers (1, &fbo_scene);
		if (fbo_depthcopy) p_DeleteFramebuffers (1, &fbo_depthcopy);
		if (fbo_ping[0]) p_DeleteFramebuffers (1, &fbo_ping[0]);
		if (fbo_ping[1]) p_DeleteFramebuffers (1, &fbo_ping[1]);
	}
	fbo_scene = fbo_depthcopy = fbo_ping[0] = fbo_ping[1] = 0;
	R_DeleteTex (&tex_color);
	R_DeleteTex (&tex_depth);
	R_DeleteTex (&tex_depthcopy);
	R_DeleteTex (&tex_ping[0]);
	R_DeleteTex (&tex_ping[1]);
	pf_w = pf_h = 0;
}

static GLuint R_MakeColorTex (int w, int h, qboolean linear)
{
	GLuint	t;
	qglGenTextures (1, &t);
	qglBindTexture (GL_TEXTURE_2D, t);
	qglTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, linear ? GL_LINEAR : GL_NEAREST);
	qglTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, linear ? GL_LINEAR : GL_NEAREST);
	qglTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	qglTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	qglTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	return t;
}

static qboolean R_CreateTargets (int w, int h)
{
	GLenum	status;
	int		qw, qh;

	if (w < 1) w = 1;
	if (h < 1) h = 1;
	qw = w / 4;
	qh = h / 4;
	if (qw < 1) qw = 1;
	if (qh < 1) qh = 1;

	if (fbo_scene && pf_w == w && pf_h == h)
		return true;

	R_DestroyTargets ();

	tex_color = R_MakeColorTex (w, h, true);
	tex_ping[0] = R_MakeColorTex (qw, qh, true);
	tex_ping[1] = R_MakeColorTex (qw, qh, true);

	qglGenTextures (1, &tex_depth);
	qglBindTexture (GL_TEXTURE_2D, tex_depth);
	qglTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	qglTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	qglTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	qglTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	qglTexImage2D (GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, w, h, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL);

	qglGenTextures (1, &tex_depthcopy);
	qglBindTexture (GL_TEXTURE_2D, tex_depthcopy);
	qglTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	qglTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	qglTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	qglTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	qglTexImage2D (GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, w, h, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL);

	p_GenFramebuffers (1, &fbo_scene);
	p_BindFramebuffer (GL_FRAMEBUFFER, fbo_scene);
	p_FramebufferTexture2D (GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex_color, 0);
	p_FramebufferTexture2D (GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, tex_depth, 0);
	status = p_CheckFramebufferStatus (GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE)
	{
		ri.Con_Printf (PRINT_ALL, "R1GL post-FX scene FBO incomplete (0x%x)\n", (int)status);
		p_BindFramebuffer (GL_FRAMEBUFFER, 0);
		R_DestroyTargets ();
		return false;
	}

	p_GenFramebuffers (1, &fbo_depthcopy);
	p_BindFramebuffer (GL_FRAMEBUFFER, fbo_depthcopy);
	p_FramebufferTexture2D (GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, tex_depthcopy, 0);
	qglDrawBuffer (GL_NONE);
	qglReadBuffer (GL_NONE);
	status = p_CheckFramebufferStatus (GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE)
	{
		/* Soft particles need this copy; bloom/rays can still run. */
		p_DeleteFramebuffers (1, &fbo_depthcopy);
		fbo_depthcopy = 0;
	}

	p_GenFramebuffers (1, &fbo_ping[0]);
	p_BindFramebuffer (GL_FRAMEBUFFER, fbo_ping[0]);
	p_FramebufferTexture2D (GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex_ping[0], 0);
	status = p_CheckFramebufferStatus (GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE)
	{
		p_BindFramebuffer (GL_FRAMEBUFFER, 0);
		R_DestroyTargets ();
		return false;
	}

	p_GenFramebuffers (1, &fbo_ping[1]);
	p_BindFramebuffer (GL_FRAMEBUFFER, fbo_ping[1]);
	p_FramebufferTexture2D (GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex_ping[1], 0);
	status = p_CheckFramebufferStatus (GL_FRAMEBUFFER);
	p_BindFramebuffer (GL_FRAMEBUFFER, 0);
	if (status != GL_FRAMEBUFFER_COMPLETE)
	{
		R_DestroyTargets ();
		return false;
	}

	pf_w = w;
	pf_h = h;
	pf_qw = qw;
	pf_qh = qh;
	qglBindTexture (GL_TEXTURE_2D, 0);
	return true;
}

static void R_DrawFSQuad (void)
{
	qglBegin (GL_TRIANGLE_STRIP);
	qglTexCoord2f (0.0f, 0.0f); qglVertex2f (-1.0f, -1.0f);
	qglTexCoord2f (1.0f, 0.0f); qglVertex2f ( 1.0f, -1.0f);
	qglTexCoord2f (0.0f, 1.0f); qglVertex2f (-1.0f,  1.0f);
	qglTexCoord2f (1.0f, 1.0f); qglVertex2f ( 1.0f,  1.0f);
	qglEnd ();
}

static void R_BindTex0 (GLuint tex)
{
	if (qglActiveTextureARB)
		qglActiveTextureARB (GL_TEXTURE0);
	qglBindTexture (GL_TEXTURE_2D, tex);
	gl_state.currenttextures[0] = tex;
	gl_state.currenttmu = 0;
}

static void R_PassTo (GLuint fbo, int w, int h, GLuint prog)
{
	p_BindFramebuffer (GL_FRAMEBUFFER, fbo);
	qglViewport (0, 0, w, h);
	p_UseProgram (prog);
	qglEnable (GL_TEXTURE_2D);
	qglDisable (GL_DEPTH_TEST);
	qglDisable (GL_CULL_FACE);
	qglDisable (GL_BLEND);
	qglDisable (GL_ALPHA_TEST);
	qglDepthMask (GL_FALSE);
}

static qboolean R_ProjectToFBO (vec3_t origin, float *u, float *v)
{
	float	eye[4], clip[4], x, y, z;
	const float	*m = r_world_matrix;
	const float	*p = r_projection_matrix;
	int		vx, vy, vw, vh;

	if (pf_w < 1 || pf_h < 1)
		return false;

	eye[0] = m[0]*origin[0] + m[4]*origin[1] + m[8]*origin[2] + m[12];
	eye[1] = m[1]*origin[0] + m[5]*origin[1] + m[9]*origin[2] + m[13];
	eye[2] = m[2]*origin[0] + m[6]*origin[1] + m[10]*origin[2] + m[14];
	eye[3] = m[3]*origin[0] + m[7]*origin[1] + m[11]*origin[2] + m[15];

	clip[0] = p[0]*eye[0] + p[4]*eye[1] + p[8]*eye[2] + p[12]*eye[3];
	clip[1] = p[1]*eye[0] + p[5]*eye[1] + p[9]*eye[2] + p[13]*eye[3];
	clip[2] = p[2]*eye[0] + p[6]*eye[1] + p[10]*eye[2] + p[14]*eye[3];
	clip[3] = p[3]*eye[0] + p[7]*eye[1] + p[11]*eye[2] + p[15]*eye[3];
	if (clip[3] <= 0.15f)
		return false;

	x = clip[0] / clip[3];
	y = clip[1] / clip[3];
	z = clip[2] / clip[3];
	if (z < -1.0f || z > 1.0f)
		return false;
	if (x < -1.35f || x > 1.35f || y < -1.35f || y > 1.35f)
		return false;

	vx = r_viewport[0];
	vy = r_viewport[1];
	vw = r_viewport[2];
	vh = r_viewport[3];
	*u = (vx + (x * 0.5f + 0.5f) * vw) / (float)pf_w;
	*v = (vy + (y * 0.5f + 0.5f) * vh) / (float)pf_h;
	return true;
}

static qboolean R_PickGodRay (float *u, float *v)
{
	int		i, n;
	float	best, dist, iu, iv, score;
	vec3_t	origin, delta;
	dlight_t	*dl;
	qboolean	found = false;

	best = 0.0f;
	*u = *v = 0.5f;

	dl = r_newrefdef.dlights;
	for (i = 0; i < r_newrefdef.num_dlights; i++, dl++)
	{
		if (dl->intensity < 80.0f)
			continue;
		if (!R_LightOriginVisible (dl->origin))
			continue;
		if (!R_ProjectToFBO (dl->origin, &iu, &iv))
			continue;
		VectorSubtract (dl->origin, r_origin, delta);
		dist = VectorLength (delta);
		if (dist < 32.0f)
			continue;
		score = dl->intensity / dist;
		if (score > best)
		{
			best = score;
			*u = iu;
			*v = iv;
			found = true;
		}
	}

	n = R_NumWorldLights ();
	for (i = 0; i < n; i++)
	{
		float	intensity;
		R_WorldLightOrigin (i, origin, &intensity);
		if (intensity < 80.0f)
			continue;
		if (!R_LightOriginVisible (origin))
			continue;
		if (!R_ProjectToFBO (origin, &iu, &iv))
			continue;
		VectorSubtract (origin, r_origin, delta);
		dist = VectorLength (delta);
		if (dist < 32.0f)
			continue;
		score = intensity / dist * 0.35f;
		if (score > best)
		{
			best = score;
			*u = iu;
			*v = iv;
			found = true;
		}
	}

	return found && best > 0.08f;
}

static qboolean R_PostFX_Wanted (void)
{
	if (!pf_ok)
		return false;
	if (r_newrefdef.rdflags & RDF_NOWORLDMODEL)
		return false;
	if (gl_bloom && gl_bloom->value > 0.0f)
		return true;
	if (gl_godrays && gl_godrays->value > 0.0f)
		return true;
	if (gl_softparticles && gl_softparticles->value > 0.0f)
		return true;
	return false;
}

static image_t *R_FindNormalMap (image_t *diffuse)
{
	char		stem[MAX_QPATH];
	image_t		*im;
	static const char *sfx[] = { "_norm", "_n", "_bump", NULL };
	int			i;

	if (!diffuse || !gl_normalmaps || FLOAT_EQ_ZERO(gl_normalmaps->value))
		return NULL;
	if (diffuse->normalmap)
	{
		if (diffuse->normalmap == r_notexture)
			return NULL;
		if (diffuse->normalmap->registration_sequence == registration_sequence)
			return diffuse->normalmap;
	}

	im = NULL;
	for (i = 0; sfx[i]; i++)
	{
		char	name[MAX_QPATH];
		size_t	length;

		/* HD packs ship _norm as png/jpg/tga. Do not fall through to
		 * textures/foo_norm.wal — vanilla wals do not exist and the
		 * miss path used to hash r_notexture (client freeze). */
		Com_sprintf (stem, sizeof(stem), "%s%s", diffuse->basename, sfx[i]);
		im = GL_FindImageBase (stem, it_wall);
		if (im && im != r_notexture)
			break;
		im = NULL;
		Com_sprintf (name, sizeof(name), "textures/%s.wal", stem);
		length = strlen (name);
		if (length < 4)
			continue;
		if (load_png_wals)
		{
			memcpy (name + length - 3, "png", 3);
			im = GL_FindImage (name, stem, it_wall);
		}
		if (!im && load_jpg_wals)
		{
			memcpy (name + length - 3, "jpg", 3);
			im = GL_FindImage (name, stem, it_wall);
		}
		if (!im && load_tga_wals)
		{
			memcpy (name + length - 3, "tga", 3);
			im = GL_FindImage (name, stem, it_wall);
		}
		if (im && im != r_notexture)
			break;
		im = NULL;
	}
	if (!im || im == r_notexture)
	{
		diffuse->normalmap = r_notexture;
		return NULL;
	}
	diffuse->normalmap = im;
	return im;
}

qboolean R_WorldShaderDlights (void)
{
	if (!prog_world || !p_UseProgram)
		return false;
	if (!gl_dlight_shader || FLOAT_EQ_ZERO(gl_dlight_shader->value))
		return false;
	if (FLOAT_EQ_ZERO(gl_dynamic->value))
		return false;
	return true;
}

qboolean R_WorldShaderBegin (msurface_t *surf, image_t *image)
{
	image_t		*nmap;
	dlight_t	*dl;
	char		uname[32];
	int			n, i;
	GLint		loc;
	float		over, fall;
	qboolean	want_dlights;

	if (!prog_world || !p_UseProgram || !surf || !image)
		return false;

	want_dlights = R_WorldShaderDlights () && surf->dlightframe == r_framecount
		&& surf->dlightbits;
	nmap = R_FindNormalMap (image);
	if (!want_dlights && !nmap)
		return false;

	p_UseProgram (prog_world);
	world_shader_bound = true;

	loc = p_GetUniformLocation (prog_world, "u_diff");
	if (loc >= 0) p_Uniform1i (loc, 0);
	loc = p_GetUniformLocation (prog_world, "u_lm");
	if (loc >= 0) p_Uniform1i (loc, 1);
	loc = p_GetUniformLocation (prog_world, "u_norm");
	if (loc >= 0) p_Uniform1i (loc, 2);

	over = (gl_overbrights && FLOAT_NE_ZERO(gl_overbrights->value)) ? 2.0f : 1.0f;
	loc = p_GetUniformLocation (prog_world, "u_over");
	if (loc >= 0) p_Uniform1f (loc, over);
	fall = (gl_dlight_falloff) ? gl_dlight_falloff->value : 0.0f;
	loc = p_GetUniformLocation (prog_world, "u_falloff");
	if (loc >= 0) p_Uniform1f (loc, fall);

	n = 0;
	if (want_dlights)
	{
		for (i = 0; i < r_newrefdef.num_dlights && n < WORLD_SHADER_LIGHTS; i++)
		{
			if (!(surf->dlightbits & (1 << i)))
				continue;
			dl = &r_newrefdef.dlights[i];
			Com_sprintf (uname, sizeof(uname), "u_lpos[%d]", n);
			loc = p_GetUniformLocation (prog_world, uname);
			if (loc >= 0 && p_Uniform4f)
				p_Uniform4f (loc, dl->origin[0], dl->origin[1], dl->origin[2], dl->intensity);
			Com_sprintf (uname, sizeof(uname), "u_lcol[%d]", n);
			loc = p_GetUniformLocation (prog_world, uname);
			if (loc >= 0 && p_Uniform3f)
				p_Uniform3f (loc, dl->color[0], dl->color[1], dl->color[2]);
			n++;
		}
	}
	loc = p_GetUniformLocation (prog_world, "u_nlights");
	if (loc >= 0) p_Uniform1i (loc, n);
	loc = p_GetUniformLocation (prog_world, "u_hasnorm");
	if (loc >= 0) p_Uniform1i (loc, nmap ? 1 : 0);

	if (nmap && qglActiveTextureARB)
	{
		qglActiveTextureARB (GL_TEXTURE2);
		qglEnable (GL_TEXTURE_2D);
		qglBindTexture (GL_TEXTURE_2D, nmap->texnum);
		qglActiveTextureARB (GL_TEXTURE0);
		gl_state.currenttmu = 0;
		gl_state.currenttarget = GL_TEXTURE0;
	}

	return true;
}

void R_WorldShaderEnd (void)
{
	if (!world_shader_bound)
		return;
	if (qglActiveTextureARB)
	{
		qglActiveTextureARB (GL_TEXTURE2);
		qglBindTexture (GL_TEXTURE_2D, 0);
		qglDisable (GL_TEXTURE_2D);
		qglActiveTextureARB (GL_TEXTURE0);
		gl_state.currenttmu = 0;
		gl_state.currenttarget = GL_TEXTURE0;
	}
	if (p_UseProgram)
		p_UseProgram (0);
	world_shader_bound = false;
}

void R_PostFX_Init (void)
{
	const char	*ext;
	qboolean	have_glsl;

	pf_ok = false;
	prog_world = 0;
	world_shader_bound = false;
	gl_config.r1gl_FBO = false;
	gl_config.r1gl_GLSL = false;

	ext = gl_config.extensions_string;
	if (!ext)
		return;

	p_BindFramebuffer = (PFNbindFBO)R_GLProc ("glBindFramebuffer", "glBindFramebufferEXT", NULL);
	p_GenFramebuffers = (PFNgenFBO)R_GLProc ("glGenFramebuffers", "glGenFramebuffersEXT", NULL);
	p_DeleteFramebuffers = (PFNdelFBO)R_GLProc ("glDeleteFramebuffers", "glDeleteFramebuffersEXT", NULL);
	p_FramebufferTexture2D = (PFNfboTex)R_GLProc ("glFramebufferTexture2D", "glFramebufferTexture2DEXT", NULL);
	p_CheckFramebufferStatus = (PFNcheckFBO)R_GLProc ("glCheckFramebufferStatus", "glCheckFramebufferStatusEXT", NULL);
	p_BlitFramebuffer = (PFNblitFBO)R_GLProc ("glBlitFramebuffer", "glBlitFramebufferEXT", NULL);

	/* Core GLSL 2.0 names only — ARB object API uses different handles. */
	p_CreateShader = (PFNcreateSh)R_GLProc ("glCreateShader", NULL, NULL);
	p_ShaderSource = (PFNshaderSrc)R_GLProc ("glShaderSource", NULL, NULL);
	p_CompileShader = (PFNcompileSh)R_GLProc ("glCompileShader", NULL, NULL);
	p_GetShaderiv = (PFNgetShiv)R_GLProc ("glGetShaderiv", NULL, NULL);
	p_GetShaderInfoLog = (PFNgetShLog)R_GLProc ("glGetShaderInfoLog", NULL, NULL);
	p_CreateProgram = (PFNcreateProg)R_GLProc ("glCreateProgram", NULL, NULL);
	p_AttachShader = (PFNattachSh)R_GLProc ("glAttachShader", NULL, NULL);
	p_LinkProgram = (PFNlinkProg)R_GLProc ("glLinkProgram", NULL, NULL);
	p_UseProgram = (PFNuseProg)R_GLProc ("glUseProgram", NULL, NULL);
	p_GetProgramiv = (PFNgetPiv)R_GLProc ("glGetProgramiv", NULL, NULL);
	p_GetProgramInfoLog = (PFNgetPLog)R_GLProc ("glGetProgramInfoLog", NULL, NULL);
	p_GetUniformLocation = (PFNuniLoc)R_GLProc ("glGetUniformLocation", NULL, NULL);
	p_Uniform1i = (PFNuni1i)R_GLProc ("glUniform1i", NULL, NULL);
	p_Uniform1f = (PFNuni1f)R_GLProc ("glUniform1f", NULL, NULL);
	p_Uniform2f = (PFNuni2f)R_GLProc ("glUniform2f", NULL, NULL);
	p_Uniform3f = (PFNuni3f)R_GLProc ("glUniform3f", NULL, NULL);
	p_Uniform4f = (PFNuni4f)R_GLProc ("glUniform4f", NULL, NULL);
	p_DeleteShader = (PFNdelSh)R_GLProc ("glDeleteShader", NULL, NULL);
	p_DeleteProgram = (PFNdelProg)R_GLProc ("glDeleteProgram", NULL, NULL);

	have_glsl = (p_CreateShader && p_CreateProgram && p_UseProgram
		&& p_GetUniformLocation && p_Uniform1i && qglActiveTextureARB);
	if (have_glsl)
	{
		prog_world = R_MakeProgram (vs_world, fs_world, "world-dlight");
		if (prog_world)
		{
			gl_config.r1gl_GLSL = true;
			ri.Con_Printf (PRINT_ALL, "...using GLSL world dlights (gl_dlight_shader)\n");
		}
		else
			ri.Con_Printf (PRINT_ALL, "...world dlight shader failed\n");
	}

	if (!p_BindFramebuffer || !p_GenFramebuffers || !p_FramebufferTexture2D || !p_CheckFramebufferStatus)
	{
		ri.Con_Printf (PRINT_ALL, "...FBO post-FX not available (no framebuffer entry points)\n");
		return;
	}
	if (!have_glsl)
	{
		ri.Con_Printf (PRINT_ALL, "...FBO post-FX not available (no GLSL entry points)\n");
		return;
	}

	prog_extract = R_MakeProgram (vs_fx, fs_extract, "extract");
	prog_blur = R_MakeProgram (vs_fx, fs_blur, "blur");
	prog_rays = R_MakeProgram (vs_fx, fs_rays, "godrays");
	prog_composite = R_MakeProgram (vs_fx, fs_composite, "composite");
	prog_particle = R_MakeProgram (vs_particle, fs_particle, "softparticle");
	if (!prog_extract || !prog_blur || !prog_rays || !prog_composite || !prog_particle)
	{
		ri.Con_Printf (PRINT_ALL, "...FBO post-FX shaders failed\n");
		return;
	}

	pf_ok = true;
	pf_have_blit = (p_BlitFramebuffer != NULL);
	gl_config.r1gl_FBO = true;
	gl_config.r1gl_GLSL = true;
	while (qglGetError () != GL_NO_ERROR)
		;
	ri.Con_Printf (PRINT_ALL, "...using FBO post-FX (bloom / god rays / soft particles)\n");
}

void R_PostFX_Shutdown (void)
{
	if (p_UseProgram)
		p_UseProgram (0);
	if (p_BindFramebuffer)
		p_BindFramebuffer (GL_FRAMEBUFFER, 0);
	R_DestroyTargets ();
	if (p_DeleteProgram)
	{
		if (prog_extract) p_DeleteProgram (prog_extract);
		if (prog_blur) p_DeleteProgram (prog_blur);
		if (prog_rays) p_DeleteProgram (prog_rays);
		if (prog_composite) p_DeleteProgram (prog_composite);
		if (prog_particle) p_DeleteProgram (prog_particle);
		if (prog_world) p_DeleteProgram (prog_world);
	}
	prog_extract = prog_blur = prog_rays = prog_composite = prog_particle = 0;
	prog_world = 0;
	world_shader_bound = false;
	pf_ok = false;
	pf_bound = false;
}

qboolean R_PostFX_BeginView (void)
{
	pf_bound = false;
	if (!R_PostFX_Wanted ())
		return false;
	if (!R_CreateTargets ((int)vid.width, (int)vid.height))
	{
		pf_ok = false;
		return false;
	}

	p_BindFramebuffer (GL_FRAMEBUFFER, fbo_scene);
	qglDrawBuffer (GL_COLOR_ATTACHMENT0);
	qglReadBuffer (GL_COLOR_ATTACHMENT0);
	qglViewport (r_viewport[0], r_viewport[1], r_viewport[2], r_viewport[3]);
	qglClearColor (0, 0, 0, 1);
	qglClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	pf_bound = true;
	return true;
}

qboolean R_PostFX_SoftParticlesActive (void)
{
	return pf_bound && gl_softparticles && gl_softparticles->value > 0.0f
		&& prog_particle && tex_depthcopy && fbo_depthcopy && pf_have_blit;
}

void R_PostFX_CaptureDepth (void)
{
	if (!R_PostFX_SoftParticlesActive ())
		return;

	p_BindFramebuffer (GL_READ_FRAMEBUFFER, fbo_scene);
	p_BindFramebuffer (GL_DRAW_FRAMEBUFFER, fbo_depthcopy);
	p_BlitFramebuffer (0, 0, pf_w, pf_h, 0, 0, pf_w, pf_h, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
	p_BindFramebuffer (GL_FRAMEBUFFER, fbo_scene);
	qglDrawBuffer (GL_COLOR_ATTACHMENT0);
	qglReadBuffer (GL_COLOR_ATTACHMENT0);
}

void R_PostFX_BeginSoftParticles (void)
{
	GLint	loc;

	if (!R_PostFX_SoftParticlesActive ())
		return;

	p_UseProgram (prog_particle);
	loc = p_GetUniformLocation (prog_particle, "u_tex");
	if (loc >= 0) p_Uniform1i (loc, 0);
	loc = p_GetUniformLocation (prog_particle, "u_depth");
	if (loc >= 0) p_Uniform1i (loc, 1);
	loc = p_GetUniformLocation (prog_particle, "u_screensize");
	if (loc >= 0) p_Uniform2f (loc, (float)pf_w, (float)pf_h);
	loc = p_GetUniformLocation (prog_particle, "u_znear");
	if (loc >= 0) p_Uniform1f (loc, 4.0f);
	loc = p_GetUniformLocation (prog_particle, "u_zfar");
	if (loc >= 0) p_Uniform1f (loc, gl_zfar ? gl_zfar->value : 8192.0f);
	loc = p_GetUniformLocation (prog_particle, "u_soft");
	if (loc >= 0) p_Uniform1f (loc, 80.0f);

	qglActiveTextureARB (GL_TEXTURE1);
	qglBindTexture (GL_TEXTURE_2D, tex_depthcopy);
	gl_state.currenttextures[1] = tex_depthcopy;
	qglActiveTextureARB (GL_TEXTURE0);
	gl_state.currenttmu = 0;
}

void R_PostFX_EndSoftParticles (void)
{
	if (!prog_particle || !p_UseProgram)
		return;
	p_UseProgram (0);
	if (qglActiveTextureARB)
	{
		qglActiveTextureARB (GL_TEXTURE1);
		qglBindTexture (GL_TEXTURE_2D, 0);
		gl_state.currenttextures[1] = 0;
		qglActiveTextureARB (GL_TEXTURE0);
		gl_state.currenttmu = 0;
	}
}

static void R_CompositeAdd (GLuint scene, GLuint addtex, float add)
{
	GLint	loc;
	float	us, vs, ub, vb;

	us = (float)r_viewport[2] / (float)pf_w;
	vs = (float)r_viewport[3] / (float)pf_h;
	ub = (float)r_viewport[0] / (float)pf_w;
	vb = (float)r_viewport[1] / (float)pf_h;

	p_UseProgram (prog_composite);
	loc = p_GetUniformLocation (prog_composite, "u_scene");
	if (loc >= 0) p_Uniform1i (loc, 0);
	loc = p_GetUniformLocation (prog_composite, "u_addtex");
	if (loc >= 0) p_Uniform1i (loc, 1);
	loc = p_GetUniformLocation (prog_composite, "u_add");
	if (loc >= 0) p_Uniform1f (loc, add);
	loc = p_GetUniformLocation (prog_composite, "u_uvscale");
	if (loc >= 0) p_Uniform2f (loc, us, vs);
	loc = p_GetUniformLocation (prog_composite, "u_uvbias");
	if (loc >= 0) p_Uniform2f (loc, ub, vb);

	R_BindTex0 (scene);
	qglActiveTextureARB (GL_TEXTURE1);
	qglBindTexture (GL_TEXTURE_2D, addtex);
	gl_state.currenttextures[1] = addtex;
	qglActiveTextureARB (GL_TEXTURE0);
	gl_state.currenttmu = 0;
	R_DrawFSQuad ();
}

void R_PostFX_EndView (void)
{
	float	bloom, rays, lu, lv;
	GLint	loc;
	qboolean	do_bloom, do_rays, have_light;

	if (!pf_bound)
		return;
	pf_bound = false;

	/* Magnitude is ignored: gl_bloom is on/off, add is hardcoded. */
	do_bloom = gl_bloom && gl_bloom->value > 0.0f;
	bloom = do_bloom ? R_BLOOM_ADD : 0.0f;
	rays = gl_godrays ? gl_godrays->value : 0.0f;
	if (rays < 0.0f) rays = 0.0f;
	if (rays > 2.0f) rays = 2.0f;
	do_rays = rays > 0.0f;
	have_light = false;

	if (do_bloom || do_rays)
	{
		/* Bright extract at 1/4 res (on-screen pixels only). */
		R_PassTo (fbo_ping[0], pf_qw, pf_qh, prog_extract);
		R_BindTex0 (tex_color);
		loc = p_GetUniformLocation (prog_extract, "u_tex");
		if (loc >= 0) p_Uniform1i (loc, 0);
		loc = p_GetUniformLocation (prog_extract, "u_threshold");
		if (loc >= 0) p_Uniform1f (loc, R_BLOOM_THRESHOLD);
		R_DrawFSQuad ();

		if (do_bloom)
		{
			/* One H+V pass — extra iterations smear glow around corners. */
			R_PassTo (fbo_ping[1], pf_qw, pf_qh, prog_blur);
			R_BindTex0 (tex_ping[0]);
			loc = p_GetUniformLocation (prog_blur, "u_tex");
			if (loc >= 0) p_Uniform1i (loc, 0);
			loc = p_GetUniformLocation (prog_blur, "u_texel");
			if (loc >= 0) p_Uniform2f (loc, 1.0f / (float)pf_qw, 0.0f);
			R_DrawFSQuad ();

			R_PassTo (fbo_ping[0], pf_qw, pf_qh, prog_blur);
			R_BindTex0 (tex_ping[1]);
			loc = p_GetUniformLocation (prog_blur, "u_tex");
			if (loc >= 0) p_Uniform1i (loc, 0);
			loc = p_GetUniformLocation (prog_blur, "u_texel");
			if (loc >= 0) p_Uniform2f (loc, 0.0f, 1.0f / (float)pf_qh);
			R_DrawFSQuad ();
		}

		if (do_rays)
		{
			have_light = R_PickGodRay (&lu, &lv);
			if (have_light)
			{
				/* Radial blur of the bright buffer toward a visible light. */
				R_PassTo (fbo_ping[1], pf_qw, pf_qh, prog_rays);
				R_BindTex0 (tex_ping[0]);
				loc = p_GetUniformLocation (prog_rays, "u_tex");
				if (loc >= 0) p_Uniform1i (loc, 0);
				loc = p_GetUniformLocation (prog_rays, "u_light");
				if (loc >= 0) p_Uniform2f (loc, lu, lv);
				loc = p_GetUniformLocation (prog_rays, "u_weight");
				if (loc >= 0) p_Uniform1f (loc, 0.085f * rays);
				R_DrawFSQuad ();
			}
		}
	}

	p_BindFramebuffer (GL_FRAMEBUFFER, 0);
	qglDrawBuffer (GL_BACK);
	qglReadBuffer (GL_BACK);
	qglViewport (r_viewport[0], r_viewport[1], r_viewport[2], r_viewport[3]);
	qglEnable (GL_TEXTURE_2D);
	qglDisable (GL_DEPTH_TEST);
	qglDisable (GL_CULL_FACE);
	qglDisable (GL_ALPHA_TEST);
	qglDepthMask (GL_FALSE);

	if (!do_bloom && !do_rays)
	{
		if (pf_have_blit)
		{
			p_BindFramebuffer (GL_READ_FRAMEBUFFER, fbo_scene);
			p_BindFramebuffer (GL_DRAW_FRAMEBUFFER, 0);
			p_BlitFramebuffer (r_viewport[0], r_viewport[1],
				r_viewport[0] + r_viewport[2], r_viewport[1] + r_viewport[3],
				r_viewport[0], r_viewport[1],
				r_viewport[0] + r_viewport[2], r_viewport[1] + r_viewport[3],
				GL_COLOR_BUFFER_BIT, GL_NEAREST);
			p_BindFramebuffer (GL_FRAMEBUFFER, 0);
		}
		else
		{
			qglDisable (GL_BLEND);
			R_CompositeAdd (tex_color, tex_ping[0], 0.0f);
		}
	}
	else
	{
		qglDisable (GL_BLEND);
		R_CompositeAdd (tex_color, tex_ping[0], do_bloom ? bloom : 0.0f);
		if (do_rays && have_light)
		{
			float	us, vs, ub, vb;

			qglEnable (GL_BLEND);
			qglBlendFunc (GL_ONE, GL_ONE);
			p_UseProgram (prog_composite);
			loc = p_GetUniformLocation (prog_composite, "u_scene");
			if (loc >= 0) p_Uniform1i (loc, 0);
			loc = p_GetUniformLocation (prog_composite, "u_addtex");
			if (loc >= 0) p_Uniform1i (loc, 1);
			loc = p_GetUniformLocation (prog_composite, "u_add");
			if (loc >= 0) p_Uniform1f (loc, 0.0f);
			us = (float)r_viewport[2] / (float)pf_w;
			vs = (float)r_viewport[3] / (float)pf_h;
			ub = (float)r_viewport[0] / (float)pf_w;
			vb = (float)r_viewport[1] / (float)pf_h;
			loc = p_GetUniformLocation (prog_composite, "u_uvscale");
			if (loc >= 0) p_Uniform2f (loc, us, vs);
			loc = p_GetUniformLocation (prog_composite, "u_uvbias");
			if (loc >= 0) p_Uniform2f (loc, ub, vb);
			R_BindTex0 (tex_ping[1]);
			R_DrawFSQuad ();
			qglBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			qglDisable (GL_BLEND);
		}
	}

	p_UseProgram (0);
	qglDepthMask (GL_TRUE);
	qglEnable (GL_DEPTH_TEST);
	if (qglActiveTextureARB)
	{
		qglActiveTextureARB (GL_TEXTURE1);
		qglBindTexture (GL_TEXTURE_2D, 0);
		gl_state.currenttextures[1] = 0;
		qglActiveTextureARB (GL_TEXTURE0);
		gl_state.currenttmu = 0;
	}
	qglBindTexture (GL_TEXTURE_2D, 0);
	gl_state.currenttextures[0] = 0;
}
