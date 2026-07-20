/**
 * @file TempSensorMAX6675.h
 *
 * @brief Handler for K type thermocouple with MAX6675 using hardware SPI
 */

#pragma once

#include "TempSensor.h"
#include <MAX6675.h>

class TempSensorMAX6675 : public TempSensor {
    public:
        TempSensorMAX6675(int GPIOPin1, SPIClass* mySPI = &SPI, int rate = 230);

    protected:
        bool sample_temperature(double& temperature) const override;

    private:
        MAX6675* kSensor_;
};