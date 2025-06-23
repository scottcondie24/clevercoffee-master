#pragma once
ESP32Encoder encoder;

unsigned long startMillisEncoderSw = 0;
unsigned long EncoderSwitchBackflushInterval = 2000;
unsigned long EncoderSwitchControlInterval = 800;
bool encoderSwPressed = false;

//variables used outside waterHandler.h

extern float inputPressure = 0;
extern float pumpFlowRate = 0;
extern float setPressure = 9.0;
extern float setPumpFlowRate = 6.0;
extern PumpControl pumpControl = PRESSURE;
extern float DimmerPower = 95.0;
extern float flowKp = 8.0;
extern float flowKi = 30.0;
extern float flowKd = 0.0;
extern float pumpintegral = 0.0;
extern float previousError = 0;
extern int featurePumpDimmer = FEATURE_PUMP_DIMMER;

enum EncoderControlID {
    MENU_POWER = 1,
    MENU_PRESSURE,
    MENU_RECIPE,
    MENU_FLOW,
    MENU_KP,
    MENU_KI,
    MENU_KD,
    MENU_DIM_METHOD
};

struct EncoderControl {
    EncoderControlID id;
    const char* label;
    float* value;
    float step;
    float min;
    float max;
};

EncoderControl encoderControls[] = {
    {}, // index 0 is encoder disabled
    {MENU_POWER,     "Power",     &DimmerPower,      1.0f, 0.0f, 100.0f},
    {MENU_PRESSURE,  "Pressure",  &setPressure,      0.2f, 4.0f, 10.0f},
    {MENU_RECIPE,    "Recipe",    nullptr,           1.0f, 0, recipesCount - 1},
    {MENU_FLOW,      "Flow",      &setPumpFlowRate,  0.2f, 0.0f, 10.0f},
    {MENU_KP,        "Kp",        &flowKp,           0.2f, 0.0f, 40.0f},
    {MENU_KI,        "Ki",        &flowKi,           0.2f, 0.0f, 40.0f},
    {MENU_KD,        "Kd",        &flowKd,           0.01f,0.0f, 4.0f},
    {MENU_DIM_METHOD,"DimMethod",nullptr,            1.0f, 1, 2}
};
const int encoderControlCount = sizeof(encoderControls) / sizeof(EncoderControl);

void initEncoder() {
    ESP32Encoder::useInternalWeakPullResistors = puType::up;
    encoder.attachHalfQuad (PIN_ROTARY_DT, PIN_ROTARY_CLK);
    encoder.setCount (0);
}

void encoderHandler() {
    if(ENCODER_CONTROL > 0) {
        static long lastencodervalue = 0;
        float tempvalue = 0.0;      //use a tempvalue so only the final result is written to each variable

        if(machineState != kBackflush) {
            long value = encoder.getCount() / 2;

            if(value != lastencodervalue) {
                LOGF(INFO, "Rotary Encoder Value: %i", value);
                if (menuLevel == 1) {
                    tempvalue = encoderControl + (value - lastencodervalue);
                    encoderControl = constrain(tempvalue, 1, encoderControlCount - 1);
                    pumpintegral = 0;
                    previousError = 0;
                    const auto& control = encoderControls[encoderControl];
                    if (control.id == MENU_RECIPE) {
                        recipeName = recipes[currentRecipeIndex].name;
                        lastTempSetpoint = temperature;
                        //temperature = recipes[currentRecipeIndex].targetTemperature;
                        lastPreinfusion = preinfusion;
                        lastPreinfusionPause = preinfusionPause;
                        lastBrewTime = brewTime;
                        preinfusion = 0;            // disable preinfusion time in s
                        preinfusionPause = 0;       // disable preinfusion pause time in s
                        LOGF(INFO, "Recipe Index: %i -- Recipe Name: %s", currentRecipeIndex, recipeName);
                    }
                    else {
                        //temperature = lastTempSetpoint;
                        preinfusion = lastPreinfusion;            // preinfusion time in s
                        preinfusionPause = lastPreinfusionPause; // preinfusion pause time in s
                        brewTime = lastBrewTime;                       // brewtime in s
                    }  
                } else if (menuLevel == 2 && encoderControl >= 1 && encoderControl < encoderControlCount) {
                    const auto& control = encoderControls[encoderControl];

                    if (control.value != nullptr) {
                        float delta = (value - lastencodervalue) * control.step;
                        *control.value = constrain(*control.value + delta, control.min, control.max);
                    } else if (control.id == MENU_RECIPE) { // Recipe selection
                        currentRecipeIndex = constrain(currentRecipeIndex + (value - lastencodervalue), 0, recipesCount - 1);
                        recipeName = recipes[currentRecipeIndex].name;
                        currentPhaseIndex = 0;
                        LOGF(INFO, "Recipe Index: %i -- Recipe Name: %s", currentRecipeIndex, recipeName);
                    } else if (control.id == MENU_DIM_METHOD) { // Dimmer control method
                        featurePumpDimmer = constrain(featurePumpDimmer + (value - lastencodervalue), 1, 2);
                    }
                }
                lastencodervalue = value;
            }
        }

        if(encoderSw->isPressed()) {
            if(encoderSwPressed == false) {
                startMillisEncoderSw = millis();
                encoderSwPressed = true;
            }
        }
        else {
            if(encoderSwPressed == true) {
                unsigned long duration = millis() - startMillisEncoderSw;
                if(duration > EncoderSwitchBackflushInterval) {   //toggle every interval
                    if(machineState == kBackflush) {
                        setBackflush(0);
                        startMillisEncoderSw = millis();
                    }
                    if(machineState == kPidNormal) {
                        setBackflush(1);
                        startMillisEncoderSw = millis();
                    }
                }
                else if(duration > EncoderSwitchControlInterval) {   //toggle every interval
                    menuLevel = 0;
                }
                else {
                    menuLevel = (menuLevel == 1) ? 2 : 1;
                }
                LOGF(INFO, "Rotary Encoder Button down for: %lu ms", duration);
            }
            encoderSwPressed = false;
        }
    }
}