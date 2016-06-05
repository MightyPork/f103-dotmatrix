#include "mode_life.h"
#include "utils/timebase.h"
#include "com/debug.h"

#include "dotmatrix.h"

static bool life_enabled = false;

static void show_board(bool do_show);
static void apply_paint(void);

#define BOARD_W SCREEN_W
#define BOARD_H SCREEN_H

#define SIZEOF_BOARD (BOARD_H*sizeof(uint32_t))

// 32-bit for easy blit
static bool is_orig_board = true; // marks that currently displayed board is the orig board to reset back to.
static uint32_t board_orig[BOARD_H];

static uint32_t board_prev[BOARD_H];
static uint32_t board[BOARD_H];

static task_pid_t task_gametick; // game update tick
static task_pid_t task_movetick; // repeated move
static task_pid_t task_start_moveticks; // scheduled task to start moveticks after a delay

static task_pid_t task_movepress; // move on press (delayed to allow for diagonals)

static task_pid_t task_blink; // periodic blink task
static task_pid_t task_blink2; // scheduled 'unblink' task

static ms_time_t step_time = 200; // game step time

#define BLINKLEN 350 // blink on time (total)
#define BLINKLEN_ONE 300 // blink on time for '1'
#define BLINKLEN_ZERO 10 // blink on time for '0'

#define MOVE_TIME 80 // mouse move
#define MOVE_START_TIME 600

#define WRAPPING 0

static int cursorX = BOARD_W / 2 - 1;
static int cursorY = BOARD_H / 2 - 1;

static bool running = false;

static bool painting_1 = false;
static bool painting_0 = false;

static bool modA_down = false;
static bool modB_down = false;

// X+,X-,Y+,Y-
typedef enum {
	DIR_NONE = 0b0000,
	// straight
	DIR_N = 0b0010,
	DIR_S = 0b0001,
	DIR_E = 0b1000,
	DIR_W = 0b0100,
	// diagonals
	DIR_NE = DIR_N | DIR_E,
	DIR_NW = DIR_N | DIR_W,
	DIR_SE = DIR_S | DIR_E,
	DIR_SW = DIR_S | DIR_W,
} MoveDir;

static MoveDir active_dir = DIR_NONE;

// ---

static void board_set(int x, int y, bool color)
{
	if (x < 0 || y < 0 || x >= BOARD_W || y >= BOARD_H) return;

	uint32_t mask = 1 << (BOARD_W - x - 1);

	if (color) {
		board[y] |= mask;
	} else {
		board[y] &= ~mask;
	}
}

static bool board_get(uint32_t *brd, int x, int y)
{
#if WRAPPING
	while (x < 0) x += BOARD_W;
	while (y < 0) y += BOARD_H;
	while (x >= BOARD_W) x -= BOARD_W;
	while (y >= BOARD_H) y -= BOARD_H;
#else
	if (x < 0 || y < 0 || x >= BOARD_W || y >= BOARD_H) return 0;
#endif

	return (brd[y] >> (BOARD_W - x - 1)) & 1;
}

static void move_cursor(void)
{
	if (active_dir & DIR_N) {
		cursorY++;
	} else if (active_dir & DIR_S) {
		cursorY--;
	}

	if (active_dir & DIR_E) {
		cursorX++;
	} else if (active_dir & DIR_W) {
		cursorX--;
	}

	// clamp
	if (cursorX >= BOARD_W) cursorX = BOARD_W - 1;
	if (cursorX < 0) cursorX = 0;

	if (cursorY >= BOARD_H) cursorY = BOARD_H - 1;
	if (cursorY < 0) cursorY = 0;

	// apply paint if active
	apply_paint();

	show_board(true);
	dmtx_set(dmtx, cursorX, cursorY, 1);
	abort_scheduled_task(task_blink2); // abort unblink task FIXME ???
}

static void apply_paint(void)
{
	if (painting_0) {
		board_set(cursorX, cursorY, 0);
	}

	if (painting_1) {
		board_set(cursorX, cursorY, 1);
	}
}

/** start periodic move */
static void start_moveticks_cb(void *unused)
{
	(void)unused;
	if (!life_enabled) return;

	task_start_moveticks = 0;
	enable_periodic_task(task_movetick, true);
}


/** Perform one game step */
static void gametick_cb(void *unused)
{
	(void)unused;
	if (!life_enabled) return;

	dbg("Game tick!");

	memcpy(board_prev, board, SIZEOF_BOARD);
	for (int i = 0; i < BOARD_W; i++) {
		for (int j = 0; j < BOARD_H; j++) {
			int count = 0;

			// Above
			count += board_get(board_prev, i - 1, j - 1);
			count += board_get(board_prev, i, j - 1);
			count += board_get(board_prev, i + 1, j - 1);

			// Sides
			count += board_get(board_prev, i - 1, j);
			count += board_get(board_prev, i + 1, j);

			// Below
			count += board_get(board_prev, i - 1, j + 1);
			count += board_get(board_prev, i, j + 1);
			count += board_get(board_prev, i + 1, j + 1);

			bool at = board_get(board_prev, i, j);

			//board_set(i, j, count == 2 || count == 3);

			if (at) {
				// live cell
				board_set(i, j, count == 2 || count == 3);
			} else {
				board_set(i, j, count == 3);
			}
		}
	}

	show_board(true);
}

/** Move cursor one step in active direction */
static void movetick_cb(void *unused)
{
	(void)unused;
	if (!life_enabled) return;

	move_cursor();
}

static void blink_cb(void *arg)
{
	if (!life_enabled) return; // pending leftover...

	uint32_t which = (uint32_t)arg;

	if (which == 0) {
		// first
		show_board(false);

		bool at = board_get(board, cursorX, cursorY);
		dmtx_set(dmtx, cursorX, cursorY, 1);
		dmtx_show(dmtx);

		// schedule unblink
		schedule_task(blink_cb, (void*)1, at ? BLINKLEN_ONE : BLINKLEN_ZERO, true);

	} else {
		show_board(false);
		dmtx_set(dmtx, cursorX, cursorY, 0);
		dmtx_show(dmtx);
	}
}

// ---

static void show_board(bool do_show)
{
	dmtx_set_block(dmtx, 0, 0, board, 16, 16);
	if (do_show) dmtx_show(dmtx);
}

// ---

void mode_life_init(void)
{
	// prepare tasks
	task_gametick = add_periodic_task(gametick_cb, NULL, step_time, true);
	task_movetick = add_periodic_task(movetick_cb, NULL, MOVE_TIME, true);
	task_blink = add_periodic_task(blink_cb, 0, BLINKLEN, true);

	// stop all - we may not be starting this mode just yet
	enable_periodic_task(task_gametick, false);
	enable_periodic_task(task_movetick, false);
	enable_periodic_task(task_blink, false);
}

void mode_life_start(void)
{
	life_enabled = true;

	enable_periodic_task(task_blink, true);

//	if (running) {
//		enable_periodic_task(task_gametick, true);
//	} else {
//		enable_periodic_task(task_blink, true);
//	}
}

void mode_life_stop(void)
{
	enable_periodic_task(task_gametick, false);
	enable_periodic_task(task_movetick, false);
	enable_periodic_task(task_blink, false);

	abort_scheduled_task(task_blink2);
	abort_scheduled_task(task_movepress);
	abort_scheduled_task(task_start_moveticks);

	painting_1 = painting_0 = false;
	modA_down = modB_down = false;
	running = false; // stop

	life_enabled = false;
}


static void toggle_game(bool run)
{
	if (run) {
		running = true;
		enable_periodic_task(task_movetick, false);
		enable_periodic_task(task_blink, false);
		abort_scheduled_task(task_movepress);
		abort_scheduled_task(task_start_moveticks);

		// Go!!
		enable_periodic_task(task_gametick, true);
	} else {
		running = false;
		enable_periodic_task(task_gametick, false);
		enable_periodic_task(task_blink, true);
	}

	show_board(true);
}

/** do move (had ample time to press both arrows for diagonal) */
static void movepress_cb(void *unused)
{
	(void)unused;
	if (!life_enabled) return;

	move_cursor();
	task_movepress = 0;

	task_start_moveticks = schedule_task(start_moveticks_cb, NULL, MOVE_START_TIME, true);
}

void mode_life_btn(char key)
{
	// Common for run / stop mode
	switch (key) {
		case 'A': modA_down = true; break;
		case 'a': modA_down = false; break;
		case 'B': modB_down = true; break;
		case 'b': modB_down = false; break;

		case 'K':
			// Reset btn
			if (modB_down) {
				// total reset
				dbg("Reset to blank");
				memset(board_orig, 0, SIZEOF_BOARD);
				memset(board, 0, SIZEOF_BOARD);

				toggle_game(false);
				is_orig_board = true;
			} else {
				// reset to orig
				dbg("Reset to original");
				memcpy(board, board_orig, SIZEOF_BOARD);
				toggle_game(false);
				is_orig_board = true;
			}
			break;
	}


	if (running) {

		switch (key) {
			case 'Y': // slower
				if (step_time < 1000) {
					step_time += 50;
					set_periodic_task_interval(task_gametick, step_time);
				}
				break;

			case 'X': // faster
				if (step_time > 50) {
					step_time -= 50;
					set_periodic_task_interval(task_gametick, step_time);
				}
				break;

			case 'J': // stop
				dbg("STOP game!");
				toggle_game(false);
				break;
		}

	} else {

		bool want_move = false;
		bool clear_moveticks = false;
		bool want_apply_paint = false;

		// Groups
		switch (key) {
			case 'U':
			case 'D':
			case 'R':
			case 'L':
				want_move = true;
				break;

			case 'u':
			case 'd':
			case 'r':
			case 'l':
				clear_moveticks = true;
				break;

			case 'Y': // AA
			case 'X': // BB
				want_apply_paint = true;
				break;
		}

		// Individual effects
		switch (key) {
			// --- ARROW DOWN ---

			case 'U':
				active_dir &= ~DIR_S;
				active_dir |= DIR_N;
				break;

			case 'D':
				active_dir &= ~DIR_N;
				active_dir |= DIR_S;
				break;

			case 'R':
				active_dir &= ~DIR_W;
				active_dir |= DIR_E;
				break;

			case 'L':
				active_dir &= ~DIR_E;
				active_dir |= DIR_W;
				break;

			// --- ARROW UP ---

			case 'u': active_dir &= ~DIR_N; break;
			case 'd': active_dir &= ~DIR_S; break;
			case 'r': active_dir &= ~DIR_E; break;
			case 'l': active_dir &= ~DIR_W; break;

			// --- Paint BTN ---

			case 'X': painting_1 = true; break;
			case 'x': painting_1 = false; break;
			case 'Y': painting_0 = true; break;
			case 'y': painting_0 = false; break;

			// --- Control ---

			case 'J':
				dbg("Start game!");

				// starting
				if (is_orig_board) {
					// save
					memcpy(board_orig, board, SIZEOF_BOARD);
				}

				is_orig_board = false;

				toggle_game(true);
				break;
		}

		if (clear_moveticks) {
			enable_periodic_task(task_movetick, false);
			abort_scheduled_task(task_start_moveticks);
		}

		if (want_apply_paint) {
			apply_paint();
		}

		if (want_move) {
			if (task_movepress == 0) {
				task_movepress = schedule_task(movepress_cb, NULL, 10, true);
			}
		}
	}
}
