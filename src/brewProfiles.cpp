#include "brewProfiles.h"

// name,pressure,flow,volume,weight,exit_flow_under,exit_flow_over,exit_pressure_over,exit_pressure_under,max_secondary,max_secondary_range,seconds,exit_type,transition, pumpmode;
//   0       1       2   3       4           5            6               7               8                   9                       10         11      12      13         14

BrewPhase springLeverPhases[] = {
    // 0    1   2   3   4  5  6   7    8  9  10  11         12                      13             14
    {"infuse", 0, 8.0, 0, 0, 0, 0.0, 4.0, 0, 0, 0.0, 20.0, EXIT_TYPE_PRESSURE_OVER, TRANSITION_FAST, FLOW},
    {"rise and hold", 8.6, 0.0, 0, 0, 0, 0, 0, 0, 0, 0.0, 4.0, EXIT_TYPE_NONE, TRANSITION_FAST, PRESSURE},
    {"decline", 6.0, 0.0, 0, 0, 0, 0.0, 0.0, 6.0, 0.0, 0.0, 30.0, EXIT_TYPE_PRESSURE_UNDER, TRANSITION_SMOOTH, PRESSURE},
    {"maintain flow", 0.0, 1.5, 0, 0, 0, 0, 0, 0, 0.0, 0.0, 30.0, EXIT_TYPE_NONE, TRANSITION_SMOOTH, FLOW},
};
const int springLeverPhasesCount = sizeof(springLeverPhases) / sizeof(BrewPhase);

BrewPhase adaptivePhases[] = {
    // 0    1  2  3  4 5  6   7  8   9  10  11        12              13             14
    {"pause", 0, 0, 0, 0, 0, 0, 0, 0, 0.0, 0.0, 1.0, EXIT_TYPE_NONE, TRANSITION_FAST, FLOW},                   // wait for currBrewWeight to be reset
    {"fill", 0, 8.0, 0, 5.0, 0, 0.0, 3.0, 0, 0.0, 0.0, 20.0, EXIT_TYPE_PRESSURE_OVER, TRANSITION_FAST, FLOW},
    {"infuse", 3.0, 1.7, 0, 4.0, 0, 0, 0, 0, 8.5, 5.0, 30.0, EXIT_TYPE_NONE, TRANSITION_FAST, PRESSURE},       // exits at 4g
    {"maintain flow", 0.0, 1.7, 0, 50.0, 0, 0, 0, 0, 8.6, 0.5, 80.0, EXIT_TYPE_NONE, TRANSITION_SMOOTH, FLOW}, // exits at 50g, ramp by weight
};
const int adaptivePhasesCount = sizeof(adaptivePhases) / sizeof(BrewPhase);

BrewPhase londiniumRPhases[] = {
    // 0   1  2  3  4  5  6  7  8   9   10   11      12              13              14
    {"pause", 0, 0, 0, 0, 0, 0, 0, 0, 0.0, 0.0, 1.0, EXIT_TYPE_NONE, TRANSITION_FAST, FLOW},                     // wait for currBrewWeight to be reset
    {"fill", 0, 12.0, 0, 5.0, 0, 0.0, 2.5, 0, 0.0, 0.0, 20.0, EXIT_TYPE_PRESSURE_OVER, TRANSITION_FAST, FLOW},
    {"infuse", 3.0, 1.7, 0, 4.0, 0, 0, 0, 0, 0.0, 0.0, 30.0, EXIT_TYPE_NONE, TRANSITION_FAST, PRESSURE},         // exits at 4g
    {"rise and hold", 8.6, 0.0, 0, 0, 0, 0, 0.0, 0.0, 0.0, 0.0, 2.0, EXIT_TYPE_NONE, TRANSITION_FAST, PRESSURE}, // ramp so OPV doesnt immediately open
    {"decline", 5.0, 0, 0, 36.0, 0, 0.0, 0.0, 0.0, 0.0, 0.0, 45.0, EXIT_TYPE_NONE, TRANSITION_SMOOTH, PRESSURE}, // ramp by weight
    {"end", 0.0, 0.1, 0, 38.0, 0, 0, 0, 0, 8.0, 0.5, 5.0, EXIT_TYPE_NONE, TRANSITION_SMOOTH, FLOW},              // ramp by weight
};
const int londiniumRPhasesCount = sizeof(londiniumRPhases) / sizeof(BrewPhase);

BrewPhase londiniumVPhases[] = {
    // 0  1   2  3  4  5  6  7  8   9   10   11      12              13              14
    {"pause", 0, 0, 0, 0, 0, 0, 0, 0, 0.0, 0.0, 1.0, EXIT_TYPE_NONE, TRANSITION_FAST, FLOW},                     // wait for currBrewWeight to be reset
    {"fill", 0, 12.0, 0, 5.0, 0, 0.0, 1.0, 0, 0.0, 0.0, 20.0, EXIT_TYPE_PRESSURE_OVER, TRANSITION_FAST, FLOW},
    {"infuse", 1.2, 1.7, 0, 4.0, 0, 0, 0, 0, 0.0, 0.0, 30.0, EXIT_TYPE_NONE, TRANSITION_FAST, PRESSURE},         // exits at 4g
    {"rise and hold", 6.5, 0.0, 0, 0, 0, 0, 0.0, 0.0, 0.0, 0.0, 2.0, EXIT_TYPE_NONE, TRANSITION_FAST, PRESSURE}, // ramp so OPV doesnt immediately open
    {"decline", 4.0, 0, 0, 36.0, 0, 0.0, 0.0, 0.0, 0.0, 0.0, 45.0, EXIT_TYPE_NONE, TRANSITION_SMOOTH, PRESSURE}, // ramp by weight
    {"end", 0.0, 0.1, 0, 38.0, 0, 0, 0, 0, 0.0, 0.0, 5.0, EXIT_TYPE_NONE, TRANSITION_SMOOTH, FLOW},              // ramp by weight
};
const int londiniumVPhasesCount = sizeof(londiniumVPhases) / sizeof(BrewPhase);

BrewPhase londiniumRPCPhases[] = {
    // 0   1  2  3  4  5  6  7  8   9   10    11    12              13               14
    {"pause", 0, 0, 0, 0, 0, 0, 0, 0, 0.0, 0.0, 1.0, EXIT_TYPE_NONE, TRANSITION_FAST, PRESSURE},                 // wait for currBrewWeight to be reset
    {"fill", 3.0, 0.0, 0, 5.0, 0, 0.0, 2.5, 0, 0.0, 0.0, 20.0, EXIT_TYPE_PRESSURE_OVER, TRANSITION_FAST, PRESSURE},
    {"infuse", 3.0, 0.0, 0, 4.0, 0, 0, 0, 0, 0.0, 0.0, 30.0, EXIT_TYPE_NONE, TRANSITION_FAST, PRESSURE},         // exits at 4g
    {"rise and hold", 8.6, 0.0, 0, 0, 0, 0, 0.0, 0.0, 0.0, 0.0, 2.0, EXIT_TYPE_NONE, TRANSITION_FAST, PRESSURE}, // ramp so OPV doesnt immediately open
    {"decline", 5.0, 0, 0, 36.0, 0, 0.0, 0.0, 0.0, 0.0, 0.0, 45.0, EXIT_TYPE_NONE, TRANSITION_SMOOTH, PRESSURE}, // ramp by weight
    {"end", 0.1, 0.0, 0, 38.0, 0, 0, 0, 0, 8.0, 0.5, 5.0, EXIT_TYPE_NONE, TRANSITION_SMOOTH, PRESSURE},          // ramp by weight
};
const int londiniumRPCPhasesCount = sizeof(londiniumRPCPhases) / sizeof(BrewPhase);

BrewPhase londiniumVPCPhases[] = {
    // 0   1  2  3  4  5  6  7  8  9    10    11   12               13              14
    {"pause", 0, 0, 0, 0, 0, 0, 0, 0, 0.0, 0.0, 1.0, EXIT_TYPE_NONE, TRANSITION_FAST, PRESSURE},                 // wait for weightBrewed to be reset
    {"fill", 3.00, 0.0, 0, 5.0, 0, 0.0, 1.0, 0, 0.0, 0.0, 20.0, EXIT_TYPE_PRESSURE_OVER, TRANSITION_FAST, PRESSURE},
    {"infuse", 1.2, 0.0, 0, 4.0, 0, 0, 0, 0, 0.0, 0.0, 30.0, EXIT_TYPE_NONE, TRANSITION_FAST, PRESSURE},         // exits at 4g
    {"rise and hold", 6.5, 0.0, 0, 0, 0, 0, 0.0, 0.0, 0.0, 0.0, 2.0, EXIT_TYPE_NONE, TRANSITION_FAST, PRESSURE}, // ramp so OPV doesnt immediately open
    {"decline", 4.0, 0, 0, 36.0, 0, 0.0, 0.0, 0.0, 0.0, 0.0, 45.0, EXIT_TYPE_NONE, TRANSITION_SMOOTH, PRESSURE}, // ramp by weight
    {"end", 0.1, 0.0, 0, 38.0, 0, 0, 0, 0, 0.0, 0.0, 5.0, EXIT_TYPE_NONE, TRANSITION_SMOOTH, PRESSURE},          // ramp by weight
};
const int londiniumVPCPhasesCount = sizeof(londiniumVPCPhases) / sizeof(BrewPhase);

BrewPhase lightRoastPhases[] = {
    // 0   1    2   3  4  5  6   7   8  9   10    11   12               13              14
    {"fill", 3.5, 0.0, 0, 0, 0, 0, 3.0, 0, 0, 0.0, 12.0, EXIT_TYPE_PRESSURE_OVER, TRANSITION_FAST, PRESSURE}, {"fill end", 3.0, 0.0, 0, 0, 1.0, 0.0, 0.0, 0, 0, 0.0, 12.0, EXIT_TYPE_FLOW_UNDER, TRANSITION_FAST, PRESSURE},
    {"infuse", 1.5, 0.0, 12.0, 0, 0, 0, 0, 0, 0, 0.0, 13.0, EXIT_TYPE_NONE, TRANSITION_SMOOTH, PRESSURE},     {"pressure", 10.0, 0.0, 0, 0, 0, 0, 8.0, 0, 0.0, 0.0, 6.0, EXIT_TYPE_PRESSURE_OVER, TRANSITION_FAST, PRESSURE},
    {"extract", 0.0, 3.4, 0, 0, 0, 0, 0, 0, 0.0, 0.0, 30.0, EXIT_TYPE_NONE, TRANSITION_SMOOTH, FLOW},
};
const int lightRoastPhasesCount = sizeof(lightRoastPhases) / sizeof(BrewPhase);

BrewPhase sixBarEspressoPhases[] = {
    // 0    1   2   3  4  5  6     7   8  9  10   11       12                      13              14
    {"infuse", 0, 8.0, 0, 0, 0, 0.0, 4.0, 0, 0, 0.0, 20.0, EXIT_TYPE_PRESSURE_OVER, TRANSITION_FAST, FLOW},
    {"rise and hold", 6.0, 0.0, 0, 0, 0, 0, 0, 0, 0, 0.0, 16.0, EXIT_TYPE_NONE, TRANSITION_FAST, PRESSURE},
    {"decline", 4.0, 0.0, 0, 36.0, 0, 0.0, 0.0, 4.0, 0.0, 0.0, 30.0, EXIT_TYPE_PRESSURE_UNDER, TRANSITION_SMOOTH, PRESSURE},
};
const int sixBarEspressoPhasesCount = sizeof(sixBarEspressoPhases) / sizeof(BrewPhase);

BrewPhase bloomingEspressoPhases[] = {
    // 0    1   2   3  4  5  6   7   8  9  10   11       12                      13              14
    {"infuse", 0, 4.0, 0, 0, 0, 0, 1.0, 0, 0, 0, 23.0, EXIT_TYPE_PRESSURE_OVER, TRANSITION_FAST, FLOW},
    {"pause", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 30.0, EXIT_TYPE_NONE, TRANSITION_FAST, FLOW},
    {"ramp", 0, 2.2, 0, 0, 0, 0, 0, 0, 0, 0, 5.0, EXIT_TYPE_NONE, TRANSITION_SMOOTH, FLOW},
    {"flow", 0, 2.2, 0, 0, 0, 0, 0, 0, 0.0, 0.0, 20.0, EXIT_TYPE_NONE, TRANSITION_SMOOTH, FLOW},
};
const int bloomingEspressoPhasesCount = sizeof(bloomingEspressoPhases) / sizeof(BrewPhase);

BrewPhase pressurizedBloomPhases[] = {
    // 0    1   2   3  4  5  6   7   8  9  10   11       12                      13           14
    {"fill", 0, 8.0, 0, 0, 0, 0, 3.0, 0, 0, 0, 15.0, EXIT_TYPE_PRESSURE_OVER, TRANSITION_FAST, FLOW},   {"pressure", 3.0, 0, 0, 0, 0.9, 0, 0, 0, 0, 0, 12.0, EXIT_TYPE_FLOW_UNDER, TRANSITION_FAST, PRESSURE},
    {"hold", 0.1, 0, 0, 0, 0, 1.0, 0, 0, 0, 0, 6.0, EXIT_TYPE_FLOW_OVER, TRANSITION_FAST, PRESSURE},    {"pressure", 11.0, 0, 0.0, 0, 0, 0.0, 8.8, 0, 0.0, 0.0, 6.0, EXIT_TYPE_PRESSURE_OVER, TRANSITION_SMOOTH, PRESSURE},
    {"extract", 0.0, 3.5, 0.0, 0, 0, 0, 0, 0, 0.0, 0.0, 60.0, EXIT_TYPE_NONE, TRANSITION_SMOOTH, FLOW},
};
const int pressurizedBloomPhasesCount = sizeof(pressurizedBloomPhases) / sizeof(BrewPhase);

BrewPhase calibrateFlowPhases[] = {
    // 0    1   2   3   4  5  6  7  8  9  10   11    12               13              14
    {"2.0mL/s", 0, 2.0, 0, 0, 0, 0, 0, 0, 0, 0.6, 8.0, EXIT_TYPE_NONE, TRANSITION_FAST, FLOW},
    {"4.0mL/s", 0, 4.0, 0, 0, 0, 0, 0, 0, 0, 0.6, 8.0, EXIT_TYPE_NONE, TRANSITION_FAST, FLOW},
    {"6.0mL/s", 0, 6.0, 0, 0, 0, 0, 0, 0, 0, 0.6, 8.0, EXIT_TYPE_NONE, TRANSITION_FAST, FLOW},
    {"8.0mL/s", 0, 8.0, 0, 0, 0, 0, 0, 0, 0, 0.6, 8.0, EXIT_TYPE_NONE, TRANSITION_FAST, FLOW},
};
const int calibrateFlowPhasesCount = sizeof(calibrateFlowPhases) / sizeof(BrewPhase);

BrewPhase testRampPhases[] = {
    // 0             1   2   3   4 5  6  7  8  9  10   11     12               13               14
    {"increasing flow", 0, 4.0, 0, 0, 0, 0, 0, 0, 0, 0.6, 4.0, EXIT_TYPE_NONE, TRANSITION_SMOOTH, FLOW},
    {"decreasing flow", 0, 1.0, 0, 0, 0, 0, 0, 0, 0, 0.6, 4.0, EXIT_TYPE_NONE, TRANSITION_SMOOTH, FLOW},
    {"increasing flow", 0, 6.0, 0, 0, 0, 0, 0, 0, 0, 0.6, 4.0, EXIT_TYPE_NONE, TRANSITION_SMOOTH, FLOW},
    {"decreasing flow", 0, 0.0, 0, 0, 0, 0, 0, 0, 0, 0.6, 4.0, EXIT_TYPE_NONE, TRANSITION_SMOOTH, FLOW},
};
const int testRampPhasesCount = sizeof(testRampPhases) / sizeof(BrewPhase);

BrewProfile profiles[] = {
    // displayname, Phases, Count, Temp, Time, Scales, Flow
    {"springLever", springLeverPhases, springLeverPhasesCount, 90.0, 0, false, true},
    {"adaptive", adaptivePhases, adaptivePhasesCount, 88.0, 0, true, true},
    {"londiniumR", londiniumRPhases, londiniumRPhasesCount, 88.0, 0, true, true},
    {"londiniumV", londiniumVPhases, londiniumVPhasesCount, 88.0, 0, true, true},
    {"londiniumRPressure", londiniumRPCPhases, londiniumRPCPhasesCount, 88.0, 0, true, false},
    {"londiniumVPressure", londiniumVPCPhases, londiniumVPCPhasesCount, 88.0, 0, true, false},
    {"lightRoast", lightRoastPhases, lightRoastPhasesCount, 92.0, 0, false, true},
    {"sixBarEspresso", sixBarEspressoPhases, sixBarEspressoPhasesCount, 90.0, 0, true, true},
    {"bloomingEspresso", bloomingEspressoPhases, bloomingEspressoPhasesCount, 92.0, 0, false, true},
    {"pressurizedBloom", pressurizedBloomPhases, pressurizedBloomPhasesCount, 93.0, 0, false, true},
    {"calibrateFlow", calibrateFlowPhases, calibrateFlowPhasesCount, 90.0, 0, false, true},
    {"testRampFlow", testRampPhases, testRampPhasesCount, 90.0, 0, false, true},
};

const int profilesCount = sizeof(profiles) / sizeof(BrewProfile);

// these profiles could be written like this for better visibility

/*BrewPhase londiniumPhases[] = {
    {
        .name = "pause",
        .pressure = 0,
        .flow = 0,
        .volume = 0,
        .weight = 0,
        .exit_flow_under = 0,
        .exit_flow_over = 0,
        .exit_pressure_over = 0,
        .exit_pressure_under = 0,
        .max_flow_or_pressure = 0,
        .max_flow_or_pressure_range = 0,
        .seconds = 1.0,
        .exit_type = EXIT_TYPE_NONE,
        .transition = TRANSITION_FAST,
        .pump = FLOW,
    },
    {
        .name = "fill",
        .pressure = 0,
        .flow = 8.0,
        .volume = 0,
        .weight = 5.0,
        .exit_flow_under = 0,
        .exit_flow_over = 0.0,
        .exit_pressure_over = 3.0,
        .exit_pressure_under = 0,
        .max_flow_or_pressure = 0,
        .max_flow_or_pressure_range = 0.0,
        .seconds = 20.0,
        .exit_type = EXIT_TYPE_PRESSURE_OVER,
        .transition = TRANSITION_FAST,
        .pump = FLOW,
    },*/