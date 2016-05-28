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

static volatile bool capture_pending = false;
static volatile bool print_next_fft = false;

static float virt_zero_value = 2045.0f;

static void poll_subsystems(void);

static DotMatrix_Cfg *dmtx;

#define SAMP_BUF_LEN 256

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

	// Convert to floats
	for (int i = 0; i < samp_count; i++) {
		samp_buf.floats[i] = (float)samp_buf.uints[i];
	}

	// normalize
	float mean;
	arm_mean_f32(samp_buf.floats, samp_count, &mean);
	virt_zero_value = mean;

	for (int i = 0; i < samp_count; i++) {
		samp_buf.floats[i] -= virt_zero_value;
	}


	if (print_next_fft) {
		printf("--- Raw (adjusted) ---\n");
		for(int i = 0; i < samp_count; i++) {
			printf("%.2f, ", samp_buf.floats[i]);
		}
		printf("\n");
	}

	for (int i = samp_count - 1; i >= 0; i--) {
		bins[i * 2 + 1] = 0;              // imaginary
		bins[i * 2] = samp_buf.floats[i]; // real
	}

	const arm_cfft_instance_f32 *S;
	S = &arm_cfft_sR_f32_len128;

	arm_cfft_f32(S, bins, 0, true); // bit reversed FFT
	arm_cmplx_mag_f32(bins, bins, bin_count); // get magnitude (extract real values)

	if (print_next_fft) {
		printf("--- Bins ---\n");
		for(int i = 0; i < bin_count; i++) {
			printf("%.2f, ", bins[i]);
		}
		printf("\n");
	}

	// normalize
	dmtx_clear(dmtx);
	float factor = (1.0f/bin_count)*0.2f;
	for(int i = 0; i < bin_count-1; i+=2) {
		bins[i] *= factor;
		bins[i+1] *= factor;

		//float avg = i==0 ? bins[1] : (bins[i] + bins[i+1])/2;
		float avg = (bins[i] + bins[i+1])/2;

		for(int j = 0; j < 1+floorf(avg); j++) {
			//dmtx_toggle(dmtx, i/2, j);
			dmtx_toggle(dmtx, i/2, j);
			//dmtx_toggle(dmtx, j, 15-i/2);
			//dmtx_toggle(dmtx, 15- i/2, 15-j);
		}
	}

	dmtx_show(dmtx);

	print_next_fft = false;
	capture_pending = false;
}


static void capture_audio(void *unused)
{
	(void)unused;
	if (capture_pending) return;

	capture_pending = true;

	start_adc_dma(samp_buf.uints, SAMP_BUF_LEN/2);
}


static void rx_char(ComIface *iface)
{
	uint8_t ch;
	while(com_rx(iface, &ch)) {
		if (ch == 'p') {
			info("PRINT_NEXT");
			print_next_fft = true;
		}
	}
}


static task_pid_t capture_task_id;


int main(void)
{
	hw_init();

	banner("*** FFT dot matrix display ***");
	banner_info("(c) Ondrej Hruska, 2016");

	debug_iface->rx_callback = rx_char;

	DotMatrix_Init dmtx_cfg;
	dmtx_cfg.CS_GPIOx = GPIOA;
	dmtx_cfg.CS_PINx = GPIO_Pin_4;
	dmtx_cfg.SPIx = SPI1;
	dmtx_cfg.cols = 2;
	dmtx_cfg.rows = 2;

	dmtx = dmtx_init(&dmtx_cfg);

	dmtx_intensity(dmtx, 7);

	for(int i = 0; i < 16; i++) {
		dmtx_set(dmtx, i, 0, 1);
		dmtx_show(dmtx);
		delay_ms(25);
	}

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
