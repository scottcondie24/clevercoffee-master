/**
 * @file TempSensorTSIC.cpp
 *
 * @brief Handler for TSIC 306 temperature sensor
 */

#include "TempSensorTSIC.h"
#include "Logger.h"

#define INITIAL_CHANGERATE 200
#define MAX_CHANGERATE     15

TempSensorTSIC::TempSensorTSIC(const int GPIOPin) {
    // Set pin to receive signal from the TSic 306
    tsicSensor_ = new ZACwire(GPIOPin, 306);
    // Start sampling the TSic sensor
    tsicSensor_->begin();
}

bool TempSensorTSIC::sample_temperature(double& temperature) const {
    static bool validTemps = false;
    float temp = 0.0;

    if (!validTemps) {
        temp = tsicSensor_->getTemp(INITIAL_CHANGERATE);

        if (temp >= 0.0 && temp <= 180.0) {
            validTemps = true;
        }
        else {
            LOGF(WARNING, "Temperature reading outside expected range: %0.01f", temp);
        }
    }
    else {
        temp = tsicSensor_->getTemp(MAX_CHANGERATE);
    }

    if (temp == 222) {
        LOG(WARNING, "Temperature reading failed");
        return false;
    }

    if (temp == 221) {
        LOG(WARNING, "Temperature sensor not connected");
        return false;
    }

    temperature = temp;

    return true;
}
