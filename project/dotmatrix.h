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

#endif // MATRIXDSP_H
