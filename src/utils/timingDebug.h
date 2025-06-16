/**
 * @file timingDebug.h
 *
 * @brief Stores and prints long duration processes to the console if DEBUG is enabled
 *
 */

#pragma once

enum ActivityType : uint16_t {
    ACT_DISPLAY_READY = 0x01,
    ACT_DISPLAY_RUNNING = 0x02,
    ACT_WEBSITE_RUNNING = 0x04,
    ACT_MQTT_RUNNING = 0x08,
    ACT_HASSIO_RUNNING = 0x10
};

const int LOOP_HISTORY_SIZE = 20;
unsigned long maxLoopTimings[LOOP_HISTORY_SIZE];
unsigned int maxActivityLoopTimings[LOOP_HISTORY_SIZE];

/**
 * @brief print what has caused the long loop time
 * @return void
 */

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


void printActivityFlags(const uint16_t* activity, int size) {
    char activityBuffer[512];
    int len = 0;

    len += snprintf(activityBuffer + len, sizeof(activityBuffer) - len, "Activity (short): [");
    for (int i = 0; i < size; ++i) {
        // Convert flags to short notation
        if (activity[i] == 0) {
            len += snprintf(activityBuffer + len, sizeof(activityBuffer) - len, "_");
        }
        else {
            if (activity[i] & ACT_DISPLAY_READY) len += snprintf(activityBuffer + len, sizeof(activityBuffer) - len, "r");
            if (activity[i] & ACT_DISPLAY_RUNNING) len += snprintf(activityBuffer + len, sizeof(activityBuffer) - len, "D");
            if (activity[i] & ACT_WEBSITE_RUNNING) len += snprintf(activityBuffer + len, sizeof(activityBuffer) - len, "W");
            if (activity[i] & ACT_MQTT_RUNNING) len += snprintf(activityBuffer + len, sizeof(activityBuffer) - len, "M");
            if (activity[i] & ACT_HASSIO_RUNNING) len += snprintf(activityBuffer + len, sizeof(activityBuffer) - len, "H");
        }
        if (i < size - 1) len += snprintf(activityBuffer + len, sizeof(activityBuffer) - len, ",");
    }
    len += snprintf(activityBuffer + len, sizeof(activityBuffer) - len, "]");

    LOGF(DEBUG, "%s", activityBuffer);
}

/**
 * @brief Print both timing and compact activity in one batch
 * @return void
 */
void printTimingAndActivityBatch(const unsigned long* timing, const uint16_t* activity, int size) {
    char timingBuffer[512];
    int tLen = 0;

    tLen += snprintf(timingBuffer + tLen, sizeof(timingBuffer) - tLen, "Loop timing (ms): [");
    for (int i = 0; i < size; ++i) {
        tLen += snprintf(timingBuffer + tLen, sizeof(timingBuffer) - tLen, "%lu", timing[i]);
        if (i < size - 1) tLen += snprintf(timingBuffer + tLen, sizeof(timingBuffer) - tLen, ",");
    }
    tLen += snprintf(timingBuffer + tLen, sizeof(timingBuffer) - tLen, "]");

    // Only two log lines total
    LOGF(DEBUG, "%s", timingBuffer);
    printActivityFlags(activity, size);
}

/**
 * @brief Store all long duration activities and their loop times, send when array is full
 * @return void
 */
void debugTimingLoop() {
    static const int LOOP_HISTORY_SIZE = 20;
    static unsigned long loopTiming[LOOP_HISTORY_SIZE];
    static uint16_t activityType[LOOP_HISTORY_SIZE];
    static unsigned long previousMillisDebug = millis();
    static unsigned long lastSendMillisDebug = millis();
    static unsigned int loopIndex = 0;
    static unsigned int loopCount = 0;
    static unsigned long maxLoop = 0;

    IFLOG(DEBUG) {
        loopCount += 1;
        unsigned long loopDuration = millis() - previousMillisDebug;
        previousMillisDebug = millis();
        if ((loopDuration > 5) || (displayUpdateRunning || displayNeedsUpdate || websiteUpdateRunning || mqttUpdateRunning || hassioUpdateRunning)) {
            if (loopDuration >= maxLoop) {
                maxLoop = loopDuration;
            }
            loopTiming[loopIndex] = loopDuration;
            activityType[loopIndex] = 0;
            if (displayBufferReady) activityType[loopIndex] |= ACT_DISPLAY_READY;
            //if (displayUpdateRunning) activityType[loopIndex] |= ACT_DISPLAY_RUNNING;
            if (displayNeedsUpdate) activityType[loopIndex] |= ACT_DISPLAY_RUNNING;
            if (websiteUpdateRunning) activityType[loopIndex] |= ACT_WEBSITE_RUNNING;
            if (mqttUpdateRunning) activityType[loopIndex] |= ACT_MQTT_RUNNING;
            if (hassioUpdateRunning) activityType[loopIndex] |= ACT_HASSIO_RUNNING;

            loopIndex = (loopIndex + 1) % LOOP_HISTORY_SIZE;
            if (loopIndex == 0) {
                printTimingAndActivityBatch(loopTiming, activityType, LOOP_HISTORY_SIZE);
                unsigned long reportTime = millis() - lastSendMillisDebug;
                float avgLoopMs = loopCount > 0 ? ((float)reportTime / loopCount) : 0;
                LOGF(DEBUG, "Max time %lu (ms) -- %i entries report time %lu (ms) -- average %0.2f (ms)", maxLoop, LOOP_HISTORY_SIZE, reportTime, avgLoopMs);
                lastSendMillisDebug = millis();
                loopCount = 0;
                maxLoop = 0;
            }
        }
    }
}

void debugTimingLoop2() {

    //Debugging timing
    static unsigned long previousMicrosDebug = 0;
    static unsigned long currentMicrosDebug = 0;
    static unsigned long intervalDebug = 2000000;
    static unsigned long maxloop = 0;
    static unsigned long maxActivity = 0;
    static unsigned long loopTimings[LOOP_HISTORY_SIZE];
    static unsigned int activityLoopTimings[LOOP_HISTORY_SIZE];
    static int loopIndex = 0;
    static bool triggered = false;
    static int triggerCountdown = 0;
    static unsigned long maxDisplay = 0;
    static unsigned long minDisplay = 100000;

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
        if (displayDuration >= maxDisplay) {
            maxDisplay = displayDuration;
        }
        if (displayDuration < minDisplay) {
            minDisplay = displayDuration;
        }

          // Store the loop duration in the circular buffer
        loopTimings[loopIndex] = loopDuration;
        activityLoopTimings[loopIndex] = 0;
        if(displayBufferReady) activityLoopTimings[loopIndex]+= 1;
        if(displayUpdateRunning) activityLoopTimings[loopIndex]+= 10;
        if(websiteUpdateRunning) activityLoopTimings[loopIndex]+= 100;
        if(mqttUpdateRunning) activityLoopTimings[loopIndex]+= 1000;
        if(hassioUpdateRunning) activityLoopTimings[loopIndex]+= 10000;
        if(displayNeedsUpdate) activityLoopTimings[loopIndex]+= 100000;

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
            LOGF(DEBUG, "display micros min: %lu, max: %lu", minDisplay, maxDisplay);
            minDisplay = 100000;
            maxDisplay = 0;
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

    currentMicrosDebug = micros();
}