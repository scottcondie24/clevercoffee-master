/**
 * @file TempSensorMAX31856.h
 *
 * @brief Handler for MAX31856 thermocouple amplifier
 */

#pragma once

#include "TempSensor.h"
#include <Adafruit_MAX31856.h>

class TempSensorMAX31856 : public TempSensor {
    public:
        TempSensorMAX31856(int GPIOPin1, int rate = 230);

    protected:
        bool sample_temperature(double& temperature) const override;

    private:
        void init() const;
        Adafruit_MAX31856* max31856_;
};