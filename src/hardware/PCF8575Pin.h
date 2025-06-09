#pragma once

#include "GPIOPin.h"
#include <PCF8575.h>

/**
 * @brief GPIOPin implementation for PCF8575 I/O expander
 */
class PCF8575Pin : public GPIOPin {
public:
    PCF8575Pin(PCF8575& expander, uint8_t pinIndex, Type type);

    void write(bool value) const override;
    int read() const override;

protected:
    void setType(Type type) override;

private:
    PCF8575& io;
    uint8_t index;  // 0-15
};