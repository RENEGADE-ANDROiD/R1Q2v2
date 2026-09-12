$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
$out = Join-Path $repo 'build/security-audit'
[void][IO.Directory]::CreateDirectory($out)
$source = [IO.File]::ReadAllText((Join-Path $repo 'ref_gl/gl_image.c'))
$start = $source.IndexOf('void EXPORT jpg_null')
$finish = $source.IndexOf('/*typedef struct _TargaHeader', $start)
if ($start -lt 0 -or $finish -lt 0) { throw 'JPEG source markers missing' }
$loader = $source.Substring($start, $finish - $start)
$prefix = @'
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <setjmp.h>
#include <jpeglib.h>
#include <jerror.h>
typedef unsigned char byte;
#define EXPORT
#define PRINT_ALL 0
#define MAX_TEXTURE_DIMENSIONS 4096
#define true 1
static unsigned char *jpeg_data;
static unsigned long jpeg_size;
static int overwrites, failures;
static void print_msg(int level, const char *fmt, ...) {
    va_list ap; va_start(ap, fmt); vprintf(fmt, ap); va_end(ap);
}
static int load_file(const char *name, void **out) {
    *out = malloc(jpeg_size); memcpy(*out, jpeg_data, jpeg_size); return (int)jpeg_size;
}
static struct { int (*FS_LoadFile)(const char *, void **); void (*FS_FreeFile)(void *);
    void (*Con_Printf)(int, const char *, ...); } ri = {load_file, free, print_msg};
static void *guard_malloc(size_t n) {
    size_t *base = malloc(sizeof(size_t) + n + 64); *base = n;
    memset((byte *)(base + 1) + n, 0xA5, 64); return base + 1;
}
static void guard_free(void *p) {
    size_t *base; size_t n, i, changed = 0;
    if (!p) return; base = (size_t *)p - 1; n = *base;
    for (i=0; i<64; i++) if (((byte *)p)[n+i] != 0xA5) changed++;
    if (changed) { printf("OVERWRITE: allocation=%zu bytes, modified guard bytes=%zu\n", n, changed); overwrites++; }
    free(base);
}
#define malloc guard_malloc
#define free guard_free
'@
$suffix = @'
#undef malloc
#undef free
static void make_jpeg(int mode) {
    struct jpeg_compress_struct c; struct jpeg_error_mgr err;
    unsigned char row[16388]; JSAMPROW ptr = row;
    memset(row, 0x37, sizeof(row)); c.err = jpeg_std_error(&err);
    jpeg_create_compress(&c); jpeg_data = NULL; jpeg_size = 0;
    jpeg_mem_dest(&c, &jpeg_data, &jpeg_size);
    c.image_width = mode == 5 ? 4097 : 8; c.image_height = 1;
    c.input_components = mode == 1 ? 4 : (mode == 4 ? 1 : 3);
    c.in_color_space = mode == 1 ? JCS_CMYK : (mode == 4 ? JCS_GRAYSCALE : JCS_RGB);
    jpeg_set_defaults(&c); jpeg_start_compress(&c, TRUE);
    jpeg_write_scanlines(&c, &ptr, 1); jpeg_finish_compress(&c); jpeg_destroy_compress(&c);
}
int main(void) {
    int mode, width, height; byte *pic;
    const char *names[] = {"RGB", "CMYK", "truncated header", "missing EOI", "grayscale", "oversized"};
    for (mode=0; mode<6; mode++) {
        printf("CASE: %s JPEG\n", names[mode]);
        make_jpeg(mode);
        if (mode == 2) { jpeg_size = 20; puts("Truncated RGB header"); }
        if (mode == 3) jpeg_size -= 2;
        LoadJPG("in-memory-fixture.jpg", &pic, &width, &height);
        if (((mode == 0 || mode == 4) && !pic) || ((mode != 0 && mode != 4) && pic)) failures++;
        printf("LoadJPG returned image=%s dimensions=%dx%d\n", pic ? "yes" : "no", width, height);
        guard_free(pic); free(jpeg_data);
    }
    puts(overwrites || failures ? "FAIL" : "PASS: 6 JPEG cases; supported images load, invalid/unsupported images rejected; guards intact");
    return overwrites || failures ? 1 : 0;
}
'@
[IO.File]::WriteAllText((Join-Path $out 'jpeg-audit.c'), $prefix + "`n" + $loader + "`n" + $suffix)
Write-Output 'Generated test with the unchanged JPEG loader/source-manager functions and padded allocation guards.'
