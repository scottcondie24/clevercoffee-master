#pragma once

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
    TRANSITION_HOLD,    //not used yet
} TransitionType;


typedef enum {
    NONE = 0,
    POWER = 1,
    PRESSURE = 2,
    FLOW = 4,
} PumpControl;

typedef struct {
    const char* name;
    float pressure, flow, volume, temperature, weight;
    float exit_flow_under, exit_flow_over;
    float exit_pressure_over, exit_pressure_under;
    float max_flow_or_pressure, max_flow_or_pressure_range;
    float seconds;
    ExitType exit_type;
    TransitionType transition;
    PumpControl pump;
} BrewPhase;

typedef struct {
    const char* name;
    BrewPhase* phases;
    int phaseCount;
    float targetTemperature;
    float targetTime;
    bool scalesRequired;
    bool flowRequired;
} BrewRecipe;

extern const int recipesCount;
extern BrewRecipe recipes[];