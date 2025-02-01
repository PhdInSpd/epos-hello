#pragma once
#include <SDL_gamecontroller.h>

int demoGameController();

void closeGamecontroller();
bool initializeGamecontroller();
bool handlelGamecontrollerEvents();
bool getAnalogInputs(float channels[]);

void showExtrauttons();
void letsRumble();
