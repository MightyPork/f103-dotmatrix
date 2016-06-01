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


static void poll_subsystems(void);

static task_pid_t capture_task_id;

static void boot_animation(void)
{
	// Boot animation (for FFT)
	for(int i = 0; i < 16; i++) {
		dmtx_set(dmtx, i, 0, 1);
		dmtx_show(dmtx);
		delay_ms(25);
	}
}


int main(void)
{
	hw_init();

	banner("*** FFT dot matrix display ***");
	banner_info("(c) Ondrej Hruska, 2016");
	boot_animation();


	//debug_iface->rx_callback = rx_char;
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


//static void rx_char(ComIface *iface)
//{
//	uint8_t ch;
//	while(com_rx(iface, &ch)) {
//		if (ch == 'p') {
//			info("PRINT_NEXT");
//			print_next_fft = true;
//		}
//	}
//}



//void dlnk_rx(SBMP_Datagram *dg)
//{
//	(void)dg;
//	//dbg("Rx dg type %d", dg->type);
//}
