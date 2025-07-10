#include "Dimmers.h"

const char* controlMethodToString(PumpDimmer::ControlMethod method) {
    switch (method) {
        case PumpDimmer::ControlMethod::PSM:
            return "PSM";

        case PumpDimmer::ControlMethod::PHASE:
            return "PHASE";

        default:
            return "Unknown";
    }
}

PumpDimmer* PumpDimmer::instance = nullptr;

PumpDimmer::PumpDimmer(GPIOPin& outputPin, GPIOPin& zeroCrossPin, int timerNum) :
    _out(outputPin), _zc(zeroCrossPin), _timerNum(timerNum), _power(0), _psmAccumulated(0), _state(false), _lastZC(0), _method(ControlMethod::PSM), _timer(nullptr) {
    instance = this;
}

void PumpDimmer::begin() {
    _out.write(LOW);

    _timer = timerBegin(_timerNum, 80, true); // 80 prescaler = 1 µs ticks (assuming 80 MHz APB clock)

    timerAttachInterrupt(
        _timer,
        []() {
            if (instance->_phaseState == TimerPhase::DELAY) {
                instance->_out.write(HIGH);
                instance->_phaseState = TimerPhase::RESET;
                timerWrite(instance->_timer, 0);
                timerAlarmWrite(instance->_timer, 100, false); // Reset in 100 µs
                timerAlarmEnable(instance->_timer);
            }
            else {
                instance->_out.write(LOW);
                timerAlarmDisable(instance->_timer); // Done for this cycle
            }
        },
        true);

    if (_method == ControlMethod::PHASE) {
        attachInterrupt(_zc.getPin(), onZeroCrossPhaseStatic, RISING);
    }
    else {
        attachInterrupt(_zc.getPin(), onZeroCrossPSMStatic, RISING);
    }
}

void PumpDimmer::setPower(int power) {
    _power = constrain((int)power, 0, 100);
}

int PumpDimmer::getPower() const {
    return _power;
}

void PumpDimmer::on() {
    if (!_state && _method == ControlMethod::PSM) {
        resetPSMCounter();
    }

    _state = true;
}

void PumpDimmer::off() {
    _state = false;
    _out.write(LOW);
}

bool PumpDimmer::getState() const {
    return _state;
}

float PumpDimmer::getFlow(float pressure) const {
    float powerMultiplier = _state ? float(_power) / 100.0f : 0.0f;

    // Shared logic; flow scaling subject to future tuning
    return powerMultiplier * (-((FlowRate1 - FlowRate2) / FlowRatePres) * pressure + FlowRate1) / 30.0f;
}

void PumpDimmer::setControlMethod(ControlMethod method) {
    if (_method == method) {
        return;
    }

    detachInterrupt(_zc.getPin());
    _method = method;

    if (_method == ControlMethod::PHASE) {
        attachInterrupt(_zc.getPin(), onZeroCrossPhaseStatic, RISING);
    }
    else {
        attachInterrupt(_zc.getPin(), onZeroCrossPSMStatic, RISING);
    }
}

PumpDimmer::ControlMethod PumpDimmer::getControlMethod() const {
    return _method;
}

// PSM method
void PumpDimmer::resetPSMCounter() {
    _psmAccumulated = 0;
}

void PumpDimmer::handlePSMZeroCross() {
    unsigned long now = millis();

    if (now - _lastZC < 15) {
        return; // Debounce
    }

    _lastZC = now;

    if (_power <= 0 || !_state) {
        _out.write(LOW);
        return;
    }

    _psmAccumulated += _power;

    if (_psmAccumulated >= 100) {
        _out.write(HIGH);
        _psmAccumulated -= 100;
    }
    else {
        _out.write(LOW);
    }
}

void IRAM_ATTR PumpDimmer::onZeroCrossPSMStatic() {
    if (instance) {
        instance->handlePSMZeroCross();
    }
}

// Phase method
void PumpDimmer::handlePhaseZeroCross() {
    if (_power <= 0 || !_state) {
        _out.write(LOW);
        return;
    }

    _phaseState = TimerPhase::DELAY;
    timerWrite(_timer, 0);
    uint32_t delayMicros = map(_power, 0, 100, 8000, 200); // Lower delay = more power (fired earlier)
    timerAlarmDisable(_timer);
    timerAlarmWrite(_timer, delayMicros, false);
    timerAlarmEnable(_timer);
}

void IRAM_ATTR PumpDimmer::onZeroCrossPhaseStatic() {
    if (instance) {
        instance->handlePhaseZeroCross();
    }
}

PumpControlType PumpDimmer::getType() const {
    return PumpControlType::DIMMER;
}
