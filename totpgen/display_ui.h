#ifndef DISPLAY_UI_H
#define DISPLAY_UI_H

#include <U8g2lib.h>

void drawProgress(int x, int y, int w, int h, int percent);

extern U8G2_SSD1306_128X64_NONAME_F_HW_I2C display;

#endif
