#include "scrolltext.h"

#include "font.h"
#include "com/debug.h"

static void printtext(const char *text, int x, int y)
{
	dmtx_clear(dmtx);
	dmtx_show(dmtx);

	int totalX = 0;

	for (int textX = 0; textX < (int)strlen(text); textX++) {
		uint8_t ch = (uint8_t)text[textX];
		if (ch < FONT_MIN) ch = '?';
		if (ch > FONT_MAX) ch = '?';
		ch -= ' '; // normalize for font table

		if (ch == 0) { // space
			totalX += 4;
			continue;
		}

		// one letter
		uint8_t blanks = 0;

		// skip empty space on right
		for (int charX = FONT_WIDTH-1; charX >= 0; charX--) {
			uint8_t col = font[ch][charX];
			if (col == 0x00) {
				blanks++;
			} else {
				break;
			}
		}

		for (int charX = 0; charX < FONT_WIDTH - blanks; charX++) {
			uint8_t col = font[ch][charX];
			for (int charY = 0; charY < 8; charY++) {
				dmtx_set(dmtx, x + totalX, y + 8 - charY, (col >> charY) & 1);
			}
			totalX++;
		}

		totalX++; // gap
	}
	dmtx_show(dmtx);
}

void scrolltext(const char *text, ms_time_t step)
{
	(void)step;

	for (int i = 0; i < (int)strlen(text)*FONT_WIDTH + 15; i++) {
		if (i > 0) delay_ms(step);

		printtext(text, 15-i, 4);
	}
}
