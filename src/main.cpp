/**
 * @file main.cpp
 *
 * @brief Main sketch
 *
 * @version 4.0.0 Master
 */

// Firmware version
#define FW_VERSION    4
#define FW_SUBVERSION 0
#define FW_HOTFIX     0
#define FW_BRANCH     "MASTER"

// STL includes
#include <map>

// Libraries & Dependencies
#include "Logger.h"
#include <ArduinoOTA.h>
#include <LittleFS.h>
#include <PID_v1.h>  // for PID calculation
#include <U8g2lib.h> // i2c display
#include <WiFiManager.h>
#include <os.h>
#include <ESP32RotaryEncoder.h>

// Includes
#include "display/bitmaps.h" // user icons for display
#include "languages.h"       // for language translation
#include "storage.h"

// Utilities:
#include "utils/Timer.h"

// Hardware classes
#include "hardware/GPIOPin.h"
#include "hardware/IOSwitch.h"
#include "hardware/LED.h"
#include "hardware/Relay.h"
#include "hardware/StandardLED.h"
#include "hardware/Switch.h"
#include "hardware/TempSensorDallas.h"
#include "hardware/TempSensorTSIC.h"
#include "hardware/TempSensorK.h"
#include "hardware/pinmapping.h"

// User configuration & defaults
#include "defaults.h"
#include "userConfig.h" // needs to be configured by the user

// Version of userConfig need to match, checked by preprocessor
#if (FW_VERSION != USR_FW_VERSION) || (FW_SUBVERSION != USR_FW_SUBVERSION) || (FW_HOTFIX != USR_FW_HOTFIX)
#error Version of userConfig file and main.cpp need to match!
#endif

hw_timer_t* timer = NULL;

#if (TEMP_SENSOR == 1)
#include "OneWire.h"
#endif

#if (FEATURE_PRESSURESENSOR == 1)
#include "hardware/pressureSensor.h"
#include <Wire.h>
#endif

#if (FEATURE_PRESSURESENSOR == 2)
#include "hardware/pressureSensorAds1115.h"
#include <Wire.h>
#endif

#if (FEATURE_PUMP_DIMMER > 0)
#include "hardware/Dimmers.h"
#endif

#if OLED_DISPLAY == 3
#include <SPI.h>
#endif

#if FEATURE_SCALE == 1
#define HX711_ADC_config_h
#define SAMPLES                32
#define IGN_HIGH_SAMPLE        1
#define IGN_LOW_SAMPLE         1
#define SCK_DELAY              1
#define SCK_DISABLE_INTERRUPTS 0
#include <HX711_ADC.h>
#endif

MACHINE machine = (enum MACHINE)MACHINEID;

#define HIGH_ACCURACY

enum MachineState {
    kInit = 0,
    kPidNormal = 20,
    kBrew = 30,
    kManualFlush = 35,
    kSteam = 40,
    kWater = 45,
    kBackflush = 50,
    kWaterTankEmpty = 70,
    kEmergencyStop = 80,
    kPidDisabled = 90,
    kStandby = 95,
    kSensorError = 100,
    kEepromError = 110,
};

MachineState machineState = kInit;
MachineState lastmachinestate = kInit;
MachineState lastmachinestatedebug = kInit;
MachineState lastmachinestatehtml = kInit;
int lastmachinestatepid = -1;

String waterstatedebug = "off";
String lastwaterstatedebug = "off";

// Definitions below must be changed in the userConfig.h file
int connectmode = CONNECTMODE;

int offlineMode = 0;
const boolean ota = OTA;

// Display
uint8_t oled_i2c = OLED_I2C;
uint8_t featureFullscreenBrewTimer = FEATURE_FULLSCREEN_BREW_TIMER;
uint8_t featureFullscreenManualFlushTimer = FEATURE_FULLSCREEN_MANUAL_FLUSH_TIMER;
double postBrewTimerDuration = POST_BREW_TIMER_DURATION;
uint8_t featureHeatingLogo = FEATURE_HEATING_LOGO;
uint8_t featurePidOffLogo = FEATURE_PID_OFF_LOGO;

// WiFi
uint8_t wifiCredentialsSaved = 0;
WiFiManager wm;
const unsigned long wifiConnectionDelay = WIFICONNECTIONDELAY;
const unsigned int maxWifiReconnects = MAXWIFIRECONNECTS;
const char* hostname = HOSTNAME;
const char* pass = PASS;
unsigned long lastWifiConnectionAttempt = millis();
unsigned int wifiReconnects = 0; // actual number of reconnects

// OTA
const char* OTApass = OTAPASS;

// Profiles
#include "brewProfiles.h"
int currentRecipeIndex = 0;
int currentPhaseIndex = 0;
float phaseTiming = 0;
bool debug_recipe = false;
const char*  recipeName = recipes[currentRecipeIndex].name;
const char*  phaseName = "infuse";
double lastPreinfusion = PRE_INFUSION_TIME;
double lastPreinfusionPause = PRE_INFUSION_PAUSE_TIME;
double lastBrewTime = BREW_TIME;


//Pressure Sensor
#if (FEATURE_PRESSURESENSOR == 1)
float inputPressureFilter = 0;
const unsigned long intervalPressure = 100;
unsigned long previousMillisPressure; // initialisation at the end of init()
#endif
#if (FEATURE_PRESSURESENSOR == 2)
float inputPressureFilter = 0;
const unsigned long intervalPressure = 20;
unsigned long previousMillisPressure; // initialisation at the end of init()
#endif

Switch* waterTankSensor;
Switch* encoderSw;

GPIOPin* statusLedPin;
GPIOPin* brewLedPin;
GPIOPin* waterLedPin;
GPIOPin* steamLedPin;

LED* statusLed;
LED* brewLed;
LED* waterLed;
LED* steamLed;

GPIOPin heaterRelayPin(PIN_HEATER, GPIOPin::OUT);
Relay heaterRelay(heaterRelayPin, HEATER_SSR_TYPE);

GPIOPin pumpRelayPin(PIN_PUMP, GPIOPin::OUT);
GPIOPin pumpZCPin(PIN_ZC, GPIOPin::IN_HARDWARE);

#if (FEATURE_PUMP_DIMMER == 0)
    Relay pumpRelay(pumpRelayPin, PUMP_WATER_SSR_TYPE);
#endif

#if (FEATURE_PUMP_DIMMER > 0)
    PumpDimmerCore pumpRelay(pumpRelayPin, pumpZCPin, 1);
    const char* controlMethodToString(PumpDimmerCore::ControlMethod method);
#endif

GPIOPin valveRelayPin(PIN_VALVE, GPIOPin::OUT);
Relay valveRelay(valveRelayPin, PUMP_VALVE_SSR_TYPE);

Switch* powerSwitch;
Switch* brewSwitch;
Switch* steamSwitch;
Switch* waterSwitch;

TempSensor* tempSensor;

RotaryEncoder rotaryEncoder(PIN_ROTARY_CLK, PIN_ROTARY_DT);     //SW interrupt crashes at the moment 05/05/2025

#include "isr.h"

// Method forward declarations
void setSteamMode(int steamMode);
void setPidStatus(int pidStatus);
void setBackflush(int backflush);
void setScaleTare(int tare);
void setScaleCalibration(int tare);
void setNormalPIDTunings();
void setBDPIDTunings();
void loopcalibrate();
void looppid();
void loopLED();
void looppump();
void checkWaterTank();
void printMachineState();
char const* machinestateEnumToString(MachineState machineState);
char* number2string(double in);
char* number2string(float in);
char* number2string(int in);
char* number2string(unsigned int in);
float filterPressureValue(float input);
int writeSysParamsToMQTT(bool continueOnError);
void updateStandbyTimer(void);
void resetStandbyTimer(void);
void wiFiReset(void);
void knobCallback(long);
void buttonCallback(unsigned long);
void printLoopTimingsAsList(void);
void printActivityTimingsAsList(void);
void printLoopPidAsList(void);
void runRecipe(int);

// system parameters
uint8_t pidON = 0;   // 1 = control loop in closed loop
uint8_t usePonM = 0; // 1 = use PonM for cold start PID, 0 = use normal PID for cold start
double brewSetpoint = SETPOINT;
double brewTempOffset = TEMPOFFSET;
double setpoint = brewSetpoint;
double steamSetpoint = STEAMSETPOINT;
double steamKp = STEAMKP;
double startKp = STARTKP;
double startTn = STARTTN;
double aggKp = AGGKP;
double aggTn = AGGTN;
double aggTv = AGGTV;
double aggIMax = AGGIMAX;

// Scale
float scaleCalibration = SCALE_CALIBRATION_FACTOR;
float scale2Calibration = SCALE_CALIBRATION_FACTOR;
float scaleKnownWeight = SCALE_KNOWN_WEIGHT;
double weightSetpoint = SCALE_WEIGHTSETPOINT;

// PID - values for offline brew detection
uint8_t useBDPID = 0;
double aggbKp = AGGBKP;
double aggbTn = AGGBTN;
double aggbTv = AGGBTV;

//Debugging timing
unsigned long previousMicrosDebug = 0;
unsigned long currentMicrosDebug = 0;
unsigned long intervalDebug = 5000000;
unsigned long maxloop = 0;
unsigned long maxActivity = 0;
float maxpressure = 0;
const int LOOP_HISTORY_SIZE = 20;
const int TYPE_HISTORY_SIZE = 9;
unsigned long loopTimings[LOOP_HISTORY_SIZE];
unsigned long maxLoopTimings[LOOP_HISTORY_SIZE];
unsigned int activityLoopTimings[LOOP_HISTORY_SIZE];
unsigned int maxActivityLoopTimings[LOOP_HISTORY_SIZE];
float PidResults[LOOP_HISTORY_SIZE][TYPE_HISTORY_SIZE]; //Output, Target, Flow, FlowTarget, brewWeight, P, I, D, Timing
int loopIndex = 0;
int loopIndexPid = 0;
bool triggered = false;
int triggerCountdown = 0;
bool buffer_ready = false;
bool display_update = false;
bool website_update = false;
bool mqtt_update = false;
bool HASSIO_update = false;

//Encoder
unsigned long currentMillisEncoderSw = 0;
unsigned long startMillisEncoderSw = 0;
unsigned long EncoderSwitchBackflushInterval = 2000;
unsigned long EncoderSwitchControlInterval = 1000;
bool encoderSwPressed = false;
int encodercontrol = ENCODER_CONTROL;

//Pump  //these shouldnt need to be volatile as no interrupts
PumpControl pumpControl = PRESSURE;
float inputPressure = 0;
float pumpFlowRate = 0;
volatile float setPressure = 9.0;
volatile float setPumpFlowRate = 6.0;
volatile float pressureKp = 20.0;//   18.0;//18.0;//13.0;//14.0;//   20.0;    //25.0;//30.0;    // Proportional gain
volatile float pressureKi = 10.0;//  9.0;//8.0;//4.0;//5.0;//   10.0;   //45.0;//75.0;     // Integral gain
volatile float pressureKd = 1.5;//   1.0;//3.0;//7.0;//6.0;//   2.0;   //3.0;       // Derivative gain
volatile float integralAntiWindup = 8.0;  //pumpintegral += error * pumpdt is capped at +-integralAntiWindup, then *pressureKi
volatile int MaxDimmerPower = 100;
volatile float flowKp = 8.0;    //9.0
volatile float flowKi = 30.0;
volatile float flowKd = 0.0;
volatile int DimmerPower = 95;
unsigned long currentMillisPumpControl = 0;
unsigned long previousMillisPumpControl = 0;
unsigned long pumpControlInterval = 50;
int featurePumpDimmer = FEATURE_PUMP_DIMMER;

unsigned long blockMicrosDisplayInterval = 20000;
unsigned long blockMicrosDisplayStart = 0;
static float pumpintegral = 0.0;
float previousError = 0;
const float pumpdt = pumpControlInterval / 1000.0;  // Time step in seconds
 
#if aggbTn == 0
double aggbKi = 0;
#else
double aggbKi = aggbKp / aggbTn;
#endif

double aggbKd = aggbTv * aggbKp;
double brewPIDDelay = BREW_PID_DELAY; // Time PID will be disabled after brew started

uint8_t standbyModeOn = 0;
double standbyModeTime = STANDBY_MODE_TIME;

#include "standby.h"

// Variables to hold PID values (Temp input, Heater output)
double temperature, pidOutput;
int steamON = 0;
int steamFirstON = 0;

#if startTn == 0
double startKi = 0;
#else
double startKi = startKp / startTn;
#endif

#if aggTn == 0
double aggKi = 0;
#else
double aggKi = aggKp / aggTn;
#endif

double aggKd = aggTv * aggKp;

PID bPID(&temperature, &pidOutput, &setpoint, aggKp, aggKi, aggKd, 1, DIRECT);

#include "brewHandler.h"

// system parameter EEPROM storage wrappers (current value as pointer to variable, minimum, maximum, optional storage ID)
SysPara<uint8_t> sysParaPidOn(&pidON, 0, 1, STO_ITEM_PID_ON);
SysPara<uint8_t> sysParaUsePonM(&usePonM, 0, 1, STO_ITEM_PID_START_PONM);
SysPara<double> sysParaPidKpStart(&startKp, PID_KP_START_MIN, PID_KP_START_MAX, STO_ITEM_PID_KP_START);
SysPara<double> sysParaPidTnStart(&startTn, PID_TN_START_MIN, PID_TN_START_MAX, STO_ITEM_PID_TN_START);
SysPara<double> sysParaPidKpReg(&aggKp, PID_KP_REGULAR_MIN, PID_KP_REGULAR_MAX, STO_ITEM_PID_KP_REGULAR);
SysPara<double> sysParaPidTnReg(&aggTn, PID_TN_REGULAR_MIN, PID_TN_REGULAR_MAX, STO_ITEM_PID_TN_REGULAR);
SysPara<double> sysParaPidTvReg(&aggTv, PID_TV_REGULAR_MIN, PID_TV_REGULAR_MAX, STO_ITEM_PID_TV_REGULAR);
SysPara<double> sysParaPidIMaxReg(&aggIMax, PID_I_MAX_REGULAR_MIN, PID_I_MAX_REGULAR_MAX, STO_ITEM_PID_I_MAX_REGULAR);
SysPara<double> sysParaPidKpBd(&aggbKp, PID_KP_BD_MIN, PID_KP_BD_MAX, STO_ITEM_PID_KP_BD);
SysPara<double> sysParaPidTnBd(&aggbTn, PID_TN_BD_MIN, PID_KP_BD_MAX, STO_ITEM_PID_TN_BD);
SysPara<double> sysParaPidTvBd(&aggbTv, PID_TV_BD_MIN, PID_TV_BD_MAX, STO_ITEM_PID_TV_BD);
SysPara<double> sysParaBrewSetpoint(&brewSetpoint, BREW_SETPOINT_MIN, BREW_SETPOINT_MAX, STO_ITEM_BREW_SETPOINT);
SysPara<double> sysParaTempOffset(&brewTempOffset, BREW_TEMP_OFFSET_MIN, BREW_TEMP_OFFSET_MAX, STO_ITEM_BREW_TEMP_OFFSET);
SysPara<double> sysParaBrewPIDDelay(&brewPIDDelay, BREW_PID_DELAY_MIN, BREW_PID_DELAY_MAX, STO_ITEM_BREW_PID_DELAY);
SysPara<uint8_t> sysParaUseBDPID(&useBDPID, 0, 1, STO_ITEM_USE_BD_PID);
SysPara<double> sysParaBrewTime(&brewTime, BREW_TIME_MIN, BREW_TIME_MAX, STO_ITEM_BREW_TIME);
SysPara<uint8_t> sysParaWifiCredentialsSaved(&wifiCredentialsSaved, 0, 1, STO_ITEM_WIFI_CREDENTIALS_SAVED);
SysPara<double> sysParaPreInfTime(&preinfusion, PRE_INFUSION_TIME_MIN, PRE_INFUSION_TIME_MAX, STO_ITEM_PRE_INFUSION_TIME);
SysPara<double> sysParaPreInfPause(&preinfusionPause, PRE_INFUSION_PAUSE_MIN, PRE_INFUSION_PAUSE_MAX, STO_ITEM_PRE_INFUSION_PAUSE);
SysPara<double> sysParaPidKpSteam(&steamKp, PID_KP_STEAM_MIN, PID_KP_STEAM_MAX, STO_ITEM_PID_KP_STEAM);
SysPara<double> sysParaSteamSetpoint(&steamSetpoint, STEAM_SETPOINT_MIN, STEAM_SETPOINT_MAX, STO_ITEM_STEAM_SETPOINT);
SysPara<double> sysParaWeightSetpoint(&weightSetpoint, WEIGHTSETPOINT_MIN, WEIGHTSETPOINT_MAX, STO_ITEM_WEIGHTSETPOINT);
SysPara<uint8_t> sysParaStandbyModeOn(&standbyModeOn, 0, 1, STO_ITEM_STANDBY_MODE_ON);
SysPara<double> sysParaStandbyModeTime(&standbyModeTime, STANDBY_MODE_TIME_MIN, STANDBY_MODE_TIME_MAX, STO_ITEM_STANDBY_MODE_TIME);
SysPara<float> sysParaScaleCalibration(&scaleCalibration, -100000, 100000, STO_ITEM_SCALE_CALIBRATION_FACTOR);
SysPara<float> sysParaScale2Calibration(&scale2Calibration, -100000, 100000, STO_ITEM_SCALE2_CALIBRATION_FACTOR);
SysPara<float> sysParaScaleKnownWeight(&scaleKnownWeight, 0, 2000, STO_ITEM_SCALE_KNOWN_WEIGHT);
SysPara<int> sysParaBackflushCycles(&backflushCycles, BACKFLUSH_CYCLES_MIN, BACKFLUSH_CYCLES_MAX, STO_ITEM_BACKFLUSH_CYCLES);
SysPara<double> sysParaBackflushFillTime(&backflushFillTime, BACKFLUSH_FILL_TIME_MIN, BACKFLUSH_FILL_TIME_MAX, STO_ITEM_BACKFLUSH_FILL_TIME);
SysPara<double> sysParaBackflushFlushTime(&backflushFlushTime, BACKFLUSH_FLUSH_TIME_MIN, BACKFLUSH_FLUSH_TIME_MAX, STO_ITEM_BACKFLUSH_FLUSH_TIME);
SysPara<uint8_t> sysParaFeatureBrewControl(&featureBrewControl, 0, 1, STO_ITEM_FEATURE_BREW_CONTROL);
SysPara<uint8_t> sysParaFeatureFullscreenBrewTimer(&featureFullscreenBrewTimer, 0, 1, STO_ITEM_FEATURE_FULLSCREEN_BREW_TIMER);
SysPara<uint8_t> sysParaFeatureFullscreenManualFlushTimer(&featureFullscreenManualFlushTimer, 0, 1, STO_ITEM_FEATURE_FULLSCREEN_MANUAL_FLUSH_TIMER);
SysPara<double> sysParaPostBrewTimerDuration(&postBrewTimerDuration, POST_BREW_TIMER_DURATION_MIN, POST_BREW_TIMER_DURATION_MAX, STO_ITEM_POST_BREW_TIMER_DURATION);
SysPara<uint8_t> sysParaFeatureHeatingLogo(&featureHeatingLogo, 0, 1, STO_ITEM_FEATURE_HEATING_LOGO);
SysPara<uint8_t> sysParaFeaturePidOffLogo(&featurePidOffLogo, 0, 1, STO_ITEM_FEATURE_PID_OFF_LOGO);

// Other variables
boolean emergencyStop = false;                // Emergency stop if temperature is too high
const double EmergencyStopTemp = 145;         // Temp EmergencyStopTemp
float inX = 0, inY = 0, inOld = 0, inSum = 0; // used for filterPressureValue()
boolean setupDone = false;

// Water tank sensor
boolean waterTankFull = true;
Timer loopWaterTank(&checkWaterTank, 200); // Check water tank level every 200 ms
int waterTankCheckConsecutiveReads = 0;    // Counter for consecutive readings of water tank sensor
const int waterTankCountsNeeded = 3;       // Number of same readings to change water tank sensing

// PID controller
unsigned long previousMillistemp; // initialisation at the end of init()

double setpointTemp;
double previousInput = 0;

// Embedded HTTP Server
#include "embeddedWebserver.h"

enum SectionNames {
    sPIDSection,
    sTempSection,
    sBrewPidSection,
    sBrewSection,
    sScaleSection,
    sDisplaySection,
    sMaintenanceSection,
    sPowerSection,
    sOtherSection
};

std::map<String, editable_t> editableVars = {};

struct cmp_str {
        bool operator()(char const* a, char const* b) const {
            return strcmp(a, b) < 0;
        }
};

// MQTT
#include "mqtt.h"

std::map<const char*, std::function<editable_t*()>, cmp_str> mqttVars = {};
std::map<const char*, std::function<double()>, cmp_str> mqttSensors = {};

unsigned long lastTempEvent = 0;
unsigned long tempEventInterval = 1000;
unsigned long lastBrewEvent = 0;
unsigned long brewEventInterval = 100;

#if MQTT_HASSIO_SUPPORT == 1
Timer hassioDiscoveryTimer(&sendHASSIODiscoveryMsg, 300000);
#endif

bool mqtt_was_connected = false;

/**
 * @brief Get Wifi signal strength and set signalBars for display
 */
int getSignalStrength() {
    if (offlineMode == 1) return 0;

    long rssi;

    if (WiFi.status() == WL_CONNECTED) {
        rssi = WiFi.RSSI();
    }
    else {
        rssi = -100;
    }

    if (rssi >= -50) {
        return 4;
    }
    else if (rssi < -50 && rssi >= -65) {
        return 3;
    }
    else if (rssi < -65 && rssi >= -75) {
        return 2;
    }
    else if (rssi < -75 && rssi >= -80) {
        return 1;
    }
    else {
        return 0;
    }
}

// Display define & template
#if OLED_DISPLAY == 1
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, PIN_I2CSCL, PIN_I2CSDA);  // e.g. 1.3"
#endif
#if OLED_DISPLAY == 2
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, PIN_I2CSCL, PIN_I2CSDA); // e.g. 0.96"
#endif
#if OLED_DISPLAY == 3
#define OLED_CS 5
#define OLED_DC 2
U8G2_SH1106_128X64_NONAME_F_4W_HW_SPI u8g2(U8G2_R0, OLED_CS, OLED_DC, /* reset=*/U8X8_PIN_NONE); // e.g. 1.3"
#endif

// Horizontal or vertical display
#if (OLED_DISPLAY != 0)
#if (DISPLAYTEMPLATE < 20) // horizontal templates
#include "display/displayCommon.h"
#endif

#if (DISPLAYTEMPLATE >= 20) // vertical templates
#include "display/displayRotateUpright.h"
#endif

#if (DISPLAYTEMPLATE == 1)
#include "display/displayTemplateStandard.h"
#elif (DISPLAYTEMPLATE == 2)
#include "display/displayTemplateMinimal.h"
#elif (DISPLAYTEMPLATE == 3)
#include "display/displayTemplateTempOnly.h"
#elif (DISPLAYTEMPLATE == 4)
#include "display/displayTemplateScale.h"
#elif (DISPLAYTEMPLATE == 20)
#include "display/displayTemplateUpright.h"
#endif
Timer printDisplayTimer(&printScreen, 100);
#endif

//initialise water switch variable
int waterON = 0;

#include "powerHandler.h"
#include "scaleHandler.h"
#include "steamHandler.h"
#include "waterHandler.h"

// Emergency stop if temp is too high
void testEmergencyStop() {
    if (temperature > EmergencyStopTemp && emergencyStop == false) {
        emergencyStop = true;
    }
    else if (temperature < (brewSetpoint + 5) && emergencyStop == true) {
        emergencyStop = false;
    }
}

/**
 * @brief Switch to offline mode if maxWifiReconnects were exceeded during boot
 */
void initOfflineMode() {
#if OLED_DISPLAY != 0
    displayMessage("", "", "", "", "Begin Fallback,", "No Wifi");
#endif

    LOG(INFO, "Start offline mode with eeprom values, no wifi :(");
    offlineMode = 1;

    if (readSysParamsFromStorage() != 0) {
#if OLED_DISPLAY != 0
        displayMessage("", "", "", "", "No eeprom,", "Values");
#endif

        LOG(INFO, "No working eeprom value, I am sorry, but use default offline value :)");
        delay(1000);
    }
}

/**
 * @brief Check if Wifi is connected, if not reconnect abort function if offline, or brew is running
 */
void checkWifi() {
    if (offlineMode == 1 || currBrewState > kBrewIdle) return;

    // There was no WIFI connection at boot -> connect and if it does not succeed, enter offline mode
    do {
        if ((millis() - lastWifiConnectionAttempt >= wifiConnectionDelay) && (wifiReconnects <= maxWifiReconnects)) {
            int statusTemp = WiFi.status();

            if (statusTemp != WL_CONNECTED) { // check WiFi connection status
                lastWifiConnectionAttempt = millis();
                wifiReconnects++;
                LOGF(INFO, "Attempting WIFI (re-)connection: %i", wifiReconnects);

                if (!setupDone) {
#if OLED_DISPLAY != 0
                    displayMessage("", "", "", "", langstring_wifirecon, String(wifiReconnects));
#endif
                }

                wm.disconnect();
                wm.autoConnect();

                int count = 1;

                while (WiFi.status() != WL_CONNECTED && count <= 20) {
                    delay(100); // give WIFI some time to connect
                    count++;    // reconnect counter, maximum waiting time for reconnect = 20*100ms
                }
            }
        }

        yield(); // Prevent WDT trigger
    } while (!setupDone && wifiReconnects < maxWifiReconnects && WiFi.status() != WL_CONNECTED);

    if (wifiReconnects >= maxWifiReconnects && WiFi.status() != WL_CONNECTED) {
        // no wifi connection after trying connection, initiate offline mode
        initOfflineMode();
    }
    else {
        wifiReconnects = 0;
    }
}

char number2string_double[22];

char* number2string(double in) {
    snprintf(number2string_double, sizeof(number2string_double), "%0.2f", in);

    return number2string_double;
}

char number2string_float[22];

char* number2string(float in) {
    snprintf(number2string_float, sizeof(number2string_float), "%0.2f", in);

    return number2string_float;
}

char number2string_int[22];

char* number2string(int in) {
    snprintf(number2string_int, sizeof(number2string_int), "%d", in);

    return number2string_int;
}

char number2string_uint[22];

char* number2string(unsigned int in) {
    snprintf(number2string_uint, sizeof(number2string_uint), "%u", in);

    return number2string_uint;
}

/**
 * @brief Filter input value using exponential moving average filter (using fixed coefficients)
 *      After ~28 cycles the input is set to 99,66% if the real input value sum of inX and inY
 *      multiplier must be 1 increase inX multiplier to make the filter faster
 */
float filterPressureValue(float input) {
    inX = input * 0.2;//0.3;
    inY = inOld * 0.8;//0.7;
    inSum = inX + inY;
    inOld = inSum;

    return inSum;
}

/**
 * @brief Handle the different states of the machine
 */
void handleMachineState() {
    switch (machineState) {
        case kInit:
            if (!waterTankFull) {
                machineState = kWaterTankEmpty;
            }

            if (tempSensor->hasError()) {
                machineState = kSensorError;
            }

            if (pidON == 0) {
                machineState = kPidDisabled;
            }
            else {
                machineState = kPidNormal;
            }

            break;

        case kPidNormal:

            if (brew()) {
                machineState = kBrew;

                if (standbyModeOn) {
                    resetStandbyTimer();
                }
            }

            if (manualFlush()) {
                machineState = kManualFlush;

                if (standbyModeOn) {
                    resetStandbyTimer();
                }
            }

            if (backflushOn) {
                machineState = kBackflush;

                if (standbyModeOn) {
                    resetStandbyTimer();
                }
            }

            if (steamON == 1) {
                machineState = kSteam;

                if (standbyModeOn) {
                    resetStandbyTimer();
                }
            }

            if (waterON == 1) {
                machineState = kWater;
                if (standbyModeOn) {
                    resetStandbyTimer();
                }
            }

            if (emergencyStop) {
                machineState = kEmergencyStop;
            }

            if (standbyModeOn && standbyModeRemainingTimeMillis == 0) {
                machineState = kStandby;
                pidON = 0;
            }

            if (pidON == 0 && machineState != kStandby) {
                machineState = kPidDisabled;
            }

            if (!waterTankFull) {
                machineState = kWaterTankEmpty;
            }

            if (tempSensor->hasError()) {
                machineState = kSensorError;
            }

            break;

        case kBrew:

            if (!brew()) {
                machineState = kPidNormal;
            }

            if (emergencyStop) {
                machineState = kEmergencyStop;
            }

            if (pidON == 0) {
                machineState = kPidDisabled;
            }

            if (tempSensor->hasError()) {
                machineState = kSensorError;
            }

            if (machineState != kBrew) {
                MQTTReCnctCount = 0;    //allow MQTT to try to reconnect if exiting brew mode
            }
            break;

        case kManualFlush:

            if (!manualFlush()) {
                machineState = kPidNormal;
            }

            if (emergencyStop) {
                machineState = kEmergencyStop;
            }

            if (pidON == 0) {
                machineState = kPidDisabled;
            }

            if (tempSensor->hasError()) {
                machineState = kSensorError;
            }
            break;

        case kSteam:
            if (steamON == 0) {
                machineState = kPidNormal;
            }

            if (emergencyStop) {
                machineState = kEmergencyStop;
            }

            if (pidON == 0) {
                machineState = kPidDisabled;
            }

            if (tempSensor->hasError()) {
                machineState = kSensorError;
            }
            break;

        case kWater:
            if (waterON == 0) {
                machineState = kPidNormal;
            }

            if (steamON == 1) {
                machineState = kSteam;

                if (standbyModeOn) {
                    resetStandbyTimer();
                }
            }

            if (emergencyStop) {
                machineState = kEmergencyStop;
            }

            if (pidON == 0) {
                machineState = kPidDisabled;
            }

            if (!waterTankFull) {
                machineState = kWaterTankEmpty;
            }

            if (tempSensor->hasError()) {
                machineState = kSensorError;
            }
            break;

        case kBackflush:

            backflush();

            if (backflushOn == 0) {
                machineState = kPidNormal;
            }

            if (emergencyStop) {
                machineState = kEmergencyStop;
            }

            if (pidON == 0) {
                machineState = kPidDisabled;
            }

            if (!waterTankFull && (currBackflushState == kBackflushIdle || currBackflushState == kBackflushFinished)) {
                machineState = kWaterTankEmpty;
            }

            if (tempSensor->hasError()) {
                machineState = kSensorError;
            }
            break;

        case kEmergencyStop:
            if (!emergencyStop) {
                machineState = kPidNormal;
            }

            if (pidON == 0) {
                machineState = kPidDisabled;
            }

            if (tempSensor->hasError()) {
                machineState = kSensorError;
            }
            break;

        case kWaterTankEmpty:
            if (waterTankFull) {
                machineState = kPidNormal;

                if (standbyModeOn) {
                    resetStandbyTimer();
                }
            }

            if (pidON == 0) {
                machineState = kPidDisabled;
            }

            if (tempSensor->hasError()) {
                machineState = kSensorError;
            }
            break;

        case kPidDisabled:
            if (pidON == 1) {
                machineState = kPidNormal;
            }

            if (tempSensor->hasError()) {
                machineState = kSensorError;
            }

            break;

        case kStandby:
            if (standbyModeRemainingTimeDisplayOffMillis == 0) {
#if OLED_DISPLAY != 0
                u8g2.setPowerSave(1);
#endif
            }

            if (pidON) {
                pidON = 1;
                resetStandbyTimer();
#if OLED_DISPLAY != 0
                u8g2.setPowerSave(0);
#endif
                machineState = kPidNormal;
            }
            if (steamON) {
                pidON = 1;
                resetStandbyTimer();
#if OLED_DISPLAY != 0
                u8g2.setPowerSave(0);
#endif
                machineState = kSteam;
            }
            if (waterON) {
                pidON = 1;
                resetStandbyTimer();
#if OLED_DISPLAY != 0
                u8g2.setPowerSave(0);
#endif
                machineState = kWater;
            }

            if (brew()) {
                pidON = 1;
                resetStandbyTimer();
#if OLED_DISPLAY != 0
                u8g2.setPowerSave(0);
#endif
                machineState = kBrew;
            }

            if (manualFlush()) {
                pidON = 1;
                resetStandbyTimer();
#if OLED_DISPLAY != 0
                u8g2.setPowerSave(0);
#endif
                machineState = kManualFlush;
            }

            if (backflushOn) {
                resetStandbyTimer();
#if OLED_DISPLAY != 0
                u8g2.setPowerSave(0);
#endif
                machineState = kBackflush;
            }

            if (tempSensor->hasError()) {
#if OLED_DISPLAY != 0
                u8g2.setPowerSave(0);
#endif
                machineState = kSensorError;
            }
            if (machineState != kStandby) {
                MQTTReCnctCount = 0;    //allow MQTT to try to reconnect if exiting standby, maybe just make this every 10mins or so not exiting standby
            }
            break;

        case kSensorError:
            machineState = kSensorError;
            break;

        case kEepromError:
            machineState = kEepromError;
            break;
    }

    if (machineState != lastmachinestate) {
        printMachineState();
        lastmachinestate = machineState;
    }
}

void printMachineState() {
    LOGF(DEBUG, "new machineState: %s -> %s", machinestateEnumToString(lastmachinestate), machinestateEnumToString(machineState));
}

char const* machinestateEnumToString(MachineState machineState) {
    switch (machineState) {
        case kInit:
            return "Init";
        case kPidNormal:
            return "PID Normal";
        case kBrew:
            return "Brew";
        case kManualFlush:
            return "Manual Flush";
        case kSteam:
            return "Steam";
        case kWater:
            return "Water";
        case kBackflush:
            return "Backflush";
        case kWaterTankEmpty:
            return "Water Tank Empty";
        case kEmergencyStop:
            return "Emergency Stop";
        case kPidDisabled:
            return "PID Disabled";
        case kStandby:
            return "Standby Mode";
        case kSensorError:
            return "Sensor Error";
        case kEepromError:
            return "EEPROM Error";
    }

    return "Unknown";
}

/**
 * @brief Set up internal WiFi hardware
 */
void wiFiSetup() {

    wm.setCleanConnect(true);
    wm.setConfigPortalTimeout(60); // sec timeout for captive portal
    wm.setConnectTimeout(10);      // using 10s to connect to WLAN, 5s is sometimes too short!
    wm.setBreakAfterConfig(true);
    wm.setConnectRetries(3);

    sysParaWifiCredentialsSaved.getStorage();

    if (wifiCredentialsSaved == 0) {
        const char hostname[] = (STR(HOSTNAME));
        LOGF(INFO, "Connecting to WiFi: %s", String(hostname));

#if OLED_DISPLAY != 0
        displayLogo("Connecting to: ", HOSTNAME);
#endif
    }

    wm.setHostname(hostname);

    if (wm.autoConnect(hostname, pass)) {
        wifiCredentialsSaved = 1;
        sysParaWifiCredentialsSaved.setStorage();
        storageCommit();
        LOGF(INFO, "WiFi connected - IP = %i.%i.%i.%i", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);
        byte mac[6];
        WiFi.macAddress(mac);
        String macaddr0 = number2string(mac[0]);
        String macaddr1 = number2string(mac[1]);
        String macaddr2 = number2string(mac[2]);
        String macaddr3 = number2string(mac[3]);
        String macaddr4 = number2string(mac[4]);
        String macaddr5 = number2string(mac[5]);
        String completemac = macaddr0 + macaddr1 + macaddr2 + macaddr3 + macaddr4 + macaddr5;
        LOGF(DEBUG, "MAC-ADDRESS: %s", completemac.c_str());
    }
    else {
        LOG(INFO, "WiFi connection timed out...");

#if OLED_DISPLAY != 0
        displayLogo(langstring_nowifi[0], langstring_nowifi[1]);
#endif

        wm.disconnect();
        delay(1000);

        offlineMode = 1;
    }

#if OLED_DISPLAY != 0
    displayLogo(langstring_connectwifi1, wm.getWiFiSSID(true));
#endif
}

void wiFiReset() {
    wm.resetSettings();
    ESP.restart();
}

/**
 * @brief Set up embedded Website
 */
void websiteSetup() {
    setEepromWriteFcn(writeSysParamsToStorage);

    readSysParamsFromStorage();

    serverSetup();
}

const char sysVersion[] = (STR(FW_VERSION) "." STR(FW_SUBVERSION) "." STR(FW_HOTFIX) " " FW_BRANCH " " AUTO_VERSION);

void setup() {
    // Start serial console
    Serial.begin(115200);

    // Initialize the logger
    Logger::init(23);

    editableVars["PID_ON"] = {
        .displayName = "Enable PID Controller", .hasHelpText = false, .helpText = "", .type = kUInt8, .section = sPIDSection, .position = 1, .show = [] { return true; }, .minValue = 0, .maxValue = 1, .ptr = (void*)&pidON};

    editableVars["START_USE_PONM"] = {.displayName = F("Enable PonM"),
                                      .hasHelpText = true,
                                      .helpText = F("Use PonM mode (<a href='http://brettbeauregard.com/blog/2017/06/"
                                                    "introducing-proportional-on-measurement/' "
                                                    "target='_blank'>details</a>) while heating up the machine. "
                                                    "Otherwise, just use the same PID values that are used later"),
                                      .type = kUInt8,
                                      .section = sPIDSection,
                                      .position = 2,
                                      .show = [] { return true; },
                                      .minValue = 0,
                                      .maxValue = 1,
                                      .ptr = (void*)&usePonM};

    editableVars["START_KP"] = {.displayName = F("Start Kp"),
                                .hasHelpText = true,
                                .helpText = F("Proportional gain for cold start controller. This value is not "
                                              "used with the the error as usual but the absolute value of the "
                                              "temperature and counteracts the integral part as the temperature "
                                              "rises. Ideally, both parameters are set so that they balance each "
                                              "other out when the target temperature is reached."),
                                .type = kDouble,
                                .section = sPIDSection,
                                .position = 3,
                                .show = [] { return true && usePonM; },
                                .minValue = PID_KP_START_MIN,
                                .maxValue = PID_KP_START_MAX,
                                .ptr = (void*)&startKp};

    editableVars["START_TN"] = {.displayName = F("Start Tn"),
                                .hasHelpText = true,
                                .helpText = F("Integral gain for cold start controller (PonM mode, <a "
                                              "href='http://brettbeauregard.com/blog/2017/06/"
                                              "introducing-proportional-on-measurement/' target='_blank'>details</a>)"),
                                .type = kDouble,
                                .section = sPIDSection,
                                .position = 4,
                                .show = [] { return true && usePonM; },
                                .minValue = PID_TN_START_MIN,
                                .maxValue = PID_TN_START_MAX,
                                .ptr = (void*)&startTn};

    editableVars["PID_KP"] = {.displayName = F("PID Kp"),
                              .hasHelpText = true,
                              .helpText = F("Proportional gain (in Watts/C°) for the main PID controller (in "
                                            "P-Tn-Tv form, <a href='http://testcon.info/EN_BspPID-Regler.html#strukturen' "
                                            "target='_blank'>Details<a>). The higher this value is, the "
                                            "higher is the output of the heater for a given temperature "
                                            "difference. E.g. 5°C difference will result in P*5 Watts of heater output."),
                              .type = kDouble,
                              .section = sPIDSection,
                              .position = 5,
                              .show = [] { return true; },
                              .minValue = PID_KP_REGULAR_MIN,
                              .maxValue = PID_KP_REGULAR_MAX,
                              .ptr = (void*)&aggKp};

    editableVars["PID_TN"] = {.displayName = F("PID Tn (=Kp/Ki)"),
                              .hasHelpText = true,
                              .helpText = F("Integral time constant (in seconds) for the main PID controller "
                                            "(in P-Tn-Tv form, <a href='http://testcon.info/EN_BspPID-Regler.html#strukturen' "
                                            "target='_blank'>Details<a>). The larger this value is, the slower the "
                                            "integral part of the PID will increase (or decrease) if the "
                                            "process value remains above (or below) the setpoint in spite of "
                                            "proportional action. The smaller this value, the faster the integral term changes."),
                              .type = kDouble,
                              .section = sPIDSection,
                              .position = 6,
                              .show = [] { return true; },
                              .minValue = PID_TN_REGULAR_MIN,
                              .maxValue = PID_TN_REGULAR_MAX,
                              .ptr = (void*)&aggTn};

    editableVars["PID_TV"] = {.displayName = F("PID Tv (=Kd/Kp)"),
                              .hasHelpText = true,
                              .helpText = F("Differential time constant (in seconds) for the main PID controller (in P-Tn-Tv form, <a "
                                            "href='http://testcon.info/EN_BspPID-Regler.html#strukturen' target='_blank'>Details<a>). "
                                            "This value determines how far the PID equation projects the current trend into the future. "
                                            "The higher the value, the greater the dampening. Select it carefully, it can cause oscillations "
                                            "if it is set too high or too low."),
                              .type = kDouble,
                              .section = sPIDSection,
                              .position = 7,
                              .show = [] { return true; },
                              .minValue = PID_TV_REGULAR_MIN,
                              .maxValue = PID_TV_REGULAR_MAX,
                              .ptr = (void*)&aggTv};

    editableVars["PID_I_MAX"] = {.displayName = F("PID Integrator Max"),
                                 .hasHelpText = true,
                                 .helpText = F("Internal integrator limit to prevent windup (in Watts). This will allow the integrator to only grow to "
                                               "the specified value. This should be approximally equal to the output needed to hold the temperature after the "
                                               "setpoint has been reached and is depending on machine type and whether the boiler is insulated or not."),
                                 .type = kDouble,
                                 .section = sPIDSection,
                                 .position = 8,
                                 .show = [] { return true; },
                                 .minValue = PID_I_MAX_REGULAR_MIN,
                                 .maxValue = PID_I_MAX_REGULAR_MAX,
                                 .ptr = (void*)&aggIMax};

    editableVars["STEAM_KP"] = {.displayName = F("Steam Kp"),
                                .hasHelpText = true,
                                .helpText = F("Proportional gain for the steaming mode (I or D are not used)"),
                                .type = kDouble,
                                .section = sPIDSection,
                                .position = 9,
                                .show = [] { return true; },
                                .minValue = PID_KP_STEAM_MIN,
                                .maxValue = PID_KP_STEAM_MAX,
                                .ptr = (void*)&steamKp};

    editableVars["TEMP"] = {.displayName = F("Temperature"),
                            .hasHelpText = false,
                            .helpText = "",
                            .type = kDouble,
                            .section = sPIDSection,
                            .position = 10,
                            .show = [] { return false; },
                            .minValue = 0,
                            .maxValue = 200,
                            .ptr = (void*)&temperature};

    editableVars["BREW_SETPOINT"] = {.displayName = F("Set point (°C)"),
                                     .hasHelpText = true,
                                     .helpText = F("The temperature that the PID will attempt to reach and hold"),
                                     .type = kDouble,
                                     .section = sTempSection,
                                     .position = 11,
                                     .show = [] { return true; },
                                     .minValue = BREW_SETPOINT_MIN,
                                     .maxValue = BREW_SETPOINT_MAX,
                                     .ptr = (void*)&brewSetpoint};

    editableVars["BREW_TEMP_OFFSET"] = {.displayName = F("Offset (°C)"),
                                        .hasHelpText = true,
                                        .helpText = F("Optional offset that is added to the user-visible "
                                                      "setpoint. Can be used to compensate sensor offsets and "
                                                      "the average temperature loss between boiler and group "
                                                      "so that the setpoint represents the approximate brew temperature."),
                                        .type = kDouble,
                                        .section = sTempSection,
                                        .position = 12,
                                        .show = [] { return true; },
                                        .minValue = BREW_TEMP_OFFSET_MIN,
                                        .maxValue = BREW_TEMP_OFFSET_MAX,
                                        .ptr = (void*)&brewTempOffset};

    editableVars["STEAM_SETPOINT"] = {.displayName = F("Steam Set point (°C)"),
                                      .hasHelpText = true,
                                      .helpText = F("The temperature that the PID will use for steam mode"),
                                      .type = kDouble,
                                      .section = sTempSection,
                                      .position = 13,
                                      .show = [] { return true; },
                                      .minValue = STEAM_SETPOINT_MIN,
                                      .maxValue = STEAM_SETPOINT_MAX,
                                      .ptr = (void*)&steamSetpoint};

    editableVars["BREWCONTROL"] = {.displayName = F("Brew Control"),
                                   .hasHelpText = true,
                                   .helpText = F("Enables brew-by-time or brew-by-weight"),
                                   .type = kUInt8,
                                   .section = sBrewSection,
                                   .position = 14,
                                   .show = [] { return true && FEATURE_BREWSWITCH == 1; },
                                   .minValue = false,
                                   .maxValue = true,
                                   .ptr = (void*)&featureBrewControl};

    editableVars["BREW_TIME"] = {.displayName = F("Brew Time (s)"),
                                 .hasHelpText = true,
                                 .helpText = F("Stop brew after this time. Set to 0 to deactivate brew-by-time-feature."),
                                 .type = kDouble,
                                 .section = sBrewSection,
                                 .position = 15,
                                 .show = [] { return true && featureBrewControl == 1; },
                                 .minValue = BREW_TIME_MIN,
                                 .maxValue = BREW_TIME_MAX,
                                 .ptr = (void*)&brewTime};

    editableVars["BREW_PREINFUSIONPAUSE"] = {.displayName = F("Preinfusion Pause Time (s)"),
                                             .hasHelpText = false,
                                             .helpText = "",
                                             .type = kDouble,
                                             .section = sBrewSection,
                                             .position = 16,
                                             .show = [] { return true && featureBrewControl == 1; },
                                             .minValue = PRE_INFUSION_PAUSE_MIN,
                                             .maxValue = PRE_INFUSION_PAUSE_MAX,
                                             .ptr = (void*)&preinfusionPause};

    editableVars["BREW_PREINFUSION"] = {.displayName = F("Preinfusion Time (s)"),
                                        .hasHelpText = false,
                                        .helpText = "",
                                        .type = kDouble,
                                        .section = sBrewSection,
                                        .position = 17,
                                        .show = [] { return true && featureBrewControl == 1; },
                                        .minValue = PRE_INFUSION_TIME_MIN,
                                        .maxValue = PRE_INFUSION_TIME_MAX,
                                        .ptr = (void*)&preinfusion};

    editableVars["BACKFLUSH_CYCLES"] = {.displayName = F("Backflush Cycles"),
                                        .hasHelpText = true,
                                        .helpText = "Number of cycles of filling and flushing during a backflush",
                                        .type = kInteger,
                                        .section = sMaintenanceSection,
                                        .position = 18,
                                        .show = [] { return true && featureBrewControl == 1; },
                                        .minValue = BACKFLUSH_CYCLES_MIN,
                                        .maxValue = BACKFLUSH_CYCLES_MAX,
                                        .ptr = (void*)&backflushCycles};

    editableVars["BACKFLUSH_FILL_TIME"] = {.displayName = F("Backflush Fill Time (s)"),
                                           .hasHelpText = true,
                                           .helpText = "Time in seconds the pump is running during one backflush cycle",
                                           .type = kDouble,
                                           .section = sMaintenanceSection,
                                           .position = 19,
                                           .show = [] { return true && featureBrewControl == 1; },
                                           .minValue = BACKFLUSH_FILL_TIME_MIN,
                                           .maxValue = BACKFLUSH_FILL_TIME_MAX,
                                           .ptr = (void*)&backflushFillTime};

    editableVars["BACKFLUSH_FLUSH_TIME"] = {.displayName = F("Backflush Flush Time (s)"),
                                            .hasHelpText = true,
                                            .helpText = "Time in seconds the selenoid valve stays open during one backflush cycle",
                                            .type = kDouble,
                                            .section = sMaintenanceSection,
                                            .position = 20,
                                            .show = [] { return true && featureBrewControl == 1; },
                                            .minValue = BACKFLUSH_FLUSH_TIME_MIN,
                                            .maxValue = BACKFLUSH_FLUSH_TIME_MAX,
                                            .ptr = (void*)&backflushFlushTime};

    editableVars["SCALE_WEIGHTSETPOINT"] = {.displayName = F("Brew weight setpoint (g)"),
                                            .hasHelpText = true,
                                            .helpText = F("Brew is running until this weight has been measured. Set to 0 to deactivate brew-by-weight-feature."),
                                            .type = kDouble,
                                            .section = sBrewSection,
                                            .position = 21,
                                            .show = [] { return true && FEATURE_SCALE == 1 && featureBrewControl == 1; },
                                            .minValue = WEIGHTSETPOINT_MIN,
                                            .maxValue = WEIGHTSETPOINT_MAX,
                                            .ptr = (void*)&weightSetpoint};

    editableVars["PID_BD_DELAY"] = {.displayName = F("Brew PID Delay (s)"),
                                    .hasHelpText = true,
                                    .helpText = F("Delay time in seconds during which the PID will be "
                                                  "disabled once a brew is detected. This prevents too "
                                                  "high brew temperatures with boiler machines like Rancilio "
                                                  "Silvia. Set to 0 for thermoblock machines."),
                                    .type = kDouble,
                                    .section = sBrewPidSection,
                                    .position = 22,
                                    .show = [] { return true; },
                                    .minValue = BREW_PID_DELAY_MIN,
                                    .maxValue = BREW_PID_DELAY_MAX,
                                    .ptr = (void*)&brewPIDDelay};

    editableVars["PID_BD_ON"] = {.displayName = F("Enable Brew PID"),
                                 .hasHelpText = true,
                                 .helpText = F("Use separate PID parameters while brew is running"),
                                 .type = kUInt8,
                                 .section = sBrewPidSection,
                                 .position = 23,
                                 .show = [] { return true && FEATURE_BREWSWITCH == 1; },
                                 .minValue = 0,
                                 .maxValue = 1,
                                 .ptr = (void*)&useBDPID};

    editableVars["PID_BD_KP"] = {.displayName = F("BD Kp"),
                                 .hasHelpText = true,
                                 .helpText = F("Proportional gain (in Watts/°C) for the PID when brewing has been "
                                               "detected. Use this controller to either increase heating during the "
                                               "brew to counter temperature drop from fresh cold water in the boiler. "
                                               "Some machines, e.g. Rancilio Silvia, actually need to heat less or not "
                                               "at all during the brew because of high temperature stability "
                                               "(<a href='https://www.kaffee-netz.de/threads/"
                                               "installation-eines-temperatursensors-in-silvia-bruehgruppe.111093/"
                                               "#post-1453641' target='_blank'>Details<a>)"),
                                 .type = kDouble,
                                 .section = sBrewPidSection,
                                 .position = 24,
                                 .show = [] { return true && FEATURE_BREWSWITCH == 1 && useBDPID; },
                                 .minValue = PID_KP_BD_MIN,
                                 .maxValue = PID_KP_BD_MAX,
                                 .ptr = (void*)&aggbKp};

    editableVars["PID_BD_TN"] = {.displayName = F("BD Tn (=Kp/Ki)"),
                                 .hasHelpText = true,
                                 .helpText = F("Integral time constant (in seconds) for the PID when "
                                               "brewing has been detected."),
                                 .type = kDouble,
                                 .section = sBrewPidSection,
                                 .position = 25,
                                 .show = [] { return true && FEATURE_BREWSWITCH == 1 && useBDPID; },
                                 .minValue = PID_TN_BD_MIN,
                                 .maxValue = PID_TN_BD_MAX,
                                 .ptr = (void*)&aggbTn};

    editableVars["PID_BD_TV"] = {.displayName = F("BD Tv (=Kd/Kp)"),
                                 .hasHelpText = true,
                                 .helpText = F("Differential time constant (in seconds) for the PID "
                                               "when brewing has been detected."),
                                 .type = kDouble,
                                 .section = sBrewPidSection,
                                 .position = 26,
                                 .show = [] { return true && FEATURE_BREWSWITCH == 1 && useBDPID; },
                                 .minValue = PID_TV_BD_MIN,
                                 .maxValue = PID_TV_BD_MAX,
                                 .ptr = (void*)&aggbTv};

    editableVars["STEAM_MODE"] = {
        .displayName = F("Steam Mode"), .hasHelpText = false, .helpText = "", .type = kUInt8, .section = sOtherSection, .position = 27, .show = [] { return false; }, .minValue = 0, .maxValue = 1, .ptr = (void*)&steamON};

    editableVars["BACKFLUSH_ON"] = {
        .displayName = F("Backflush"), .hasHelpText = false, .helpText = "", .type = kUInt8, .section = sOtherSection, .position = 28, .show = [] { return false; }, .minValue = 0, .maxValue = 1, .ptr = (void*)&backflushOn};

    editableVars["STANDBY_MODE_ON"] = {.displayName = F("Enable Standby Timer"),
                                       .hasHelpText = true,
                                       .helpText = F("Turn heater off after standby time has elapsed."),
                                       .type = kUInt8,
                                       .section = sPowerSection,
                                       .position = 29,
                                       .show = [] { return true; },
                                       .minValue = 0,
                                       .maxValue = 1,
                                       .ptr = (void*)&standbyModeOn};

    editableVars["STANDBY_MODE_TIMER"] = {.displayName = F("Standby Time"),
                                          .hasHelpText = true,
                                          .helpText = F("Time in minutes until the heater is turned off. Timer is reset by brew, manual flush, backflush and steam."),
                                          .type = kDouble,
                                          .section = sPowerSection,
                                          .position = 30,
                                          .show = [] { return true; },
                                          .minValue = STANDBY_MODE_TIME_MIN,
                                          .maxValue = STANDBY_MODE_TIME_MAX,
                                          .ptr = (void*)&standbyModeTime};

    editableVars["BREWCONTROL"] = {.displayName = F("Enable Brew Control"),
                                   .hasHelpText = true,
                                   .helpText = F("Enables brew-by-time or brew-by-weight"),
                                   .type = kUInt8,
                                   .section = sBrewSection,
                                   .position = 31,
                                   .show = [] { return true && FEATURE_BREWSWITCH == 1; },
                                   .minValue = false,
                                   .maxValue = true,
                                   .ptr = (void*)&featureBrewControl};

#if FEATURE_SCALE == 1
    editableVars["TARE_ON"] = {
        .displayName = F("Tare"), .hasHelpText = false, .helpText = "", .type = kUInt8, .section = sScaleSection, .position = 32, .show = [] { return false; }, .minValue = 0, .maxValue = 1, .ptr = (void*)&scaleTareOn};

    editableVars["CALIBRATION_ON"] = {.displayName = F("Calibration"),
                                      .hasHelpText = false,
                                      .helpText = "",
                                      .type = kUInt8,
                                      .section = sScaleSection,
                                      .position = 33,
                                      .show = [] { return false; },
                                      .minValue = 0,
                                      .maxValue = 1,
                                      .ptr = (void*)&scaleCalibrationOn};

    editableVars["SCALE_KNOWN_WEIGHT"] = {.displayName = F("Known weight in g"),
                                          .hasHelpText = false,
                                          .helpText = "",
                                          .type = kFloat,
                                          .section = sScaleSection,
                                          .position = 34,
                                          .show = [] { return true; },
                                          .minValue = 0,
                                          .maxValue = 2000,
                                          .ptr = (void*)&scaleKnownWeight};

    editableVars["SCALE_CALIBRATION"] = {.displayName = F("Calibration factor scale 1"),
                                         .hasHelpText = false,
                                         .helpText = "",
                                         .type = kFloat,
                                         .section = sScaleSection,
                                         .position = 35,
                                         .show = [] { return true; },
                                         .minValue = -100000,
                                         .maxValue = 100000,
                                         .ptr = (void*)&scaleCalibration};

    editableVars["SCALE2_CALIBRATION"] = {.displayName = F("Calibration factor scale 2"),
                                          .hasHelpText = false,
                                          .helpText = "",
                                          .type = kFloat,
                                          .section = sScaleSection,
                                          .position = 36,
                                          .show = [] { return SCALE_TYPE == 0; },
                                          .minValue = -100000,
                                          .maxValue = 100000,
                                          .ptr = (void*)&scale2Calibration};
#endif

    editableVars["FULLSCREEN_BREW_TIMER"] = {.displayName = F("Enable Fullscreen Brew Timer"),
                                             .hasHelpText = true,
                                             .helpText = "Enable fullscreen overlay during brew",
                                             .type = kUInt8,
                                             .section = sDisplaySection,
                                             .position = 37,
                                             .show = [] { return true; },
                                             .minValue = 0,
                                             .maxValue = 1,
                                             .ptr = (void*)&featureFullscreenBrewTimer};

    editableVars["FULLSCREEN_MANUAL_FLUSH_TIMER"] = {.displayName = F("Enable Fullscreen Manual Flush Timer"),
                                                     .hasHelpText = true,
                                                     .helpText = "Enable fullscreen overlay during manual flush",
                                                     .type = kUInt8,
                                                     .section = sDisplaySection,
                                                     .position = 38,
                                                     .show = [] { return true; },
                                                     .minValue = 0,
                                                     .maxValue = 1,
                                                     .ptr = (void*)&featureFullscreenManualFlushTimer};

    editableVars["POST_BREW_TIMER_DURATION"] = {.displayName = F("Post Brew Timer Duration (s)"),
                                                .hasHelpText = true,
                                                .helpText = "time in s that brew timer will be shown after brew finished",
                                                .type = kDouble,
                                                .section = sDisplaySection,
                                                .position = 39,
                                                .show = [] { return true; },
                                                .minValue = POST_BREW_TIMER_DURATION_MIN,
                                                .maxValue = POST_BREW_TIMER_DURATION_MAX,
                                                .ptr = (void*)&postBrewTimerDuration};

    editableVars["HEATING_LOGO"] = {.displayName = F("Enable Heating Logo"),
                                    .hasHelpText = true,
                                    .helpText = "full screen logo will be shown if temperature is 5°C below setpoint",
                                    .type = kUInt8,
                                    .section = sDisplaySection,
                                    .position = 40,
                                    .show = [] { return true; },
                                    .minValue = 0,
                                    .maxValue = 1,
                                    .ptr = (void*)&featureHeatingLogo};

    editableVars["PID_OFF_LOGO"] = {.displayName = F("Enable ´PID Disabled´ Logo"),
                                    .hasHelpText = true,
                                    .helpText = "full screen logo will be shown if pid is disabled",
                                    .type = kUInt8,
                                    .section = sDisplaySection,
                                    .position = 41,
                                    .show = [] { return true; },
                                    .minValue = 0,
                                    .maxValue = 1,
                                    .ptr = (void*)&featurePidOffLogo};
    editableVars["VERSION"] = {
        .displayName = F("Version"), .hasHelpText = false, .helpText = "", .type = kCString, .section = sOtherSection, .position = 42, .show = [] { return false; }, .minValue = 0, .maxValue = 1, .ptr = (void*)sysVersion};
    // when adding parameters, set EDITABLE_VARS_LEN to max of .position

#if (FEATURE_PRESSURESENSOR == 1)
    Wire.begin();
#endif
#if (FEATURE_PRESSURESENSOR == 2)
    Wire.begin();
    pressureInit();
#endif

    // Editable values reported to MQTT
    mqttVars["pidON"] = [] { return &editableVars.at("PID_ON"); };
    mqttVars["brewSetpoint"] = [] { return &editableVars.at("BREW_SETPOINT"); };
    mqttVars["brewTempOffset"] = [] { return &editableVars.at("BREW_TEMP_OFFSET"); };
    mqttVars["steamON"] = [] { return &editableVars.at("STEAM_MODE"); };
    mqttVars["steamSetpoint"] = [] { return &editableVars.at("STEAM_SETPOINT"); };
    mqttVars["startUsePonM"] = [] { return &editableVars.at("START_USE_PONM"); };
    mqttVars["startKp"] = [] { return &editableVars.at("START_KP"); };
    mqttVars["startTn"] = [] { return &editableVars.at("START_TN"); };
    mqttVars["aggKp"] = [] { return &editableVars.at("PID_KP"); };
    mqttVars["aggTn"] = [] { return &editableVars.at("PID_TN"); };
    mqttVars["aggTv"] = [] { return &editableVars.at("PID_TV"); };
    mqttVars["aggIMax"] = [] { return &editableVars.at("PID_I_MAX"); };
    mqttVars["steamKp"] = [] { return &editableVars.at("STEAM_KP"); };
    mqttVars["standbyModeOn"] = [] { return &editableVars.at("STANDBY_MODE_ON"); };
    mqttVars["brewTime"] = [] { return &editableVars.at("BREW_TIME"); };
    mqttVars["preinfusion"] = [] { return &editableVars.at("BREW_PREINFUSION"); };
    mqttVars["preinfusionPause"] = [] { return &editableVars.at("BREW_PREINFUSIONPAUSE"); };
    mqttVars["backflushOn"] = [] { return &editableVars.at("BACKFLUSH_ON"); };
    mqttVars["backflushCycles"] = [] { return &editableVars.at("BACKFLUSH_CYCLES"); };
    mqttVars["backflushFillTime"] = [] { return &editableVars.at("BACKFLUSH_FILL_TIME"); };
    mqttVars["backflushFlushTime"] = [] { return &editableVars.at("BACKFLUSH_FLUSH_TIME"); };
    mqttVars["brewPidDelay"] = [] { return &editableVars.at("PID_BD_DELAY"); };
    mqttVars["pidUseBD"] = [] { return &editableVars.at("PID_BD_ON"); };
    mqttVars["aggbKp"] = [] { return &editableVars.at("PID_BD_KP"); };
    mqttVars["aggbTn"] = [] { return &editableVars.at("PID_BD_TN"); };
    mqttVars["aggbTv"] = [] { return &editableVars.at("PID_BD_TV"); };

    // Values reported to MQTT
    mqttSensors["temperature"] = [] { return temperature; };
    mqttSensors["heaterPower"] = [] { return pidOutput / 10; };
    mqttSensors["standbyModeTimeRemaining"] = [] { return standbyModeRemainingTimeMillis / 1000; };
    mqttSensors["currentKp"] = [] { return bPID.GetKp(); };
    mqttSensors["currentKi"] = [] { return bPID.GetKi(); };
    mqttSensors["currentKd"] = [] { return bPID.GetKd(); };
    mqttSensors["machineState"] = [] { return machineState; };

#if FEATURE_BREWSWITCH == 1
    mqttSensors["timeBrewed"] = [] { return timeBrewed / 1000; };
#endif

#if FEATURE_SCALE == 1
    mqttVars["weightSetpoint"] = [] { return &editableVars.at("SCALE_WEIGHTSETPOINT"); };
    mqttVars["scaleCalibration"] = [] { return &editableVars.at("SCALE_CALIBRATION"); };
#if SCALE_TYPE == 0
    mqttVars["scale2Calibration"] = [] { return &editableVars.at("SCALE2_CALIBRATION"); };
#endif
    mqttVars["scaleKnownWeight"] = [] { return &editableVars.at("SCALE_KNOWN_WEIGHT"); };
    mqttVars["scaleTareOn"] = [] { return &editableVars.at("TARE_ON"); };
    mqttVars["scaleCalibrationOn"] = [] { return &editableVars.at("CALIBRATION_ON"); };

    mqttSensors["currWeight"] = [] { return currWeight; };
    mqttSensors["weightBrewed"] = [] { return weightBrewed; };
#endif

#if (FEATURE_PRESSURESENSOR == 1) || (FEATURE_PRESSURESENSOR == 2)
    mqttSensors["pressure"] = [] { return inputPressureFilter; };
#endif
    initTimer1();

    storageSetup();

    
#if (FEATURE_PUMP_DIMMER == 1)
    pumpRelay.begin();
    pumpRelay.setControlMethod(PumpDimmerCore::ControlMethod::PSM);
    pumpRelay.setPower(0); // start off with zero power
#endif

#if (FEATURE_PUMP_DIMMER == 2)
    pumpRelay.begin();
    pumpRelay.setControlMethod(PumpDimmerCore::ControlMethod::PHASE);
    pumpRelay.setPower(0);
#endif

    heaterRelay.off();
    valveRelay.off();
    pumpRelay.off();

    if (FEATURE_POWERSWITCH) {
        powerSwitch = new IOSwitch(PIN_POWERSWITCH, GPIOPin::IN_HARDWARE, POWERSWITCH_TYPE, POWERSWITCH_MODE);
    }

    if (FEATURE_STEAMSWITCH) {
        steamSwitch = new IOSwitch(PIN_STEAMSWITCH, GPIOPin::IN_HARDWARE, STEAMSWITCH_TYPE, STEAMSWITCH_MODE);
    }

    if (FEATURE_BREWSWITCH) {
        brewSwitch = new IOSwitch(PIN_BREWSWITCH, GPIOPin::IN_HARDWARE, BREWSWITCH_TYPE, BREWSWITCH_MODE);
    }
    
    if (FEATURE_WATERSWITCH) {
        waterSwitch = new IOSwitch(PIN_WATERSWITCH, GPIOPin::IN_HARDWARE, WATERSWITCH_TYPE, WATERSWITCH_MODE);
    }

    if (LED_TYPE == LED::STANDARD) {
        statusLedPin = new GPIOPin(PIN_STATUSLED, GPIOPin::OUT);
        brewLedPin = new GPIOPin(PIN_BREWLED, GPIOPin::OUT);
        steamLedPin = new GPIOPin(PIN_STEAMLED, GPIOPin::OUT);
        waterLedPin = new GPIOPin(PIN_WATERLED, GPIOPin::OUT);

        statusLed = new StandardLED(*statusLedPin, FEATURE_STATUS_LED);
        brewLed = new StandardLED(*brewLedPin, FEATURE_BREW_LED);
        steamLed = new StandardLED(*steamLedPin, FEATURE_STEAM_LED);
        waterLed = new StandardLED(*waterLedPin, FEATURE_WATER_LED);

        brewLed->turnOff();
        steamLed->turnOff();
        waterLed->turnOff();
    }
    else {
        // TODO Addressable LEDs
    }

    if (FEATURE_WATERTANKSENSOR == 1) {
        waterTankSensor = new IOSwitch(PIN_WATERTANKSENSOR, (WATERTANKSENSOR_TYPE == Switch::NORMALLY_OPEN ? GPIOPin::IN_PULLDOWN : GPIOPin::IN_PULLUP), Switch::TOGGLE, WATERTANKSENSOR_TYPE);
    }

#if OLED_DISPLAY != 0
    u8g2.setI2CAddress(oled_i2c * 2);
    u8g2.begin();
    u8g2_prepare();
    displayLogo(String("Version "), String(sysVersion));
    delay(2000); // caused crash with wifi manager on esp8266, should be ok on esp32
#endif

    // Fallback offline
    if (connectmode == 1) { // WiFi Mode
        wiFiSetup();
        websiteSetup();

        // OTA Updates
        if (ota && WiFi.status() == WL_CONNECTED) {
            ArduinoOTA.setHostname(hostname); //  Device name for OTA
            ArduinoOTA.setPassword(OTApass);  //  Password for OTA
            ArduinoOTA.begin();
        }

        if (FEATURE_MQTT == 1) {
            snprintf(topic_will, sizeof(topic_will), "%s%s/%s", mqtt_topic_prefix, hostname, "status");
            snprintf(topic_set, sizeof(topic_set), "%s%s/+/%s", mqtt_topic_prefix, hostname, "set");
            mqtt.setServer(mqtt_server_ip, mqtt_server_port);
            mqtt.setCallback(mqtt_callback);
            checkMQTT();
#if MQTT_HASSIO_SUPPORT == 1 // Send Home Assistant MQTT discovery messages
            sendHASSIODiscoveryMsg();
#endif
        }
    }
    else if (connectmode == 0) {
        wm.disconnect();            // no wm
        readSysParamsFromStorage(); // get all parameters from storage
        offlineMode = 1;            // offline mode
        pidON = 1;                  // pid on
    }

    // Start the logger
    Logger::begin();
    Logger::setLevel(LOGLEVEL);

    // Initialize PID controller
    bPID.SetSampleTime(windowSize);
    bPID.SetOutputLimits(0, windowSize);
    bPID.SetIntegratorLimits(0, AGGIMAX);
    bPID.SetSmoothingFactor(EMA_FACTOR);
    bPID.SetMode(AUTOMATIC);

    if (TEMP_SENSOR == 1) {
        tempSensor = new TempSensorDallas(PIN_TEMPSENSOR);
    }
    else if (TEMP_SENSOR == 2) {
        tempSensor = new TempSensorTSIC(PIN_TEMPSENSOR);
    }
    else if (TEMP_SENSOR == 3) {
        tempSensor = new TempSensorK(PIN_TEMPERATURE_CLK,PIN_TEMPERATURE_CS,PIN_TEMPERATURE_SO);
    }

    temperature = tempSensor->getCurrentTemperature();

    temperature -= brewTempOffset;

// Init Scale
#if FEATURE_SCALE == 1
    initScale();
#endif

    // Initialisation MUST be at the very end of the init(), otherwise the
    // time comparision in loop() will have a big offset
    unsigned long currentTime = millis();
    previousMillistemp = currentTime;
    windowStartTime = currentTime;
    previousMillisMQTT = currentTime;

#if FEATURE_SCALE == 1
    previousMillisScale = currentTime;
#endif

#if (FEATURE_PRESSURESENSOR == 1) || (FEATURE_PRESSURESENSOR == 2)
    previousMillisPressure = currentTime;
#endif

    
    if(ENCODER_CONTROL > 0) {
        rotaryEncoder.setEncoderType( EncoderType::HAS_PULLUP );
        rotaryEncoder.setBoundaries(0,100,true);
        rotaryEncoder.onTurned( &knobCallback );
        //rotaryEncoder.onPressed( &buttonCallback );
        rotaryEncoder.begin();
    }
    encoderSw = new IOSwitch(PIN_ROTARY_SW, GPIOPin::IN_PULLUP, Switch::TOGGLE, Switch::NORMALLY_CLOSED);

    setupDone = true;

    enableTimer1();

    double fsUsage = ((double)LittleFS.usedBytes() / LittleFS.totalBytes()) * 100;
    LOGF(INFO, "LittleFS: %d%% (used %ld bytes from %ld bytes)", (int)ceil(fsUsage), LittleFS.usedBytes(), LittleFS.totalBytes());
}

void knobCallback(long value){
    static long lastencodervalue = 0;
    float tempvalue = 0.0;      //use a tempvalue so only the final result is written to each variable

    LOGF(INFO, "Rotary Encoder Value: %i", value);

    if((value < 10) && (lastencodervalue > 90)) {
        lastencodervalue -= 101;
    }
    else if((value > 90) && (lastencodervalue < 10)) {
        lastencodervalue += 101;
    }

    if(encodercontrol == 1) {
        tempvalue = DimmerPower + (value-lastencodervalue);
        DimmerPower = constrain(tempvalue, 0, 100);
    }
    else if(encodercontrol == 2) {
        tempvalue = (value-lastencodervalue)/5.0;
        tempvalue = setPressure + tempvalue;
        setPressure = constrain(tempvalue, 4.0, 10.0);    //4-10
    }
    else if(encodercontrol == 3) {
        tempvalue = (value-lastencodervalue); 
        tempvalue = currentRecipeIndex + tempvalue;
        currentRecipeIndex = constrain(tempvalue, 0.0, recipesCount - 1);    //0-recipesCount
        currentPhaseIndex = 0;
        recipeName = recipes[currentRecipeIndex].name;

        LOGF(INFO, "Recipe Index: %i -- Recipe Name: %s", currentRecipeIndex, recipeName);
    }
    else if(encodercontrol == 4) {
        tempvalue = (value-lastencodervalue)/5.0; 
        tempvalue = setPumpFlowRate + tempvalue;
        setPumpFlowRate = constrain(tempvalue, 0.0, 10.0);    //0-10
    }
    else if(encodercontrol == 5) {
        tempvalue = (value-lastencodervalue)/5.0; 
        tempvalue = flowKp + tempvalue;
        flowKp = constrain(tempvalue, 0.0, 40.0);    //0-40
    }
    else if(encodercontrol == 6) {
        tempvalue = (value-lastencodervalue)/5.0; 
        tempvalue = flowKi + tempvalue;
        flowKi = constrain(tempvalue, 0.0, 40.0);    //0-40
    }
    else if(encodercontrol == 7) {
        tempvalue = (value-lastencodervalue)/100.0; 
        tempvalue = flowKd + tempvalue;
        flowKd = constrain(tempvalue, 0.0, 4.0);    //0-4
    }
    else if(encodercontrol == 8) {
        tempvalue = (value-lastencodervalue); 
        tempvalue = featurePumpDimmer + tempvalue;
        featurePumpDimmer = constrain(tempvalue, 1, 2);    //1-2
    }
    lastencodervalue = value;
}
void buttonCallback(unsigned long duration){
    LOGF(INFO, "Rotary Encoder Button down for: %u ms", duration);
    // not used
}

void loop() {
    currentMicrosDebug = micros();
    // Accept potential connections for remote logging
    Logger::update();

    // Update water tank sensor
    loopWaterTank();

    // Update PID output for pump control
    looppump();
    
    // Update PID settings & machine state
    looppid();
    
    // Update LED output based on machine state
    loopLED();



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
                encodercontrol += 1;
                if(encodercontrol > 8) {
                    encodercontrol = 1;
                }
                LOGF(INFO, "Rotary Encoder Mode Changed: %i", encodercontrol);
                pumpintegral = 0;
                previousError = 0;
                if(encodercontrol == 3) {//Recipes
                    recipeName = recipes[currentRecipeIndex].name;
                    lastPreinfusion = preinfusion;
                    lastPreinfusionPause = preinfusionPause;
                    lastBrewTime = brewTime;
                    preinfusion = 0;            // disable preinfusion time in s
                    preinfusionPause = 0;       // disable preinfusion pause time in s
                }
                else {
                    preinfusion = lastPreinfusion;            // preinfusion time in s
                    preinfusionPause = lastPreinfusionPause; // preinfusion pause time in s
                    brewTime = lastBrewTime;                       // brewtime in s
                }
            }
            LOGF(INFO, "Rotary Encoder Button down for: %lu ms", duration);
        }
        encoderSwPressed = false;
    }

    // Every interval, log and reset
    IFLOG(DEBUG) {
        unsigned long loopDuration = micros() - currentMicrosDebug;

        // Track max loop time
        if (loopDuration >= maxloop) {// && loopDuration < 100000) {
            maxloop = loopDuration;
            triggered = true;
            triggerCountdown = 10;
            maxActivity = 0;
        }

          // Store the loop duration in the circular buffer
        loopTimings[loopIndex] = loopDuration;
        activityLoopTimings[loopIndex] = 0;
        if(buffer_ready) activityLoopTimings[loopIndex]+= 1;
        if(display_update) activityLoopTimings[loopIndex]+= 10;
        if(website_update) activityLoopTimings[loopIndex]+= 100;
        if(mqtt_update) activityLoopTimings[loopIndex]+= 1000;
        if(HASSIO_update) activityLoopTimings[loopIndex]+= 10000;
        display_update = false;
        website_update = false;
        mqtt_update = false;
        HASSIO_update = false;

        if(triggered){
            if (--triggerCountdown <= 0) {
                triggered = false;
                for (int i = 0; i < LOOP_HISTORY_SIZE; i++) {
                    int idx = (loopIndex + i) % LOOP_HISTORY_SIZE;
                    maxLoopTimings[i] = loopTimings[idx];
                    maxActivityLoopTimings[i] = activityLoopTimings[idx];
                    if(activityLoopTimings[idx] > maxActivity) {
                        maxActivity = activityLoopTimings[idx];
                    }
                }
            }
        }

        loopIndex = (loopIndex + 1) % LOOP_HISTORY_SIZE;

        // Track average
        static unsigned long loopTotal = 0;
        static unsigned int loopCount = 0;
        loopTotal += loopDuration;
        loopCount++;

        if (currentMicrosDebug - previousMicrosDebug >= intervalDebug) {
            previousMicrosDebug = currentMicrosDebug;

            unsigned long avgLoopMicros = loopTotal / loopCount;
            LOGF(DEBUG, "max loop micros: %lu, avg: %lu", maxloop, avgLoopMicros);
            if((maxActivity > 10)||(maxloop > 40000)){
                printLoopTimingsAsList();
                printActivityTimingsAsList();
            }

            // Reset trackers
            maxloop = 0;
            maxActivity = 0;
            loopTotal = 0;
            loopCount = 0;
        }
    }
}

void printLoopTimingsAsList() {
  char buffer[512];  // Make sure this is large enough
  int len = 0;
  len += snprintf(buffer + len, sizeof(buffer) - len, "Loop timings (us): [");
  for (int i = 0; i < LOOP_HISTORY_SIZE; i++) {
    len += snprintf(buffer + len, sizeof(buffer) - len, "%lu", maxLoopTimings[i]);
    if (i < LOOP_HISTORY_SIZE - 1) {
      len += snprintf(buffer + len, sizeof(buffer) - len, ", ");
    }
  }
  len += snprintf(buffer + len, sizeof(buffer) - len, "]");
  LOGF(DEBUG, "%s", buffer);
}

void printActivityTimingsAsList() {
  char buffer[512];  // Make sure this is large enough
  int len = 0;
  len += snprintf(buffer + len, sizeof(buffer) - len, "Activity timings: [");
  for (int i = 0; i < LOOP_HISTORY_SIZE; i++) {
    len += snprintf(buffer + len, sizeof(buffer) - len, "%lu", maxActivityLoopTimings[i]);
    if (i < LOOP_HISTORY_SIZE - 1) {
      len += snprintf(buffer + len, sizeof(buffer) - len, ", ");
    }
  }
  len += snprintf(buffer + len, sizeof(buffer) - len, "]");
  LOGF(DEBUG, "%s", buffer);
}

void printLoopPidAsList() {
  char buffer[512];  // Make sure this is large enough
  for (int j = 0; j < TYPE_HISTORY_SIZE; j++) {
    int len = 0;
    len += snprintf(buffer + len, sizeof(buffer) - len, "PID ");
    len += snprintf(buffer + len, sizeof(buffer) - len, "%d", j);
    len += snprintf(buffer + len, sizeof(buffer) - len, ": [");
    for (int i = 0; i < LOOP_HISTORY_SIZE; i++) {
        len += snprintf(buffer + len, sizeof(buffer) - len, "%0.2f", PidResults[i][j]);
        if (i < LOOP_HISTORY_SIZE - 1) {
            len += snprintf(buffer + len, sizeof(buffer) - len, ", ");
        }
    }
    len += snprintf(buffer + len, sizeof(buffer) - len, "]");
    LOGF(DEBUG, "%s", buffer);
  }
}

void looppump() {
    if(machineState != kBrew) { //moved here from recipes
        debug_recipe = false;
        currentPhaseIndex = 0;
    }
#if (FEATURE_PUMP_DIMMER > 0) 
    static float inputPID = 0.0;
    static float targetPID = 0.0;                
    static float inputKp = 0.0;
    static float inputKi = 0.0;
    static float inputKd = 0.0;


    // --- PID control variables ---
    /*if(featurePumpDimmer == 1) {
        pressureKp = 20.0;//   18.0;//18.0;//13.0;//14.0;//   20.0;    //25.0;//30.0;    // Proportional gain
        pressureKi = 10.0;//  9.0;//8.0;//4.0;//5.0;//   10.0;   //45.0;//75.0;     // Integral gain
        pressureKd = 1.5;//   1.0;//3.0;//7.0;//6.0;//   2.0;   //3.0;       // Derivative gain
        integralAntiWindup = 8.0;  //pumpintegral += error * pumpdt is capped at +-integralAntiWindup, then *pumpKi
        MaxDimmerPower = 100;
    }
    if(featurePumpDimmer == 2) {
        pressureKp = 20.0;//   18.0;//18.0;//13.0;//14.0;//   20.0;    //25.0;//30.0;    // Proportional gain
        pressureKi = 10.0;//  9.0;//8.0;//4.0;//5.0;//   10.0;   //45.0;//75.0;     // Integral gain
        pressureKd = 1.5;//   1.0;//3.0;//7.0;//6.0;//   2.0;   //3.0;       // Derivative gain
        integralAntiWindup = 8.0;  //pumpintegral += error * pumpdt is capped at +-integralAntiWindup, then *pumpKi
        MaxDimmerPower = 100;
    }*/

    if(pumpRelay.getState()) {
        currentMillisPumpControl = millis();
        if (currentMillisPumpControl - previousMillisPumpControl >= pumpControlInterval) {
            PidResults[loopIndexPid][8] = currentMillisPumpControl - previousMillisPumpControl;
            previousMillisPumpControl = currentMillisPumpControl;

            PidResults[loopIndexPid][0] = inputPressure;
            PidResults[loopIndexPid][1] = setPressure;
            PidResults[loopIndexPid][2] = pumpFlowRate;
            PidResults[loopIndexPid][3] = setPumpFlowRate;
            PidResults[loopIndexPid][4] = weightBrewed;

            if(encodercontrol == 1) {   //power
                pumpControl = POWER;
            }
            if(encodercontrol == 2) {   //pressure
                pumpControl = PRESSURE;
            }
            if(encodercontrol == 3) {   //recipes
                runRecipe(currentRecipeIndex);
            }
            else if(encodercontrol >= 4) { //flow and PID tuning
                pumpControl = FLOW;
            }


            if(pumpControl == PRESSURE) {   //pressure   and temporary recipes
                inputPID = inputPressureFilter;//inputPressure;
                targetPID = setPressure;
                inputKp = pressureKp;
                inputKi = pressureKi;
                inputKd = pressureKd;
            }
            else if (pumpControl == FLOW) { //flow and PID tuning
                inputPID = pumpFlowRate;
                targetPID = setPumpFlowRate;
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
        previousMillisPumpControl = millis() - pumpControlInterval; //stops large spikes in log data
        PumpDimmerCore::ControlMethod method = (featurePumpDimmer == 2) ? PumpDimmerCore::ControlMethod::PHASE : PumpDimmerCore::ControlMethod::PSM;
        pumpRelay.setControlMethod(method);
    }
#endif
    blockMicrosDisplayStart = micros(); //give other functions like display and MQTT some time to refresh
}

void runRecipe(int recipeIndex) {
    if (recipeIndex < 0 || recipeIndex >= recipesCount) return;
    
    static float lastPressure = 0.0;
    static float lastFlow = 0.0;

    BrewRecipe* recipe = &recipes[recipeIndex];
    BrewPhase* phase = &recipe->phases[currentPhaseIndex];

    if(currentPhaseIndex < recipe->phaseCount) {
        phaseName = phase->name;
    }
    else {
        return;
    }

    if (debug_recipe == false) {
        debug_recipe = true;
        brewTime = 0;               // disable  brewtime in s
        lastPressure = 0.0;
        lastFlow = 0.0;
        phaseTiming = 0;
        LOGF(DEBUG, "Running recipe: %s\n", recipe->name);
        LOGF(DEBUG, "Phase %d: %s for %.1f seconds", currentPhaseIndex, phase->name, phase->seconds);
    }


    // Transition to next phase if exit condition met
    bool exitReached = false;

    switch (phase->exit_type) {
        case EXIT_TYPE_NONE:
            exitReached = (timeBrewed > (phase->seconds)*1000 + phaseTiming);
            break;

        case EXIT_TYPE_PRESSURE_OVER:
            exitReached = (inputPressureFilter >= phase->exit_pressure_over);
            break;

        case EXIT_TYPE_PRESSURE_UNDER:
            exitReached = (inputPressureFilter <= phase->exit_pressure_under);
            break;

        case EXIT_TYPE_FLOW_OVER:
            exitReached = (pumpFlowRate >= phase->exit_flow_over);
            break;

        case EXIT_TYPE_FLOW_UNDER:
            exitReached = (pumpFlowRate <= phase->exit_flow_under);
            break;
    }
    if (phase->weight > 0 && weightBrewed >= phase->weight) {
        exitReached = true;
    }

    if (exitReached || (timeBrewed > (phase->seconds)*1000 + phaseTiming)) {
        lastPressure = inputPressureFilter;//phase->pressure;
        lastFlow = pumpFlowRate;//phase->flow;
        currentPhaseIndex += 1;
        if (currentPhaseIndex < recipe->phaseCount) {
            phase = &recipe->phases[currentPhaseIndex];
            LOGF(DEBUG, "Phase %d: %s for %.1f seconds", currentPhaseIndex, phase->name, phase->seconds);
            phaseTiming = timeBrewed;
        } 
        else {
            LOG(DEBUG, "Brew recipe complete");
            brewTime = timeBrewed/1000;
        }
    }

    // Control logic based on phase settings

    //check if still in phases, otherwise skip control
    if(currentPhaseIndex < recipe->phaseCount) {
        if (phase->pump == FLOW) {
            pumpControl = FLOW;
            if (phase->transition == TRANSITION_SMOOTH) {
                float elapsed = (timeBrewed - phaseTiming) / 1000.0;
                float t = elapsed / phase->seconds;
                if (t > 1.0) t = 1.0;
                setPumpFlowRate = lastFlow + (phase->flow - lastFlow) * t;
            } 
            else {
                setPumpFlowRate = phase->flow;
            }
            setPressure = 0;
        }
        else if (phase->pump == PRESSURE) {
            pumpControl = PRESSURE;
            if (phase->transition == TRANSITION_SMOOTH) {
                float elapsed = (timeBrewed - phaseTiming) / 1000.0;
                float t = elapsed / phase->seconds;
                if (t > 1.0) t = 1.0;
                setPressure = lastPressure + (phase->pressure - lastPressure) * t;
            } 
            else {
                setPressure = phase->pressure;
            }
            setPumpFlowRate = 0;
        }
            // otherwise run old settings for next phase


        else {  //shouldnt ever get here
            if(phase->pressure > 0) {
                pumpControl = PRESSURE;
                setPressure = phase->pressure;
                setPumpFlowRate = 0;
            }
            else {
                pumpControl = FLOW;
                setPumpFlowRate = phase->flow;
                setPressure = 0;
            }
        }
    }
}

void looppid() {
    // Only do Wifi stuff, if Wifi is connected
    if (WiFi.status() == WL_CONNECTED && offlineMode == 0) {
        if ((FEATURE_MQTT == 1)&&(micros() - blockMicrosDisplayStart < blockMicrosDisplayInterval)) {
            checkMQTT();
            if(!buffer_ready) {
                writeSysParamsToMQTT(true); // Continue on error
            }

            if (mqtt.connected() == 1) {
                mqtt.loop();
                previousMqttConnection = millis();
#if MQTT_HASSIO_SUPPORT == 1
                //resend discovery messages if not during a main function and MQTT has been disconnected but has now reconnected
                //this could mean mqtt_was_connected stays false for up to 5 mins, could change it to sendHASSIODiscoveryMsg();
                if(!((machineState >= kBrew) && (machineState <= kBackflush)) && (!mqtt_was_connected) && (!buffer_ready)) {
                    hassioDiscoveryTimer();
                    mqtt_was_connected = true;
                }
#endif
                
            }
            // Supress debug messages until we have a connection etablished
            else if (mqtt_was_connected) {
                LOG(INFO, "MQTT disconnected");
                mqtt_was_connected = false;
            }
        }

        ArduinoOTA.handle(); // For OTA

        // Disable interrupt if OTA is starting, otherwise it will not work
        ArduinoOTA.onStart([]() {
            disableTimer1();
            heaterRelay.off();
        });

        ArduinoOTA.onError([](ota_error_t error) { enableTimer1(); });

        // Enable interrupts if OTA is finished
        ArduinoOTA.onEnd([]() { enableTimer1(); });

        wifiReconnects = 0; // reset wifi reconnects if connected
    }
    else {
        checkWifi();
    }
    // Update the temperature:
    temperature = tempSensor->getCurrentTemperature();

    if (machineState != kSteam) {
        temperature -= brewTempOffset;
    }

    testEmergencyStop(); // test if temp is too high
    bPID.Compute();      // the variable pidOutput now has new values from PID (will be written to heater pin in ISR.cpp)
    if((machineState == kBrew) && (lastmachinestatehtml != kBrew)) {
        startBrewEvent();
        lastmachinestatehtml = machineState;
    }
    if((machineState != kBrew) && (lastmachinestatehtml == kBrew)) {
        stopBrewEvent();
        lastmachinestatehtml = machineState;
    }

    if (((millis() - lastBrewEvent) > brewEventInterval) && (machineState == kBrew)) {
        website_update = true;
        // send brew data to website endpoint
        sendBrewEvent(inputPressure, setPressure, pumpFlowRate, setPumpFlowRate, weightBrewed, DimmerPower); // pidOutput is promill, so /10 to get percent value
        lastBrewEvent = millis();
    }

    if (((millis() - lastTempEvent) > tempEventInterval)&&(!mqtt_update)&&(!HASSIO_update)&&(!buffer_ready)) {
        website_update = true;
        // send temperatures to website endpoint
        sendTempEvent(temperature, brewSetpoint, pidOutput / 10); // pidOutput is promill, so /10 to get percent value
        lastTempEvent = millis();

        if (pidON) {
            LOGF(TRACE, "Current PID mode: %s", bPID.GetPonE() ? "PonE" : "PonM");

            // P-Part
            LOGF(TRACE, "Current PID input error: %f", bPID.GetInputError());
            LOGF(TRACE, "Current PID P part: %f", bPID.GetLastPPart());
            LOGF(TRACE, "Current PID kP: %f", bPID.GetKp());
            // I-Part
            LOGF(TRACE, "Current PID I sum: %f", bPID.GetLastIPart());
            LOGF(TRACE, "Current PID kI: %f", bPID.GetKi());
            // D-Part
            LOGF(TRACE, "Current PID diff'd input: %f", bPID.GetDeltaInput());
            LOGF(TRACE, "Current PID D part: %f", bPID.GetLastDPart());
            LOGF(TRACE, "Current PID kD: %f", bPID.GetKd());
            // Combined PID output
            LOGF(TRACE, "Current PID Output: %f", pidOutput);
            LOGF(TRACE, "Current Machinestate: %s", machinestateEnumToString(machineState));
            // Brew
            LOGF(TRACE, "timeBrewed %f", timeBrewed);
            LOGF(TRACE, "Brew detected %i", brew());
            LOGF(TRACE, "brewPIDdisabled %i", brewPIDDisabled);
        }
    }

#if FEATURE_SCALE == 1
    checkWeight();    // Check Weight Scale in the loop
    shottimerscale(); // Calculation of weight of shot while brew is running
#endif

#if (FEATURE_BREWSWITCH == 1)
    brew();
    manualFlush();
#endif

#if (FEATURE_PRESSURESENSOR == 1) || (FEATURE_PRESSURESENSOR == 2)
    unsigned long currentMillisPressure = millis();

    if (currentMillisPressure - previousMillisPressure >= intervalPressure) {
        previousMillisPressure = currentMillisPressure;
        inputPressure = measurePressure();
        inputPressureFilter = filterPressureValue(inputPressure);
        pumpFlowRate = pumpRelay.getFlow(inputPressureFilter);
    }
#endif

    checkSteamSwitch();
    checkPowerSwitch();
    checkWaterSwitch();

    // set setpoint depending on steam or brew mode
    if (steamON == 1) {
        setpoint = steamSetpoint;
    }
    else if (steamON == 0) {
        setpoint = brewSetpoint;
    }

    updateStandbyTimer();
    handleMachineState();

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

    // Check if brew timer should be shown
#if (FEATURE_BREWSWITCH == 1)
    shouldDisplayBrewTimer();
#endif

    //temporary - update display if not too close to pumpPID timing
#if OLED_DISPLAY != 0
    if((micros() - blockMicrosDisplayStart < blockMicrosDisplayInterval)&&(!website_update)&&(!mqtt_update)&&(!HASSIO_update)&&(standbyModeRemainingTimeDisplayOffMillis > 0)) {
        if(buffer_ready) {
            u8g2.sendBuffer();
            buffer_ready = false;
            display_update = true;
        }
        else {
            printDisplayTimer();
        }
    }
#endif

    // Check if PID should run or not. If not, set to manual and force output to zero
    if (machineState == kPidDisabled || machineState == kWaterTankEmpty || machineState == kSensorError || machineState == kEmergencyStop || machineState == kEepromError || machineState == kStandby ||
        machineState == kBackflush || brewPIDDisabled) {
        if (bPID.GetMode() == 1) {
            // Force PID shutdown
            bPID.SetMode(0);
            pidOutput = 0;
            heaterRelay.off();
        }
    }
    else { // no sensorerror, no pid off or no Emergency Stop
        if (bPID.GetMode() == 0) {
            bPID.SetMode(1);
        }
    }

    // Regular PID operation
    if (machineState == kPidNormal) {
        if (usePonM) {
            if (startTn != 0) {
                startKi = startKp / startTn;
            }
            else {
                startKi = 0;
            }

            if (lastmachinestatepid != machineState) {
                LOGF(DEBUG, "new PID-Values: P=%.1f  I=%.1f  D=%.1f", startKp, startKi, 0.0);
                lastmachinestatepid = machineState;
            }

            bPID.SetTunings(startKp, startKi, 0, P_ON_M);
        }
        else {
            setNormalPIDTunings();
        }
    }

    // Brew PID
    if (machineState == kBrew) {
        if (brewPIDDelay > 0 && timeBrewed > 0 && timeBrewed < brewPIDDelay * 1000) {
            // disable PID for brewPIDDelay seconds, enable PID again with new tunings after that
            if (!brewPIDDisabled) {
                brewPIDDisabled = true;
                bPID.SetMode(MANUAL);
                LOGF(DEBUG, "disabled PID, waiting for %.0f seconds before enabling PID again", brewPIDDelay);
            }
        }
        else {
            if (brewPIDDisabled) {
                // enable PID again
                bPID.SetMode(AUTOMATIC);
                brewPIDDisabled = false;
                LOG(DEBUG, "Enabled PID again after delay");
            }

            if (useBDPID) {
                setBDPIDTunings();
            }
            else {
                setNormalPIDTunings();
            }
        }
    }
    // Reset brewPIDdisabled if brew was aborted
    if (machineState != kBrew && brewPIDDisabled) {
        // enable PID again
        bPID.SetMode(AUTOMATIC);
        brewPIDDisabled = false;
        LOG(DEBUG, "Enabled PID again after brew was manually stopped");
    }

    // Steam on
    if (machineState == kSteam) {
        if (lastmachinestatepid != machineState) {
            LOGF(DEBUG, "new PID-Values: P=%.1f  I=%.1f  D=%.1f", 150.0, 0.0, 0.0);
            lastmachinestatepid = machineState;
        }

        bPID.SetTunings(steamKp, 0, 0, 1);
    }
}

void loopLED() {
    if ((machineState == kPidNormal && (fabs(temperature - setpoint) < 0.3)) || (temperature > 115 && fabs(temperature - setpoint) < 5)) {
        statusLed->turnOn();
    }
    else {
        statusLed->turnOff();
    }

    brewLed->setGPIOState(machineState == kBrew);
    steamLed->setGPIOState(machineState == kSteam);
    waterLed->setGPIOState(machineState == kWater);
}

void checkWaterTank() {
    if (FEATURE_WATERTANKSENSOR != 1) {
        return;
    }

    bool isWaterDetected = waterTankSensor->isPressed();

    if (isWaterDetected && !waterTankFull) {
        waterTankFull = true;
        LOG(INFO, "Water tank full");
    }
    else if (!isWaterDetected && waterTankFull) {
        waterTankFull = false;
        LOG(WARNING, "Water tank empty");
    }
}

void setBackflush(int backflush) {
    backflushOn = backflush;
}

#if FEATURE_SCALE == 1
void setScaleCalibration(int calibration) {
    scaleCalibrationOn = calibration;
}

void setScaleTare(int tare) {
    scaleTareOn = tare;
}
#endif

void setSteamMode(int steamMode) {
    steamON = steamMode;

    if (steamON == 1) {
        steamFirstON = 1;
    }

    if (steamON == 0) {
        steamFirstON = 0;
    }
}

void setPidStatus(int pidStatus) {
    pidON = pidStatus;
    writeSysParamsToStorage();
}

void setNormalPIDTunings() {
    // Prevent overwriting of brewdetection values
    // calc ki, kd
    if (aggTn != 0) {
        aggKi = aggKp / aggTn;
    }
    else {
        aggKi = 0;
    }

    aggKd = aggTv * aggKp;

    bPID.SetIntegratorLimits(0, aggIMax);

    if (lastmachinestatepid != machineState) {
        LOGF(DEBUG, "new PID-Values: P=%.1f  I=%.1f  D=%.1f", aggKp, aggKi, aggKd);
        lastmachinestatepid = machineState;
    }

    bPID.SetTunings(aggKp, aggKi, aggKd, 1);
}

void setBDPIDTunings() {
    // calc ki, kd
    if (aggbTn != 0) {
        aggbKi = aggbKp / aggbTn;
    }
    else {
        aggbKi = 0;
    }

    aggbKd = aggbTv * aggbKp;

    if (lastmachinestatepid != machineState) {
        LOGF(DEBUG, "new PID-Values: P=%.1f  I=%.1f  D=%.1f", aggbKp, aggbKi, aggbKd);
        lastmachinestatepid = machineState;
    }

    bPID.SetTunings(aggbKp, aggbKi, aggbKd, 1);
}

/**
 * @brief Reads all system parameter values from non-volatile storage
 *
 * @return 0 = success, < 0 = failure
 */
int readSysParamsFromStorage(void) {
    if (sysParaPidOn.getStorage() != 0) return -1;
    if (sysParaUsePonM.getStorage() != 0) return -1;
    if (sysParaPidKpStart.getStorage() != 0) return -1;
    if (sysParaPidTnStart.getStorage() != 0) return -1;
    if (sysParaPidKpReg.getStorage() != 0) return -1;
    if (sysParaPidTnReg.getStorage() != 0) return -1;
    if (sysParaPidTvReg.getStorage() != 0) return -1;
    if (sysParaPidIMaxReg.getStorage() != 0) return -1;
    if (sysParaBrewSetpoint.getStorage() != 0) return -1;
    if (sysParaTempOffset.getStorage() != 0) return -1;
    if (sysParaBrewPIDDelay.getStorage() != 0) return -1;
    if (sysParaUseBDPID.getStorage() != 0) return -1;
    if (sysParaPidKpBd.getStorage() != 0) return -1;
    if (sysParaPidTnBd.getStorage() != 0) return -1;
    if (sysParaPidTvBd.getStorage() != 0) return -1;
    if (sysParaBrewTime.getStorage() != 0) return -1;
    if (sysParaPreInfTime.getStorage() != 0) return -1;
    if (sysParaPreInfPause.getStorage() != 0) return -1;
    if (sysParaPidKpSteam.getStorage() != 0) return -1;
    if (sysParaSteamSetpoint.getStorage() != 0) return -1;
    if (sysParaWeightSetpoint.getStorage() != 0) return -1;
    if (sysParaWifiCredentialsSaved.getStorage() != 0) return -1;
    if (sysParaStandbyModeOn.getStorage() != 0) return -1;
    if (sysParaStandbyModeTime.getStorage() != 0) return -1;
    if (sysParaScaleCalibration.getStorage() != 0) return -1;
    if (sysParaScale2Calibration.getStorage() != 0) return -1;
    if (sysParaScaleKnownWeight.getStorage() != 0) return -1;
    if (sysParaBackflushCycles.getStorage() != 0) return -1;
    if (sysParaBackflushFillTime.getStorage() != 0) return -1;
    if (sysParaBackflushFlushTime.getStorage() != 0) return -1;
    if (sysParaFeatureBrewControl.getStorage() != 0) return -1;
    if (sysParaFeatureFullscreenBrewTimer.getStorage() != 0) return -1;
    if (sysParaFeatureFullscreenManualFlushTimer.getStorage() != 0) return -1;
    if (sysParaPostBrewTimerDuration.getStorage() != 0) return -1;
    if (sysParaFeatureHeatingLogo.getStorage() != 0) return -1;
    if (sysParaFeaturePidOffLogo.getStorage() != 0) return -1;

    return 0;
}

/**
 * @brief Writes all current system parameter values to non-volatile storage
 *
 * @return 0 = success, < 0 = failure
 */
int writeSysParamsToStorage(void) {
    if (sysParaPidOn.setStorage() != 0) return -1;
    if (sysParaUsePonM.setStorage() != 0) return -1;
    if (sysParaPidKpStart.setStorage() != 0) return -1;
    if (sysParaPidTnStart.setStorage() != 0) return -1;
    if (sysParaPidKpReg.setStorage() != 0) return -1;
    if (sysParaPidTnReg.setStorage() != 0) return -1;
    if (sysParaPidTvReg.setStorage() != 0) return -1;
    if (sysParaPidIMaxReg.setStorage() != 0) return -1;
    if (sysParaBrewSetpoint.setStorage() != 0) return -1;
    if (sysParaTempOffset.setStorage() != 0) return -1;
    if (sysParaBrewPIDDelay.setStorage() != 0) return -1;
    if (sysParaUseBDPID.setStorage() != 0) return -1;
    if (sysParaPidKpBd.setStorage() != 0) return -1;
    if (sysParaPidTnBd.setStorage() != 0) return -1;
    if (sysParaPidTvBd.setStorage() != 0) return -1;
    if (sysParaBrewTime.setStorage() != 0) return -1;
    if (sysParaPreInfTime.setStorage() != 0) return -1;
    if (sysParaPreInfPause.setStorage() != 0) return -1;
    if (sysParaPidKpSteam.setStorage() != 0) return -1;
    if (sysParaSteamSetpoint.setStorage() != 0) return -1;
    if (sysParaWeightSetpoint.setStorage() != 0) return -1;
    if (sysParaWifiCredentialsSaved.setStorage() != 0) return -1;
    if (sysParaStandbyModeOn.setStorage() != 0) return -1;
    if (sysParaStandbyModeTime.setStorage() != 0) return -1;
    if (sysParaScaleCalibration.setStorage() != 0) return -1;
    if (sysParaScale2Calibration.setStorage() != 0) return -1;
    if (sysParaScaleKnownWeight.setStorage() != 0) return -1;
    if (sysParaBackflushCycles.setStorage() != 0) return -1;
    if (sysParaBackflushFillTime.setStorage() != 0) return -1;
    if (sysParaBackflushFlushTime.setStorage() != 0) return -1;
    if (sysParaFeatureBrewControl.setStorage() != 0) return -1;
    if (sysParaFeatureFullscreenBrewTimer.setStorage() != 0) return -1;
    if (sysParaFeatureFullscreenManualFlushTimer.setStorage() != 0) return -1;
    if (sysParaPostBrewTimerDuration.setStorage() != 0) return -1;
    if (sysParaFeatureHeatingLogo.setStorage() != 0) return -1;
    if (sysParaFeaturePidOffLogo.setStorage() != 0) return -1;

    return storageCommit();
}

/**
 * @brief Performs a factory reset.
 *
 * @return 0 = success, < 0 = failure
 */
int factoryReset(void) {
    int stoStatus;

    if ((stoStatus = storageFactoryReset()) != 0) {
        return stoStatus;
    }

    return readSysParamsFromStorage();
}
