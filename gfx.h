/* gfx.h - 2D drawing primitives on top of the PSP GU (no textures needed) */
#ifndef GFX_H
#define GFX_H

#include <psptypes.h>

#define SCREEN_W 480
#define SCREEN_H 272

/* PSP GU_COLOR_8888 is stored little-endian as R,G,B,A bytes. */
#define RGBA(r,g,b,a) ((u32)((u32)(r) | ((u32)(g)<<8) | ((u32)(b)<<16) | ((u32)(a)<<24)))
#define RGB(r,g,b)    RGBA(r,g,b,255)

void gfx_init(void);
void gfx_start_frame(u32 clearColor);
void gfx_end_frame(void);

void gfx_rect(float x, float y, float w, float h, u32 c);
void gfx_rect_grad(float x, float y, float w, float h, u32 top, u32 bot);
void gfx_quad(float x, float y, float w, float h, u32 tl, u32 tr, u32 br, u32 bl);
void gfx_tri(float x0, float y0, float x1, float y1, float x2, float y2, u32 c);
void gfx_circle(float cx, float cy, float r, u32 c);
void gfx_circle_grad(float cx, float cy, float r, u32 inner, u32 outer);
void gfx_ring(float cx, float cy, float r, float thick, u32 c);
void gfx_line(float x0, float y0, float x1, float y1, float thick, u32 c);

#endif
