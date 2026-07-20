/**
 * @file TempSensorMAX31856.cpp
 *
 * @brief Handler for MAX31856 thermocouple amplifier
 */

#include "TempSensorMAX31856.h"
#include "Logger.h"

TempSensorMAX31856::TempSensorMAX31856(int GPIOPin1, int rate) :
    TempSensor(rate) {
    max31856_ = new Adafruit_MAX31856(GPIOPin1);
    init();
}

void TempSensorMAX31856::init() const {
    if (!max31856_->begin()) {
        LOG(ERROR, "Could not initialize thermocouple.");
    }
    else {
        max31856_->setThermocoupleType(MAX31856_TCTYPE_K);
        max31856_->setConversionMode(MAX31856_CONTINUOUS);
        max31856_->setNoiseFilter(MAX31856_NOISE_FILTER_50HZ);
    }
}

bool TempSensorMAX31856::sample_temperature(double& temperature) const {
    auto currentMode = max31856_->getConversionMode();
    auto temp = max31856_->readThermocoupleTemperature();
    uint8_t fault = max31856_->readFault();

    if (currentMode != MAX31856_CONTINUOUS || temp == 0.0) {
        LOG(ERROR, "MAX31856 error. Re-initializing...");
        init();
        return false;
    }

    if (fault) {
        if (fault & MAX31856_FAULT_CJRANGE) LOG(ERROR, "Cold Junction Range Fault");
        if (fault & MAX31856_FAULT_TCRANGE) LOG(ERROR, "Thermocouple Range Fault");
        if (fault & MAX31856_FAULT_CJHIGH) LOG(ERROR, "Cold Junction High Fault");
        if (fault & MAX31856_FAULT_CJLOW) LOG(ERROR, "Cold Junction Low Fault");
        if (fault & MAX31856_FAULT_TCHIGH) LOG(ERROR, "Thermocouple High Fault");
        if (fault & MAX31856_FAULT_TCLOW) LOG(ERROR, "Thermocouple Low Fault");
        if (fault & MAX31856_FAULT_OVUV) LOG(ERROR, "Over/Under Voltage Fault");
        if (fault & MAX31856_FAULT_OPEN) LOG(ERROR, "Thermocouple Open Fault");
        init();
        return false;
    }

    temperature = temp;
    return true;
}