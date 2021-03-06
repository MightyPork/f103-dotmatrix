#ifndef MATRIXDSP_H
#define MATRIXDSP_H

#include "main.h"
#include "max2719.h"

typedef struct {
	MAX2719_Cfg drv;
	uint8_t *screen; /*!< Screen array, organized as series of [all #1 digits], [all #2 digits] ... */
	uint32_t cols; /*!< Number of drivers horizontally */
	uint32_t rows; /*!< Number of drivers vertically */
} DotMatrix_Cfg;

typedef struct {
	SPI_TypeDef *SPIx; /*!< SPI iface used by this instance */
	GPIO_TypeDef *CS_GPIOx; /*!< Chip select GPIO port */
	uint16_t CS_PINx; /*!< Chip select pin mask */
	uint32_t cols; /*!< Number of drivers horizontally */
	uint32_t rows; /*!< Number of drivers vertically */
} DotMatrix_Init;


DotMatrix_Cfg* dmtx_init(DotMatrix_Init *init);

/**
 * @brief Display the whole screen array
 * @param dmtx : driver struct
 */
void dmtx_show(DotMatrix_Cfg* dmtx);

/** Set intensity 0-16 */
void dmtx_intensity(DotMatrix_Cfg* dmtx, uint8_t intensity);

/** Display on/off */
void dmtx_blank(DotMatrix_Cfg* dmtx, bool blank);

/**
 * @brief Send a single bit
 * @param dmtx : driver struct
 * @param x : pixel X
 * @param y : pixel Y
 * @param bit : 1 or 0
 */
void dmtx_set(DotMatrix_Cfg* dmtx, int32_t x, int32_t y, bool bit);

/** Get a single bit */
bool dmtx_get(DotMatrix_Cfg* dmtx, int32_t x, int32_t y);

/** Toggle a single bit */
void dmtx_toggle(DotMatrix_Cfg* dmtx, int32_t x, int32_t y);

/** Clear the screen (not showing) */
void dmtx_clear(DotMatrix_Cfg* dmtx);

#endif // MATRIXDSP_H
