/**
 * @file TempSensorMAX6675.cpp
 *
 * @brief Handler for K type thermocouple with hardware SPI
 */

#include "TempSensorMAX6675.h"
#include "Logger.h"

TempSensorMAX6675::TempSensorMAX6675(int GPIOPin1, SPIClass* mySPI, int rate) :
    TempSensor(rate) {
    kSensor_ = new MAX6675(GPIOPin1, mySPI);
    kSensor_->begin();
}

bool TempSensorMAX6675::sample_temperature(double& temperature) const {
    int status = kSensor_->read();

    if (status != STATUS_OK) {
        LOGF(WARNING, "Temperature reading failed, status: %d", status);
        return false;
    }

    temperature = kSensor_->getCelsius();
    return true;
}