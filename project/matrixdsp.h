#ifndef MATRIXDSP_H
#define MATRIXDSP_H

#include <main.h>

#define MDSP_SPIx SPI1
#define MDSP_NSS_GPIO GPIOA
#define MDSP_NSS_PIN GPIO_Pin_4;

#define MDSP_CHAIN_COUNT 4
#define MDSP_COLUMN_COUNT (MDSP_CHAIN_COUNT*8)

typedef enum {
	CMD_NOOP = 0x00,

	CMD_DIGIT0 = 0x01,
	CMD_DIGIT1 = 0x02,
	CMD_DIGIT2 = 0x03,
	CMD_DIGIT3 = 0x04,
	CMD_DIGIT4 = 0x05,
	CMD_DIGIT5 = 0x06,
	CMD_DIGIT6 = 0x07,
	CMD_DIGIT7 = 0x08,

	CMD_DECODE_MODE = 0x09,
	CMD_INTENSITY = 0x0A,
	CMD_SCAN_LIMIT = 0x0B,
	CMD_SHUTDOWN = 0x0C,
	CMD_DISPLAY_TEST = 0x0F,
} MDSP_Command;

void mdsp_set(uint8_t column, uint8_t bits);

void mdsp_clear(void);

/**
 * @brief Send a command to n-th chained driver
 * @param idx Driver index (0, 1, 2 ...)
 * @param cmd command to send
 * @param data command argument
 */
void mdsp_send_command(uint8_t idx, MDSP_Command cmd, uint8_t data);


void mdsp_send_command_all(MDSP_Command cmd, uint8_t data);

#endif // MATRIXDSP_H
