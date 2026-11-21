/* gfx.c - immediate-mode 2D primitives for the PSP GU.
 *
 * Everything is drawn with GU_TRANSFORM_2D so vertex coordinates are plain
 * screen pixels: (0,0) top-left, +x right, +y down.  Draw order is the
 * painter's order (depth test is off).
 */
#include <pspgu.h>
#include <math.h>
#include "gfx.h"

#define BUF_WIDTH 512
#define PIXEL_SIZE 4
#define FRAME_SIZE (BUF_WIDTH * SCREEN_H * PIXEL_SIZE) /* 0x88000 */

#define PI 3.14159265f

static unsigned int __attribute__((aligned(16))) gu_list[0x20000];

/* Vertex layout must match the draw format below: colour, then position. */
typedef struct {
	u32 c;
	float x, y, z;
} Vtx;

#define VT_FMT (GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_2D)

void gfx_init(void)
{
	sceGuInit();
	sceGuStart(GU_DIRECT, gu_list);

	sceGuDrawBuffer(GU_PSM_8888, (void *)0, BUF_WIDTH);
	sceGuDispBuffer(SCREEN_W, SCREEN_H, (void *)(FRAME_SIZE), BUF_WIDTH);
	sceGuDepthBuffer((void *)(FRAME_SIZE * 2), BUF_WIDTH);

	sceGuOffset(2048 - SCREEN_W / 2, 2048 - SCREEN_H / 2);
	sceGuViewport(2048, 2048, SCREEN_W, SCREEN_H);
	sceGuScissor(0, 0, SCREEN_W, SCREEN_H);

	sceGuEnable(GU_SCISSOR_TEST);
	sceGuDisable(GU_DEPTH_TEST);
	sceGuDisable(GU_CULL_FACE);
	sceGuDisable(GU_TEXTURE_2D);
	sceGuDisable(GU_LIGHTING);
	sceGuShadeModel(GU_SMOOTH);
	sceGuEnable(GU_BLEND);
	sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);

	sceGuClearColor(0xff000000);
	sceGuClearDepth(0);

	sceGuFinish();
	sceGuSync(0, 0);
	sceDisplayWaitVblankStart();
	sceGuDisplay(GU_TRUE);
}

void gfx_start_frame(u32 clearColor)
{
	sceGuStart(GU_DIRECT, gu_list);
	sceGuClearColor(clearColor);
	sceGuClear(GU_COLOR_BUFFER_BIT);
}

void gfx_end_frame(void)
{
	sceGuFinish();
	sceGuSync(0, 0);
	sceDisplayWaitVblankStart();
	sceGuSwapBuffers();
}

void gfx_rect(float x, float y, float w, float h, u32 c)
{
	Vtx v[2];
	v[0].c = c; v[0].x = x;       v[0].y = y;       v[0].z = 0.0f;
	v[1].c = c; v[1].x = x + w;   v[1].y = y + h;   v[1].z = 0.0f;
	sceGuDrawArray(GU_SPRITES, VT_FMT, 2, NULL, v);
}

void gfx_quad(float x, float y, float w, float h, u32 tl, u32 tr, u32 br, u32 bl)
{
	Vtx v[4];
	v[0].c = tl; v[0].x = x;     v[0].y = y;     v[0].z = 0.0f;
	v[1].c = tr; v[1].x = x + w; v[1].y = y;     v[1].z = 0.0f;
	v[2].c = br; v[2].x = x + w; v[2].y = y + h; v[2].z = 0.0f;
	v[3].c = bl; v[3].x = x;     v[3].y = y + h; v[3].z = 0.0f;
	sceGuDrawArray(GU_TRIANGLE_FAN, VT_FMT, 4, NULL, v);
}

void gfx_rect_grad(float x, float y, float w, float h, u32 top, u32 bot)
{
	gfx_quad(x, y, w, h, top, top, bot, bot);
}

void gfx_tri(float x0, float y0, float x1, float y1, float x2, float y2, u32 c)
{
	Vtx v[3];
	v[0].c = c; v[0].x = x0; v[0].y = y0; v[0].z = 0.0f;
	v[1].c = c; v[1].x = x1; v[1].y = y1; v[1].z = 0.0f;
	v[2].c = c; v[2].x = x2; v[2].y = y2; v[2].z = 0.0f;
	sceGuDrawArray(GU_TRIANGLES, VT_FMT, 3, NULL, v);
}

void gfx_line(float x0, float y0, float x1, float y1, float thick, u32 c)
{
	float dx = x1 - x0, dy = y1 - y0;
	float len = sqrtf(dx * dx + dy * dy);
	float nx, ny;
	Vtx v[4];
	if (len < 0.0001f)
		return;
	nx = -dy / len * (thick * 0.5f);
	ny =  dx / len * (thick * 0.5f);
	v[0].c = c; v[0].x = x0 + nx; v[0].y = y0 + ny; v[0].z = 0.0f;
	v[1].c = c; v[1].x = x1 + nx; v[1].y = y1 + ny; v[1].z = 0.0f;
	v[2].c = c; v[2].x = x1 - nx; v[2].y = y1 - ny; v[2].z = 0.0f;
	v[3].c = c; v[3].x = x0 - nx; v[3].y = y0 - ny; v[3].z = 0.0f;
	sceGuDrawArray(GU_TRIANGLE_FAN, VT_FMT, 4, NULL, v);
}

void gfx_circle(float cx, float cy, float r, u32 c)
{
	static Vtx v[26];
	const int n = 24;
	int i;
	v[0].c = c; v[0].x = cx; v[0].y = cy; v[0].z = 0.0f;
	for (i = 0; i <= n; i++) {
		float a = (float)i * (2.0f * PI / (float)n);
		v[i + 1].c = c;
		v[i + 1].x = cx + cosf(a) * r;
		v[i + 1].y = cy + sinf(a) * r;
		v[i + 1].z = 0.0f;
	}
	sceGuDrawArray(GU_TRIANGLE_FAN, VT_FMT, n + 2, NULL, v);
}

void gfx_circle_grad(float cx, float cy, float r, u32 inner, u32 outer)
{
	static Vtx v[26];
	const int n = 24;
	int i;
	v[0].c = inner; v[0].x = cx; v[0].y = cy; v[0].z = 0.0f;
	for (i = 0; i <= n; i++) {
		float a = (float)i * (2.0f * PI / (float)n);
		v[i + 1].c = outer;
		v[i + 1].x = cx + cosf(a) * r;
		v[i + 1].y = cy + sinf(a) * r;
		v[i + 1].z = 0.0f;
	}
	sceGuDrawArray(GU_TRIANGLE_FAN, VT_FMT, n + 2, NULL, v);
}

void gfx_ring(float cx, float cy, float r, float thick, u32 c)
{
	static Vtx v[52];
	const int n = 24;
	int i, k = 0;
	for (i = 0; i <= n; i++) {
		float a = (float)i * (2.0f * PI / (float)n);
		float ca = cosf(a), sa = sinf(a);
		v[k].c = c; v[k].x = cx + ca * (r - thick * 0.5f); v[k].y = cy + sa * (r - thick * 0.5f); v[k].z = 0.0f; k++;
		v[k].c = c; v[k].x = cx + ca * (r + thick * 0.5f); v[k].y = cy + sa * (r + thick * 0.5f); v[k].z = 0.0f; k++;
	}
	sceGuDrawArray(GU_TRIANGLE_STRIP, VT_FMT, k, NULL, v);
}
