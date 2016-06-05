#include "mode_snake.h"
#include "com/debug.h"
#include "dotmatrix.h"

#define BOARD_W SCREEN_W
#define BOARD_H SCREEN_H

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
static const Cell EMPTY_TILE = {CELL_EMPTY, 0};

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
static bool wall_pwm_on = 1;
static bool body_pwm_on = 1;
static bool head_pwm_on = 1;
static bool food_pwm_on = 1;

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

	for (int x = tail.x; x < head.x; x++) {
		board[tail.y][x].type = CELL_BODY;
	}

	place_food();
}

static void show_board(void)
{
	dmtx_clear(dmtx);

	for (int x = 0; x < BOARD_W; x++) {
		for (int y = 0; y < BOARD_H; y++) {
			Cell *cell = cell_at_xy(x, y);

			bool set = 0;

			switch (cell->type) {
				case CELL_EMPTY: set = 0; break;
				case CELL_BODY: set = body_pwm_on; break;
				case CELL_FOOD:
					dbg("Food found @ [%d, %d]", x, y);
					set = food_pwm_on;
					break;

				case CELL_WALL: set = wall_pwm_on; break;
			}

			if (x == head.x && y == head.y) set = head_pwm_on;

			dmtx_set(dmtx, x, y, set);
		}
	}

	dmtx_show(dmtx);
}


void mode_snake_init(void)
{
	//
}


void mode_snake_start(void)
{
	new_game();

	show_board();
}

void mode_snake_stop(void)
{
	//
}

void mode_snake_btn(char key)
{
	//
}
