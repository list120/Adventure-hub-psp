/* render.c - the "beautiful" part: painterly 2D world, characters and UI.
 *
 * All drawing goes straight to the GU as flat-shaded geometry, so there are
 * no texture files to load.  Soft gradients, rim light, glow and parallax
 * layers are used to make the flat shapes read as a hand-drawn adventure.
 */
#include <math.h>
#include "render.h"
#include "gfx.h"
#include "text.h"

#define PI 3.14159265f

static float clamp01(float v){ return v<0?0:(v>1?1:v); }

static u32 lerpcol(u32 a, u32 b, float t)
{
	int ar=a&255,ag=(a>>8)&255,ab=(a>>16)&255,aa=(a>>24)&255;
	int br=b&255,bg=(b>>8)&255,bb=(b>>16)&255,ba=(b>>24)&255;
	int r=(int)(ar+(br-ar)*t), g=(int)(ag+(bg-ag)*t);
	int bl=(int)(ab+(bb-ab)*t), al=(int)(aa+(ba-aa)*t);
	return RGBA(r,g,bl,al);
}
static u32 mulcol(u32 c, float f)
{
	int r=(int)((c&255)*f), g=(int)(((c>>8)&255)*f);
	int b=(int)(((c>>16)&255)*f), a=(c>>24)&255;
	if(r>255)r=255; if(g>255)g=255; if(b>255)b=255;
	return RGBA(r,g,b,a);
}

/* world->screen offset, set once per frame */
static float g_sx, g_sy;
static float SX(float x){ return x - g_sx; }
static float SY(float y){ return y - g_sy; }

/* =====================  BACKGROUND  ==================================== */
static void draw_sky(const Game *g)
{
	float t, i;
	const Level *L = &g->level_data;
	/* vertical gradient in bands (smooth enough, cheap) */
	for (i = 0; i < 12; i++) {
		t = i / 11.0f;
		gfx_rect_grad(0, i * 23.0f, SCREEN_W, 24.0f,
		              lerpcol(L->skyTop, L->skyBot, t),
		              lerpcol(L->skyTop, L->skyBot, (i + 1) / 11.0f));
	}
}

static void draw_sun(const Game *g)
{
	float cx = SCREEN_W * 0.78f - g->camX * 0.03f;
	float cy = 58.0f;
	gfx_circle_grad(cx, cy, 70.0f, rgba(255,246,214,60), rgba(255,246,214,0));
	gfx_circle(cx, cy, 34.0f, rgba(255,247,220,255));
	gfx_circle(cx - 8, cy - 8, 20.0f, rgba(255,252,238,255));
}

static void draw_clouds(const Game *g)
{
	int i;
	float t = g->timer * 0.12f;
	u32 c = rgba(255, 255, 255, 150);
	u32 c2 = rgba(255, 255, 255, 90);
	for (i = 0; i < 6; i++) {
		float bx = fmodf(i * 520.0f + t * (0.6f + i * 0.1f), (float)SCREEN_W + 240.0f) - 120.0f;
		float by = 30.0f + (float)((i * 37) % 70);
		float s = 1.0f + (float)(i % 3) * 0.35f;
		gfx_circle(bx, by, 16 * s, c);
		gfx_circle(bx + 20 * s, by + 4, 20 * s, c2);
		gfx_circle(bx + 42 * s, by, 14 * s, c);
		gfx_rect(bx, by + 6, 46 * s, 10 * s, c);
	}
}

/* rolling hills drawn as chunky cosine ridges */
static void draw_hills(const Game *g)
{
	const Level *L = &g->level_data;
	int layer;
	for (layer = 0; layer < 3; layer++) {
		float par = 0.15f + layer * 0.17f;
		float base = 150.0f + layer * 34.0f;
		float amp  = 26.0f - layer * 6.0f;
		float period = 260.0f + layer * 70.0f;
		u32 col = (layer == 0) ? mulcol(L->hillFar, 0.82f)
		        : (layer == 1) ? L->hillFar : L->hillMid;
		float x;
		for (x = -40; x < SCREEN_W + 40; x += 40) {
			float wx = x + g_sx * par;
			float y0 = base - sinf(wx / period) * amp
			          - sinf(wx / (period * 0.37f)) * amp * 0.3f;
			float wx2 = wx + 40;
			float y1 = base - sinf(wx2 / period) * amp
			          - sinf(wx2 / (period * 0.37f)) * amp * 0.3f;
			gfx_quad(x, y0, 41, SCREEN_H - y0, col, col, col, col);
			gfx_tri(x, y0, x + 41, y1, x + 41, y0, col); /* fill the seam */
		}
	}
}

/* =====================  PLATFORMS  ===================================== */
static void draw_platform(const Platform *p)
{
	float x = SX(p->x), y = SY(p->y), w = p->w, h = p->h;
	u32 top, body, dark, rim;

	switch (p->type) {
	case PL_STONE:
		top = rgba(150,150,162,255); body = rgba(104,104,120,255);
		dark = rgba(70,70,86,255);   rim = rgba(196,196,210,255);
		break;
	case PL_WOOD:
		top = rgba(186,132,80,255);  body = rgba(142,94,52,255);
		dark = rgba(104,66,36,255);  rim = rgba(220,176,120,255);
		break;
	case PL_ICE:
		top = rgba(196,232,250,255); body = rgba(120,178,214,255);
		dark = rgba(84,132,176,255); rim = rgba(232,248,255,255);
		break;
	case PL_LEAF:
		top = rgba(126,196,96,255);  body = rgba(84,150,66,255);
		dark = rgba(54,108,48,255);  rim = rgba(178,232,140,255);
		break;
	default: /* grass */
		top = rgba(112,190,88,255);  body = rgba(122,86,52,255);
		dark = rgba(88,60,38,255);   rim = rgba(170,230,130,255);
		break;
	}

	/* body */
	gfx_quad(x, y, w, h, body, body, dark, dark);
	/* sunlit top band */
	gfx_rect(x, y, w, 5.0f, top);
	gfx_rect(x, y, w, 2.0f, rim);
	/* bottom shadow line */
	gfx_rect(x, y + h - 3.0f, w, 3.0f, dark);
	/* texture flecks along the top */
	{
		int i, n = (int)(w / 26.0f);
		for (i = 0; i < n; i++) {
			float fx = x + 8.0f + (float)i * 26.0f;
			gfx_rect(fx, y + 6.0f, 3.0f, 2.0f, mulcol(rim, 0.8f));
		}
	}
	if (p->type == PL_GRASS) {
		/* little grass tufts hanging off the front */
		int i, n = (int)(w / 18.0f);
		for (i = 0; i < n; i++) {
			float fx = x + 6.0f + (float)i * 18.0f;
			gfx_tri(fx, y + 5, fx + 3, y + 5, fx + 1.5f, y + 11, mulcol(top, 0.9f));
		}
	} else if (p->type == PL_ICE) {
		gfx_rect(x + 4, y + 6, w - 8, 2.0f, rgba(255,255,255,120));
	}
}

/* =====================  HAZARDS  ======================================= */
static void draw_hazard(const Game *g, const Hazard *hz)
{
	float x = SX(hz->x), y = SY(hz->y), w = hz->w, h = hz->h;
	float t = g->timer * 0.09f;
	if (hz->kind == HZ_SPIKE) {
		int i, n = (int)(w / 12.0f);
		for (i = 0; i < n; i++) {
			float sx = x + i * 12.0f;
			gfx_tri(sx, y + h, sx + 12, y + h, sx + 6, y, rgba(206,212,222,255));
			gfx_tri(sx + 3, y + h, sx + 6, y + h, sx + 6, y + h - 5, rgba(150,158,170,255));
		}
	} else {
		int lava = (hz->kind == HZ_LAVA);
		u32 deep = lava ? rgba(150,44,18,255) : rgba(28,86,150,255);
		u32 surf = lava ? rgba(255,140,48,255) : rgba(96,180,232,255);
		int i, n = (int)(w / 16.0f);
		gfx_rect(x, y, w, h, deep);
		gfx_rect(x, y, w, 4.0f, mulcol(surf, 0.75f));
		for (i = 0; i <= n; i++) {
			float wx = i * 16.0f + t * 6.0f;
			float wy = y + sinf(wx * 0.2f + t) * 2.0f;
			gfx_circle(x + (float)i * 16.0f, wy, 5.0f, surf);
		}
		if (lava) {
			gfx_rect(x, y - 2, w, 2, rgba(255,200,120,120));
		}
	}
}

/* =====================  DECOR  ========================================= */
static void draw_decor_one(const Decor *d, float groundY)
{
	float x = SX(d->x), y = SY(d->y), s = d->s;
	switch (d->kind) {
	case DC_TREE: {
		float trunkH = 34 * s;
		gfx_rect(x - 3 * s, y - trunkH, 6 * s, trunkH, rgba(104,70,42,255));
		gfx_circle(x, y - trunkH - 6 * s, 16 * s, rgba(72,146,70,255));
		gfx_circle(x - 13 * s, y - trunkH + 2 * s, 12 * s, rgba(72,146,70,255));
		gfx_circle(x + 13 * s, y - trunkH + 2 * s, 12 * s, rgba(72,146,70,255));
		gfx_circle(x - 6 * s, y - trunkH - 12 * s, 9 * s, rgba(120,196,104,255));
		break;
	}
	case DC_PINE: {
		float trunkH = 26 * s;
		gfx_rect(x - 2.5f * s, y - trunkH, 5 * s, trunkH, rgba(96,64,40,255));
		gfx_tri(x, y - trunkH - 34 * s, x - 18 * s, y - trunkH + 4, x + 18 * s, y - trunkH + 4, rgba(52,120,74,255));
		gfx_tri(x, y - trunkH - 50 * s, x - 13 * s, y - trunkH - 14 * s, x + 13 * s, y - trunkH - 14 * s, rgba(66,142,86,255));
		break;
	}
	case DC_BUSH:
		gfx_circle(x - 10 * s, y - 8 * s, 11 * s, rgba(66,138,66,255));
		gfx_circle(x + 10 * s, y - 8 * s, 11 * s, rgba(66,138,66,255));
		gfx_circle(x, y - 14 * s, 13 * s, rgba(84,162,80,255));
		break;
	case DC_ROCK:
		gfx_circle(x, y - 6 * s, 12 * s, rgba(120,120,132,255));
		gfx_circle(x - 8 * s, y - 4 * s, 9 * s, rgba(104,104,118,255));
		gfx_circle(x + 9 * s, y - 3 * s, 8 * s, rgba(134,134,146,255));
		break;
	case DC_LAMP: {
		float hgt = 44 * s;
		gfx_rect(x - 2, y - hgt, 4, hgt, rgba(60,58,70,255));
		gfx_circle(x, y - hgt - 4, 8 * s, rgba(255,232,150,255));
		gfx_circle_grad(x, y - hgt - 4, 22 * s, rgba(255,232,150,60), rgba(255,232,150,0));
		break;
	}
	case DC_FLOWER: {
		u32 c = (d->s > 1.0f) ? rgba(255,180,210,255) : rgba(255,236,140,255);
		gfx_rect(x, y - 6 * s, 1.5f, 6 * s, rgba(70,140,70,255));
		gfx_circle(x, y - 8 * s, 2.5f * s, c);
		break;
	}
	case DC_RUIN: {
		float hgt = 40 * s;
		gfx_rect(x - 6 * s, y - hgt, 12 * s, hgt, rgba(150,146,158,255));
		gfx_rect(x - 8 * s, y - hgt, 16 * s, 4 * s, rgba(178,174,188,255));
		gfx_rect(x - 8 * s, y - 6 * s, 16 * s, 6 * s, rgba(120,116,128,255));
		gfx_rect(x - 3 * s, y - hgt - 6, 6, 6, rgba(150,146,158,255));
		break;
	}
	}
	(void)groundY;
}

/* =====================  RUNES  ========================================= */
static void draw_rune(const Game *g, const Rune *r, int index)
{
	float bob = sinf(g->timer * 0.06f + r->bob) * 3.0f;
	float x = SX(r->x), y = SY(r->y) + bob;
	u32 c;
	if (r->taken) return;
	switch (r->kind) {
	case RUNE_EMBER: c = rgba(255,150,70,255);  break;
	case RUNE_TIDE:  c = rgba(96,190,255,255);  break;
	case RUNE_LEAF:  c = rgba(130,230,120,255); break;
	default:         c = rgba(255,232,120,255); break;
	}
	/* glow + diamond body + sparkle */
	gfx_circle_grad(x, y, 22.0f, (c & 0x00FFFFFFu) | (130u << 24), c & 0x00FFFFFFu);
	gfx_quad(x - 1, y - 10, 2, 20, mulcol(c, 1.2f), mulcol(c, 1.2f), c, c);
	gfx_quad(x - 10, y - 1, 20, 2, mulcol(c, 1.2f), mulcol(c, 1.2f), c, c);
	gfx_circle(x, y, 6.0f, rgba(255,255,255,255));
	gfx_circle(x, y, 3.0f, c);
	(void)index;
}

/* =====================  ENEMIES  ======================================= */
static void draw_enemy(const Game *g, const Enemy *e)
{
	float x = SX(e->x), y = SY(e->y), w = e->w, h = e->h;
	if (!e->alive) return;
	switch (e->kind) {
	case EN_SLIME: {
		float sq = sinf(e->t) * 2.0f;
		gfx_circle_grad(x + w * 0.5f, y + h - 6, w * 0.62f - sq,
		                rgba(150,235,130,255), rgba(70,160,90,255));
		gfx_rect(x + 2, y + h - 8, w - 4, 8, rgba(70,160,90,255));
		gfx_circle(x + 6, y + h * 0.45f, 3.0f, rgba(255,255,255,255));
		gfx_circle(x + w - 6, y + h * 0.45f, 3.0f, rgba(255,255,255,255));
		gfx_circle(x + 6.5f, y + h * 0.45f, 1.6f, rgba(20,30,20,255));
		gfx_circle(x + w - 5.5f, y + h * 0.45f, 1.6f, rgba(20,30,20,255));
		break;
	}
	case EN_BAT: {
		float flap = sinf(e->t * 3.0f) * 7.0f;
		gfx_tri(x - 10, y + 6, x + 2, y + 2, x + 2, y + 12 - flap, rgba(96,74,120,255));
		gfx_tri(x + w + 10, y + 6, x + w - 2, y + 2, x + w - 2, y + 12 - flap, rgba(96,74,120,255));
		gfx_circle(x + w * 0.5f, y + h * 0.5f, 9.0f, rgba(70,52,92,255));
		gfx_circle(x + 6, y + 5, 2.0f, rgba(255,120,120,255));
		gfx_circle(x + w - 6, y + 5, 2.0f, rgba(255,120,120,255));
		break;
	}
	default: { /* spikeball */
		gfx_ring(x + w * 0.5f, y + h * 0.5f, w * 0.55f, 3.0f, rgba(90,90,102,255));
		gfx_circle(x + w * 0.5f, y + h * 0.5f, w * 0.36f, rgba(70,70,84,255));
		gfx_circle(x + w * 0.4f, y + h * 0.4f, 3.0f, rgba(150,150,164,255));
		gfx_line(x + w * 0.5f, y + h * 0.5f, x - 2, y - 4, 2, rgba(120,120,132,255));
		break;
	}
	}
	(void)g;
}

/* =====================  PLAYER  ======================================== */
static void draw_player(const Game *g)
{
	const Player *p = &g->player;
	float x = SX(p->x), y = SY(p->y);
	float cx = x + p->w * 0.5f;
	int blink = (p->invuln > 0) && ((g->timer / 3) & 1);
	float bodyH = p->h;

	if (blink) return;

	/* soft shadow on the ground below */
	gfx_circle_grad(cx, y + p->h + 2, 11.0f, rgba(20,40,20,90), rgba(20,40,20,0));

	/* legs animate while running */
	{
		float run = p->animT;
		float legOff = (p->onGround && fabsf(p->vx) > 0.6f) ? sinf(run * 3.0f) * 4.0f : 0.0f;
		gfx_rect(cx - 5 + legOff, y + bodyH - 6, 4, 6, rgba(58,44,86,255));
		gfx_rect(cx + 1 - legOff, y + bodyH - 6, 4, 6, rgba(58,44,86,255));
	}
	/* cloak + body */
	gfx_quad(x + 1, y + 6, p->w - 2, bodyH - 12,
	         rgba(96,180,220,255), rgba(96,180,220,255),
	         rgba(60,120,190,255), rgba(60,120,190,255));
	/* cloak sway */
	{
		float sway = sinf(g->timer * 0.12f) * 1.5f + (float)p->facing * 2.0f;
		gfx_tri(x + 2, y + 10, x + 2, y + bodyH - 6,
		        x + 2 - 8 * p->facing + sway, y + bodyH - 2,
		        rgba(54,110,180,255));
	}
	/* belt */
	gfx_rect(x + 1, y + bodyH - 13, p->w - 2, 3, rgba(220,180,90,255));
	/* head */
	gfx_circle(cx, y + 7, 7.0f, rgba(240,200,170,255));
	/* hair / hood */
	gfx_circle(cx, y + 4, 7.5f, rgba(120,70,50,255));
	gfx_quad(cx - 7.5f, y + 3, 15, 6,
	         rgba(120,70,50,255), rgba(120,70,50,255),
	         rgba(90,52,38,255), rgba(90,52,38,255));
	/* face (facing) */
	{
		int f = p->facing;
		gfx_circle(cx + f * 2.0f, y + 7, 1.6f, rgba(40,30,30,255));
	}
	/* scarf highlight, gives motion read */
	gfx_rect(x + 1, y + bodyH - 18, p->w - 2, 2, rgba(230,245,255,180));

	/* dash streak */
	if (p->dash > 0) {
		gfx_rect(cx - p->facing * 18, y + 6, 16, bodyH - 12, rgba(150,220,255,90));
	}
	/* invulnerability shield ring */
	if (p->invuln > 0 && !blink) {
		gfx_ring(cx, y + p->h * 0.5f, 17.0f, 1.5f, rgba(180,230,255,140));
	}
}

/* =====================  PORTAL  ======================================== */
static void draw_portal(const Game *g)
{
	const Level *L = &g->level_data;
	float x = SX(L->exitX), y = SY(L->exitY);
	float pulse = 0.5f + 0.5f * sinf(g->exitGlow * 2.0f);
	u32 core = g->exitOpen ? rgba(255,240,170,255) : rgba(120,120,140,255);
	u32 ring = g->exitOpen ? rgba(180,230,255,255) : rgba(80,80,96,255);

	gfx_circle_grad(x, y, 34.0f, mulcol(core, 1.0f), core & 0x00FFFFFFu);
	gfx_ring(x, y, 22.0f, 4.0f, ring);
	gfx_ring(x, y, 22.0f + pulse * 4.0f, 1.5f, mulcol(ring, 1.0f));
	if (g->exitOpen) {
		int i;
		for (i = 0; i < 5; i++) {
			float a = g->exitGlow * 0.7f + i * (2.0f * PI / 5.0f);
			gfx_circle(x + cosf(a) * 26.0f, y + sinf(a) * 26.0f, 2.5f, rgba(255,255,255,220));
		}
		gfx_circle(x, y, 8.0f, rgba(255,255,255,255));
	} else {
		gfx_rect(x - 7, y - 8, 14, 16, rgba(40,40,54,255));
		gfx_rect(x - 5, y - 6, 10, 12, rgba(60,60,80,255));
	}
}

/* =====================  PARTICLES  ===================================== */
static void draw_particles(const Game *g)
{
	int i;
	for (i = 0; i < g->particleCount; i++) {
		const Particle *p = &g->particles[i];
		float t = p->life / p->maxLife;
		u32 c = p->color;
		int a = (int)((float)((c >> 24) & 255) * clamp01(t));
		gfx_rect(SX(p->x), SY(p->y), p->size * t + 0.6f, p->size * t + 0.6f,
		         (c & 0x00FFFFFFu) | ((u32)a << 24));
	}
}

/* =====================  HUD  =========================================== */
static void draw_heart(float x, float y, int fill)
{
	u32 red = fill ? rgba(235,70,90,255) : rgba(70,50,60,180);
	float s = 1.0f;
	gfx_circle(x - 4 * s, y, 4 * s, red);
	gfx_circle(x + 4 * s, y, 4 * s, red);
	gfx_tri(x - 8 * s, y + 1, x + 8 * s, y + 1, x, y + 11 * s, red);
	if (fill) gfx_circle(x - 4 * s, y - 1, 1.6f, rgba(255,180,190,220));
}

static void draw_hud(const Game *g, int remaining)
{
	const Player *p = &g->player;
	int i;
	/* health panel */
	gfx_rect(8, 8, 84, 24, rgba(20,24,40,150));
	gfx_rect(8, 8, 84, 2, rgba(255,255,255,40));
	for (i = 0; i < MAX_HEALTH; i++)
		draw_heart(24 + i * 22, 20, i < p->health);

	/* rune counter */
	{
		char buf[8];
		int n = remaining;
		gfx_rect(8, 36, 84, 22, rgba(20,24,40,150));
		/* little rune icon */
		gfx_circle(24, 47, 7.0f, rgba(255,232,120,255));
		gfx_quad(22, 40, 4, 14, rgba(255,255,255,255), rgba(255,255,255,255), rgba(255,236,140,255), rgba(255,236,140,255));
		buf[0] = 'X'; buf[1] = ' '; buf[2] = (char)('0' + (n % 10)); buf[3] = 0;
		if (n >= 10) { buf[2] = (char)('0' + n / 10); buf[3] = (char)('0' + n % 10); buf[4] = 0; }
		text_draw(buf, 40, 43, 1.6f, rgba(255,255,255,255));
	}
}

static void draw_level_banner(const Game *g)
{
	const Level *L = &g->level_data;
	/* small label bottom-left with the level name */
	text_draw_shadow(L->name, 10, SCREEN_H - 16, 1.4f,
	                 rgba(255,255,255,235), rgba(0,0,0,150));
}

/* =====================  OVERLAYS  ====================================== */
static void draw_controls_hint(void)
{
	gfx_rect(72, SCREEN_H - 46, SCREEN_W - 144, 22, rgba(20,24,40,160));
	text_draw_center("D-PAD MOVE   X JUMP   O DASH   START MENU",
	                 SCREEN_W * 0.5f, SCREEN_H - 42, 1.2f, rgba(235,240,255,255));
}

static void draw_title(const Game *g)
{
	int i;
	/* decorative animated background */
	for (i = 0; i < 5; i++) {
		float a = g->timer * 0.01f + i * 1.3f;
		gfx_circle(SCREEN_W * 0.5f + cosf(a) * 60, 108 + sinf(a) * 12, 40 + i * 3,
		           rgba(255, 220, 130, 12));
	}
	/* title card */
	gfx_rect(60, 40, SCREEN_W - 120, 74, rgba(16,22,44,180));
	gfx_rect(60, 40, SCREEN_W - 120, 3, rgba(255,220,130,220));
	text_draw_center("LOST RUNE", SCREEN_W * 0.5f, 52, 4.0f, rgba(255,232,140,255));
	text_draw_center("ADVENTURE", SCREEN_W * 0.5f, 86, 2.6f, rgba(180,230,255,255));

	text_draw_center("A 2D ADVENTURE FOR PSP", SCREEN_W * 0.5f, 128, 1.5f, rgba(220,230,255,220));

	if ((g->timer / 30) & 1)
		text_draw_center("PRESS  START", SCREEN_W * 0.5f, 158, 2.2f, rgba(255,255,255,255));

	draw_controls_hint();
	text_draw_center("COLLECT EVERY RUNE THEN ENTER THE GATE",
	                 SCREEN_W * 0.5f, 186, 1.3f, rgba(200,210,240,220));
	text_draw_center("THREE LANDS - ONE FINAL REWARD",
	                 SCREEN_W * 0.5f, 204, 1.3f, rgba(160,200,240,200));
}

static void draw_pause(void)
{
	gfx_rect(0, 0, SCREEN_W, SCREEN_H, rgba(10,14,30,150));
	gfx_rect(140, 96, 200, 80, rgba(16,22,44,220));
	gfx_rect(140, 96, 200, 3, rgba(255,220,130,220));
	text_draw_center("PAUSED", SCREEN_W * 0.5f, 112, 3.0f, rgba(255,232,140,255));
	text_draw_center("START OR X TO RESUME", SCREEN_W * 0.5f, 150, 1.3f, rgba(220,230,255,255));
}

static void draw_level_clear(const Game *g)
{
	const Level *L = &g->level_data;
	gfx_rect(0, 0, SCREEN_W, SCREEN_H, rgba(10,14,30,120 + (int)(40 * (1 - clamp01(g->timer / 110.0f)))));
	gfx_rect(80, 92, SCREEN_W - 160, 92, rgba(16,22,44,225));
	gfx_rect(80, 92, SCREEN_W - 160, 3, rgba(255,220,130,220));
	text_draw_center("GATE PASSED!", SCREEN_W * 0.5f, 104, 2.6f, rgba(255,232,140,255));
	text_draw_center(L->name, SCREEN_W * 0.5f, 138, 1.6f, rgba(180,230,255,255));
	text_draw_center("ONWARD...", SCREEN_W * 0.5f, 160, 1.4f, rgba(220,230,255,255));
}

static void draw_game_over(const Game *g)
{
	(void)g;
	gfx_rect(0, 0, SCREEN_W, SCREEN_H, rgba(40,8,18,150));
	gfx_rect(90, 92, SCREEN_W - 180, 92, rgba(40,14,26,225));
	gfx_rect(90, 92, SCREEN_W - 180, 3, rgba(255,90,110,220));
	text_draw_center("THE LIGHT FADES", SCREEN_W * 0.5f, 104, 2.6f, rgba(255,120,140,255));
	text_draw_center("PRESS START TO RISE AGAIN", SCREEN_W * 0.5f, 146, 1.5f, rgba(255,220,225,255));
}

static void draw_win(const Game *g)
{
	int i;
	for (i = 0; i < 10; i++) {
		float a = g->timer * 0.02f + i * 0.63f;
		gfx_circle(SCREEN_W * 0.5f + cosf(a) * (60 + i * 8), 90 + sinf(a * 1.3f) * 30,
		           5.0f, rgba(255, 220 - i * 8, 120 + i * 8, 220));
	}
	gfx_rect(60, 70, SCREEN_W - 120, 118, rgba(16,22,44,220));
	gfx_rect(60, 70, SCREEN_W - 120, 3, rgba(255,220,130,220));
	text_draw_center("YOU FOUND THE RUNE", SCREEN_W * 0.5f, 84, 2.6f, rgba(255,232,140,255));
	text_draw_center("OF DAWN", SCREEN_W * 0.5f, 114, 2.6f, rgba(255,232,140,255));
	{
		char buf[16];
		int n = g->totalRunes;
		buf[0]='R'; buf[1]='U'; buf[2]='N'; buf[3]='E'; buf[4]='S'; buf[5]=':'; buf[6]=' ';
		if (n >= 10) { buf[7]=(char)('0'+n/10); buf[8]=(char)('0'+n%10); buf[9]=0; }
		else { buf[7]=(char)('0'+n); buf[8]=0; }
		text_draw_center(buf, SCREEN_W * 0.5f, 148, 1.4f, rgba(220,230,255,255));
	}
	text_draw_center("THANK YOU FOR PLAYING", SCREEN_W * 0.5f, 166, 1.4f, rgba(180,230,255,255));
	text_draw_center("PRESS START", SCREEN_W * 0.5f, 182, 1.2f, rgba(255,255,255,200));
}

/* =====================  MAIN ENTRY  ==================================== */
void render_game(const Game *g)
{
	const Level *L = &g->level_data;
	int i, remaining = 0;

	/* camera shake offset */
	g_sx = g->camX;
	g_sy = g->camY;
	if (g->shake > 0.1f) {
		g_sx += sinf(g->timer * 1.7f) * g->shake;
		g_sy += cosf(g->timer * 2.3f) * g->shake;
	}

	if (g->state == GS_TITLE) {
		draw_sky(g);
		draw_sun(g);
		draw_clouds(g);
		draw_hills(g);
		/* a strip of ground for the title screen */
		{
			u32 gr = rgba(112,190,88,255);
			gfx_rect(0, 236, SCREEN_W, 36, rgba(122,86,52,255));
			gfx_rect(0, 236, SCREEN_W, 6, gr);
		}
		draw_title(g);
		return;
	}

	/* ---- world ---- */
	draw_sky(g);
	draw_sun(g);
	draw_clouds(g);
	draw_hills(g);

	/* decor behind platforms */
	for (i = 0; i < L->decorCount; i++) {
		const Decor *d = &L->decor[i];
		if (d->kind == DC_TREE || d->kind == DC_PINE || d->kind == DC_RUIN ||
		    d->kind == DC_ROCK || d->kind == DC_LAMP)
			draw_decor_one(d, d->y);
	}
	for (i = 0; i < L->platformCount; i++)
		draw_platform(&L->platforms[i]);
	for (i = 0; i < L->hazardCount; i++)
		draw_hazard(g, &L->hazards[i]);
	/* small decor on top of ground */
	for (i = 0; i < L->decorCount; i++) {
		const Decor *d = &L->decor[i];
		if (d->kind == DC_BUSH || d->kind == DC_FLOWER)
			draw_decor_one(d, d->y);
	}

	draw_portal(g);

	for (i = 0; i < L->runeCount; i++) {
		if (!L->runes[i].taken) { remaining++; }
		draw_rune(g, &L->runes[i], i);
	}
	for (i = 0; i < L->enemyCount; i++)
		draw_enemy(g, &L->enemies[i]);

	draw_particles(g);
	draw_player(g);

	/* ---- HUD ---- */
	draw_hud(g, remaining);
	draw_level_banner(g);

	/* damage / pickup flash */
	if (g->flash > 0.01f)
		gfx_rect(0, 0, SCREEN_W, SCREEN_H,
		         rgba(255, 255, 255, (int)(90 * g->flash)));

	/* ---- state overlays ---- */
	if (g->state == GS_PAUSE)       draw_pause();
	else if (g->state == GS_LEVELCLEAR) draw_level_clear(g);
	else if (g->state == GS_GAMEOVER)   draw_game_over(g);
	else if (g->state == GS_WIN)        draw_win(g);
}





