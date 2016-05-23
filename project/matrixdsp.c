#include <main.h>
#include <matrixdsp.h>

#include "com/debug.h"

#include "utils/timebase.h"

static void send_byte(uint8_t b)
{
	MDSP_SPIx->DR = b;
	while (!(MDSP_SPIx->SR & SPI_SR_TXE));
}

static void set_nss(bool nss)
{
	if (nss) {
		MDSP_NSS_GPIO->BSRR = MDSP_NSS_PIN;
	} else {
		MDSP_NSS_GPIO->BRR = MDSP_NSS_PIN;
	}
}

static void send_word(MDSP_Command cmd, uint8_t data)
{
	send_byte(cmd);
	send_byte(data);
}

/**
 * @brief Send a command to n-th chained driver
 * @param idx Driver index (0, 1, 2 ...)
 * @param cmd command to send
 * @param data command argument
 */
void mdsp_send_command(uint8_t idx, MDSP_Command cmd, uint8_t data)
{
	dbg("Set %d: cmd 0x%02x, data 0x%02x", idx, cmd, data);

	set_nss(false);
	while (MDSP_SPIx->SR & SPI_SR_BSY);

	for (uint8_t i = 0; i < MDSP_CHAIN_COUNT - idx - 1; i++) {
		send_word(CMD_NOOP, 0);
	}

	send_word(cmd, data);

	for (uint8_t i = 0; i < idx; i++) {
		send_word(CMD_NOOP, 0);
	}

	while (MDSP_SPIx->SR & SPI_SR_BSY);
	set_nss(false);
	set_nss(true);
}


void mdsp_send_command_all(MDSP_Command cmd, uint8_t data)
{
	dbg("Set cmd 0x%02x, data 0x%02x ALL", cmd, data);

	set_nss(false);
	while (MDSP_SPIx->SR & SPI_SR_BSY);

	for (uint8_t i = 0; i < MDSP_CHAIN_COUNT; i++) {
		send_word(cmd, data);
	}

	while (MDSP_SPIx->SR & SPI_SR_BSY);

	set_nss(false);
	set_nss(true);
}

void mdsp_set(uint8_t column, uint8_t bits)
{
	if (column >= MDSP_COLUMN_COUNT) return;

	mdsp_send_command(column >> 3, CMD_DIGIT0 + (column & 0x7), bits);
}

void mdsp_clear(void)
{
	for (uint8_t i = 0; i < 8; i++) {
		mdsp_send_command_all(CMD_DIGIT0+i, 0);
	}
}
