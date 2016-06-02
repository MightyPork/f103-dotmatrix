#include "main.h"
#include "hw_init.h"

#include "com/debug.h"
#include "com/com_fileio.h"
#include "com/com_iface.h"
#include "bus/event_queue.h"
#include "bus/event_handler.h"
#include "utils/timebase.h"

#include "colorled.h"
#include "display.h"
#include <math.h>
//#include <sbmp.h>

#include "dotmatrix.h"

#include <arm_math.h>

#include "mode_audio.h"
#include "mode_snake.h"
#include "mode_life.h"

#include "scrolltext.h"

/** Functional mode */
typedef enum {
	MODE_AUDIO,
	MODE_LIFE,
	MODE_SNAKE,
	MODE_END,
} GameMode;

static void poll_subsystems(void);
static void gamepad_rx(ComIface *iface);
static void switch_mode(void *unused); // circle b/w modes
static void activate_mode(void); // activate currently selected mode

static GameMode app_mode;


static void switch_mode(void *unused)
{
	(void)unused;

	if (++app_mode == MODE_END) {
		app_mode = 0;
	}

	activate_mode();

	// discard buffer
	uint8_t x;
	while(com_rx(gamepad_iface, &x));
}


#define SCROLL_STEP 22
static void activate_mode(void)
{
	// --- Audio FFT mode ---

	if (app_mode == MODE_AUDIO) {
		info("MODE: Audio");
		scrolltext("AUDIO", SCROLL_STEP);

		mode_audio_start();
	} else {
		mode_audio_stop();
	}

	// --- Game Of Life ---

	if (app_mode == MODE_LIFE) {
		info("MODE: Life");
		scrolltext("CONWAY", SCROLL_STEP);

		mode_life_start();
	} else {
		mode_life_stop();
	}

	// --- Snake Minigame ---

	if (app_mode == MODE_SNAKE) {
		info("MODE: Snake");
		scrolltext("SNAKE", SCROLL_STEP);

		mode_snake_start();
	} else {
		mode_snake_stop();
	}
}


int main(void)
{
	hw_init();

	banner("*** FFT dot matrix display ***");
	banner_info("(c) Ondrej Hruska, 2016");

	gamepad_iface->rx_callback = gamepad_rx;

	mode_audio_init();
	mode_life_init();
	mode_snake_init();

	mode_audio_start();

	ms_time_t last;
	while (1) {
		if (ms_loop_elapsed(&last, 500)) {
			GPIOC->ODR ^= 1 << 13;
		}

		poll_subsystems();
	}
}


static void poll_subsystems(void)
{
	// poll serial buffers (runs callback)
	com_poll(debug_iface);
	//com_poll(data_iface);
	com_poll(gamepad_iface);

	// run queued tasks
	tq_poll();

	// handle queued events
	Event evt;
	until_timeout(2) { // take 2 ms max
		if (eq_take(&evt)) {
			run_event_handler(&evt);
		} else {
			break;
		}
	}
}



static void gamepad_rx(ComIface *iface)
{
	char ch;
	while(com_rx(iface, (uint8_t*)&ch)) {
		switch (ch) {
			case 'I': // Select pressed
				tq_post(switch_mode, NULL);
				break;

			case 'i': // Select released
				break;

			default:
				switch (app_mode) {
					case MODE_AUDIO:
						// discard
						break;

					case MODE_LIFE:
						mode_life_btn(ch);
						break;

					case MODE_SNAKE:
						mode_snake_btn(ch);
						break;
				}
		}
	}
}


//void dlnk_rx(SBMP_Datagram *dg)
//{
//	(void)dg;
//	//dbg("Rx dg type %d", dg->type);
//}
