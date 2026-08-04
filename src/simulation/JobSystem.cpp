#include "simulation/JobSystem.h"
#include <cmath>

namespace sim {

EntityID JobSystem::findNearestFoodNode(SimulationRegistry& registry, float x, float y) {
    EntityID best = 0;
    float bestDist = 999999.f;
    for (auto& pair : registry.getAllResources()) {
        if (pair.second.type == ResourceType::Food && pair.second.amount > 0) {
            float dist = std::abs(pair.second.worldX - x);
            if (dist < bestDist) {
                bestDist = dist;
                best = pair.first;
            }
        }
    }
    return best;
}

void JobSystem::assignLeaderJobs(SimulationRegistry& registry, VillageData& village) {
    ApeData* leader = registry.getApe(village.leaderId);
    if (!leader || !leader->alive) return;
    leader->currentJob = Job::Guard;
    leader->currentTargetNode = 0;
}

void JobSystem::updateJobs(SimulationRegistry& registry, float timeOfDay) {
    bool isNight = (timeOfDay > 0.8f || timeOfDay < 0.2f);
    bool isEvening = (timeOfDay >= 0.7f && timeOfDay <= 0.8f);

    for (auto& pair : registry.getAllVillages()) {
        assignLeaderJobs(registry, pair.second);
    }

    for (auto& pair : registry.getAllResources()) {
        if (pair.second.amount < pair.second.maxAmount) {
            pair.second.regrowTimer += 1.0f;
            if (pair.second.regrowTimer > 500.f) {
                pair.second.amount++;
                pair.second.regrowTimer = 0.f;
            }
        }
    }

    for (auto& pair : registry.getAllApes()) {
        ApeData& ape = pair.second;
        if (!ape.alive) continue;

        ape.hunger -= 0.05f;
        if (ape.hunger < 0.f) ape.hunger = 0.f;

        VillageData* village = registry.getVillage(ape.villageId);
        
        if (isNight) {
            ape.currentJob = Job::Sleep;
            ape.currentTargetNode = 0;
            continue;
        }

        if (isEvening) {
            if (ape.heldFood > 0) {
                ape.currentJob = Job::CarryFood;
            } else {
                ape.currentJob = Job::Socialize;
            }
            ape.currentTargetNode = 0;
            continue;
        }

        if (ape.heldFood > 0) {
            ape.currentJob = Job::CarryFood;
            if (village && std::abs(ape.worldX - village->centerX) < 50.f) {
                village->food += ape.heldFood;
                ape.heldFood = 0;
                ape.currentJob = Job::Idle;
            }
            continue;
        }

        if (ape.hunger < 40.f) {
            if (village && village->food > 0 && std::abs(ape.worldX - village->centerX) < 150.f) {
                ape.currentJob = Job::Eat;
                village->food--;
                ape.hunger = 100.f;
            } else {
                ape.currentJob = Job::Forage;
                if (ape.currentTargetNode == 0) {
                    ape.currentTargetNode = findNearestFoodNode(registry, ape.worldX, ape.worldY);
                }
            }
            continue;
        }

        if (village && ape.id != village->leaderId) {
            if (village->food < village->members.size() * 2) {
                ape.currentJob = Job::Forage;
                if (ape.currentTargetNode == 0) {
                    ape.currentTargetNode = findNearestFoodNode(registry, ape.worldX, ape.worldY);
                }
            } else {
                ape.currentJob = Job::Wander;
            }
        }
    }
}
}