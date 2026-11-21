/* game.c - Lost Rune Adventure : level data, physics and the game loop. */
#include <string.h>
#include <math.h>
#include "gfx.h"      /* RGBA() and u32 */
#include "game.h"

/* ------------------------------- tuning -------------------------------- */
#define GRAV      0.55f
#define MOVE_SPD  2.45f
#define JUMP_V   -9.60f
#define MAX_FALL  13.0f
#define DASH_V    6.20f
#define COYOTE    6
#define JUMP_BUF  6
#define DASH_TIME 10
#define DASH_CD   42
#define INVULN    90
#define RUNE_HALF 14.0f   /* rune pickup half-size (generous, easy to grab) */
#define EXIT_HW   18.0f
#define EXIT_HH   26.0f

#define PI 3.14159265f
/* Jump apex = JUMP_V^2 / (2*GRAV) ~= 84 px.  Levels keep every reachable
 * ledge within that rise, and every gap within a jump (~90px) or a dash. */

static float clampf(float v, float lo, float hi){ return v<lo?lo:(v>hi?hi:v); }

/* deterministic little PRNG so scenery is identical every run */
static unsigned int g_rng = 12345u;
static float frand(void){ g_rng = g_rng*1664525u + 1013904223u; return (float)((g_rng>>8)&0xFFFF)/65535.0f; }

static void add_plat(Level *L,float x,float y,float w,float h,int t){
	if(L->platformCount<MAX_PLATFORMS){ Platform*p=&L->platforms[L->platformCount++]; p->x=x;p->y=y;p->w=w;p->h=h;p->type=t; }
}
static void add_haz(Level *L,float x,float y,float w,float h,int k){
	if(L->hazardCount<MAX_HAZARDS){ Hazard*z=&L->hazards[L->hazardCount++]; z->x=x;z->y=y;z->w=w;z->h=h;z->kind=k; }
}
static void add_rune(Level *L,float x,float y,int k){
	if(L->runeCount<MAX_RUNES){ Rune*r=&L->runes[L->runeCount++]; r->x=x;r->y=y;r->kind=k;r->taken=0;r->bob=frand()*6.28f; }
}
static void add_enemy(Level *L,float x,float y,int k,float range){
	if(L->enemyCount<MAX_ENEMIES){ Enemy*e=&L->enemies[L->enemyCount++];
		e->x=x;e->y=y;e->homeX=x;e->homeY=y;e->range=range;e->kind=k;
		e->vx=0;e->vy=0;e->alive=1;e->dir=1;e->t=frand()*6.28f;
		e->w=(k==EN_BAT)?20.0f:22.0f; e->h=(k==EN_BAT)?14.0f:18.0f; }
}
static void add_decor(Level *L,float x,float y,float s,int k){
	if(L->decorCount<MAX_DECOR){ Decor*d=&L->decor[L->decorCount++]; d->x=x;d->y=y;d->s=s;d->kind=k; }
}

/* ------------------------- level 1 : meadow ---------------------------- */
static void load_level_1(Level *L)
{
	int i;
	L->name="THE WHISPERING MEADOW";
	L->hint="FIND ALL RUNES - THEN REACH THE PORTAL";
	L->width=2600; L->height=272;
	L->spawnX=60; L->spawnY=200;
	L->exitX=2500; L->exitY=202;
	L->skyTop=RGBA(86,168,232,255); L->skyBot=RGBA(206,238,255,255);
	L->hillFar=RGBA(139,196,142,255); L->hillMid=RGBA(96,164,104,255);

	/* ground (top y = 232), five 80px gaps spanned by short jumps */
	add_plat(L,0,232,420,40,PL_GRASS);
	add_plat(L,500,232,380,40,PL_GRASS);
	add_plat(L,960,232,360,40,PL_GRASS);
	add_plat(L,1400,232,300,40,PL_GRASS);
	add_plat(L,1780,232,260,40,PL_GRASS);
	add_plat(L,2120,232,480,40,PL_GRASS);

	/* staircase + stepping stones (every rise <= ~74) */
	add_plat(L,470,190,70,16,PL_STONE);
	add_plat(L,620,158,64,16,PL_WOOD);
	add_plat(L,790,126,74,16,PL_STONE);
	add_plat(L,1030,186,84,16,PL_GRASS);
	add_plat(L,1210,162,66,16,PL_WOOD);
	add_plat(L,1490,186,72,16,PL_STONE);
	add_plat(L,1650,140,72,16,PL_WOOD);
	add_plat(L,1880,180,74,16,PL_STONE);
	add_plat(L,2330,186,96,16,PL_GRASS);

	/* water in the gaps, spikes on two ground segments */
	add_haz(L,420,250,80,22,HZ_WATER);
	add_haz(L,880,250,80,22,HZ_WATER);
	add_haz(L,1320,250,80,22,HZ_WATER);
	add_haz(L,1700,250,80,22,HZ_WATER);
	add_haz(L,2040,250,80,22,HZ_WATER);
	add_haz(L,762,220,52,12,HZ_SPIKE);
	add_haz(L,1622,220,52,12,HZ_SPIKE);

	add_rune(L,196,210,RUNE_EMBER);   /* ground */
	add_rune(L,654,126,RUNE_LEAF);    /* on wood  top158 */
	add_rune(L,826,98,RUNE_TIDE);     /* on stone top126 */
	add_rune(L,1242,130,RUNE_STAR);   /* on wood  top162 */
	add_rune(L,1686,108,RUNE_EMBER);  /* on wood  top140 */
	add_rune(L,2378,154,RUNE_LEAF);   /* on grass top186 */

	add_enemy(L,640,214,EN_SLIME,140);
	add_enemy(L,1120,214,EN_SLIME,150);
	add_enemy(L,1180,110,EN_BAT,0);
	add_enemy(L,1900,214,EN_SLIME,120);

	for(i=0;i<40;i++) add_decor(L,frand()*L->width,232,0.7f+frand()*0.6f,DC_FLOWER);
	for(i=0;i<26;i++) add_decor(L,frand()*L->width,232,0.9f+frand()*0.5f,(frand()<0.5f)?DC_TREE:DC_PINE);
	add_decor(L,240,232,1.1f,DC_BUSH);
	add_decor(L,760,232,1.0f,DC_BUSH);
	add_decor(L,1450,232,1.2f,DC_BUSH);
	add_decor(L,2050,232,1.0f,DC_ROCK);
	add_decor(L,120,232,1.0f,DC_LAMP);
	add_decor(L,1550,232,1.0f,DC_LAMP);
}

/* ------------------------- level 2 : ice caverns ----------------------- */
static void load_level_2(Level *L)
{
	int i;
	L->name="THE FROZEN CAVERNS";
	L->hint="SLIPPERY ICE - EIGHT RUNES";
	L->width=3000; L->height=272;
	L->spawnX=60; L->spawnY=200;
	L->exitX=2900; L->exitY=210;
	L->skyTop=RGBA(24,38,68,255); L->skyBot=RGBA(78,110,150,255);
	L->hillFar=RGBA(60,84,120,255); L->hillMid=RGBA(40,58,90,255);

	/* floating ice shelves over lava; consistent ~50-60px gaps */
	add_plat(L,0,240,300,32,PL_ICE);
	add_plat(L,360,205,150,18,PL_ICE);
	add_plat(L,570,170,150,18,PL_ICE);
	add_plat(L,780,205,150,18,PL_ICE);
	add_plat(L,990,240,220,32,PL_ICE);
	add_plat(L,1260,205,150,18,PL_ICE);
	add_plat(L,1460,165,150,18,PL_ICE);
	add_plat(L,1660,125,150,18,PL_ICE);
	add_plat(L,1860,165,150,18,PL_ICE);
	add_plat(L,2060,205,150,18,PL_ICE);
	add_plat(L,2260,165,150,18,PL_ICE);
	add_plat(L,2460,205,150,18,PL_ICE);
	add_plat(L,2660,240,340,32,PL_STONE);

	add_haz(L,300,250,60,22,HZ_LAVA);
	add_haz(L,510,250,60,22,HZ_LAVA);
	add_haz(L,720,250,60,22,HZ_LAVA);
	add_haz(L,930,250,60,22,HZ_LAVA);
	add_haz(L,1210,250,50,22,HZ_LAVA);
	add_haz(L,1410,250,50,22,HZ_LAVA);
	add_haz(L,1610,250,50,22,HZ_LAVA);
	add_haz(L,1810,250,50,22,HZ_LAVA);
	add_haz(L,2010,250,50,22,HZ_LAVA);
	add_haz(L,2210,250,50,22,HZ_LAVA);
	add_haz(L,2410,250,50,22,HZ_LAVA);
	add_haz(L,2610,250,50,22,HZ_LAVA);

	add_rune(L,420,173,RUNE_TIDE);    /* on top205 */
	add_rune(L,645,138,RUNE_EMBER);   /* on top170 */
	add_rune(L,855,173,RUNE_STAR);    /* on top205 */
	add_rune(L,1335,173,RUNE_LEAF);   /* on top205 */
	add_rune(L,1535,133,RUNE_TIDE);   /* on top165 */
	add_rune(L,1735,93,RUNE_STAR);    /* on top125 */
	add_rune(L,2335,133,RUNE_EMBER);  /* on top165 */
	add_rune(L,2535,173,RUNE_LEAF);   /* on top205 */

	add_enemy(L,1060,222,EN_SLIME,120);
	add_enemy(L,1500,110,EN_BAT,0);
	add_enemy(L,1720,90,EN_BAT,0);
	add_enemy(L,2100,187,EN_SLIME,120);

	for(i=0;i<22;i++) add_decor(L,frand()*L->width,272,0.8f+frand()*0.6f,DC_ROCK);
	for(i=0;i<10;i++) add_decor(L,frand()*L->width,240,0.9f,DC_RUIN);
	add_decor(L,900,272,1.2f,DC_RUIN);
	add_decor(L,2200,272,1.1f,DC_ROCK);
}

/* ------------------------- level 3 : cloud ascent ---------------------- */
static void load_level_3(Level *L)
{
	int i;
	L->name="THE CLOUD ASCENT";
	L->hint="CLIMB THE LEAVES - FIVE RUNES";
	L->width=3200; L->height=272;
	L->spawnX=60; L->spawnY=200;
	L->exitX=3100; L->exitY=118;
	L->skyTop=RGBA(150,112,196,255); L->skyBot=RGBA(248,182,192,255);
	L->hillFar=RGBA(198,150,214,255); L->hillMid=RGBA(168,118,192,255);

	add_plat(L,0,248,360,32,PL_GRASS);
	add_plat(L,440,244,96,14,PL_LEAF);
	add_plat(L,620,208,96,14,PL_LEAF);
	add_plat(L,800,172,96,14,PL_LEAF);
	add_plat(L,980,136,96,14,PL_LEAF);
	add_plat(L,1160,168,110,14,PL_LEAF);
	add_plat(L,1340,204,110,14,PL_LEAF);
	add_plat(L,1520,168,110,14,PL_LEAF);
	add_plat(L,1700,132,110,14,PL_LEAF);
	add_plat(L,1880,166,150,16,PL_STONE);
	add_plat(L,2080,130,110,14,PL_LEAF);
	add_plat(L,2260,164,110,14,PL_LEAF);
	add_plat(L,2440,128,110,14,PL_LEAF);
	add_plat(L,2620,162,110,14,PL_LEAF);
	add_plat(L,2810,148,390,20,PL_STONE);

	add_haz(L,360,250,80,22,HZ_LAVA);
	add_haz(L,536,250,84,22,HZ_LAVA);
	add_haz(L,716,250,84,22,HZ_LAVA);
	add_haz(L,896,250,84,22,HZ_LAVA);
	add_haz(L,1076,250,84,22,HZ_LAVA);
	add_haz(L,1270,250,70,22,HZ_LAVA);
	add_haz(L,1450,250,70,22,HZ_LAVA);
	add_haz(L,1630,250,70,22,HZ_LAVA);
	add_haz(L,1810,250,70,22,HZ_LAVA);
	add_haz(L,2030,250,50,22,HZ_LAVA);
	add_haz(L,2190,250,70,22,HZ_LAVA);
	add_haz(L,2370,250,70,22,HZ_LAVA);
	add_haz(L,2550,250,70,22,HZ_LAVA);
	add_haz(L,2730,250,80,22,HZ_LAVA);

	add_rune(L,1028,104,RUNE_STAR);   /* on top136 */
	add_rune(L,1950,134,RUNE_LEAF);   /* on stone top166 */
	add_rune(L,2495,96,RUNE_TIDE);    /* on top128 */
	add_rune(L,2675,130,RUNE_EMBER);  /* on top162 */
	add_rune(L,2960,116,RUNE_STAR);   /* on stone top148 */

	add_enemy(L,1820,150,EN_SPIKEBALL,140);
	add_enemy(L,1560,110,EN_BAT,0);
	add_enemy(L,2350,110,EN_BAT,0);
	add_enemy(L,2880,130,EN_SLIME,180);

	for(i=0;i<40;i++) add_decor(L,frand()*L->width,L->height+35,0.5f+frand()*0.6f,DC_TREE);
	for(i=0;i<24;i++) add_decor(L,frand()*L->width,272,0.6f+frand()*0.5f,DC_BUSH);
	add_decor(L,180,248,1.0f,DC_RUIN);
	add_decor(L,3050,148,1.2f,DC_RUIN);
}
/* @@ENGINE@@ */

/* ======================= load / init =================================== */
static void load_level(Level *L, int index)
{
	memset(L, 0, sizeof(Level));
	switch (index) {
		case 0: load_level_1(L); break;
		case 1: load_level_2(L); break;
		default: load_level_3(L); break;
	}
}

static void reset_player_at(Player *p, const Level *L)
{
	p->x = L->spawnX; p->y = L->spawnY;
	p->w = 16.0f; p->h = 24.0f;
	p->vx = 0; p->vy = 0;
	p->onGround = 0; p->facing = 1;
	p->health = MAX_HEALTH; p->runes = 0;
	p->invuln = 0; p->coyote = 0; p->jumpBuf = 0;
	p->dash = 0; p->dashCd = 0; p->animT = 0; p->alive = 1;
	p->safeX = L->spawnX; p->safeY = L->spawnY;
}

void game_init(Game *g)
{
	memset(g, 0, sizeof(Game));
	g->state = GS_TITLE;
	g->level = 0;
	g->totalRunes = 0;
	load_level(&g->level_data, 0);
	reset_player_at(&g->player, &g->level_data);
	g->camX = 0; g->camY = 0;
	g->flash = 0; g->shake = 0; g->exitGlow = 0; g->exitOpen = 0;
}

void game_spawn_particles(Game *g, float x, float y, int count,
                          u32 color, float speed, float grav)
{
	int i;
	for (i = 0; i < count; i++) {
		Particle *p;
		float a, s;
		if (g->particleCount >= MAX_PARTICLES)
			g->particleCount = MAX_PARTICLES - 1; /* recycle */
		p = &g->particles[g->particleCount++];
		a = frand() * 6.2831f;
		s = speed * (0.4f + frand() * 0.8f);
		p->x = x; p->y = y;
		p->vx = cosf(a) * s;
		p->vy = sinf(a) * s - speed * 0.5f;
		p->life = p->maxLife = 24.0f + frand() * 22.0f;
		p->color = color;
		p->size = 1.5f + frand() * 2.5f;
		p->grav = grav;
	}
}

/* square AABB overlap test */
static int aabb(float ax, float ay, float aw, float ah,
                float bx, float by, float bw, float bh)
{
	return ax < bx + bw && ax + aw > bx && ay < by + bh && ay + ah > by;
}

/* Platform type under the player's feet this frame (drives friction). */
static int ground_type(const Level *L, float x, float y, float w, float h)
{
	int i;
	float foot = y + h + 1.0f;
	for (i = 0; i < L->platformCount; i++) {
		const Platform *p = &L->platforms[i];
		if (x + w > p->x + 1 && x < p->x + p->w - 1 &&
		    foot >= p->y && foot <= p->y + 6.0f)
			return p->type;
	}
	return PL_GRASS;
}

/* Move the player with per-axis resolution against every platform. */
static void move_collide(Game *g, float dx, float dy)
{
	Player *p = &g->player;
	Level *L = &g->level_data;
	int i;

	p->x += dx;
	for (i = 0; i < L->platformCount; i++) {
		Platform *q = &L->platforms[i];
		if (aabb(p->x, p->y, p->w, p->h, q->x, q->y, q->w, q->h)) {
			if (dx > 0)      p->x = q->x - p->w;
			else if (dx < 0) p->x = q->x + q->w;
			p->vx = 0;
		}
	}

	p->onGround = 0;
	p->y += dy;
	for (i = 0; i < L->platformCount; i++) {
		Platform *q = &L->platforms[i];
		if (aabb(p->x, p->y, p->w, p->h, q->x, q->y, q->w, q->h)) {
			if (dy > 0) { p->y = q->y - p->h; p->vy = 0; p->onGround = 1; }
			else if (dy < 0) { p->y = q->y + q->h; p->vy = 0; }
		}
	}
}

static void hurt_player(Game *g)
{
	Player *p = &g->player;
	if (p->invuln > 0 || !p->alive)
		return;
	p->health--;
	p->invuln = INVULN;
	g->shake = 8.0f;
	g->flash = 0.7f;
	game_spawn_particles(g, p->x + p->w * 0.5f, p->y + p->h * 0.5f,
	                     14, RGBA(255, 90, 90, 255), 3.0f, 0.25f);
	if (p->health <= 0) {
		p->alive = 0;
		g->state = GS_GAMEOVER;
		g->timer = 0;
	}
}

static void respawn_player(Game *g)
{
	Player *p = &g->player;
	p->x = p->safeX;
	p->y = p->safeY;
	p->vx = p->vy = 0;
	p->invuln = INVULN;
	g->flash = 0.5f;
}

static void update_enemies(Game *g)
{
	Player *p = &g->player;
	Level *L = &g->level_data;
	int i;

	for (i = 0; i < L->enemyCount; i++) {
		Enemy *e = &L->enemies[i];
		if (!e->alive) continue;
		e->t += 0.05f;

		if (e->kind == EN_BAT) {
			e->x = e->homeX + sinf(e->t) * e->range;
			e->y = e->homeY + cosf(e->t * 1.6f) * 22.0f;
		} else if (e->kind == EN_SPIKEBALL) {
			e->x = e->homeX + sinf(e->t * 0.7f) * e->range;
		} else {
			e->x += e->dir * 0.7f;
			if (e->x < e->homeX - e->range) { e->x = e->homeX - e->range; e->dir = 1; }
			if (e->x > e->homeX + e->range) { e->x = e->homeX + e->range; e->dir = -1; }
		}

		if (!aabb(p->x, p->y, p->w, p->h, e->x, e->y, e->w, e->h))
			continue;

		if (p->vy > 1.0f && (p->y + p->h) - e->y < 14.0f) {
			e->alive = 0;
			p->vy = -7.5f;
			g->shake = 4.0f;
			game_spawn_particles(g, e->x + e->w * 0.5f, e->y,
			                     12, RGBA(180, 230, 120, 255), 2.5f, 0.2f);
		} else {
			hurt_player(g);
		}
	}
}

static void update_particles(Game *g)
{
	int i;
	for (i = 0; i < g->particleCount; i++) {
		Particle *q = &g->particles[i];
		q->life -= 1.0f;
		q->vy += q->grav;
		q->x += q->vx;
		q->y += q->vy;
		if (q->life <= 0) {
			g->particles[i] = g->particles[g->particleCount - 1];
			g->particleCount--;
			i--;
		}
	}
}

static void collect_runes(Game *g)
{
	Player *p = &g->player;
	Level *L = &g->level_data;
	int i;
	for (i = 0; i < L->runeCount; i++) {
		Rune *r = &L->runes[i];
		if (r->taken) continue;
		if (aabb(p->x, p->y, p->w, p->h,
		         r->x - RUNE_HALF, r->y - RUNE_HALF, RUNE_HALF * 2, RUNE_HALF * 2)) {
			r->taken = 1;
			p->runes++;
			g->totalRunes++;
			g->flash = 0.35f;
			game_spawn_particles(g, r->x, r->y, 18, RGBA(255, 224, 120, 255), 3.0f, -0.02f);
		}
	}
}

static void check_hazards(Game *g)
{
	Player *p = &g->player;
	Level *L = &g->level_data;
	int i;
	for (i = 0; i < L->hazardCount; i++) {
		Hazard *h = &L->hazards[i];
		if (!aabb(p->x, p->y, p->w, p->h, h->x, h->y, h->w, h->h))
			continue;
		if (h->kind == HZ_SPIKE) {
			hurt_player(g);
		} else if (p->invuln <= 0) {
			hurt_player(g);
			if (p->alive) respawn_player(g);
		}
	}
	if (p->y > L->height + 40.0f && p->invuln <= 0) {
		hurt_player(g);
		if (p->alive) respawn_player(g);
	}
}

static void update_camera(Game *g)
{
	float targetX = g->player.x + g->player.w * 0.5f - VIEW_W * 0.5f;
	float targetY = g->player.y + g->player.h * 0.5f - VIEW_H * 0.5f;
	float maxX = g->level_data.width - VIEW_W;
	float maxY = g->level_data.height - VIEW_H;
	if (maxX < 0) maxX = 0;
	if (maxY < 0) maxY = 0;
	g->camX += (clampf(targetX, 0, maxX) - g->camX) * 0.12f;
	g->camY += (clampf(targetY, 0, maxY) - g->camY) * 0.10f;
}

static void start_level(Game *g, int index)
{
	g->level = index;
	load_level(&g->level_data, index);
	reset_player_at(&g->player, &g->level_data);
	g->camX = 0; g->camY = 0;
	g->particleCount = 0;
	g->exitOpen = 0;
	g->exitGlow = 0;
	g->flash = 0.6f;
	g->state = GS_PLAY;
}

void game_update(Game *g, const Input *in)
{
	Player *p = &g->player;
	Level *L = &g->level_data;

	switch (g->state) {
	case GS_TITLE:
		g->timer++;
		g->exitGlow += 0.05f;
		if (in->start || in->jump) {
			g->totalRunes = 0;
			start_level(g, 0);
		}
		return;

	case GS_PAUSE:
		if (in->start || in->jump)
			g->state = GS_PLAY;
		return;

	case GS_LEVELCLEAR:
		if (g->timer > 0) g->timer--;
		update_particles(g);
		if (g->timer <= 0) {
			if (g->level + 1 >= MAX_LEVELS) {
				g->state = GS_WIN;
				g->timer = 0;
			} else {
				start_level(g, g->level + 1);
			}
		}
		return;

	case GS_GAMEOVER:
		g->timer++;
		if (in->start || in->jump)
			start_level(g, g->level);
		return;

	case GS_WIN:
		g->timer++;
		if (in->start || in->jump) {
			g->state = GS_TITLE;
			g->timer = 0;
		}
		return;

	default:
		break;
	}

	/* ---------------- GS_PLAY ---------------- */
	if (in->start) { g->state = GS_PAUSE; return; }

	g->timer++;
	if (p->invuln > 0) p->invuln--;
	if (p->dashCd > 0) p->dashCd--;
	if (g->shake > 0.1f) g->shake *= 0.85f; else g->shake = 0;
	if (g->flash > 0.01f) g->flash *= 0.90f; else g->flash = 0;
	g->exitGlow += 0.05f;

	/* horizontal intent, with ice friction */
	{
		int gt = p->onGround ? ground_type(L, p->x, p->y, p->w, p->h) : PL_GRASS;
		float friction = (gt == PL_ICE) ? 0.94f : 0.66f;
		if (in->left && !in->right)       { p->vx -= MOVE_SPD * 0.42f; p->facing = -1; }
		else if (in->right && !in->left)  { p->vx += MOVE_SPD * 0.42f; p->facing =  1; }
		else                              { p->vx *= friction; }
		if (p->vx >  MOVE_SPD) p->vx =  MOVE_SPD;
		if (p->vx < -MOVE_SPD) p->vx = -MOVE_SPD;
		if (fabsf(p->vx) < 0.05f) p->vx = 0;
	}

	/* jump buffering + coyote time keep the platforming forgiving */
	if (in->jump) p->jumpBuf = JUMP_BUF;
	if (p->jumpBuf > 0) p->jumpBuf--;
	if (p->onGround) p->coyote = COYOTE;
	else if (p->coyote > 0) p->coyote--;

	if (p->jumpBuf > 0 && p->coyote > 0) {
		p->vy = JUMP_V;
		p->jumpBuf = 0;
		p->coyote = 0;
		p->onGround = 0;
		game_spawn_particles(g, p->x + p->w * 0.5f, p->y + p->h, 6,
		                     RGBA(230, 240, 255, 200), 1.6f, 0.12f);
	}
	if (!in->jumpHeld && p->vy < -3.0f)
		p->vy += 0.45f;   /* release X early for a shorter hop */

	/* dash: also usable in the air to clear the widest gaps */
	if (in->action && p->dashCd <= 0) {
		p->dash = DASH_TIME;
		p->dashCd = DASH_CD;
		p->vy = -1.0f;
		game_spawn_particles(g, p->x + p->w * 0.5f, p->y + p->h * 0.5f, 8,
		                     RGBA(140, 210, 255, 220), 2.2f, 0.0f);
	}
	if (p->dash > 0) {
		p->dash--;
		p->vx = p->facing * DASH_V;
		p->vy *= 0.4f;
	}

	p->vy += GRAV;
	if (p->vy > MAX_FALL) p->vy = MAX_FALL;

	/* integrate, sub-stepped so thin platforms never get tunnelled through */
	{
		float dx = p->vx, dy = p->vy;
		int steps = (int)(fabsf(dx) / 8.0f) + (int)(fabsf(dy) / 8.0f) + 1;
		int s;
		if (steps > 8) steps = 8;
		for (s = 0; s < steps; s++)
			move_collide(g, dx / steps, dy / steps);
	}
	p->animT += (p->onGround && fabsf(p->vx) > 0.6f) ? 0.28f : 0.05f;

	/* remember the last safe foothold for fair respawns */
	if (p->onGround && p->vy >= 0) {
		int safeHere = 1, hi;
		for (hi = 0; hi < L->hazardCount; hi++) {
			Hazard *h = &L->hazards[hi];
			if (aabb(p->x - 24, p->y, p->w + 48, p->h + 8,
			         h->x, h->y, h->w, h->h)) { safeHere = 0; break; }
		}
		if (safeHere) { p->safeX = p->x; p->safeY = p->y; }
	}

	update_enemies(g);
	check_hazards(g);
	collect_runes(g);
	update_particles(g);

	/* exit gate: opens once every rune in the level is taken */
	{
		int got = 0, i;
		for (i = 0; i < L->runeCount; i++) if (L->runes[i].taken) got++;
		g->exitOpen = (got >= L->runeCount);
	}
	if (g->exitOpen &&
	    aabb(p->x, p->y, p->w, p->h,
	         L->exitX - EXIT_HW, L->exitY - EXIT_HH, EXIT_HW * 2, EXIT_HH * 2)) {
		g->state = GS_LEVELCLEAR;
		g->timer = 110;
		g->flash = 0.8f;
		game_spawn_particles(g, L->exitX, L->exitY, 40,
		                     RGBA(255, 245, 200, 255), 4.0f, -0.01f);
	}

	update_camera(g);
}

