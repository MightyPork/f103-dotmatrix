#include "max2719.h"
#include "dotmatrix.h"
#include "malloc_safe.h"
#include "com/debug.h"

DotMatrix_Cfg* dmtx_init(DotMatrix_Init *init)
{
	DotMatrix_Cfg *dmtx = calloc_s(1, sizeof(DotMatrix_Cfg));

	dmtx->drv.SPIx = init->SPIx;
	dmtx->drv.CS_GPIOx = init->CS_GPIOx;
	dmtx->drv.CS_PINx = init->CS_PINx;
	dmtx->drv.chain_len = init->cols * init->rows;
	dmtx->cols = init->cols;
	dmtx->rows = init->rows;

	dmtx->screen = calloc_s(init->cols * init->rows * 8, 1); // 8 bytes per driver

	max2719_cmd_all(&dmtx->drv, MAX2719_CMD_DECODE_MODE, 0x00); // no decode
	max2719_cmd_all(&dmtx->drv, MAX2719_CMD_SCAN_LIMIT, 0x07); // scan all 8
	max2719_cmd_all(&dmtx->drv, MAX2719_CMD_SHUTDOWN, 0x01); // not shutdown
	max2719_cmd_all(&dmtx->drv, MAX2719_CMD_DISPLAY_TEST, 0x00); // not test
	max2719_cmd_all(&dmtx->drv, MAX2719_CMD_INTENSITY, 0x07); // half intensity

	// clear
	for (uint8_t i = 0; i < 8; i++) {
		max2719_cmd_all(&dmtx->drv, MAX2719_CMD_DIGIT0+i, 0);
	}

	return dmtx;
}

void dmtx_show(DotMatrix_Cfg* dmtx)
{
	for (uint8_t i = 0; i < 8; i++) {
		// show each digit's array in turn
		max2719_cmd_all_data(&dmtx->drv, MAX2719_CMD_DIGIT0+i, dmtx->screen + (i * dmtx->drv.chain_len));
	}
}

void dmtx_clear(DotMatrix_Cfg* dmtx)
{
	memset(dmtx->screen, 0, dmtx->drv.chain_len*8);
}

void dmtx_intensity(DotMatrix_Cfg* dmtx, uint8_t intensity)
{
	max2719_cmd_all(&dmtx->drv, MAX2719_CMD_INTENSITY, intensity & 0x0F);
}

void dmtx_blank(DotMatrix_Cfg* dmtx, bool blank)
{
	max2719_cmd_all(&dmtx->drv, MAX2719_CMD_SHUTDOWN, blank & 0x01);
}

void dmtx_set(DotMatrix_Cfg* dmtx, int32_t x, int32_t y, bool bit)
{
	if (x < 0 || y < 0) return;
	if ((uint32_t)x >= dmtx->cols*8 || (uint32_t)y >= dmtx->rows*8) return;

	uint32_t cell_x = (uint32_t)x >> 3;
	uint8_t xd = x & 7;

	// resolve cell
	uint32_t digit = y & 7;
	cell_x += ((uint32_t)y >> 3) * dmtx->cols;

	uint32_t cell_idx = (digit * dmtx->drv.chain_len) + cell_x;

	uint8_t *cell = &dmtx->screen[cell_idx];

	if (bit) {
		*cell |= bit << xd;
	} else {
		*cell &= ~(bit << xd);
	}
}
