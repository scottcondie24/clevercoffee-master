/**
 * @file pressureSensorAnalog.h
 *
 * @brief ADC0 for analog pressure sensor, used an xdb401 0-1.2Mpa 0-5V through a voltage divider
 */

#pragma once

//need to check which ones are needed, should need Logger for LOGF
#include "Logger.h"
#include "GPIOPin.h"
#include "userConfig.h"
#include <Arduino.h>

//sensor is supplied by 5V and measured through a voltage divider of 24k/12k
//calibrate adc read range and set mx + b in the range offset (b) and supplyvoltage (m)
#define RANGEOFFSET 0.145 // in V
#define ADC_RESOLUTION 4096
#define SUPPLYVOLTAGE 3.265 //in V

#define CONVERSION_FACTOR 216 // in mV/Bar      linear region - (2645-360)/10.58
#define READOFFSET 0.360 // in V

//360mV to 3120mV range


double analog_pressure = 0.0;               // pressure reading [bar, psi, kPa, etc.]
const unsigned long intervalPressureDebug = 1000; //ms
unsigned long previousMillisPressureDebug = 0;


float measurePressure() {

    double sensorValue = analogRead(PIN_PRESSURESENSOR)*(SUPPLYVOLTAGE/ADC_RESOLUTION) + RANGEOFFSET; //in V
    analog_pressure =  (1000.0/CONVERSION_FACTOR) * (sensorValue - READOFFSET); //in Bar, need 1000.0 so it evaluates as a double (4.62 vs 4.0)
    if(analog_pressure < 0)
        analog_pressure = 0;

    IFLOG(TRACE) {
        unsigned long currentMillisPressureDebug = millis();

        if (currentMillisPressureDebug - previousMillisPressureDebug >= intervalPressureDebug) {
            LOGF(TRACE, "Raw: %f, Pressure: %f", sensorValue, analog_pressure);
            previousMillisPressureDebug = currentMillisPressureDebug;
        }
    }

    //return (float)analog_pressure;
    return (float)sensorValue*1000;        //debugging, used to validate measured voltage accuracy before conversion to pressure
}
