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
#include <sbmp.h>

#include "matrixdsp.h"


static void poll_subsystems(void)
{
	// poll serial buffers (runs callback)
	com_poll(debug_iface);
	com_poll(data_iface);

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



int main(void)
{
	hw_init();
//	display_init();

	banner("*** LED MATRIX DEMO ***");
	banner_info("(c) Ondrej Hruska, 2016");
	banner_info("Katedra mereni K338, CVUT FEL");

	ms_time_t last;

	mdsp_send_command_all(CMD_DECODE_MODE, 0x00);
	mdsp_send_command_all(CMD_SCAN_LIMIT, 0x07);
	mdsp_send_command_all(CMD_SHUTDOWN, 0x01);
	mdsp_send_command_all(CMD_DISPLAY_TEST, 0x00);
	mdsp_send_command_all(CMD_INTENSITY, 0x05);

	mdsp_clear();

	// ---

	const uint16_t inva0[] = {
		0b00100000100,
		0b00010001000,
		0b00111111100,
		0b01101110110,
		0b11111111111,
		0b10111111101,
		0b10100000101,
		0b00011011000,
	};

	const uint16_t inva1[] = {
		0b00100000100,
		0b10010001001,
		0b10111111101,
		0b11101110111,
		0b11111111111,
		0b01111111110,
		0b00100000100,
		0b01000000010,
	};

	while (1) {
		if (ms_loop_elapsed(&last, 500)) {
			GPIOC->ODR ^= 1 << 13;
		}

		poll_subsystems();

		for (int i = 0; i < 8; i++) {
			uint32_t x = __RBIT(inva0[7-i]);
			mdsp_set(i, x >> 21);
			mdsp_set(8+i, (x >> 29) & 0b111);
		}
		delay_ms(500);

		for (int i = 0; i < 8; i++) {
			uint32_t x = __RBIT(inva1[7-i]);
			mdsp_set(i, x >> 21);
			mdsp_set(8+i, (x >> 29) & 0b111);
		}
		delay_ms(500);
	}
}


void dlnk_rx(SBMP_Datagram *dg)
{
	dbg("Rx dg type %d", dg->type);
}
