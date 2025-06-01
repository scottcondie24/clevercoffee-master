/**
 * @file waterHandler.h
 *
 * @brief Handler for digital hot water switch
 */

uint8_t currStateWaterSwitch;

MachineState lastmachinestatedebug = kInit;

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

void waterHandler(void) {
    static String waterstatedebug = "off";
    static String lastwaterstatedebug = "off";
    // turn on pump if water switch is on, only turn off if not in a brew or flush state
    if (machineState == kWaterTankEmpty) {
        pumpRelay.off();
        waterstatedebug = "off-we";                                                // turned off due to water empty
    }
    else if (machineState == kWater || (machineState == kSteam && waterON == 1)) { // was waterON == 1 && machineState != kWaterEmpty not needed now kWaterEmpty check is first
        pumpRelay.on();
        waterstatedebug = "on-sw";                                                 // turned on due to switch input
    }
    else { // was (waterON == 0) but not needed, currently need currBrewSwitchState as machineState doesnt change quick enough and turns the pump off when flushing
        if (machineState != kBrew && machineState != kBackflush && machineState != kManualFlush) { //} && currBrewSwitchState != kBrewSwitchLongPressed && currBrewSwitchState != kBrewSwitchShortPressed) {
            pumpRelay.off();
            waterstatedebug = "off-sw";                                                            // switch turned off and not in brew or flush
        }
        else {
            if (waterstatedebug != "on" && waterstatedebug != "off") {                             // on or off added into brewhandler
                waterstatedebug = "brew or flush";                                                 // brew or backflush
            }
        }
    }

    if (machineState != lastmachinestatedebug || waterstatedebug != lastwaterstatedebug) {
        LOGF(DEBUG, "main.cpp - water state: %s, machineState=%s", waterstatedebug, machinestateEnumToString(machineState));
        lastmachinestatedebug = machineState;
        lastwaterstatedebug = waterstatedebug;
    }
}