/**
 * @file pinmapping.h
 *
 * @brief Default GPIO pin mapping
 *
 */

#pragma once

#ifdef BOARD_ESP32_S3
#define TARGET_BOARD_ESP32_S3
#elif defined(BOARD_ESP32_CLASSIC)
#define TARGET_BOARD_ESP32_CLASSIC
#else
// Default to ESP32 Classic for backward compatibility
#define TARGET_BOARD_ESP32_CLASSIC
#endif

#ifdef TARGET_BOARD_ESP32_S3
// ESP32-S3 Pin Mapping (16MB Flash, 8MB PSRAM)
#define PIN_POWERSWITCH 39
#define PIN_BREWSWITCH  40
#define PIN_STEAMSWITCH 41
#define PIN_WATERSWITCH 42

#define PIN_ROTARY_DT  21
#define PIN_ROTARY_CLK 47
#define PIN_ROTARY_SW  0 // shared with boot mode strapping pin - high for code, low for download

#define PIN_TEMPSENSOR      7
#define PIN_TEMPSENSOR2     15
#define PIN_FLOWSENSOR      16
#define PIN_WATERTANKSENSOR 48 // shared with onboard RGB LED
#define PIN_HXDAT           4
#define PIN_HXDAT2          5
#define PIN_HXCLK           6

#define PIN_VALVE   12
#define PIN_PUMP    13
#define PIN_PUMP2   10
#define PIN_HEATER  14
#define PIN_HEATER2 11

#define PIN_STATUSLED 18
#define PIN_BREWLED   17

#define PIN_ZC 38

#define PIN_I2CSCL 9
#define PIN_I2CSDA 8

#define PIN_SERIAL1TX 1
#define PIN_SERIAL1RX 2

// Pin mapping for SPI
#define PIN_SPI_MISO 44 // RX
#define PIN_SPI_MOSI 43 // TX
#define PIN_SPI_SCK  10 // shared with pump2

#else
// ESP32 Classic Pin Mapping
#define PIN_POWERSWITCH 39
#define PIN_BREWSWITCH  3
#define PIN_STEAMSWITCH 35
#define PIN_WATERSWITCH 36

#define PIN_ROTARY_DT  4
#define PIN_ROTARY_CLK 3
#define PIN_ROTARY_SW  5

#define PIN_TEMPSENSOR      16
#define PIN_WATERTANKSENSOR 23
#define PIN_HXDAT           32
#define PIN_HXDAT2          25
#define PIN_HXCLK           33

#define PIN_VALVE  17
#define PIN_PUMP   27
#define PIN_HEATER 2

#define PIN_STATUSLED 26
#define PIN_BREWLED   19
#define PIN_STEAMLED  1

#define PIN_ZC 18

#define PIN_I2CSCL 22
#define PIN_I2CSDA 21

#endif
