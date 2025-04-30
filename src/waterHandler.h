/**
 * @file waterHandler.h
 *
 * @brief Handler for digital hot water switch
 */

uint8_t currStateWaterSwitch;

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
