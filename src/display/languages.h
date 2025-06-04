/**
 * @file languages.h
 *
 * @brief Localized strings
 */

#pragma once

#include "Config.h"

static const char* langstring_set_temp;
static const char* langstring_current_temp;
static const char* langstring_brew;
static const char* langstring_weight;
static const char* langstring_manual_flush;
static const char* langstring_hot_water;
static const char* langstring_pressure;
static const char* langstring_uptime;
static const char* langstring_offlinemode;
static const char* langstring_wifirecon;
static const char* langstring_connectwifi;
static const char* langstring_connectip;
static const char* langstring_nowifi[2];
static const char* langstring_offlineAP;
static const char* langstring_portalAP;
static const char* langstring_error_tsensor[2];
static const char* langstring_scale_Failure;
static const char* langstring_backflush_press;
static const char* langstring_backflush_start;
static const char* langstring_backflush_finish;
static const char* langstring_set_temp_ur;
static const char* langstring_current_temp_ur;
static const char* langstring_brew_ur;
static const char* langstring_manual_flush_ur;
static const char* langstring_hot_water_ur;
static const char* langstring_weight_ur;
static const char* langstring_pressure_ur;
static const char* langstring_calibrate_start;
static const char* langstring_calibrate_in_progress;
static const char* langstring_calibrate_complete;

inline void initLangStrings(const Config& config) {

    langstring_set_temp = "Set:   ";
    langstring_current_temp = "Temp:  ";
    langstring_brew = "Brew: ";
    langstring_weight = "Weight: ";
    langstring_manual_flush = "Flush: ";
    langstring_hot_water = "Water: ";
    langstring_pressure = "Pressure: ";
    langstring_uptime = "Uptime:  ";

    langstring_set_temp_ur = "S: ";
    langstring_current_temp_ur = "T: ";
    langstring_brew_ur = "B: ";
    langstring_manual_flush_ur = "F: ";
    langstring_hot_water_ur = "Wp: ";
    langstring_weight_ur = "W: ";
    langstring_pressure_ur = "P: ";

    langstring_offlinemode = "Offline";
    langstring_wifirecon = "Wifi reconnect:";
    langstring_connectwifi = "Connecting to WiFi:";
    langstring_connectip = "IP Address:";
    langstring_nowifi[0] = "No ";
    langstring_nowifi[1] = "WiFi";
    langstring_offlineAP = "Starting Offline AP";
    langstring_portalAP = "Starting Portal AP";

    langstring_error_tsensor[0] = "Error, Temp: ";
    langstring_error_tsensor[1] = "Check temperature sensor!";
    langstring_scale_Failure = "Fault";

    langstring_backflush_press = "Press brew switch";
    langstring_backflush_start = "to start...";
    langstring_backflush_finish = "to finish...";

    langstring_calibrate_start = "Calibration coming up\n\nEmpty scale ";
    langstring_calibrate_in_progress = "Calibration in progress. Place known weight on scale in next 10 seconds ";
    langstring_calibrate_complete = "Calibration done!\nNew result: ";
}