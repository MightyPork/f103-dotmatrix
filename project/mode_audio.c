#include "mode_audio.h"
#include <arm_math.h>
#include "bus/event_queue.h"
#include "utils/timebase.h"

#include "dotmatrix.h"

static bool audio_mode_active = true;

static volatile bool capture_pending = false;
//static volatile bool print_next_fft = false;

#define SAMP_BUF_LEN 256

union samp_buf_union {
	uint32_t uints[SAMP_BUF_LEN];
	float floats[SAMP_BUF_LEN];
	uint8_t as_bytes[SAMP_BUF_LEN*sizeof(uint32_t)];
};

// sample buffers (static - invalidated when sampling starts anew).
static union samp_buf_union samp_buf;

static task_pid_t capture_task_id;



// prototypes
static void audio_capture_done(void* unused);


static void boot_animation(void)
{
	dmtx_clear(dmtx);

	// Boot animation (for FFT)
	for(int i = 0; i < 16; i++) {
		dmtx_set(dmtx, i, 0, 1);
		dmtx_show(dmtx);
		delay_ms(20);
	}
}


/** Init audio mode */
void mode_audio_init(void)
{
	capture_task_id = add_periodic_task(capture_audio, NULL, 10, false);
	enable_periodic_task(capture_task_id, false);
}

/** Start audio mode */
void mode_audio_start(void)
{
	boot_animation();

	audio_mode_active = true;
	enable_periodic_task(capture_task_id, true);
}

/** Stop audio mode */
void mode_audio_stop(void)
{
	audio_mode_active = false;
	enable_periodic_task(capture_task_id, false);
}

/** Start DMA capture */
static void start_adc_dma(uint32_t *memory, uint32_t count)
{
	ADC_Cmd(ADC1, DISABLE);
	DMA_DeInit(DMA1_Channel1);
	DMA_InitTypeDef dma_cnf;
	dma_cnf.DMA_PeripheralBaseAddr = (uint32_t)&ADC1->DR;
	dma_cnf.DMA_MemoryBaseAddr = (uint32_t)memory;
	dma_cnf.DMA_DIR = DMA_DIR_PeripheralSRC;
	dma_cnf.DMA_BufferSize = count;
	dma_cnf.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	dma_cnf.DMA_MemoryInc = DMA_MemoryInc_Enable;
	dma_cnf.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Word;
	dma_cnf.DMA_MemoryDataSize = DMA_MemoryDataSize_Word;
	dma_cnf.DMA_Mode = DMA_Mode_Normal;
	dma_cnf.DMA_Priority = DMA_Priority_Low;
	dma_cnf.DMA_M2M = DMA_M2M_Disable;
	DMA_Init(DMA1_Channel1, &dma_cnf);
	DMA_ITConfig(DMA1_Channel1, DMA1_IT_TC1, ENABLE);

	ADC_Cmd(ADC1, ENABLE);
	ADC_DMACmd(ADC1, ENABLE);
	DMA_Cmd(DMA1_Channel1, ENABLE);
	TIM_Cmd(TIM3, ENABLE);
}


/** IRQ */
void DMA1_Channel1_IRQHandler(void)
{
	DMA_ClearITPendingBit(DMA1_IT_TC1);
	DMA_ClearITPendingBit(DMA1_IT_TE1);

	DMA_DeInit(DMA1_Channel1);
	TIM_Cmd(TIM3, DISABLE);
	ADC_DMACmd(ADC1, DISABLE);

	if (audio_mode_active) {
		tq_post(audio_capture_done, NULL);
	} else {
		// unset 'pending'
		capture_pending = false;
	}
}


/** Capture done callback */
static void audio_capture_done(void* unused)
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

	for (int i = 0; i < samp_count; i++) {
		samp_buf.floats[i] -= mean;
	}

//	if (print_next_fft) {
//		printf("--- Raw (adjusted) ---\n");
//		for(int i = 0; i < samp_count; i++) {
//			printf("%.2f, ", samp_buf.floats[i]);
//		}
//		printf("\n");
//	}

	for (int i = samp_count - 1; i >= 0; i--) {
		bins[i * 2 + 1] = 0;              // imaginary
		bins[i * 2] = samp_buf.floats[i]; // real
	}

	const arm_cfft_instance_f32 *S;
	S = &arm_cfft_sR_f32_len128;

	arm_cfft_f32(S, bins, 0, true); // bit reversed FFT
	arm_cmplx_mag_f32(bins, bins, bin_count); // get magnitude (extract real values)

//	if (print_next_fft) {
//		printf("--- Bins ---\n");
//		for(int i = 0; i < bin_count; i++) {
//			printf("%.2f, ", bins[i]);
//		}
//		printf("\n");
//	}

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

//	print_next_fft = false;
	capture_pending = false;
}


void capture_audio(void *unused)
{
	(void)unused;
	if (capture_pending) return;
	if (! audio_mode_active) return;

	capture_pending = true;

	start_adc_dma(samp_buf.uints, SAMP_BUF_LEN/2);
}
