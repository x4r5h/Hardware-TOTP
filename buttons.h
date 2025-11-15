#ifndef BUTTONS_H
#define BUTTONS_H

#include <Arduino.h>

void initButtons();
bool isNextPressed();
bool isPastePressed();

extern int currentAccount;
extern int lastNextState;
extern unsigned long lastDebounce;
extern int lastPasteState;
extern bool pasteTriggered;

#endif
