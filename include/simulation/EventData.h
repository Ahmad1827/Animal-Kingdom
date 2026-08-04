#pragma once
#include "simulation/EntityID.h"
#include <string>

namespace sim {

enum class EventType { Drought, Flood, HeavyRain, Storm, HeatWave, ColdWave, Disease, FruitBoom, ForestFire, PredatorMigration };
enum class Season { Spring, Summer, Autumn, Winter };

struct WorldEvent {
    EventID id;
    EventType type;
    uint64_t startTick;
    uint64_t durationTicks;
    float centerX;
    float radius;
    float intensity;
};

struct HistoricalRecord {
    int year;
    int day;
    std::string description;
};

}