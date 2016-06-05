#include "mode_snake.h"
#include "com/debug.h"
#include "dotmatrix.h"
#include "utils/timebase.h"

#define BOARD_W SCREEN_W
#define BOARD_H SCREEN_H

static bool snake_active = false;

/** Snake movement direction */
typedef enum {
	NORTH,
	SOUTH,
	EAST,
	WEST
} Direction;

typedef enum {
	CELL_EMPTY,
	CELL_BODY,
	CELL_FOOD,
	CELL_WALL
} CellType;

/** Board cell */
typedef struct __attribute__((packed)) {
	CellType type : 2; // Cell type
	Direction dir : 2; // direction the head moved to from this tile, if body = 1
} Cell;

/** Wall tile, invariant, used for out-of-screen coords */
static const Cell WALL_TILE = {CELL_WALL, 0};

/** Game board */
static Cell board[BOARD_H][BOARD_W];

typedef struct {
	int x;
	int y;
} Coord;

/** Snake coords */
static Coord head;
static Coord tail;
static Direction head_dir;

/** blinking to visually change 'color' */

typedef struct {
	bool pwm_bit;
	task_pid_t on_task; // periodic
	task_pid_t off_task; // scheduled
	ms_time_t interval_ms;
	ms_time_t offtime_ms;
} snake_pwm;


static bool moving = false;
static task_pid_t task_snake_move;


static snake_pwm food_pwm = {
	.interval_ms = 400,
	.offtime_ms = 330
};


static snake_pwm head_pwm = {
	.interval_ms = 100,
	.offtime_ms = 90
};

static ms_time_t move_interval = 500;


/** Get board cell at coord (x,y) */
static Cell *cell_at_xy(int x, int y)
{
	if (x < 0 || x >= BOARD_W || y < 0 || y >= BOARD_H ) {
		return &WALL_TILE; // discards const - OK
	}

	return &board[y][x];
}

/** Get board cell at coord */
static Cell *cell_at(Coord *coord)
{
	return cell_at_xy(coord->x, coord->y);
}

/** Place food in a random empty tile */
static void place_food(void)
{
	Coord food;

	// 1000 tries - timeout in case board is full of snake (?)
	for (int i = 0; i < 1000; i++) {
		food.x = rand() % BOARD_W;
		food.y = rand() % BOARD_H;

		Cell *cell = cell_at(&food);
		if (cell->type == CELL_EMPTY) {
			cell->type = CELL_FOOD;

			dbg("Food at [%d, %d]", food.x, food.y);
			break;
		}
	}
}

/** Clear the board and prepare for a new game */
static void new_game(void)
{
	// randomize (we can assume it takes random time before the user switches to the Snake mode)
	srand(SysTick->VAL);

	// Erase the board
	memset(board, 0, sizeof(board));

	// Place initial snake

	tail.x = BOARD_W/2-5;
	tail.y = BOARD_H/2;

	head.x = tail.x + 4;
	head.y = tail.y;

	head_dir = EAST;

	for (int x = tail.x; x < head.x; x++) {
		board[tail.y][x].type = CELL_BODY;
	}

	place_food();
}

static void show_board(void)
{
	if (!snake_active) return;

	dmtx_clear(dmtx);

	for (int x = 0; x < BOARD_W; x++) {
		for (int y = 0; y < BOARD_H; y++) {
			Cell *cell = cell_at_xy(x, y);

			bool set = 0;

			switch (cell->type) {
				case CELL_EMPTY: set = 0; break;
				case CELL_BODY: set = 1; break;
				case CELL_FOOD:
					set = food_pwm.pwm_bit;
					break;

				case CELL_WALL: set = 1; break;
			}

			if (x == head.x && y == head.y) set = head_pwm.pwm_bit;

			dmtx_set(dmtx, x, y, set);
		}
	}

	dmtx_show(dmtx);
}

static void snake_move_cb(void *unused)
{
	(void)unused;

	dbg("Move.");
}

// --- Snake PWM ---

/** Turn off a PWM bit (scheduled callback) */
static void task_pwm_off_cb(void *ptr)
{
	if (!snake_active) return;

	((snake_pwm*)ptr)->pwm_bit = 0;
	show_board();
}

/** Turn on a PWM bit and schedule the turn-off task (periodic callback) */
static void task_pwm_on_cb(void *ptr)
{
	if (!snake_active) return;

	((snake_pwm*)ptr)->pwm_bit = 1;
	show_board();

	schedule_task(task_pwm_off_cb, ptr, ((snake_pwm*)ptr)->offtime_ms, true);
}

/** Initialize a snake PWM channel */
static void snake_pwm_init(snake_pwm *ptr)
{
	ptr->on_task = add_periodic_task(task_pwm_on_cb, ptr, ptr->interval_ms, true);
	enable_periodic_task(ptr->on_task, false);
}

/** Clear &  start a snake PWM channel */
static void snake_pwm_start(snake_pwm *ptr)
{
	ptr->pwm_bit = 1;
	reset_periodic_task(ptr->on_task);
	enable_periodic_task(ptr->on_task, true);
}

/** Stop a snake PWM channel */
static void snake_pwm_stop(snake_pwm *ptr)
{
	enable_periodic_task(ptr->on_task, false);
	abort_scheduled_task(ptr->off_task);
}



/** INIT snake */
void mode_snake_init(void)
{
	snake_pwm_init(&head_pwm);
	snake_pwm_init(&food_pwm);

	task_snake_move = add_periodic_task(snake_move_cb, NULL, move_interval, true);
	enable_periodic_task(task_snake_move, false);
}

/** START playing */
void mode_snake_start(void)
{
	snake_active = true;

	snake_pwm_start(&head_pwm);
	snake_pwm_start(&food_pwm);

	// Stop snake (make sure it's stopped)
	enable_periodic_task(task_snake_move, false);
	moving = false;

	new_game();
}

/** STOP playing */
void mode_snake_stop(void)
{
	snake_active = false;

	snake_pwm_stop(&head_pwm);
	snake_pwm_stop(&food_pwm);

	enable_periodic_task(task_snake_move, false);
}

/** User button */
void mode_snake_btn(char key)
{
	switch (key) {
		case 'U': head_dir = NORTH; break;
		case 'D': head_dir = SOUTH; break;
		case 'L': head_dir = WEST; break;
		case 'R': head_dir = EAST; break;

		case 'K': // clear
			// TODO reset animation
			mode_snake_stop();
			mode_snake_start();
			break;
	}

	if (!moving && (key == 'U' || key == 'D' || key == 'L' || key == 'R' || key == 'J')) {
		// start moving
		reset_periodic_task(task_snake_move);
		enable_periodic_task(task_snake_move, true);
	}

	// running + start -> 'pause'
	if (moving && key == 'J') {
		enable_periodic_task(task_snake_move, false);
	}
}
