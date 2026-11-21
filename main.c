/* main.c - Lost Rune Adventure
 * PSP entry point: initialise GU + controls, then run the frame loop.
 *
 * Controls
 *   D-Pad / Analog : move
 *   X (cross)      : jump  (hold for a higher jump)
 *   O (circle)     : dash
 *   START          : start / pause
 */
#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspctrl.h>
#include <pspgu.h>
#include <stdlib.h>

#include "gfx.h"
#include "game.h"
#include "render.h"

PSP_MODULE_INFO("LostRuneAdventure", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);
PSP_HEAP_SIZE_KB(6 * 1024);

static int running = 1;

static int exit_callback(int arg1, int arg2, void *common)
{
	(void)arg1; (void)arg2; (void)common;
	running = 0;
	return 0;
}
static int callback_thread(SceSize args, void *argp)
{
	(void)args; (void)argp;
	int cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
	sceKernelRegisterExitCallback(cbid);
	sceKernelSleepThreadCB();
	return 0;
}
static void setup_callbacks(void)
{
	int thid = sceKernelCreateThread("update_thread", callback_thread,
	                                 0x11, 0xFA0, 0, 0);
	if (thid >= 0)
		sceKernelStartThread(thid, 0, 0);
}

/* dead-zone helper for the analog nub */
static int axis_active(int v)
{
	return (v > 90 || v < -90);
}

int main(void)
{
	Game game;
	SceCtrlData pad, prev;
	Input in;
	int first = 1;

	setup_callbacks();

	sceCtrlSetSamplingCycle(0);
	sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);

	gfx_init();
	game_init(&game);

	while (running) {
		sceCtrlPeekBufferPositive(&pad, 1);
		if (first) { prev = pad; first = 0; }

		/* --- build the input struct, using edges for the action buttons --- */
		in.left      = (pad.Buttons & PSP_CTRL_LEFT)  ? 1 : 0;
		in.right     = (pad.Buttons & PSP_CTRL_RIGHT) ? 1 : 0;
		in.up        = (pad.Buttons & PSP_CTRL_UP)    ? 1 : 0;
		in.down      = (pad.Buttons & PSP_CTRL_DOWN)  ? 1 : 0;
		if (axis_active(pad.Lx - 128)) {
			if (pad.Lx - 128 < 0) in.left = 1; else in.right = 1;
		}
		in.jumpHeld  = (pad.Buttons & PSP_CTRL_CROSS) ? 1 : 0;
		in.jump      = ((pad.Buttons & PSP_CTRL_CROSS) && !(prev.Buttons & PSP_CTRL_CROSS)) ? 1 : 0;
		in.action    = ((pad.Buttons & PSP_CTRL_CIRCLE) && !(prev.Buttons & PSP_CTRL_CIRCLE)) ? 1 : 0;
		in.start     = ((pad.Buttons & PSP_CTRL_START) && !(prev.Buttons & PSP_CTRL_START)) ? 1 : 0;

		game_update(&game, &in);

		gfx_start_frame(game.state == GS_TITLE ? RGB(40, 60, 110) : 0xff000000u);
		render_game(&game);
		gfx_end_frame();

		prev = pad;
	}

	sceGuTerm();
	sceKernelExitGame();
	return 0;
}
