#include "display_ui.h"
#include <Arduino.h>

void drawProgress(int x, int y, int w, int h, int percent) {
  display.drawFrame(x, y, w, h);
  int fillWidth = map(percent, 0, 100, 0, w - 2);
  if (fillWidth > 0) display.drawBox(x + 1, y + 1, fillWidth, h - 2);
}
