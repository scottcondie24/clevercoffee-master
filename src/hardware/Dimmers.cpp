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

    unsigned long now = 0;

    while (!_zc.read())
        ;
    _lastZC = micros();
    delay(5);
    while (_zc.read())
        ;
    delay(5);
    while (!_zc.read())
        ;
    now = micros();
    _hz = 1000000.0f / (float)(now - _lastZC);

    if (_hz > 55.0f) {
        _maxDelay = 4717; // 60hz
        _minDelay = 167;
    }
    else {
        _maxDelay = 5660; // 50hz
        _minDelay = 200;
    }

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

float PumpDimmer::getFrequency() const {
    return _hz;
}

void PumpDimmer::setCalibration(float flowRate1, float flowRate2, float opvPressure) {
    _opvPressureInv = 1.0f / opvPressure;
    _flowRate1 = flowRate1 * 0.03333333f;
    _flowRate2 = flowRate2 * 0.03333333f;
    _deltaFlow = _flowRate2 - _flowRate1;
}

float PumpDimmer::getFlow(float pressure) const {
    float result = 0.0f;

    if (_method == ControlMethod::PSM) {
        float powerMultiplier = _state ? float(_power) * 0.01f : 0.0f;
        result = powerMultiplier * (_deltaFlow * _opvPressureInv * pressure + _flowRate1);
    }
    else {
        float powerMultiplier = _state ? float(_power) * 0.01f : 0.0f;
        powerMultiplier = (-0.98725659f * (powerMultiplier * powerMultiplier) + 2.00758877f * powerMultiplier - 0.01675258f);
        result = (_deltaFlow * _opvPressureInv * pressure + powerMultiplier * _flowRate1);
    }

    return result > 0.0f ? result : 0.0f;
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
    uint32_t delayMicros = map(_power, 0, 100, _maxDelay, _minDelay); // Lower delay = more power (fired earlier)
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