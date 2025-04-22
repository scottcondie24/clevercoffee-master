/**
 * @file TempSensorK.cpp
 *
 * @brief Handler for K type thermocouple
 */

#include "TempSensorK.h"
#include "Logger.h"


TempSensorK::TempSensorK(int GPIOPin1,int GPIOPin2,int GPIOPin3) {
    kSensor_ = new MAX6675(GPIOPin1, GPIOPin2, GPIOPin3);
}

bool TempSensorK::sample_temperature(double& temperature) const {
    auto temp = kSensor_->readCelsius();

    if (temp == 0) {
        LOG(WARNING, "Temperature reading failed");
        return false;
    }

    //calibrate for normal brew range
    //temp = temp*0.92+-0.64;

    temperature = temp;
    return true;
}