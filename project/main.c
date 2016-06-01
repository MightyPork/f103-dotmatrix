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

#include "arm_math.h"
#include "mode_audio.h"

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
static void boot_animation(void);
static void switch_mode(void *unused); // circle b/w modes
static void activate_mode(void); // activate currently selected mode

static task_pid_t capture_task_id;

static GameMode app_mode;


static void switch_mode(void *unused)
{
	(void)unused;

	if (++app_mode == MODE_END) {
		app_mode = 0;
	}

	activate_mode();
}


#define SCROLL_STEP 20
static void activate_mode(void)
{
	// --- Audio FFT mode ---

	if (app_mode == MODE_AUDIO) {
		info("MODE: Audio");

		scrolltext("Audio FFT", SCROLL_STEP);

		audio_mode_active = true;
		enable_periodic_task(capture_task_id, true);
	} else {
		audio_mode_active = false;
		enable_periodic_task(capture_task_id, false);
	}

	// --- Game Of Life ---

	if (app_mode == MODE_LIFE) {
		info("MODE: Life");

		scrolltext("Game of Life", SCROLL_STEP);

		dmtx_clear(dmtx);
		dmtx_set(dmtx, 5, 5, 1);
		dmtx_show(dmtx);
		//
	} else {
		//
	}

	// --- Snake Minigame ---

	if (app_mode == MODE_SNAKE) {
		info("MODE: Snake");

		scrolltext("Snake", SCROLL_STEP);

		dmtx_clear(dmtx);
		dmtx_set(dmtx, 13, 13, 1);
		dmtx_show(dmtx);
		//
	} else {
		//
	}
}


int main(void)
{
	hw_init();

	banner("*** FFT dot matrix display ***");
	banner_info("(c) Ondrej Hruska, 2016");
	boot_animation();

	gamepad_iface->rx_callback = gamepad_rx;

	capture_task_id = add_periodic_task(capture_audio, NULL, 10, false);

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


static void boot_animation(void)
{
	// Boot animation (for FFT)
	for(int i = 0; i < 16; i++) {
		dmtx_set(dmtx, i, 0, 1);
		dmtx_show(dmtx);
		delay_ms(25);
	}
}


static void gamepad_rx(ComIface *iface)
{
	uint8_t ch;
	while(com_rx(iface, &ch)) {
//		com_tx(debug_iface, ch);
//		com_tx(debug_iface, '\n');

		/* SELECT */
		if (ch == 'I') {
			tq_post(switch_mode, NULL);
		}
	}
}


//void dlnk_rx(SBMP_Datagram *dg)
//{
//	(void)dg;
//	//dbg("Rx dg type %d", dg->type);
//}
