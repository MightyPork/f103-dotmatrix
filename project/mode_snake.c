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
typedef struct __attribute__((packed))
{
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

typedef struct __attribute__((packed)) {
	bool pwm_bit : 1;
	bool offtask_pending : 1;
	task_pid_t on_task; // periodic
	task_pid_t off_task; // scheduled
	ms_time_t interval_ms;
	ms_time_t offtime_ms;
} snake_pwm;


static bool alive = false;
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

#define MIN_MOVE_INTERVAL 64
#define POINTS_TO_LEVEL_UP 5
#define PIECES_REMOVED_ON_LEVEL_UP 3

#define PREDEFINED_LEVELS 10
static const ms_time_t speeds_for_levels[PREDEFINED_LEVELS] = {
	320, 240, 180, 140, 95, 80, 65, 55, 40, 30
};

static ms_time_t move_interval;

static uint32_t score;
static uint32_t level_up_score; // counter
static uint32_t level;


static Cell *cell_at_xy(int x, int y);


// ---

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


// --- Snake PWM ---

/** Turn off a PWM bit (scheduled callback) */
static void task_pwm_off_cb(void *ptr)
{
	if (!snake_active) return;
	snake_pwm* pwm = ptr;

	if (!pwm->offtask_pending) return;
	pwm->offtask_pending = false;

	pwm->pwm_bit = 0;
	show_board();
}

/** Turn on a PWM bit and schedule the turn-off task (periodic callback) */
static void task_pwm_on_cb(void *ptr)
{
	if (!snake_active) return;
	snake_pwm* pwm = ptr;

	pwm->pwm_bit = 1;
	show_board();

	pwm->offtask_pending = true;
	schedule_task(task_pwm_off_cb, ptr, pwm->offtime_ms, true);
}

/** Initialize a snake PWM channel */
static void snake_pwm_init(snake_pwm *pwm)
{
	pwm->on_task = add_periodic_task(task_pwm_on_cb, pwm, pwm->interval_ms, true);
	enable_periodic_task(pwm->on_task, false);
}

/** Clear &  start a snake PWM channel */
static void snake_pwm_start(snake_pwm *pwm)
{
	pwm->pwm_bit = 1;
	pwm->offtask_pending = false;
	reset_periodic_task(pwm->on_task);
	enable_periodic_task(pwm->on_task, true);
}

/** Stop a snake PWM channel */
static void snake_pwm_stop(snake_pwm *pwm)
{
	pwm->offtask_pending = false;
	enable_periodic_task(pwm->on_task, false);
	abort_scheduled_task(pwm->off_task);
}



/** Get board cell at coord (x,y) */
static Cell *cell_at_xy(int x, int y)
{
	if (x < 0 || x >= BOARD_W || y < 0 || y >= BOARD_H) {
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

			//dbg("Food at [%d, %d]", food.x, food.y);
			break;
		}
	}

	// avoid "doubleblink" glitch
	reset_periodic_task(food_pwm.on_task);
	food_pwm.pwm_bit = 1;
	food_pwm.offtask_pending = 0;
}

/** Clear the board and prepare for a new game */
static void new_game(void)
{
	// Stop snake (make sure it's stopped)
	reset_periodic_task(task_snake_move);
	enable_periodic_task(task_snake_move, false);

	moving = false;
	alive = true;

	level = 0;
	score = 0;
	move_interval = speeds_for_levels[level];
	level_up_score = 0;
	set_periodic_task_interval(task_snake_move, move_interval);

	// randomize (we can assume it takes random time before the user switches to the Snake mode)
	srand(SysTick->VAL);

	// Erase the board
	memset(board, 0, sizeof(board));

	// Place initial snake

	tail.x = BOARD_W / 2 - 5;
	tail.y = BOARD_H / 2;

	head.x = tail.x + 4;
	head.y = tail.y;

	head_dir = EAST;

	for (int x = tail.x; x < head.x; x++) {
		board[tail.y][x].type = CELL_BODY;
		board[tail.y][x].dir = EAST;
	}

	place_food();

	snake_pwm_start(&head_pwm);
	snake_pwm_start(&food_pwm);

	show_board();
}

static void snake_died(void)
{
	dbg("R.I.P. Snake");

	moving = false;
	alive = false;
	enable_periodic_task(task_snake_move, false);

	// stop blinky animation of the snake head.
	snake_pwm_stop(&head_pwm);

	// TODO death animation
	// TODO show score
}

static void move_coord_by_dir(Coord *coord, Direction dir)
{
	switch (dir) {
		case NORTH: coord->y++; break;
		case SOUTH: coord->y--; break;
		case EAST: coord->x++; break;
		case WEST: coord->x--; break;
	}

	// Wrap-around
	if (coord->x < 0) coord->x = BOARD_W - 1;
	if (coord->y < 0) coord->y = BOARD_H - 1;
	if (coord->x >= BOARD_W) coord->x = 0;
	if (coord->y >= BOARD_H) coord->y = 0;
}

static void move_tail(void)
{
	Cell *tail_cell = cell_at(&tail);
	tail_cell->type = CELL_EMPTY;
	move_coord_by_dir(&tail, tail_cell->dir);
}

static void snake_move_cb(void *unused)
{
	(void)unused;

	bool want_new_food = false;

	Coord shadowhead = head;

	move_coord_by_dir(&shadowhead, head_dir);

	Cell *head_cell = cell_at(&head);
	Cell *future_head_cell = cell_at(&shadowhead);

	switch (future_head_cell->type) {
		case CELL_BODY:
		case CELL_WALL:
			// R.I.P.
			snake_died();
			return;

		case CELL_FOOD:
			// grow - no head move
			score++;
			want_new_food = 1;
			level_up_score++;
			break;

		case CELL_EMPTY:
			// move tail
			move_tail();
			break;
	}

	// Move head
	// (if died, already returned)
	head_cell->dir = head_dir;
	head_cell->type = CELL_BODY;

	future_head_cell->type = CELL_BODY;

	// apply movement
	head = shadowhead;

	if (want_new_food) {
		place_food();
	}

	// render
	show_board();

	// level up
	if (level_up_score == POINTS_TO_LEVEL_UP) {
		enable_periodic_task(task_snake_move, false);

		// remove some pieces
		for (int i = 0; i < PIECES_REMOVED_ON_LEVEL_UP; i++) {
			move_tail();
			until_timeout(move_interval/2) {
				tq_poll(); // take care of periodic tasks (anim)
			}
		}

		level++;
		level_up_score = 0;

		// go faster if not too fast yet
		if (level < PREDEFINED_LEVELS) {
			move_interval = speeds_for_levels[level];
		}

		set_periodic_task_interval(task_snake_move, move_interval);
		enable_periodic_task(task_snake_move, true);
	}
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

/** Change dir (safely) */
static void change_direction(Direction dir)
{
	// This is a compensation for shitty gamepads
	// with accidental arrow hits

	Coord shadowhead = head;
	move_coord_by_dir(&shadowhead, dir);

	Cell *target = cell_at(&shadowhead);

	if (target->type == CELL_BODY) return;

	head_dir = dir;
}

/** User button */
void mode_snake_btn(char key)
{
	switch (key) {
		case 'U': change_direction(NORTH); break;
		case 'D': change_direction(SOUTH); break;
		case 'L': change_direction(WEST); break;
		case 'R': change_direction(EAST); break;

		case 'J':
			if (alive) break;
			// dead - reset by pressing 'start'

		case 'K': // clear
			// TODO reset animation
			new_game();
			return;
	}

	if (!moving && alive && (key == 'U' || key == 'D' || key == 'L' || key == 'R' || key == 'J')) {
		// start moving
		moving = true;
		reset_periodic_task(task_snake_move);
		enable_periodic_task(task_snake_move, true);
	}

	// running + start -> 'pause'
	if (alive && moving && key == 'J') {
		moving = false;
		enable_periodic_task(task_snake_move, false);
	}
}
