#include "GPIOFactory.h"

constexpr int PCF8575_BASE_PIN = 40;
PCF8575 ioExpander(0x20);  // SDA, SCL

void initGPIOFactory() {
    ioExpander.begin();
}

GPIOPin* createGPIOPin(int pinNumber, GPIOPin::Type type) {
    if (pinNumber >= PCF8575_BASE_PIN) {
        int expanderPin = pinNumber - PCF8575_BASE_PIN;
        return new PCF8575Pin(ioExpander, expanderPin, type);
    } else {
        return new GPIOPin(pinNumber, type);
    }
}