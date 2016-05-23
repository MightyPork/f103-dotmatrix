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

//#include "matrixdsp.h"

#include "max2719.h"
#include "dotmatrix.h"

static void poll_subsystems(void);

static DotMatrix_Cfg *dmtx;


int main(void)
{
	hw_init();

	banner("*** LED MATRIX DEMO ***");
	banner_info("(c) Ondrej Hruska, 2016");
	banner_info("Katedra mereni K338, CVUT FEL");

	DotMatrix_Init dmtx_cfg;
	dmtx_cfg.CS_GPIOx = GPIOA;
	dmtx_cfg.CS_PINx = GPIO_Pin_4;
	dmtx_cfg.SPIx = SPI1;
	dmtx_cfg.cols = 4;
	dmtx_cfg.rows = 1;

	dmtx = dmtx_init(&dmtx_cfg);

	dmtx->screen[0] = 0xF0;
	dmtx->screen[4] = 0x0F;

	dmtx_show(dmtx);

	ms_time_t last;
	while (1) {
		if (ms_loop_elapsed(&last, 500)) {
			GPIOC->ODR ^= 1 << 13;
		}
		poll_subsystems();

		dmtx_blank(dmtx, false);
		delay_ms(250);
		dmtx_blank(dmtx, true);
		delay_ms(250);
	}
}


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


void dlnk_rx(SBMP_Datagram *dg)
{
	dbg("Rx dg type %d", dg->type);
}
