param([string]$Revision)
$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
$out = Join-Path $repo 'build-focus/input-tests'
[void][IO.Directory]::CreateDirectory($out)
function Source($file) {
    if ($Revision) {
        $source = & git -C $repo show "${Revision}:$file"
        if ($LASTEXITCODE) { throw "Cannot read $file at $Revision" }
        return $source -join "`n"
    }
    return [IO.File]::ReadAllText((Join-Path $repo $file))
}
function FunctionText($source, $signature) {
    $start = $source.IndexOf($signature)
    if ($start -lt 0) { throw "Missing $signature" }
    $brace = $source.IndexOf('{', $start)
    $end = $source.IndexOf("`n}", $brace)
    if ($end -lt 0) { throw "Missing function end: $signature" }
    return "`n" + $source.Substring($start, $end + 2 - $start) + "`n"
}
$inputSource = Source 'client/cl_input.c'
$mouseSource = Source 'win32/in_win.c'
$c = @'
#include <windows.h>
#include "client/client.h"
#undef Com_Error
#define Com_Error(...) abort()
client_state_t cl;
client_static_t cls;
uint32 sys_frame_time, old_sys_frame_time, frame_msec;
kbutton_t in_strafe, in_attack, in_use;
int in_impulse, anykeydown;
qboolean send_packet_now;
static cvar_t yes, no, sens, yaw, pitch, side, forward, light;
cvar_t *in_mouse=&yes, *sensitivity=&sens, *m_yaw=&yaw, *m_pitch=&pitch;
cvar_t *m_side=&side, *m_forward=&forward, *freelook=&yes, *lookstrafe=&no;
cvar_t *cl_lightlevel=&light;
static cvar_t *m_directinput=&no, *m_filter=&no;
static int in_appactive, mouseactive, mouseinitialized=1, mlooking;
static int in_drop_mouse_sample, dropped, cursor_reads, input_samples;
static int window_center_x, window_center_y;
static float mouse_x, mouse_y, old_mouse_x, old_mouse_y;
static void *g_pMouse;
static POINT current_pos, desktop_pos, unfocused_pos;
static qboolean unfocused_pos_valid, in_mouse_activity;
static BOOL test_GetCursorPos(POINT *p) { cursor_reads++; *p=desktop_pos; return TRUE; }
static BOOL test_SetCursorPos(int x,int y) { desktop_pos.x=x; desktop_pos.y=y; return TRUE; }
#define GetCursorPos test_GetCursorPos
#define SetCursorPos test_SetCursorPos
static void IN_ApplyAutoSens(float *x,float *y) { }
static void IN_DropPendingMouseSample(void) { in_drop_mouse_sample=0; dropped++; }
static void IN_ReadBufferedData(usercmd_t *cmd) { input_samples++; }
static void IN_ReadImmediateData(usercmd_t *cmd) { input_samples++; }
void CL_BaseMove(usercmd_t *cmd) { input_samples++; }
int Cvar_IntValue(const char *name) { return 120; }
'@
$c += FunctionText $mouseSource 'static void IN_ApplyMouseLook ('
if ($mouseSource.Contains('qboolean IN_ConsumeMouseActivity (')) {
    $c += FunctionText $mouseSource 'qboolean IN_ConsumeMouseActivity ('
    $c += FunctionText $mouseSource 'static void IN_UnfocusedMouseMove ('
}
$c += FunctionText $mouseSource 'void IN_MouseMove ('
$c += "`nvoid IN_Move(usercmd_t *cmd) { IN_MouseMove(cmd); }`n"
foreach ($signature in @('void CL_ResetInputClock (','void CL_ClampPitch (','__inline void CL_InitCmd (','static int CL_CmdMsec (','void CL_RefreshCmd (','void CL_FinalizeCmd (','void CL_FinishMove (')) {
    $c += FunctionText $inputSource $signature
}
$c += @'
static int failures, checks;
#define CHECK(c) do { checks++; if (!(c)) { failures++; printf("FAIL: %s (line %d)\n", #c, __LINE__); } } while(0)
/* Legacy Arena p_client.c ClientThink: repeated A/B/A pitch AND yaw pairs.
 * The server's check is retained here as the regression oracle, not changed
 * in the client. https://github.com/packetflinger/ra2/blob/main/p_client.c */
static short history[2][2];
static int reversals;
static void arena_check(const usercmd_t *cmd) {
    if (history[0][0]==cmd->angles[0] && history[1][0]!=cmd->angles[0] &&
        history[0][1]==cmd->angles[1] && history[1][1]!=cmd->angles[1]) reversals++;
    history[0][0]=history[1][0]; history[0][1]=history[1][1];
    history[1][0]=cmd->angles[0]; history[1][1]=cmd->angles[1];
}
static void reset(void) {
    memset(&cl,0,sizeof(cl)); memset(&cls,0,sizeof(cls));
    memset(history,0,sizeof(history)); reversals=0;
    cl.viewangles[0]=15; cl.viewangles[1]=60;
    cl.refresh_prepped=true; cls.state=ca_active; cls.key_dest=key_game;
    cls.frametime=.016f; sys_frame_time=old_sys_frame_time=1000;
    desktop_pos.x=desktop_pos.y=0; unfocused_pos_valid=false;
    in_mouse_activity=false; in_appactive=mouseactive=0;
    cursor_reads=input_samples=0;
}
static usercmd_t *sample(int elapsed) {
    CL_InitCmd(); sys_frame_time+=elapsed; CL_RefreshCmd(); CL_FinalizeCmd();
    return &cl.cmds[0];
}
int main(void) {
    int i, mode, flagged; usercmd_t *cmd; usercmd_t sync;
    yes.intvalue=1; yes.value=1; sens.value=1;
    yaw.value=pitch.value=1; side.value=forward.value=1;
    /* Run actual Win32 mouse + command assembly in both client modes.
     * Cursor oscillation on a different app must never move the player. */
    for(mode=0; mode<2; mode++) {
        reset();
        for(i=0; i<240; i++) {
            desktop_pos.x=desktop_pos.y=(i%2)*8;
            if(mode) cmd=sample(16);
            else {
                memset(&sync,0,sizeof(sync)); IN_Move(&sync);
                CL_FinishMove(&sync); cmd=&sync;
            }
            arena_check(cmd);
        }
        flagged=reversals>10;
        printf("%s desktop motion: reversals=%d, bot=%d, cursor_reads=%d\n",
               mode ? "async" : "sync",reversals,flagged,cursor_reads);
        CHECK(!flagged); CHECK(cursor_reads==0);
        CHECK(cl.viewangles[0]==15 && cl.viewangles[1]==60);
        CHECK(!(cmd->buttons & BUTTON_ANY));
    }
    reset();
    for(i=0;i<240;i++) {
        if(i%2) CL_ResetInputClock();
        cmd=sample(i%2 ? 0 : 16); arena_check(cmd);
    }
    printf("Clock reset / same-tick commands: reversals=%d, bot=%d\n",reversals,reversals>10);
    CHECK(reversals==0); CHECK(cmd->angles[0]==ANGLE2SHORT(15));
    CHECK(cmd->angles[1]==ANGLE2SHORT(60)); CHECK(cmd->msec==16);
    /* A zero-time refresh must not consume a second input sample. */
    i=input_samples; sample(0); CHECK(input_samples==i);
    /* Verify active Win32 look and initial focus-return sample discard. */
    reset(); in_appactive=mouseactive=1;
    desktop_pos.x=4; desktop_pos.y=3; cmd=sample(16);
    CHECK(cl.viewangles[0]==18 && cl.viewangles[1]==56);
    CHECK(cursor_reads==1);
    in_drop_mouse_sample=1; desktop_pos.x=1000; desktop_pos.y=500;
    cmd=sample(16); CHECK(dropped==1);
    CHECK(cl.viewangles[0]==18 && cl.viewangles[1]==56);
    /* Also reject a stale captured state after focus has gone away. */
    in_appactive=0; cmd=sample(16);
    CHECK(cl.viewangles[0]==18 && cl.viewangles[1]==56);
    reset();
    in_attack.state=3; in_use.state=3; anykeydown=1; in_impulse=5;
    cmd=sample(16);
    CHECK((cmd->buttons & (BUTTON_ATTACK|BUTTON_USE|BUTTON_ANY))==(BUTTON_ATTACK|BUTTON_USE|BUTTON_ANY));
    CHECK(cmd->impulse==5 && in_impulse==0);
    cls.frametime=0; CHECK(CL_CmdMsec()==1);
    cls.frametime=.016f; CHECK(CL_CmdMsec()==16);
    cls.frametime=.100f; CHECK(CL_CmdMsec()==100);
    cls.frametime=.500f; CHECK(CL_CmdMsec()==250);
    printf("%d checks, %d failures\n",checks,failures);
    return failures ? 1 : 0;
}
'@
$label = if ($Revision) { 'baseline' } else { 'fixed' }
[IO.File]::WriteAllText((Join-Path $out "$label.c"), $c)
$vs = 'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools'
Push-Location $repo
try {
    $cmd = 'cl /nologo /O2 /MD /D_CRT_SECURE_NO_WARNINGS /I . /I build\vcpkg_installed\x86-windows\include build-focus\input-tests\' + $label + '.c /Fo:build-focus\input-tests\' + $label + '.obj /Fe:build-focus\input-tests\' + $label + '.exe'
    # CI already initializes MSVC; local runs can use the installed Build Tools.
    if (!(Get-Command cl.exe -ErrorAction SilentlyContinue)) {
        $cmd = '"' + $vs + '\VC\Auxiliary\Build\vcvarsall.bat" x86 >nul && ' + $cmd
    }
    & cmd.exe /d /s /c $cmd
    if ($LASTEXITCODE) { throw 'Test compilation failed' }
    & (Join-Path $out "$label.exe")
    if ($LASTEXITCODE) { throw 'Background input regression failed' }
} finally { Pop-Location }
