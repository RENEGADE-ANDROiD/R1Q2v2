$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
$out = Join-Path $repo 'build/security-audit'
[void][IO.Directory]::CreateDirectory($out)
function Get-FunctionText($file, $signature) {
    $text = [IO.File]::ReadAllText((Join-Path $repo $file))
    $start = $text.IndexOf($signature)
    # Skip forward declarations; definitions have their opening brace before ';'.
    while ($start -ge 0) {
        $brace = $text.IndexOf('{', $start)
        $semi = $text.IndexOf(';', $start)
        if ($brace -ge 0 -and ($semi -lt 0 -or $brace -lt $semi)) { break }
        $start = $text.IndexOf($signature, $start + $signature.Length)
    }
    if ($start -lt 0) { throw "Function missing: $signature" }
    $end = $text.IndexOf("`n}", $brace)
    if ($end -lt 0) { throw "Function end missing: $signature" }
    return "`n" + $text.Substring($start, $end + 2 - $start) + "`n"
}
$common = @'
#include "client/client.h"
#include "client/snd_loc.h"
#include <limits.h>
#include <setjmp.h>
#undef Com_DPrintf
#define Com_DPrintf(...) ((void)0)
#define Com_Printf(...) ((void)0)
static jmp_buf error_jump;
static void test_error(void) { longjmp(error_jump, 1); }
#define Com_Error(...) test_error()
static void *test_alloc(int n, int tag) { return calloc(1, n); }
#define Z_TagMalloc test_alloc
#define Z_Free free
static int checks;
#define CHECK(c) do { checks++; if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); return 1; } } while(0)
'@

$wav = $common + @'

byte *data_p, *iff_end, *last_chunk, *iff_data;
int iff_chunk_len;
static byte fixture[128];
static int fixture_size;
static int test_load(const char *name, void **p) {
    *p = malloc(fixture_size); memcpy(*p, fixture, fixture_size); return fixture_size;
}
#define FS_LoadFile test_load
#define FS_FreeFile free
'@
foreach ($sig in @('int16 GetLittleShort(', 'int32 GetLittleLong(', 'void FindNextChunk(', 'void FindChunk(', 'static qboolean S_OpenAL_LoadWAV (', 'wavinfo_t GetWavinfo (')) {
    $wav += Get-FunctionText 'client/snd_mem.c' $sig
}
$wav += @'
static void put32(int off, unsigned n) { memcpy(fixture+off, &n, 4); }
static void make_wav(int bits) {
    memset(fixture, 0, sizeof(fixture)); fixture_size=48;
    memcpy(fixture,"RIFF",4); put32(4,40); memcpy(fixture+8,"WAVEfmt ",8);
    put32(16,16); fixture[20]=1; fixture[22]=1; put32(24,22050);
    put32(28,22050*(bits/8)); fixture[32]=(byte)(bits/8); fixture[34]=(byte)bits;
    memcpy(fixture+36,"data",4); put32(40,4); memset(fixture+44,128,4);
}
int main(void) {
    int n; wavinfo_t info; wavInfo_t al; byte *pcm;
    for (n=8;n<=16;n+=8) {
        make_wav(n); info=GetWavinfo("valid",fixture,fixture_size);
        CHECK(info.samples==4/(n/8) && info.width==n/8 && info.dataofs==44);
        CHECK(S_OpenAL_LoadWAV("valid",&pcm,&al)); CHECK(al.samples==info.samples); free(pcm);
    }
    for(n=0;n<5;n++) {
        make_wav(8);
        if(n==0) fixture[34]=0;
        if(n==1) put32(24,0);
        if(n==2) put32(40,32); /* declared data exceeds file */
        if(n==3) put32(16,1);  /* short fmt payload */
        if(n==4) { fixture_size=5; }
        info=GetWavinfo("invalid",fixture,fixture_size); CHECK(info.channels==0);
        CHECK(!S_OpenAL_LoadWAV("invalid",&pcm,&al));
    }
    /* Stock CoolEdit cue/LIST loop metadata must retain its sample range. */
    make_wav(8); memmove(fixture+112,fixture+36,12);
    memset(fixture+36,0,76); memcpy(fixture+36,"cue ",4); put32(40,28);
    put32(44,1); put32(68,1); memcpy(fixture+72,"LIST",4); put32(76,32);
    put32(96,2); memcpy(fixture+100,"mark",4); fixture_size=124; put32(4,116);
    info=GetWavinfo("loop",fixture,fixture_size);
    CHECK(info.loopstart==1 && info.samples==3 && info.dataofs==120);
    printf("PASS WAV: %d checks (software and OpenAL)\n",checks); return 0;
}
'@
[IO.File]::WriteAllText((Join-Path $out 'wav-safety.c'), $wav)

$http = $common + @'

client_static_t cls;
static int abortDownloads;
static cvar_t filelists;
cvar_t *cl_http_filelists=&filelists;
static int queued;
#define MAX_HTTP_FILELIST_SIZE (16 * 1024 * 1024)
static void CL_CheckAndQueueDownload(char *path) { queued++; }
'@
foreach ($sig in @('static size_t EXPORT CL_HTTP_Recv (', 'static int EXPORT CL_HTTP_Progress (', 'static void CL_ParseFileList (')) {
    $http += Get-FunctionText 'client/cl_http.c' $sig
}
$http += @'
int main(void) {
    dlhandle_t dl={0}; dlqueue_t queue={0}; char *data=malloc(300000);
    memset(data,'x',300000); filelists.intvalue=1; dl.queueEntry=&queue;
    strcpy(queue.quakePath,"arena.filelist");
    CL_ParseFileList(&dl); CHECK(queued==0);
    CHECK(CL_HTTP_Recv(data,1,131072,&dl)==131072);
    CHECK(dl.tempBuffer[dl.position]==0 && dl.fileSize>dl.position);
    CL_HTTP_Progress(&dl,100,10,0,0); CHECK(dl.position==131072);
    CHECK(CL_HTTP_Recv(data,1,300000,&dl)==300000);
    CHECK(dl.position==431072 && dl.tempBuffer[431071]=='x' && dl.tempBuffer[431072]==0);
    CHECK(CL_HTTP_Recv(data,1,MAX_HTTP_FILELIST_SIZE,&dl)==0);
    CHECK(CL_HTTP_Recv(data,(size_t)-1,2,&dl)==0);
    free(dl.tempBuffer); memset(&dl,0,sizeof(dl));
    CHECK(CL_HTTP_Recv("sound/test.wav\n",1,15,&dl)==15);
    CL_ParseFileList(&dl); CHECK(queued==1 && !dl.tempBuffer);
    free(data); printf("PASS HTTP: %d checks\n",checks); return 0;
}
'@
[IO.File]::WriteAllText((Join-Path $out 'http-safety.c'), $http)

$network = $common + @'

sizebuf_t net_message;
static int parsed;
qboolean CL_ParseServerMessage(void) { parsed++; return true; }
int ZLibDecompress(byte *in,int n,byte *out,int cap,int bits) { memset(out,0,cap); return cap; }
void SZ_Init(sizebuf_t *b,byte *data,int len) { memset(b,0,sizeof(*b)); b->data=data; b->maxsize=len; }
'@
foreach ($sig in @('int MSG_ReadByte (', 'int MSG_ReadShort (', 'void MSG_ReadData (')) {
    $network += Get-FunctionText 'qcommon/common.c' $sig
}
$network += Get-FunctionText 'client/cl_ents.c' 'int CL_ParseEntityBits ('
$network += Get-FunctionText 'client/cl_parse.c' 'void CL_ParseZPacket ('
$network += @'
static void message(byte *p,int len) { SZ_Init(&net_message,p,len); net_message.cursize=len; }
int main(void) {
    uint32 bits; byte data[16]={0}; int n;
    data[0]=U_MOREBITS1; data[1]=(byte)(U_NUMBER16>>8);
    n=MAX_EDICTS-1; memcpy(data+2,&n,2); message(data,4);
    CHECK(CL_ParseEntityBits(&bits)==MAX_EDICTS-1);
    n=MAX_EDICTS; memcpy(data+2,&n,2); message(data,4);
    if(!setjmp(error_jump)) { CL_ParseEntityBits(&bits); CHECK(0); } else CHECK(1);
    message(data,1);
    if(!setjmp(error_jump)) { CL_ParseEntityBits(&bits); CHECK(0); } else CHECK(1);
    memset(data,0,sizeof(data)); data[0]=1; data[2]=1; message(data,5);
    CL_ParseZPacket(); CHECK(parsed==1 && net_message.readcount==5);
    data[0]=0; data[1]=0x20; message(data,4);
    if(!setjmp(error_jump)) { CL_ParseZPacket(); CHECK(0); } else CHECK(parsed==1);
    data[0]=5; data[1]=0; message(data,4);
    if(!setjmp(error_jump)) { CL_ParseZPacket(); CHECK(0); } else CHECK(parsed==1);
    printf("PASS network guards: %d checks (decompressor stubbed)\n",checks); return 0;
}
'@
[IO.File]::WriteAllText((Join-Path $out 'network-safety.c'), $network)

$md3 = @'
#include "ref_gl/gl_local.h"
#include "ref_gl/md3.h"
#include <limits.h>
refimport_t ri;
static void *allocations[64];
static int allocation_count, checks;
void *Hunk_Alloc(int size) {
    void *p=calloc(1,size); allocations[allocation_count++]=p; return p;
}
static void release_allocations(void) { while(allocation_count) free(allocations[--allocation_count]); }
static void print_msg(int level,const char *fmt,...) { }
#undef fast_strlwr
#define fast_strlwr(s) ((void)0)
image_t *GL_FindSkin(const char *name,const char *modelpath) { return NULL; }
static void MD3_ResolveMissingSkins(model_t *mod,md3model_t *md3) { }
void AddPointToBounds(vec3_t v,vec3_t mins,vec3_t maxs) {
    int i; for(i=0;i<3;i++) { if(v[i]<mins[i]) mins[i]=v[i]; if(v[i]>maxs[i]) maxs[i]=v[i]; }
}
#define CHECK(c) do { checks++; if(!(c)) { printf("FAIL MD3 line %d\n",__LINE__); return 1; } } while(0)
'@
$md3 += Get-FunctionText 'ref_gl/gl_model.c' 'qboolean Mod_LoadMD3Model ('
$md3 += @'
typedef struct {
    dmd3header_t header; dmd3frame_t frame; dmd3mesh_t mesh;
    int indexes[3]; dmd3coord_t coords[3]; dmd3vertex_t verts[3];
} fixture_t;
int main(void) {
    fixture_t f; model_t model={0}; int mode, len; qboolean result;
    ri.Con_Printf=print_msg;
    for(mode=0;mode<7;mode++) {
        memset(&f,0,sizeof(f)); len=sizeof(f);
        f.header.ident=IDMD3HEADER; f.header.version=MD3_VERSION;
        f.header.num_frames=1; f.header.num_meshes=1;
        f.header.ofs_frames=offsetof(fixture_t,frame); f.header.ofs_meshes=offsetof(fixture_t,mesh);
        f.header.ofs_end=sizeof(f); f.mesh.ident=IDMD3HEADER;
        f.mesh.num_frames=1; f.mesh.num_verts=3; f.mesh.num_tris=1;
        f.mesh.meshsize=sizeof(f)-offsetof(fixture_t,mesh);
        f.mesh.ofs_skins=sizeof(f.mesh); f.mesh.ofs_indexes=sizeof(f.mesh);
        f.mesh.ofs_tcs=offsetof(fixture_t,coords)-offsetof(fixture_t,mesh);
        f.mesh.ofs_verts=offsetof(fixture_t,verts)-offsetof(fixture_t,mesh);
        f.indexes[0]=0; f.indexes[1]=1; f.indexes[2]=2;
        if(mode==1) f.header.ofs_frames=INT_MAX-1;
        if(mode==2) f.mesh.ofs_tcs=INT_MAX;
        if(mode==3) f.mesh.num_skins=-1;
        if(mode==4) f.mesh.num_skins=INT_MAX;
        if(mode==5) len--;
        if(mode==6) f.indexes[2]=3;
        result=Mod_LoadMD3Model(&model,&f,len); CHECK(result==(mode==0));
        release_allocations();
    }
    printf("PASS MD3: %d checks (allocation/skin callbacks stubbed)\n",checks); return 0;
}
'@
[IO.File]::WriteAllText((Join-Path $out 'md3-safety.c'), $md3)

$vs = 'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools'
Push-Location $repo
try {
    foreach ($name in @('wav','http','network','md3')) {
        $cmd = '"' + $vs + '\VC\Auxiliary\Build\vcvarsall.bat" x86 >nul && cl /nologo /O2 /MD /D_CRT_SECURE_NO_WARNINGS /DUSE_OPENAL /DUSE_CURL /I . /I build\vcpkg_installed\x86-windows\include build\security-audit\' + $name + '-safety.c /Fo:build\security-audit\' + $name + '-safety.obj /Fe:build\security-audit\' + $name + '-safety.exe'
        & cmd.exe /d /s /c $cmd
        if ($LASTEXITCODE) { throw "$name compile failed" }
        & (Join-Path $out ($name + '-safety.exe'))
        if ($LASTEXITCODE) { throw "$name tests failed" }
    }
} finally { Pop-Location }
