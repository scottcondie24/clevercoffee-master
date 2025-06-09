#pragma once

#include "GPIOPin.h"
#include "PCF8575Pin.h"
#include <PCF8575.h>

GPIOPin* createGPIOPin(int pinNumber, GPIOPin::Type type);
void initGPIOFactory();  // To initialize shared PCF8575 if needed