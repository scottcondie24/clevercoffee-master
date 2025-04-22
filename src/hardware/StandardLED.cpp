/**
 * @file StandardLED.cpp
 *
 * @brief An LED connected to a GPIO pin
 */

#include "StandardLED.h"
#include "GPIOPin.h"

StandardLED::StandardLED(GPIOPin& gpioInstance) :
    gpio(gpioInstance) {
}

void StandardLED::turnOn() {
    gpio.write(HIGH);
}

void StandardLED::turnOff() {
    gpio.write(LOW);
}

void StandardLED::turnOnInv() {
    gpio.write(LOW);
}

void StandardLED::turnOffInv() {
    gpio.write(HIGH);
}

void StandardLED::setColor(int red, int green, int blue) {
    // Not applicable for standard LEDs
}

void StandardLED::setBrightness(int value) {
    // Not applicable for standard LEDs
}
