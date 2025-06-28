//using madhephaestus/ESP32Encoder library

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

//encoder menu
int pidParamIndex = 0; // 0 = pressureP, 1 = pressureI, 2 = pressureD, 3 = flowP, 4 = flowI, 5 = FlowD
bool editingPID = false;
int selectedProfile = 0;
int profileMenuIndex = 0;
int profileScrollOffset = 0;
const int maxVisibleItems = 4;
int selectedIndex = 0;  // current selected item
int menuScrollOffset = 0; // top of visible menu window
unsigned long lastInteractionTime;
const unsigned long menuTimeout = 10000; // 10s
int filteredProfileIndices[MAX_PROFILES];
int filteredProfileCount = 0;

const char* menuItems[] = {
  "Select Control",
  "Select Profile",
  "Tune Pressure",
  "Tune Flow",
  "Set Temp",
  "Backflush Machine",
  "Select Dim Method",
  "Exit"
};
const int menuItemCount = sizeof(menuItems) / sizeof(menuItems[0]);

enum UIState {
  UI_MAIN,
  UI_MENU,
  UI_PROFILE_LIST,
  UI_TUNE_PID,
  UI_BREW_RUNNING
};

UIState uiState = UI_MAIN;

enum EncoderControlID {
    MENU_POWER = 1,
    MENU_PRESSURE,
    MENU_PROFILE,
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
    {MENU_PROFILE,   "Profile",    nullptr,           1.0f, 0, profilesCount - 1},
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

void buildProfileMenu() {
    filteredProfileCount = 0;
    for (int i = 0; i < profilesCount; i++) {
        BrewProfile* r = &profiles[i];

        //if (r->disabled || r->needsCalibration || r->experimental) {
        //    continue      //jump to the next i
        //}
        
        if(filteredProfileCount >= MAX_PROFILES) {
            break;
        }
        filteredProfileIndices[filteredProfileCount++] = i; //increments after applying i to the index
        
    }
    if (filteredProfileCount == 0) {
        //displayText("No profiles available");
        return;
    }
}


/*

void drawTunePID(float pidP, float pidI, float pidD) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x13_tf);
  u8g2.drawStr(0, 10, "== Tune PID ==");

  char line[32];

  snprintf(line, sizeof(line), "P: %.2f", pidP);
  if (pidParamIndex == 0) u8g2.drawBox(0, 12, 128, 12);
  u8g2.setDrawColor(pidParamIndex == 0 ? 0 : 1);
  u8g2.drawStr(4, 22, line);

  snprintf(line, sizeof(line), "I: %.2f", pidI);
  if (pidParamIndex == 1) u8g2.drawBox(0, 24, 128, 12);
  u8g2.setDrawColor(pidParamIndex == 1 ? 0 : 1);
  u8g2.drawStr(4, 34, line);

  snprintf(line, sizeof(line), "D: %.2f", pidD);
  if (pidParamIndex == 2) u8g2.drawBox(0, 36, 128, 12);
  u8g2.setDrawColor(pidParamIndex == 2 ? 0 : 1);
  u8g2.drawStr(4, 46, line);

  u8g2.setDrawColor(1);
  u8g2.sendBuffer();
}


}


void displayProfileAtIndex(int menuIndex) {
    int realIndex = filteredProfileIndices[menuIndex];
    BrewProfile* r = &profiles[realIndex];
    displayText(r->name);  // however you display text in your system
}


void onProfileSelected(int menuIndex) {
    int realIndex = filteredProfileIndices[menuIndex];
    BrewProfile* selectedProfile = &profiles[realIndex];

    // Proceed to use `selectedProfile` as needed
    startBrewWithProfile(selectedProfile);
}

void drawProfileList() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x13_tf);
  u8g2.drawStr(0, 10, "== Select Profile ==");

  for (int i = 0; i < maxVisibleItems; i++) {
    int idx = profileScrollOffset + i;
    if (idx >= profileCount) break;
    int y = 20 + i * 12;
    if (idx == profileMenuIndex) {
      u8g2.drawBox(0, y - 10, 128, 12);
      u8g2.setDrawColor(0);
      u8g2.drawStr(4, y, profiles[idx]);
      u8g2.setDrawColor(1);
    } else {
      u8g2.drawStr(4, y, profiles[idx]);
    }
  }

  u8g2.sendBuffer();
}

void handleProfileMenu(int delta) {
  profileMenuIndex = constrain(profileMenuIndex + delta, 0, profileCount - 1);
  if (profileMenuIndex < profileScrollOffset)
    profileScrollOffset = profileMenuIndex;
  else if (profileMenuIndex >= profileScrollOffset + maxVisibleItems)
    profileScrollOffset = profileMenuIndex - maxVisibleItems + 1;
}

void handleProfileClick() {
  selectedProfile = profileMenuIndex;
  uiState = UI_MENU;
}

void handleEncoderDuringBrew(int delta) {
  if (brewControlMode == CONTROL_PRESSURE) {
    setPressure = constrain(setPressure + delta * 0.1, 0.0, 12.0);
  } else {
    setFlow = constrain(setFlow + delta * 0.1, 0.0, 5.0);
  }
}

void handlePIDInput(int delta) {
  if (!editingPID) {
    pidParamIndex = constrain(pidParamIndex + delta, 0, 2);
  } else {
    float* param = (pidParamIndex == 0) ? &pidP :
                   (pidParamIndex == 1) ? &pidI : &pidD;
    *param += delta * 0.1;
    *param = max(0.0f, *param);
  }
}

void handlePIDClick() {
  if (!editingPID) {
    editingPID = true;
  } else {
    editingPID = false;
    uiState = UI_MENU;  // or stay for further tuning
  }
}


void drawBrewScreen() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x13_tf);

  char line[32];

  snprintf(line, sizeof(line), "Mode: %s",
           brewControlMode == CONTROL_PRESSURE ? "Pressure" : "Flow");
  u8g2.drawStr(0, 12, line);

  snprintf(line, sizeof(line), "Target: %.1f %s",
           brewControlMode == CONTROL_PRESSURE ? setPressure : setPumpFlowRate,
           brewControlMode == CONTROL_PRESSURE ? "bar" : "ml/s");
  u8g2.drawStr(0, 26, line);

  snprintf(line, sizeof(line), "Actual: %.1f %s",
           brewControlMode == CONTROL_PRESSURE ? inputPressure : pumpFlowRate,
           brewControlMode == CONTROL_PRESSURE ? "bar" : "ml/s");
  u8g2.drawStr(0, 40, line);

  snprintf(line, sizeof(line), "Time: %.1f s", (millis() - brewStartTime) / 1000.0);
  u8g2.drawStr(0, 54, line);

  u8g2.sendBuffer();
}

void drawMenu() {
  u8g2.clearBuffer();

  u8g2.setFont(u8g2_font_6x13_tf);
  u8g2.drawStr(0, 10, "== Main Menu ==");

  for (int i = 0; i < maxVisibleItems; i++) {
    int itemIndex = menuScrollOffset + i;
    if (itemIndex >= menuItemCount) break;

    int y = 20 + i * 12;
    if (itemIndex == selectedIndex) {
      u8g2.drawBox(0, y - 10, 128, 12); // highlight box
      u8g2.setDrawColor(0);             // invert text color
      u8g2.drawStr(4, y, menuItems[itemIndex]);
      u8g2.setDrawColor(1);
    } else {
      u8g2.drawStr(4, y, menuItems[itemIndex]);
    }
  }

  u8g2.sendBuffer();
}

void updateUIState() {
  if ((uiState != UI_MAIN_DISPLAY) && millis() - lastInteractionTime > menuTimeout) {
    uiState = UI_MAIN_DISPLAY;
  }
}

void drawUI() {
  switch (uiState) {
    case UI_MAIN: drawMainScreen(); break;
    case UI_MENU: drawMenu(); break;
    case UI_PROFILE_LIST: drawProfileList(); break;
    case UI_TUNE_PID: drawTunePID(); break;
  }
}

*/




/*void handleEncoder(int delta) {
  selectedIndex = constrain(selectedIndex + delta, 0, menuItemCount - 1);
  // Add a scroll offset if outside visible range
  if (selectedIndex < menuScrollOffset) {
    menuScrollOffset = selectedIndex;
  } else if (selectedIndex >= menuScrollOffset + maxVisibleItems) {
    menuScrollOffset = selectedIndex - maxVisibleItems + 1;
  }
}

void handleEncoderClick() {
  if (uiState == UI_MENU) {
    const char* selected = menuItems[selectedIndex];

    if (strcmp(selected, "Exit") == 0) {
      uiState = UI_MAIN;
    } else {
      // Perform action or change state
      LOGF(DEBUG,"Selected: %s\n", selected);
    }
  } else {
    uiState = UI_MENU;
    selectedIndex = 0;
    menuScrollOffset = 0;
  }
}*/

int getEncoderDelta(void) {
    static long lastencodervalue = 0;
    long value = encoder.getCount() / 2;
    int delta = value - lastencodervalue;
    lastencodervalue = value;
    LOGF(INFO, "Rotary Encoder Value: %i", value);
    return delta;
}







void encoderHandler() {
    if(ENCODER_CONTROL > 0) {
        
        float tempvalue = 0.0;      //use a tempvalue so only the final result is written to each variable

        /*if(machineState == kBrew) {
            uiState = UI_BREW_RUNNING;
        }

        if (uiState == UI_BREW_RUNNING) {
            int delta = getEncoderDelta();
            if (delta != 0) handleEncoderDuringBrew(delta);

            if (encoderClicked()) handleClickDuringBrew();

            drawBrewScreen();
        }*/
        int delta = getEncoderDelta(); // +1 or -1 or 0
        //if (delta != 0) {
        //    handleEncoder(delta);
        //}

        if(machineState != kBackflush) {
            long value = encoder.getCount() / 2;

            if(delta != 0) {
                LOGF(INFO, "Rotary Encoder Value: %i", value);
                if (menuLevel == 1) {
                    tempvalue = encoderControl + delta;
                    encoderControl = constrain(tempvalue, 1, encoderControlCount - 1);
                    pumpintegral = 0;
                    previousError = 0;
                    const auto& control = encoderControls[encoderControl];
                    if (control.id == MENU_PROFILE) {
                        profileName = profiles[currentProfileIndex].name;
                        lastBrewSetpoint = brewSetpoint;
                        //brewSetpoint = profiles[currentProfileIndex].targetTemperature;
                        lastPreinfusion = preinfusion;
                        lastPreinfusionPause = preinfusionPause;
                        lastBrewTime = brewTime;
                        preinfusion = 0;            // disable preinfusion time in s
                        preinfusionPause = 0;       // disable preinfusion pause time in s
                        LOGF(INFO, "Profile Index: %i -- Profile Name: %s", currentProfileIndex, profileName);
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
                        float delta = delta * control.step;
                        *control.value = constrain(*control.value + delta, control.min, control.max);
                    } else if (control.id == MENU_PROFILE) { // Profile selection
                        currentProfileIndex = constrain(currentProfileIndex + delta, 0, profilesCount - 1);
                        profileName = profiles[currentProfileIndex].name;
                        currentPhaseIndex = 0;
                        LOGF(INFO, "Profile Index: %i -- Profile Name: %s", currentProfileIndex, profileName);
                    } else if (control.id == MENU_DIM_METHOD) { // Dimmer control method
                        featurePumpDimmer = constrain(featurePumpDimmer + delta, 1, 2);
                    }
                }
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
                    //handleEncoderClick();
                    menuLevel = (menuLevel == 1) ? 2 : 1;
                }
                LOGF(INFO, "Rotary Encoder Button down for: %lu ms", duration);
            }
            encoderSwPressed = false;
        }
    }
}
