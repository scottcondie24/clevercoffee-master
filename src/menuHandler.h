#pragma once

#include <ESP32Encoder.h>
#include <Menu.h>
#include <button.h>
#include <hardware/pinmapping.h>
#include <icons/menuIcons.h>

enum MENUINPUT {
    BUTTONS,
    ROTARY,
};

Menu* menu;
ESP32Encoder encoder;
#define MENU_INPUT MENUINPUT::ROTARY

unsigned long startMillisEncoderSwitch = 0;
unsigned long EncoderSwitchBackflushInterval = 2000;
unsigned long EncoderSwitchControlInterval = 800;
bool encoderSwitchPressed = false;

// QueueHandle_t button_events;
// button_event_t ev;

int last = 0;

template <typename T>
inline auto makeSaveCallback(const char* param, T& value) {
    return [param, &value]() {
        if (!ParameterRegistry::getInstance().setParameterValue(param, value)) {
            LOG(ERROR, "Failed to save config to filesystem!");
        }
    };
}

/*void saveBrewTemp() {
    if (!ParameterRegistry::getInstance().setParameterValue("brew.setpoint", brewSetpoint)) {
        LOG(ERROR, "Failed to save config to filesystem!");
    }
}

void saveSteamTemp() {
    if (!ParameterRegistry::getInstance().setParameterValue("steam.setpoint", steamSetpoint)) {
        LOG(ERROR, "Failed to save config to filesystem!");
    }
}*/

/*void savePIDOn() {
    sysParaPidOn.setStorage(true);
}

void saveStandby() {
    sysParaStandbyModeOn.setStorage(true);
}

void saveStandbyTime() {
    sysParaStandbyModeTime.setStorage(true);
}

bool hasBrewControl() {
    return FEATURE_BREWCONTROL > 0;
}

bool hasScale() {
    return FEATURE_SCALE > 0;
}

bool hasSoftwareDetection() {
    return BREWDETECTION_TYPE == 1;
}*/

void menuInputInit() {
    switch (MENU_INPUT) {
        /*case MENUINPUT::BUTTONS:
            menuEnterPin = new GPIOPin(PIN_MENU_ENTER, GPIOPin::IN_PULLUP);
            menuUpPin = new GPIOPin(PIN_MENU_OUT_A, GPIOPin::IN_PULLUP);
            menuDownPin = new GPIOPin(PIN_MENU_OUT_B, GPIOPin::IN_PULLUP);

            button_events = pulled_button_init(PIN_BIT(menuEnterPin->getPinNumber()) | PIN_BIT(menuUpPin->getPinNumber()) | PIN_BIT(menuDownPin->getPinNumber()), GPIO_PULLUP_ONLY);
            break;*/
        case MENUINPUT::ROTARY:
            /*menuEnterPin = new GPIOPin(PIN_MENU_ENTER, GPIOPin::IN_PULLUP);
            menuUpPin = new GPIOPin(PIN_MENU_OUT_A, GPIOPin::IN_PULLUP);
            menuDownPin = new GPIOPin(PIN_MENU_OUT_B, GPIOPin::IN_PULLUP);

            button_events = pulled_button_init(PIN_BIT(menuEnterPin->getPinNumber()), GPIO_PULLUP_ONLY);*/

            encoder.useInternalWeakPullResistors = puType::up;
            // encoder.attachHalfQuad(PIN_ROTARY_DT, PIN_ROTARY_CLK);
            encoder.attachFullQuad(PIN_ROTARY_DT, PIN_ROTARY_CLK);
            encoder.setCount(0);

            break;
        default:
            break;
    }
}

void initMenu(U8G2* display) {
    menu = new Menu(*display);

    auto& params = ParameterRegistry::getInstance();

    menuInputInit();

    /* Main Menu */
    menu->AddInputItem("Brew Temp.", "Brew Temperature", "", "°C", BREW_SETPOINT_MIN, BREW_SETPOINT_MAX, makeSaveCallback("brew.setpoint", brewSetpoint), brewSetpoint, bitmap_icon_temp, 0.1, 0.5);
    menu->AddInputItem("Steam Temp.", "Steam Temperature", "", "°C", STEAM_SETPOINT_MIN, STEAM_SETPOINT_MAX, makeSaveCallback("steam.setpoint", steamSetpoint), steamSetpoint, bitmap_icon_steam, 0.1, 0.5);

    // menu->AddToggleItem("PID", savePIDOn, reinterpret_cast<bool&>(pidON), bitmap_icon_pid);

    /*menu->SetEventHandler([&]() {
        if (xQueueReceive(button_events, &ev, 1 / portTICK_PERIOD_MS)) {
            if (ev.pin == menuEnterPin->getPinNumber()) {
                if (standbyModeRemainingTimeMillis == 0) {
                    resetStandbyTimer();
                    display.setPowerSave(0);
                    pidON = 1;
                    if (steamON) {
                        machineState = kSteam;
                    }
                    else if (isBrewDetected) {
                        machineState = kBrew;
                    }
                    else {
                        machineState = kPidDisabled;
                    }
                    return;
                }
                if (ev.event == EventState::STATE_DOWN) {
                    resetStandbyTimer();
                }
                menu->Event(EVENT_ENTER, EventState(ev.event));
            }
            else {
                if (MENU_INPUT == MENUINPUT::BUTTONS) {
                    if (ev.pin == menuUpPin->getPinNumber()) {
                        resetStandbyTimer();
                        menu->Event(EVENT_UP, EventState(ev.event));
                    }
                    else if (ev.pin == menuDownPin->getPinNumber()) {
                        resetStandbyTimer();
                        menu->Event(EVENT_DOWN, EventState(ev.event));
                    }
                }
            }
        }
        if (MENU_INPUT == MENUINPUT::ROTARY) {
            int32_t pos = encoder.getCount() / ENCODER_CLICKS_PER_NOTCH;
            if (pos > last) {
                menu->Event(EVENT_UP, EventState(EventState::STATE_DOWN));
                LOG(DEBUG, "Menu: Up\n");
                menu->Event(EVENT_UP, EventState(EventState::STATE_UP));
            }
            else if (pos < last) {
                menu->Event(EVENT_DOWN, EventState(EventState::STATE_DOWN));
                LOG(DEBUG, "Menu: Down\n");
                menu->Event(EVENT_DOWN, EventState(EventState::STATE_UP));
            }

            last = pos;
        }
    });*/

    /* Brew Weight & Time */
    Menu* weightNTime = new Menu(*display);
    weightNTime->AddInputItem("Brew by Time", "Brew Time", "", " s", TARGET_BREW_TIME_MIN, TARGET_BREW_TIME_MAX, makeSaveCallback("brew.by_time.target_time", targetBrewTime), targetBrewTime, bitmap_icon_clock, 0.1, 0.5,
                              false, config.get<bool>("brew.by_time.enabled"));
    // static double targetBrewWeight = params.getParameterById("brew.by_weight.target_weight")->getValueAs<double>();
    // double targetBrewWeight = params.getParameterById("brew.by_weight.target_weight")->getValueAs<double>();
    // weightNTime->AddInputItem("Brew by Weight", "Brew Weight", "", "g", TARGET_BREW_WEIGHT_MIN, TARGET_BREW_WEIGHT_MAX, makeSaveCallback("brew.by_weight.target_weight", targetBrewWeight), targetBrewWeight,
    // bitmap_icon_scale, 0.1, 0.5, false, config.get<bool>("brew.by_weight.enabled"));

    weightNTime->AddBackItem("Back", bitmap_icon_back);
    menu->AddSubMenu("Brew Time & Weight", *weightNTime, bitmap_icon_clock, (config.get<int>("brew.mode") == 1) && (config.get<bool>("brew.by_time.enabled") || config.get<bool>("brew.by_weight.enabled")));

    /* Preinfusion */
    Menu* preInfusion = new Menu(*display);
    preInfusion->AddInputItem("Preinfusion", "Time", "", "s", PRE_INFUSION_TIME_MIN, PRE_INFUSION_TIME_MAX, makeSaveCallback("brew.pre_infusion.time", preinfusion), preinfusion, 1.0, 2.0, true,
                              config.get<bool>("brew.pre_infusion.enabled"));
    preInfusion->AddInputItem("Preinfusion Pause", "Pause", "", "s", PRE_INFUSION_PAUSE_MIN, PRE_INFUSION_PAUSE_MAX, makeSaveCallback("brew.pre_infusion.pause", preinfusionPause), preinfusionPause, 1.0, 2.0, true,
                              config.get<bool>("brew.pre_infusion.enabled"));
    preInfusion->AddBackItem("Back", bitmap_icon_back);
    menu->AddSubMenu("Preinfusion", *preInfusion, config.get<bool>("brew.pre_infusion.enabled"));

    /*
     * Maintenance Menu
     * */
    Menu* maintenanceMenu = new Menu(*display);
    maintenanceMenu->AddToggleItem("Backflush", reinterpret_cast<bool&>(backflushOn), bitmap_icon_refresh);
    maintenanceMenu->AddBackItem("Back", bitmap_icon_back);

    menu->AddSubMenu("Maintenance", *maintenanceMenu, bitmap_icon_tools, true);

    /*
     * Advanced Menu
     */

    // Menu* advancedMenu = new Menu(*display);
    // advancedMenu->AddInputItem("Brew Temp. Offset", "Brew temp. offset", "", "°C", BREW_TEMP_OFFSET_MIN, BREW_TEMP_OFFSET_MAX, []() { sysParaTempOffset.setStorage(true); }, brewTempOffset, bitmap_icon_temp);
    /*
     * Standby Menu
     */
    /*Menu* standbyMenu = new Menu(*display);
    standbyMenu->AddToggleItem("Standby", saveStandby, reinterpret_cast<bool&>(standbyModeOn), true);
    standbyMenu->AddInputItem("Standby Time", "Standby Time", "", " m", STANDBY_MODE_TIME_MIN, STANDBY_MODE_TIME_MAX, saveStandbyTime, standbyModeTime, bitmap_icon_clock, 1.0, 2.0, true);

    standbyMenu->AddBackItem("Back", bitmap_icon_back);
    advancedMenu->AddSubMenu("Standby", *standbyMenu, bitmap_icon_sleep_mode);*/

    /* PID Settings */
    /*Menu* pidSettings = new Menu(*display);
    pidSettings->AddToggleItem("Enable PonM", []() { sysParaUsePonM.setStorage(true); }, reinterpret_cast<bool&>(usePonM));
    pidSettings->AddInputItem("Start Kp", "Start Kp", "", "", PID_KP_START_MIN, PID_KP_START_MAX, []() { (sysParaPidKpStart.setStorage(true)); }, startKp);
    pidSettings->AddInputItem("Start Tn", "Start Tn", "", "", PID_TN_START_MIN, PID_TN_START_MAX, []() { sysParaPidTnStart.setStorage(true); }, startTn);
    pidSettings->AddInputItem("Kp", "Kp", "", "", PID_KP_REGULAR_MIN, PID_KP_REGULAR_MAX, []() { sysParaPidKpReg.setStorage(true); }, aggKp);
    pidSettings->AddInputItem("Tn", "Tn (=Kp/Ki)", "", "", PID_TN_REGULAR_MIN, PID_TN_REGULAR_MAX, []() { sysParaPidTnReg.setStorage(true); }, aggTn);
    pidSettings->AddInputItem("Tv", "Tv (=Kd/Kp)", "", "", PID_TV_REGULAR_MIN, PID_TV_REGULAR_MAX, []() { sysParaPidTvReg.setStorage(true); }, aggTv);
    pidSettings->AddInputItem("Integrator Max", "Integrator Max", "", "", PID_I_MAX_REGULAR_MIN, PID_I_MAX_REGULAR_MAX, []() { sysParaPidIMaxReg.setStorage(true); }, aggIMax);
    pidSettings->AddInputItem("Steam Kp", "Steam Kp", "", "", PID_KP_STEAM_MIN, PID_KP_STEAM_MAX, []() { sysParaPidKpSteam.setStorage(true); }, steamKp);
*/
    /* Brew PID Settings */
    /*Menu* brewPidSettings = new Menu(*display);
    brewPidSettings->AddToggleItem("Enable Brew PID", []() { sysParaUsePonM.setStorage(true); }, reinterpret_cast<bool&>(useBDPID));
    brewPidSettings->AddInputItem("BD Kp", "BD Kp", "", "", PID_KP_BD_MIN, PID_KP_BD_MAX, []() { sysParaPidKpBd.setStorage(true); }, aggbKp);
    brewPidSettings->AddInputItem("BD Tn", "BD Tn (=Kp/Ki)", "", "", PID_TN_BD_MIN, PID_TN_BD_MAX, []() { sysParaPidTnBd.setStorage(true); }, aggbTn);
    brewPidSettings->AddInputItem("BD Tv", "BD Tv (=Kd/Kp)", "", "", PID_TV_BD_MIN, PID_TV_BD_MAX, []() { sysParaPidTvBd.setStorage(true); }, aggbTv);
    brewPidSettings->AddInputItem("PID BD Time", "PID BD Time", "", "s", BREW_SW_TIME_MIN, BREW_SW_TIME_MAX, []() { sysParaBrewSwTime.setStorage(true); }, brewtimesoftware, hasSoftwareDetection());
    brewPidSettings->AddInputItem("PID BD Sensitivity", "Sensitivity", "", "", BD_THRESHOLD_MIN, BD_THRESHOLD_MAX, []() { sysParaBrewThresh.setStorage(true); }, brewSensitivity, hasSoftwareDetection());
    brewPidSettings->AddBackItem("Back", bitmap_icon_back);

    pidSettings->AddSubMenu("Brew PID", *brewPidSettings);
    pidSettings->AddBackItem("Back", bitmap_icon_back);

    advancedMenu->AddSubMenu("PID Settings", *pidSettings, bitmap_icon_pid);
    advancedMenu->AddBackItem("Back", bitmap_icon_back);
    menu->AddSubMenu("Advanced", *advancedMenu, bitmap_icon_settings);*/

    menu->AddBackItem("Close Menu", bitmap_icon_back);
    menu->Init();
}

int getEncoderDelta(void) {
    static long lastencodervalue = 0;
    long value = encoder.getCount() / 4;
    int delta = value - lastencodervalue;

    if (lastencodervalue != value) {
        LOGF(INFO, "Rotary Encoder Value: %i", value);
    }

    lastencodervalue = value;

    return delta;
}

void menuLoop() {
    if (!config.get<bool>("hardware.switches.encoder.enabled") || encoderSwitch == nullptr) {
        return;
    }

    int delta = getEncoderDelta(); // +1 or -1 or 0

    if (delta > 0) {
        for (int i = 0; i < delta; i++) {
            menu->Event(EVENT_UP, EventState(EventState::STATE_DOWN));
            // LOG(DEBUG, "Menu: Up");
            menu->Event(EVENT_UP, EventState(EventState::STATE_UP));
        }
    }

    if (delta < 0) {
        for (int i = 0; i > delta; i--) {
            menu->Event(EVENT_DOWN, EventState(EventState::STATE_DOWN));
            // LOG(DEBUG, "Menu: Down");
            menu->Event(EVENT_DOWN, EventState(EventState::STATE_UP));
        }
    }

    if (encoderSwitch->isPressed()) {
        if (encoderSwitchPressed == false) {
            encoderSwitchPressed = true;
            menu->Event(EVENT_ENTER, STATE_DOWN);
            LOG(INFO, "Switch Pressed");
        }
    }
    else if (encoderSwitchPressed == true) {
        encoderSwitchPressed = false;
        menu->Event(EVENT_ENTER, STATE_UP);
        LOG(INFO, "Switch Released");
    }

    menu->Loop();
}