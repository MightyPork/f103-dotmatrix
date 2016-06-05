#ifndef SCROLLTEXT_H
#define SCROLLTEXT_H

#include "main.h"
#include "dotmatrix.h"
#include "utils/timebase.h"

void printtext(const char *text, int x, int y);

void scrolltext(const char *text, ms_time_t step);

#endif // SCROLLTEXT_H
