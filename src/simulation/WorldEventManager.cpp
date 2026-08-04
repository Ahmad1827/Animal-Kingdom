#include "simulation/WorldEventManager.h"
#include <cstdlib>
#include <cmath>
#include <vector>

namespace sim {

void WorldEventManager::handleSeasons(SimulationRegistry& registry, uint64_t ticks) {
    int ticksPerDay = 1440;
    int daysPerSeason = 10;
    int totalDays = static_cast<int>(ticks / ticksPerDay);
    
    registry.setDate((totalDays / (daysPerSeason * 4)) + 1, (totalDays % (daysPerSeason * 4)) + 1);
    
    int seasonIndex = (totalDays / daysPerSeason) % 4;
    registry.setSeason(static_cast<Season>(seasonIndex));
}

void WorldEventManager::generateRandomEvents(SimulationRegistry& registry, uint64_t ticks) {
    if (ticks % 5000 != 0) return;

    if (std::rand() % 100 < 15) {
        WorldEvent e;
        e.id = IDGenerator::generateStructureID();
        e.type = static_cast<EventType>(std::rand() % 10);
        e.startTick = ticks;
        e.durationTicks = 3000 + (std::rand() % 5000);
        e.centerX = (std::rand() % 20000) - 10000.f;
        e.radius = 2000.f + (std::rand() % 3000);
        e.intensity = (std::rand() % 100) / 100.f;
        registry.registerEvent(e);

        HistoricalRecord rec;
        rec.year = registry.getYear();
        rec.day = registry.getDay();
        if (e.type == EventType::Drought) rec.description = "A severe drought began.";
        else if (e.type == EventType::Flood) rec.description = "Flooding destroyed local resources.";
        else if (e.type == EventType::FruitBoom) rec.description = "An abundant harvest occurred.";
        else if (e.type == EventType::Disease) rec.description = "A mysterious plague spread.";
        else rec.description = "A regional anomaly was recorded.";
        registry.addHistory(rec);
    }

    std::vector<EventID> toRemove;
    for (auto& pair : registry.getAllEvents()) {
        if (ticks > pair.second.startTick + pair.second.durationTicks) {
            toRemove.push_back(pair.first);
        }
    }
    for (EventID id : toRemove) registry.removeEvent(id);
}

void WorldEventManager::updateDiseases(SimulationRegistry& registry, uint64_t ticks) {
    if (ticks % 100 != 0) return;

    auto& apes = registry.getAllApes();
    for (auto it1 = apes.begin(); it1 != apes.end(); ++it1) {
        if (!it1->second.alive || it1->second.currentDisease == DiseaseType::None) continue;

        it1->second.health -= (0.5f * it1->second.diseaseSeverity);
        it1->second.diseaseSeverity -= 0.01f;
        
        if (it1->second.health <= 0.f) {
            it1->second.alive = false;
            HistoricalRecord rec;
            rec.year = registry.getYear();
            rec.day = registry.getDay();
            rec.description = it1->second.name + " succumbed to disease.";
            registry.addHistory(rec);
            continue;
        }

        if (it1->second.diseaseSeverity <= 0.f) {
            it1->second.currentDisease = DiseaseType::None;
        }

        for (auto it2 = apes.begin(); it2 != apes.end(); ++it2) {
            if (it1->first != it2->first && it2->second.alive && it2->second.currentDisease == DiseaseType::None) {
                if (std::abs(it1->second.worldX - it2->second.worldX) < 50.f && std::rand() % 100 < 5) {
                    it2->second.currentDisease = it1->second.currentDisease;
                    it2->second.diseaseSeverity = 1.0f;
                }
            }
        }
    }
}

void WorldEventManager::updateAnimals(SimulationRegistry& registry, uint64_t ticks) {
    if (ticks % 600 == 0 && registry.getAllAnimals().size() < 10) {
        AnimalData a;
        a.id = IDGenerator::generateStructureID();
        a.type = static_cast<AnimalType>(std::rand() % 4);
        a.worldX = (std::rand() % 20000) - 10000.f;
        a.worldY = 0.f;
        a.health = 100.f;
        a.state = AnimalState::Roaming;
        a.targetId = 0;
        registry.registerAnimal(a);
    }

    for (auto& pair : registry.getAllAnimals()) {
        if (pair.second.state == AnimalState::Roaming) {
            pair.second.worldX += ((std::rand() % 11) - 5) * 2.0f;
            for (auto& apePair : registry.getAllApes()) {
                if (apePair.second.alive && std::abs(apePair.second.worldX - pair.second.worldX) < 300.f) {
                    pair.second.state = AnimalState::Hunting;
                    pair.second.targetId = apePair.first;
                    break;
                }
            }
        } else if (pair.second.state == AnimalState::Hunting) {
            ApeData* target = registry.getApe(pair.second.targetId);
            if (target && target->alive) {
                if (target->worldX > pair.second.worldX) pair.second.worldX += 3.0f;
                else pair.second.worldX -= 3.0f;
                
                if (std::abs(target->worldX - pair.second.worldX) < 30.f && ticks % 30 == 0) {
                    target->health -= 5.0f;
                    if (target->equippedTool == ToolType::WoodenSpear) pair.second.health -= 50.f;
                }
            } else {
                pair.second.state = AnimalState::Roaming;
            }
        }
        if (pair.second.health <= 0.f) pair.second.state = AnimalState::Sleeping;
    }
}

void WorldEventManager::update(SimulationRegistry& registry, uint64_t ticks) {
    handleSeasons(registry, ticks);
    generateRandomEvents(registry, ticks);
    updateDiseases(registry, ticks);
    updateAnimals(registry, ticks);
}

float WorldEventManager::getFoodGrowthModifier(SimulationRegistry& registry, float x) {
    float mod = 1.0f;
    if (registry.getSeason() == Season::Winter) mod *= 0.2f;
    if (registry.getSeason() == Season::Spring) mod *= 1.5f;

    for (const auto& pair : registry.getAllEvents()) {
        if (std::abs(pair.second.centerX - x) < pair.second.radius) {
            if (pair.second.type == EventType::Drought) mod *= 0.1f;
            if (pair.second.type == EventType::FruitBoom) mod *= 3.0f;
            if (pair.second.type == EventType::ColdWave) mod *= 0.3f;
        }
    }
    return mod;
}

float WorldEventManager::getWeatherModifier(SimulationRegistry& registry, float x) {
    float mod = 1.0f;
    for (const auto& pair : registry.getAllEvents()) {
        if (std::abs(pair.second.centerX - x) < pair.second.radius) {
            if (pair.second.type == EventType::Storm) mod = 3.0f;
            if (pair.second.type == EventType::HeavyRain) mod = 2.0f;
        }
    }
    return mod;
}

}