#include "PCF8575Pin.h"

PCF8575Pin::PCF8575Pin(PCF8575& expander, uint8_t pinIndex, Type type)
    : GPIOPin(40 + pinIndex, type), io(expander), index(pinIndex)
{
    setType(type);
}

void PCF8575Pin::write(bool value) const {
    io.write(index, value);
}

int PCF8575Pin::read() const {
    return io.read(index);
}

void PCF8575Pin::setType(Type type) {
    // PCF8575 does not have real pinMode; direction is controlled by writing HIGH (input) or LOW (output).
    switch (type) {
        case OUT:
            io.write(index, 0); // Drives pin LOW (output)
            break;
        case IN_STANDARD:
        case IN_PULLUP:
        case IN_HARDWARE:
        case IN_PULLDOWN:
            io.write(index, 1); // Releases pin (input with weak pull-up)
            break;
        case IN_ANALOG:
            // Not supported — PCF8575 has only digital I/O
            break;
    }
}