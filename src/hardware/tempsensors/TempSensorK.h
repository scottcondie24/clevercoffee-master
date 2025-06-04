/**
 * @file TempSensorK.h
 *
 * @brief Handler for K type thermocouple with MAX6675 using adafruit/MAX6675 library
 */

#pragma once

#include "TempSensor.h"
#include <max6675.h>

class TempSensorK : public TempSensor {
    public:
        TempSensorK(int GPIOPin1, int GPIOPin2, int GPIOPin3);

    protected:
        bool sample_temperature(double& temperature) const override;

    private:
        MAX6675* kSensor_;
};