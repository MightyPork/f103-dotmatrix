#ifndef MODE_AUDIO_H
#define MODE_AUDIO_H

#include "main.h"

void capture_audio(void *unused);

void mode_audio_init(void);
void mode_audio_start(void);
void mode_audio_stop(void);

#endif // MODE_AUDIO_H
