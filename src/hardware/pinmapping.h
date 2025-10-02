/**
 * @file pinmapping.h
 *
 * @brief Default GPIO pin mapping
 *
 */

#pragma once

/**
 * Input Pins
 */

// Switches/Buttons
#define PIN_POWERSWITCH 36 // not used
#define PIN_BREWSWITCH  34
#define PIN_STEAMSWITCH 35
#define PIN_WATERSWITCH 39 // reusing poweswitch

// #define PIN_ROTARY_DT  4 // Rotary encoder data pin
// #define PIN_ROTARY_CLK 3 // Rotary encoder clock pin
// #define PIN_ROTARY_SW  5 // Rotary encoder switch

#define PIN_ROTARY_DT  5 // Rotary encoder data pin
#define PIN_ROTARY_CLK 4 // Rotary encoder clock pin
#define PIN_ROTARY_SW  3 // Rotary encoder switch

// Sensors
#define PIN_TEMPSENSOR      16
#define PIN_WATERTANKSENSOR 23
#define PIN_HXDAT           33 // Brew scale data pin 1
#define PIN_HXDAT2          25 // Brew scale data pin 2
#define PIN_HXCLK           23 // Brew scale clock pin 1
#define PIN_HXCLK2          32 // Brew scale clock pin 2
// Pin mapping for MAX6675 temperature sensor
#define PIN_TEMPERATURE_SO  12
#define PIN_TEMPERATURE_CS  13
#define PIN_TEMPERATURE_CLK 16 // this can be the same as PIN_TEMPSENSOR as it wouldnt otherwise be used

/**
 * Output pins
 */

// Relays
#define PIN_VALVE  17
#define PIN_PUMP   27
#define PIN_HEATER 2

// LEDs
#define PIN_STATUSLED   15 // not connected
#define PIN_BREWLED     19
#define PIN_STEAMLED    1
#define PIN_HOTWATERLED 26 // reusing statusled pin

// Periphery
#define PIN_ZC 18 // Dimmer circuit Zero Crossing

/**
 * Bidirectional Pins
 */
#define PIN_I2CSCL 22
#define PIN_I2CSDA 21
