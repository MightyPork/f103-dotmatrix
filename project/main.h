#ifndef MAIN_H
#define MAIN_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include <stm32f10x.h>

#define MIN(a,b) ((a)>(b)?(b):(a))
#define MAX(a,b) ((a)<(b)?(b):(a))
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)


void audio_capture_done(void* unused);

#endif // MAIN_H
