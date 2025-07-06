#pragma once

#include "GPIOPin.h"
#include "pumpControl.h"

class PumpDimmer : public PumpControl {
    public:
        enum class ControlMethod {
            PHASE,
            PSM
        };

        PumpDimmer(GPIOPin& outputPin, GPIOPin& zeroCrossPin, int timerNum);

        void begin();
        void setPower(int power);
        int getPower() const;
        void on();
        void off();
        bool getState() const;
        float getFlow(float pressure) const;

        void setControlMethod(ControlMethod method);
        ControlMethod getControlMethod() const;
        PumpControlType getType() const override;

    private:
        GPIOPin& _out;
        GPIOPin& _zc;
        int _timerNum;
        int _power;
        int _psmAccumulated;
        bool _state;
        unsigned long _lastZC;
        ControlMethod _method;
        hw_timer_t* _timer;

        enum class TimerPhase {
            DELAY,
            RESET
        }; // Nested inside the class
        TimerPhase _phaseState;                         // Class member for phase dimmer state

        void resetPSMCounter();                         // PSM
        void handlePSMZeroCross();                      // PSM
        static void IRAM_ATTR onZeroCrossPSMStatic();   // PSM

        void handlePhaseZeroCross();                    // Phase
        static void IRAM_ATTR onZeroCrossPhaseStatic(); // Phase

        static PumpDimmer* instance;

        static constexpr float FlowRate1 = 292.4f;   // g in 30s using flush
        static constexpr float FlowRate2 = 135.6f;   // g in 30s using water switch
        static constexpr float FlowRatePres = 10.0f; // Bars at OPV
};