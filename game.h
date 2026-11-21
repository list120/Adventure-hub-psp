/* game.h - Lost Rune Adventure : data model, state machine and update loop. */
#ifndef GAME_H
#define GAME_H

#include <psptypes.h>

#define MAX_PLATFORMS 96
#define MAX_HAZARDS   48
#define MAX_RUNES     24
#define MAX_ENEMIES   24
#define MAX_DECOR     96
#define MAX_PARTICLES 160
#define MAX_LEVELS    3
#define MAX_HEALTH    3

/* ---- control input, filled by main.c each frame ------------------------ */
typedef struct {
	int left, right, up, down;
	int jump;      /* cross   - edge triggered by caller */
	int action;    /* circle  - edge triggered by caller */
	int jumpHeld;  /* cross held */
	int start;     /* start   - edge triggered by caller */
} Input;

/* ---- world objects ---------------------------------------------------- */
/* platform types drive both collision feel and the look. */
enum { PL_GRASS, PL_STONE, PL_WOOD, PL_ICE, PL_LEAF };

typedef struct { float x, y, w, h; int type; } Platform;

enum { HZ_SPIKE, HZ_WATER, HZ_LAVA };
typedef struct { float x, y, w, h; int kind; } Hazard;

enum { RUNE_EMBER, RUNE_TIDE, RUNE_LEAF, RUNE_STAR };
typedef struct { float x, y; int kind; int taken; float bob; } Rune;

enum { EN_SLIME, EN_BAT, EN_SPIKEBALL };
typedef struct {
	float x, y, w, h;
	float vx, vy;
	float homeX, homeY, range;
	int kind, alive, dir;
	float t;
} Enemy;

enum { DC_TREE, DC_PINE, DC_BUSH, DC_ROCK, DC_LAMP, DC_FLOWER, DC_RUIN };
typedef struct { float x, y; float s; int kind; } Decor;

typedef struct { float x, y, vx, vy, life, maxLife; u32 color; float size; float grav; } Particle;

/* ---- the player ------------------------------------------------------- */
typedef struct {
	float x, y, w, h;
	float vx, vy;
	int onGround, facing;   /* facing: -1 left, +1 right */
	int health, runes;
	int invuln;             /* frames of invulnerability after a hit */
	int coyote;             /* coyote-time frames */
	int jumpBuf;            /* buffered jump frames */
	int dash;               /* dash frames remaining */
	int dashCd;
	float animT;            /* running animation clock */
	float safeX, safeY;     /* last safe ground position (checkpoint) */
	int alive;
} Player;

/* ---- a level ---------------------------------------------------------- */
typedef struct {
	const char *name;
	const char *hint;
	float width, height;
	float spawnX, spawnY;
	float exitX, exitY;      /* portal, opens when all runes collected */
	u32 skyTop, skyBot;
	u32 hillFar, hillMid;
	Platform platforms[MAX_PLATFORMS];
	int      platformCount;
	Hazard   hazards[MAX_HAZARDS];
	int      hazardCount;
	Rune     runes[MAX_RUNES];
	int      runeCount;
	Enemy    enemies[MAX_ENEMIES];
	int      enemyCount;
	Decor    decor[MAX_DECOR];
	int      decorCount;
} Level;

/* ---- high level game states ------------------------------------------ */
enum { GS_TITLE, GS_PLAY, GS_PAUSE, GS_LEVELCLEAR, GS_GAMEOVER, GS_WIN };

typedef struct {
	int state;
	int level;               /* current level index */
	int timer;               /* generic state timer */
	int totalRunes;          /* collected across the run, for the win screen */
	Player player;
	Level level_data;        /* the live, mutable copy of the current level */
	Particle particles[MAX_PARTICLES];
	int particleCount;
	float camX, camY;
	float shake;
	float flash;             /* screen flash 0..1 (damage, teleport) */
	float exitGlow;
	int exitOpen;
} Game;

/* Camera view size (smaller than the 480x272 screen? -> matches screen). */
#define VIEW_W 480.0f
#define VIEW_H 272.0f

void game_init(Game *g);
void game_update(Game *g, const Input *in);

/* particle helper shared with the renderer / game code */
void game_spawn_particles(Game *g, float x, float y, int count, u32 color, float speed, float grav);

#endif
