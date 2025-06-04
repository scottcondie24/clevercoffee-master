/**
 * @file pressureSensorAds1115.h
 *
 * @brief analog pressure sensor connected to 16bit analog to digital converter using alokonosst/ADS1115 library
 */

#pragma once

#include "Logger.h"
#include <ADS1115.h>
#include <Arduino.h>

using namespace ADS1115;
ADS1115_ADC adc(I2CAddress::Gnd);

const unsigned long intervalPressureAdsDebug = 1000;
unsigned long previousMillisPressureAdsDebug = 0;
bool errorDetected = false;

double analog_pressure = 0.0;     // pressure reading in bar
#define CONVERSION_FACTOR_P 3     // sensor is 12 Bar over 4V
#define RANGEOFFSET_P       0.500 // minimum reading is 500mV

void pressureInit() {
    adc.setMux(Mux::P0_GND);
    adc.setPga(Pga::FSR_6_144V);
    adc.setDataRate(DataRate::SPS_128);

    Status status = adc.uploadConfig();

    if (status != Status::Ok) {
        // handle the error
    }

    status = adc.startContinuousConversion(0);

    if (status != Status::Ok) {
        // handle the error
    }
}

float measurePressureAds() {
    float voltage;

    Status status = adc.readConversionVoltage(voltage);

    if (status != Status::Ok || voltage < 0.1f) {
        if (!errorDetected) {
            LOG(ERROR, "ADS1115: bad read detected, recovering...");
            errorDetected = true;
        }

        pressureInit();

        return (float)analog_pressure;
    }
    else {
        errorDetected = false;
        analog_pressure = (voltage - RANGEOFFSET_P) * CONVERSION_FACTOR_P;

        IFLOG(TRACE) {
            unsigned long currentMillisPressureAdsDebug = millis();

            if (currentMillisPressureAdsDebug - previousMillisPressureAdsDebug >= intervalPressureAdsDebug) {
                LOGF(TRACE, "Voltage: %f, Pressure: %f", voltage, analog_pressure);
                previousMillisPressureAdsDebug = currentMillisPressureAdsDebug;
            }
        }
    }

    return (float)analog_pressure;
}
