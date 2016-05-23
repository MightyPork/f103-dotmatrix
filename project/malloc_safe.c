#include "com/debug.h"

#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

static void reset_when_done(void)
{
	for (uint32_t i = 0; i < 20000; i++) {
		if (com_tx_done(debug_iface)) break;
	}

	NVIC_SystemReset();
}


void *malloc_safe_do(size_t size, const char* file, uint32_t line)
{
	void *mem = malloc(size);
	if (mem == NULL) {
		// malloc failed
		error("Malloc failed in file %s on line %"PRIu32, file, line);
		reset_when_done();
	}

	return mem;
}


void *calloc_safe_do(size_t nmemb, size_t size, const char* file, uint32_t line)
{
	void *mem = calloc(nmemb, size);
	if (mem == NULL) {
		// malloc failed
		error("Malloc failed in file %s on line %"PRIu32, file, line);
		reset_when_done();
	}

	return mem;
}
