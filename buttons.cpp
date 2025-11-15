#include "buttons.h"
#include "config.h"

void initButtons() {
  pinMode(BTN_NEXT, INPUT_PULLUP);
  pinMode(BTN_PASTE, INPUT_PULLUP);
}

bool isNextPressed() {
  int reading = digitalRead(BTN_NEXT);
  if (reading != lastNextState) lastDebounce = millis();
  if ((millis() - lastDebounce) > DEBOUNCE_DELAY) {
    static int stable = HIGH;
    if (reading != stable) {
      stable = reading;
      if (stable == LOW) {
        lastNextState = reading;
        return true;
      }
    }
  }
  lastNextState = reading;
  return false;
}

bool isPastePressed() {
  int reading = digitalRead(BTN_PASTE);
  if (lastPasteState == HIGH && reading == LOW && !pasteTriggered) {
    pasteTriggered = true;
    lastPasteState = reading;
    return true;
  }
  if (lastPasteState == LOW && reading == HIGH) pasteTriggered = false;
  lastPasteState = reading;
  return false;
}
