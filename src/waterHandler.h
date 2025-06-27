/**
 * @file waterHandler.h
 *
 * @brief Handler for digital hot water switch
 */
#include "hardware/rotaryEncoder.h"

uint8_t currStateWaterSwitch;

//Pump  //these shouldnt need to be volatile as no interrupts


float pressureKp = 20.0;//   18.0;//18.0;//13.0;//14.0;//   20.0;    //25.0;//30.0;    // Proportional gain
float pressureKi = 10.0;//  9.0;//8.0;//4.0;//5.0;//   10.0;   //45.0;//75.0;     // Integral gain
float pressureKd = 1.5;//   1.0;//3.0;//7.0;//6.0;//   2.0;   //3.0;       // Derivative gain
float integralAntiWindup = 8.0;  //pumpintegral += error * pumpdt is capped at +-integralAntiWindup, then *pressureKi
int MaxDimmerPower = 100;
unsigned long currentMillisPumpControl = 0;
unsigned long previousMillisPumpControl = 0;
unsigned long pumpControlInterval = 50;
unsigned long maxPumpControlInterval = 100;

unsigned long blockMQTTInterval = 10000;
unsigned long blockWebsiteInterval = 40000;
unsigned long blockDisplayInterval = 12000;
unsigned long blockStart = 0;
float pumpdt = pumpControlInterval / 1000.0;  // Time step in seconds

float flowPressureCeiling = 0;
float flowPressureRange = 0;



void checkWaterSwitch() {
    if (FEATURE_WATERSWITCH) {
        uint8_t waterSwitchReading = waterSwitch->isPressed();

        if (WATERSWITCH_TYPE == Switch::TOGGLE) {
            // Set waterON to 1 when waterswitch is HIGH
            if (waterSwitchReading == HIGH) {
                waterON = 1;
            }

            // Set waterON to 0 when waterswitch is LOW
            if (waterSwitchReading == LOW) {
                waterON = 0;
            }
        }
        else if (WATERSWITCH_TYPE == Switch::MOMENTARY) {
            if (waterSwitchReading != currStateWaterSwitch) {
                currStateWaterSwitch = waterSwitchReading;

                // only toggle water power if the new button state is HIGH
                if (currStateWaterSwitch == HIGH) {
                    if (waterON == 0) {
                        waterON = 1;
                    }
                    else {
                        waterON = 0;
                    }
                }
            }
        }
    }
}

void waterHandler(void){
        //turn on pump if water switch is on, only turn off if not in a brew or flush state
    if (machineState == kWaterTankEmpty) {
        pumpRelay.off();
        waterstatedebug = "off-we";
    }
    else if (machineState == kWater || (machineState == kSteam && waterON == 1)) { //was waterON == 1 && machineState != kWaterEmpty not needed now kWaterEmpty check is first
        pumpRelay.on();
        waterstatedebug = "on-sw";
    }
    else {    // was (waterON == 0) but not needed, currently need currBrewSwitchState as machineState doesnt change quick enough and turns the pump off when flushing
        if(machineState != kBrew && machineState != kBackflush && machineState != kManualFlush) { //} && currBrewSwitchState != kBrewSwitchLongPressed && currBrewSwitchState != kBrewSwitchShortPressed) {
            pumpRelay.off();
            waterstatedebug = "off-sw";
        }
        else {
            if(waterstatedebug != "on" && waterstatedebug != "off") {
                waterstatedebug = "brew or flush";
            }
        }
    }

    if(machineState != lastmachinestatedebug || waterstatedebug != lastwaterstatedebug) {
        LOGF(DEBUG, "main.cpp - water state: %s, machineState=%s", waterstatedebug, machinestateEnumToString(machineState));
        lastmachinestatedebug = machineState;
        lastwaterstatedebug = waterstatedebug;
    }
}

float applySmoothOverride(float target, float input, float ceiling, float range, int curve = 1) {
    if (ceiling > 0 && range > 0 && input > ceiling) {
        float t = (input - ceiling) / range;
        t = constrain(t, 0.0f, 1.0f);
        if (curve == 2) t = t * t;         // quadratic
        else if (curve == 3) t = t * t * t; // cubic
        //reset integral to reduce windup, could be harsh
        //pumpintegral = 0;
        return target * (1.0f - t);
    }
    return target;
}

void looppump() {
    if(machineState != kBrew) { //moved here from profiles
        debug_profile = false;
        currentPhaseIndex = 0;
        flowPressureCeiling = 0.0;
        flowPressureRange = 0.0;
    }
#if (FEATURE_PUMP_DIMMER > 0) 
    static float inputPID = 0.0;
    static float targetPID = 0.0;                
    static float inputKp = 0.0;
    static float inputKi = 0.0;
    static float inputKd = 0.0;

    if(pumpRelay.getState()) {
        currentMillisPumpControl = millis();
        if (currentMillisPumpControl - previousMillisPumpControl >= pumpControlInterval) {  //50ms timing
            if(currentMillisPumpControl - previousMillisPumpControl > maxPumpControlInterval) { //if greater than 100ms, set pumpdt to 100ms to stop jumps due to an extended freeze
                pumpdt = maxPumpControlInterval / 1000.0;
            }
            else {
                pumpdt = (currentMillisPumpControl - previousMillisPumpControl) / 1000.0;   //set to between 50ms and 100ms
            }
            PidResults[loopIndexPid][8] = currentMillisPumpControl - previousMillisPumpControl;
            previousMillisPumpControl = currentMillisPumpControl;

            PidResults[loopIndexPid][0] = inputPressure;
            PidResults[loopIndexPid][1] = setPressure;
            PidResults[loopIndexPid][2] = pumpFlowRate;
            PidResults[loopIndexPid][3] = setPumpFlowRate;
            PidResults[loopIndexPid][4] = weightBrewed;

            const auto& control = encoderControls[encoderControl];
            if (control.id == MENU_POWER) {
                pumpControl = POWER;
            }
            else if (control.id == MENU_PRESSURE) {
                pumpControl = PRESSURE;
                //testing flow limits
                //flowPressureCeiling = 8.0; //flow mL/s
                //flowPressureRange = 6.0;    //reduce to 0 output over 14mL/s
            }
            else if (control.id == MENU_PROFILE) {
                if(machineState == kBrew) {
                    runProfile(currentProfileIndex);
                }
            }
            else if (control.id >= MENU_FLOW) {
                pumpControl = FLOW;
                flowPressureCeiling = 9.0; //pressure bar
                flowPressureRange = 0.2;    //reduce to 0 output over 9.2bar
            }

            //override for flush and backflush
            if(machineState == kBackflush) {
                pumpControl = PRESSURE;
            }
            if(machineState == kManualFlush) {
                pumpControl = FLOW;
            }


            if(pumpControl == PRESSURE) {   //pressure
                inputPID = inputPressureFilter;//inputPressure;
                targetPID = (machineState == kBackflush) ? 9.0f : setPressure;
                // Smooth flow override, doesnt work well in pressure
                targetPID = applySmoothOverride(targetPID, pumpFlowRate, flowPressureCeiling, flowPressureRange, 2);  //1 is linear reduction, 2 quadratic, 3 cubic
                inputKp = pressureKp;
                inputKi = pressureKi;
                inputKd = pressureKd;
            }
            else if (pumpControl == FLOW) { //flow and PID tuning
                inputPID = pumpFlowRate;
                targetPID = (machineState == kManualFlush) ? 10.0f : setPumpFlowRate;
                // Smooth pressure override
                targetPID = applySmoothOverride(targetPID, inputPressureFilter, flowPressureCeiling, flowPressureRange, 2);  //1 is linear reduction, 2 quadratic, 3 cubic
                
                inputKp = flowKp;
                inputKi = flowKi;
                inputKd = flowKd;
            }
            else {
                inputPID = 0.0;
                targetPID = 0.0;
                inputKp = 0.0;
                inputKi = 0.0;
                inputKd = 0.0;
            }

            if(pumpControl == POWER) {
                DimmerPower = constrain((int)DimmerPower, 0, MaxDimmerPower);
            }
            else {
                float error = targetPID - inputPID;
                pumpintegral += error * pumpdt; // Integrate error
                pumpintegral = constrain(pumpintegral, -integralAntiWindup, integralAntiWindup);
                float pumpderivative = (error - previousError) / pumpdt;
                previousError = error;
                //PID output
                float output = (inputKp * error) + (inputKi * pumpintegral) + (inputKd * pumpderivative);
                
                PidResults[loopIndexPid][5] = inputKp * error;
                PidResults[loopIndexPid][6] = inputKi * pumpintegral;
                PidResults[loopIndexPid][7] = inputKd * pumpderivative;

                DimmerPower = constrain((int)output, 0, MaxDimmerPower);
            }
            // Only update if power changed
            if (pumpRelay.getPower() != DimmerPower) {
                pumpRelay.setPower(DimmerPower);
            }

            //DEBUGGING
            if (inputPressure > maxpressure) {
                maxpressure = inputPressure;
            }
            loopIndexPid = (loopIndexPid + 1) % LOOP_HISTORY_SIZE;

            //Count down and log when ready
            if(loopIndexPid == 0){
                if(maxpressure > 0.10) {
                    printLoopPidAsList();
                }
                maxpressure = 0;
            }
        }
    }
    else {  //Pump turned off
        pumpintegral = 0;
        previousError = 0;
        previousMillisPumpControl = millis() - pumpControlInterval; //stops large spikes in log data timing
        //PumpDimmerCore::ControlMethod method = (featurePumpDimmer == 2) ? PumpDimmerCore::ControlMethod::PHASE : PumpDimmerCore::ControlMethod::PSM;
        //pumpRelay.setControlMethod(method);
        pumpRelay.setControlMethod((featurePumpDimmer == 2) ? PumpDimmerCore::ControlMethod::PHASE : PumpDimmerCore::ControlMethod::PSM);
    }
#endif
    blockStart = micros(); //give other functions like display and MQTT some time to refresh
}