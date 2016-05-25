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

#include "arm_math.h"

static void poll_subsystems(void);

static DotMatrix_Cfg *dmtx;

#define SAMP_BUF_LEN 128

union samp_buf_union {
	uint32_t uints[SAMP_BUF_LEN];
	float floats[SAMP_BUF_LEN];
	uint8_t as_bytes[SAMP_BUF_LEN*sizeof(uint32_t)];
};

// sample buffers (static - invalidated when sampling starts anew).
static union samp_buf_union samp_buf;

void audio_capture_done(void* unused)
{
	(void)unused;

	const int samp_count = SAMP_BUF_LEN/2;
	const int bin_count = SAMP_BUF_LEN/4;

	float *bins = samp_buf.floats;

	for (int i = 0; i < samp_count; i++) {
		samp_buf.floats[i] = samp_buf.uints[i] - 2045.0f;
	}

	for (int i = samp_count - 1; i >= 0; i--) {
		bins[i * 2 + 1] = 0;              // imaginary
		bins[i * 2] = samp_buf.floats[i]; // real
	}

	const arm_cfft_instance_f32 *S;
	S = &arm_cfft_sR_f32_len64;

	arm_cfft_f32(S, bins, 0, true); // bit reversed FFT
	arm_cmplx_mag_f32(bins, bins, bin_count); // get magnitude (extract real values)

	// normalize
	dmtx_clear(dmtx);
	float factor = (1.0f/bin_count)*0.1f;
	for(int i = 0; i < bin_count-1; i+=2) {
		bins[i] *= factor;
		bins[i+1] *= factor;

		float avg = i==0 ? bins[1] : (bins[i] + bins[i+1])/2;

		for(int j = 0; j < ceilf(avg); j++) {
			dmtx_set(dmtx, i/2, j, true);
		}
	}

	dmtx_show(dmtx);
}


static void capture_audio(void *unused)
{
	(void)unused;

	start_adc_dma(samp_buf.uints, SAMP_BUF_LEN/2);
}


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
	dmtx_cfg.cols = 2;
	dmtx_cfg.rows = 2;

	dmtx = dmtx_init(&dmtx_cfg);

	dmtx_intensity(dmtx, 7);

	add_periodic_task(capture_audio, NULL, 10, false);

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
