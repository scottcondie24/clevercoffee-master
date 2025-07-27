#pragma once

#include <Stream.h>
#include <vector>

typedef enum {
    EXIT_TYPE_NONE,
    EXIT_TYPE_FLOW_UNDER,
    EXIT_TYPE_FLOW_OVER,
    EXIT_TYPE_PRESSURE_UNDER,
    EXIT_TYPE_PRESSURE_OVER,
} ExitType;

typedef enum {
    TRANSITION_NONE,
    TRANSITION_SMOOTH,
    TRANSITION_FAST,
    TRANSITION_HOLD, // not used yet
} TransitionType;

typedef enum {
    POWER,
    PRESSURE,
    FLOW,
    PROFILE,
} PumpMode;

typedef struct {
        const char* name;
        float pressure, flow, volume, weight;
        float exit_flow_under, exit_flow_over;
        float exit_pressure_over, exit_pressure_under;
        float max_secondary, max_secondary_range;
        float seconds;
        ExitType exit_type;
        TransitionType transition;
        PumpMode pump;
} BrewPhase;

typedef struct {
        const char* name;
        const char* shortname;
        BrewPhase* phases;
        int phaseCount;
        float temperature;
        float time;
        bool scales;
        bool flow;
} BrewProfile;

struct BrewProfileInfo {
        String name;
        String shortname;
        // description
};

// converting to JSON for profiles

extern BrewProfile* currentProfile;
extern size_t profilesCount;
extern std::vector<BrewProfileInfo> profileInfo;

// void populateProfileNames();
// BrewProfile* getProfile(size_t i);
ExitType parseExitType(const char* str);
TransitionType parseTransition(const char* str);
PumpMode parsePump(const char* str);

void loadProfileMetadata();
void selectProfileByName(const String& name);

// void parseDefaultProfiles();
// bool loadProfile(const char* json, BrewPhase* phases, size_t maxPhases, size_t& outCount);
// void saveProfile(BrewPhase* phases, size_t count, Stream& out);