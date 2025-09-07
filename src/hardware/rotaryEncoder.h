#pragma once

ESP32Encoder encoder;

unsigned long startMillisEncoderSwitch = 0;
unsigned long EncoderSwitchBackflushInterval = 2000;
unsigned long EncoderSwitchControlInterval = 800;
bool encoderSwitchPressed = false;

void initEncoder() {
    ESP32Encoder::useInternalWeakPullResistors = puType::up;
    encoder.attachHalfQuad(PIN_ROTARY_DT, PIN_ROTARY_CLK);
    encoder.setCount(0);
}

int getEncoderDelta(void) {
    static long lastencodervalue = 0;
    long value = encoder.getCount() / 2;
    int delta = value - lastencodervalue;

    if (lastencodervalue != value) {
        LOGF(INFO, "Rotary Encoder Value: %i", value);
    }

    lastencodervalue = value;

    return delta;
}

void encoderHandler() {
    if (!config.get<bool>("hardware.switches.encoder.enabled") || encoderSwitch == nullptr) {
        return;
    }

    int delta = getEncoderDelta(); // +1 or -1 or 0

    if (machineState != kBackflush) {
        if (delta != 0) {
            if (menuLevel == 1) {
                config.set<int>("dimmer.mode", constrain(config.get<int>("dimmer.mode") + delta, 0, 3));
            }
            else if (menuLevel == 2) {
                switch (config.get<int>("dimmer.mode")) {
                    case POWER:
                        config.set<double>("dimmer.setpoint.power", constrain(config.get<float>("dimmer.setpoint.power") + delta, PUMP_POWER_SETPOINT_MIN, PUMP_POWER_SETPOINT_MAX));
                        break;

                    case PRESSURE:
                        config.set<double>("dimmer.setpoint.pressure", constrain(config.get<float>("dimmer.setpoint.pressure") + ((float)delta * 0.1), PUMP_PRESSURE_SETPOINT_MIN, PUMP_PRESSURE_SETPOINT_MAX));
                        break;

                    case FLOW:
                        config.set<double>("dimmer.setpoint.flow", constrain(config.get<float>("dimmer.setpoint.flow") + ((float)delta * 0.1), PUMP_FLOW_SETPOINT_MIN, PUMP_FLOW_SETPOINT_MAX));
                        break;

                    case PROFILE:
                        config.set<int>("dimmer.profile", constrain(config.get<int>("dimmer.profile") + delta, 0, 11));
                        break;

                    default:
                        break;
                }
            }
        }
    }

    if (encoderSwitch->isPressed()) {
        if (encoderSwitchPressed == false) {
            startMillisEncoderSwitch = millis();
            encoderSwitchPressed = true;
        }
    }
    else {
        if (encoderSwitchPressed == true) {
            unsigned long duration = millis() - startMillisEncoderSwitch;
            if (duration > EncoderSwitchBackflushInterval) { // toggle every interval
                if (machineState == kBackflush) {
                    backflushOn = false;
                    startMillisEncoderSwitch = millis();
                }

                if (machineState == kPidNormal) {
                    backflushOn = true;
                    startMillisEncoderSwitch = millis();
                }
            }
            else if (duration > EncoderSwitchControlInterval) { // toggle every interval
                menuLevel = 0;
                if (!config.save()) {
                    LOG(ERROR, "Failed to save config to filesystem!");
                }
            }
            else {
                menuLevel = (menuLevel == 1) ? 2 : 1;
            }

            LOGF(INFO, "Rotary Encoder Button down for: %lu ms", duration);
        }

        encoderSwitchPressed = false;
    }
}