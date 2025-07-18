#include "brewProfiles.h"
#include "Logger.h"
#include <ArduinoJson.h>
#include <LittleFS.h>

std::vector<BrewProfile> loadedProfiles;
std::vector<const char*> profileNames;

size_t profilesCount = 0;

const char* exitTypeStrs[] = {"none", "flow_under", "flow_over", "pressure_under", "pressure_over"};
const char* transitionStrs[] = {"none", "smooth", "fast", "hold"};
const char* pumpModeStrs[] = {"power", "pressure", "flow"};

bool loadDefaultProfilesFromFS(JsonDocument& doc) {
    File file = LittleFS.open("/profiles/defaultProfiles.json", "r");

    if (!file) {
        LOG(WARNING, "Failed to open default profiles file!");
        return false;
    }

    DeserializationError error = deserializeJson(doc, file);

    if (error) {
        LOGF(WARNING, "Failed to parse default profiles: %s", error.c_str());
        return false;
    }

    return true;
}

void populateProfileNames() {
    profileNames.clear();
    for (auto& profile : loadedProfiles) {
        profileNames.push_back(profile.name);
    }
}

BrewProfile* getProfile(size_t i) {
    if (i < loadedProfiles.size()) {
        return &loadedProfiles[i];
    }

    return nullptr;
}

ExitType parseExitType(const char* str) {
    for (int i = 0; i < sizeof(exitTypeStrs) / sizeof(exitTypeStrs[0]); ++i) {
        if (strcmp(str, exitTypeStrs[i]) == 0) {
            return static_cast<ExitType>(i);
        }
    }

    LOGF(WARNING, "Unknown exit_type string: '%s'", str);
    return EXIT_TYPE_NONE;
}

TransitionType parseTransition(const char* str) {
    for (int i = 0; i < sizeof(transitionStrs) / sizeof(transitionStrs[0]); ++i) {
        if (strcmp(str, transitionStrs[i]) == 0) {
            return static_cast<TransitionType>(i);
        }
    }

    LOGF(WARNING, "Unknown transition string: '%s'", str);
    return TRANSITION_NONE;
}

PumpMode parsePumpMode(const char* str) {
    for (int i = 0; i < sizeof(pumpModeStrs) / sizeof(pumpModeStrs[0]); ++i) {
        if (strcmp(str, pumpModeStrs[i]) == 0) {
            return static_cast<PumpMode>(i);
        }
    }

    LOGF(WARNING, "Unknown pump mode string: '%s'", str);
    return POWER;
}

void parseDefaultProfiles() {
    JsonDocument doc;

    if (!loadDefaultProfilesFromFS(doc)) {
        return;
    }

    for (JsonObject profileJson : doc.as<JsonArray>()) {
        // bool requiresScales = profileJson["scales"] | false;
        // bool requiresFlow = profileJson["flow"] | false;

        // if ((requiresScales && !config.get<bool>("hardware.sensors.scale.enabled")) || (requiresFlow && config.get<bool>("dimmer.type"))) {
        //     continue;
        // }

        BrewProfile profile;
        profile.name = strdup(profileJson["name"]); // optional: use strdup to persist
        profile.shortname = profileJson["shortname"];
        profile.temperature = profileJson["temperature"];
        profile.scales = profileJson["scales"];
        profile.flow = profileJson["flow"];

        JsonArray phasesJson = profileJson["phases"];
        profile.phaseCount = phasesJson.size();
        profile.phases = new BrewPhase[profile.phaseCount]; // allocate dynamically

        int i = 0;

        for (JsonObject phaseJson : phasesJson) {
            BrewPhase& p = profile.phases[i++];
            memset(&p, 0, sizeof(BrewPhase)); // zero everything by default

            p.name = strdup(phaseJson["name"]);

            // Optional fields
            p.pressure = phaseJson["pressure"] | 0.0;
            p.flow = phaseJson["flow"] | 0.0;
            p.volume = phaseJson["volume"] | 0.0;
            p.weight = phaseJson["weight"] | 0.0;
            p.exit_flow_under = phaseJson["exit_flow_under"] | 0.0;
            p.exit_flow_over = phaseJson["exit_flow_over"] | 0.0;
            p.exit_pressure_over = phaseJson["exit_pressure_over"] | 0.0;
            p.exit_pressure_under = phaseJson["exit_pressure_under"] | 0.0;
            p.max_secondary = phaseJson["max_secondary"] | 0.0;
            p.max_secondary_range = phaseJson["max_secondary_range"] | 0.0;
            p.seconds = phaseJson["seconds"] | 0.0;

            // Enums from string
            const char* exitTypeStr = phaseJson["exit_type"] | "none";
            const char* transitionStr = phaseJson["transition"] | "fast";
            const char* pumpStr = phaseJson["pump"] | "pressure";

            p.exit_type = parseExitType(exitTypeStr);
            p.transition = parseTransition(transitionStr);
            p.pump = parsePumpMode(pumpStr);
        }

        loadedProfiles.push_back(profile);
    }
}

bool loadProfile(const char* json, BrewPhase* phases, size_t maxPhases, size_t& outCount) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, json);
    if (err) {
        return false;
    }

    JsonArray arr = doc.as<JsonArray>();
    size_t count = 0;

    for (JsonObject obj : arr) {
        if (count >= maxPhases) {
            break;
        }

        BrewPhase& p = phases[count];
        p = {};
        p.name = obj["name"] | "";
        p.pressure = obj["pressure"] | 0.0;
        p.flow = obj["flow"] | 0.0;
        p.volume = obj["volume"] | 0.0;
        p.weight = obj["weight"] | 0.0;
        p.exit_flow_under = obj["exit_flow_under"] | 0.0;
        p.exit_flow_over = obj["exit_flow_over"] | 0.0;
        p.exit_pressure_over = obj["exit_pressure_over"] | 0.0;
        p.exit_pressure_under = obj["exit_pressure_under"] | 0.0;
        p.max_secondary = obj["max_secondary"] | 0.0;
        p.max_secondary_range = obj["max_secondary_range"] | 0.0;
        p.seconds = obj["seconds"] | 0.0;
        p.exit_type = parseExitType(obj["exit_type"] | "none");
        p.transition = parseTransition(obj["transition"] | "fast");
        p.pump = parsePump(obj["pump"] | "pressure");

        count++;
    }

    outCount = count;
    return true;
}

void saveProfile(BrewPhase* phases, size_t count, Stream& out) {
    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();

    for (size_t i = 0; i < count; ++i) {
        BrewPhase& p = phases[i];
        JsonObject o = arr.add<JsonObject>();

        o["name"] = p.name;
        if (p.pressure != 0.0) o["pressure"] = p.pressure;
        if (p.flow != 0.0) o["flow"] = p.flow;
        if (p.volume != 0.0) o["volume"] = p.volume;
        if (p.weight != 0.0) o["weight"] = p.weight;
        if (p.exit_flow_under != 0.0) o["exit_flow_under"] = p.exit_flow_under;
        if (p.exit_flow_over != 0.0) o["exit_flow_over"] = p.exit_flow_over;
        if (p.exit_pressure_under != 0.0) o["exit_pressure_under"] = p.exit_pressure_under;
        if (p.exit_pressure_over != 0.0) o["exit_pressure_over"] = p.exit_pressure_over;
        if (p.max_secondary != 0.0) o["max_secondary"] = p.max_secondary;
        if (p.max_secondary_range != 0.0) o["max_secondary_range"] = p.max_secondary_range;
        if (p.seconds != 0.0) o["seconds"] = p.seconds;
        o["exit_type"] = exitTypeStrs[p.exit_type];
        o["transition"] = transitionStrs[p.transition];
        o["pump"] = pumpModeStrs[p.pump];
    }

    serializeJsonPretty(doc, out);
}
