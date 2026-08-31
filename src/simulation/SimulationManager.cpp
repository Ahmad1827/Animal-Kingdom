#include "simulation/SimulationManager.h"
#include "simulation/JobSystem.h"
#include <cmath>
#include <algorithm>

namespace sim {

SimulationManager::SimulationManager() : isPaused(false), controlledApeID(0) {}

void SimulationManager::update(float dt) {
    if (isPaused) return;
    clock.update(dt);
    while (clock.consumeTick()) {
        tick();
    }
}

void SimulationManager::tick() {
    JobSystem::updateJobs(registry, clock.getTimeOfDay(), clock.getTotalTicks());
    simulateAging();
    simulatePregnancies();
    simulateDiplomacy();
    simulateAI();

    for (auto& pair : registry.getAllApes()) {
        ApeData& ape = pair.second;
        if (ape.alive && ape.hasTravelDestination) {
            float dist = ape.travelDestinationX - ape.worldX;
            if (std::abs(dist) > 5.0f) {
                ape.worldX += (dist > 0 ? 1.0f : -1.0f) * 6.0f; 
            } else {
                ape.worldX = ape.travelDestinationX;
                if (ape.currentJob == Job::March) {
                    ape.currentJob = Job::Gathering;
                }
            }
        }
    }
}

void SimulationManager::simulateAging() {}
void SimulationManager::simulateHunger() {} 
void SimulationManager::simulatePregnancies() {}
void SimulationManager::simulateResourceProduction() {}
void SimulationManager::simulateDiplomacy() {}

void SimulationManager::simulateAI() {
    for (auto& vPair : registry.getAllVillages()) {
        VillageData& village = vPair.second;

        if (village.isGatheringActive) {
            std::vector<EntityID> mainApes;
            for (EntityID mId : village.members) {
                ApeData* ape = registry.getApe(mId);
                if (ape && ape->alive && ape->isMainApe && ape->id != controlledApeID) {
                    mainApes.push_back(ape->id);
                }
            }

            float spacing = 80.0f;
            int count = static_cast<int>(mainApes.size());
            for (int i = 0; i < count; ++i) {
                ApeData* ape = registry.getApe(mainApes[i]);
                if (!ape) continue;

                float dir = (i % 2 == 0) ? -1.0f : 1.0f;
                float slotOffset = ((i / 2) + 1) * spacing;
                float targetSpot = village.throneX + (dir * slotOffset);

                ape->hasTravelDestination = true;
                ape->travelDestinationX = targetSpot;
                if (std::abs(ape->worldX - targetSpot) > 5.0f) {
                    ape->currentJob = Job::March;
                } else {
                    ape->currentJob = Job::Gathering;
                }
            }
            continue;
        }

        std::vector<EntityID> lesserApes;
        for (EntityID mId : village.members) {
            ApeData* ape = registry.getApe(mId);
            if (ape && ape->alive && !ape->isMainApe && ape->id != controlledApeID) {
                lesserApes.push_back(ape->id);
            }
        }

        if (village.warChiefId != 0) {
            ApeData* warChief = registry.getApe(village.warChiefId);
            if (warChief && warChief->alive) {
                int assignedGuards = 0;
                for (EntityID lId : lesserApes) {
                    ApeData* ape = registry.getApe(lId);
                    if (!ape) continue;
                    if (ape->currentOccupation == Occupation::Guard) {
                        assignedGuards++;
                    }
                }
                for (EntityID lId : lesserApes) {
                    if (assignedGuards >= 2) break;
                    ApeData* ape = registry.getApe(lId);
                    if (ape && ape->currentOccupation == Occupation::Unemployed) {
                        ape->currentOccupation = Occupation::Guard;
                        ape->currentJob = Job::Guard;
                        assignedGuards++;
                    }
                }
            }
        }

        if (village.chiefBuilderId != 0) {
            ApeData* chiefBuilder = registry.getApe(village.chiefBuilderId);
            if (chiefBuilder && chiefBuilder->alive) {
                int assignedBuilders = 0;
                for (EntityID lId : lesserApes) {
                    ApeData* ape = registry.getApe(lId);
                    if (!ape) continue;
                    if (ape->currentOccupation == Occupation::Builder) {
                        assignedBuilders++;
                    }
                }
                for (EntityID lId : lesserApes) {
                    if (assignedBuilders >= 2) break;
                    ApeData* ape = registry.getApe(lId);
                    if (ape && ape->currentOccupation == Occupation::Unemployed) {
                        ape->currentOccupation = Occupation::Builder;
                        ape->currentJob = Job::Builder;
                        assignedBuilders++;
                    }
                }
            }
        }
    }
}

void SimulationManager::pause() { isPaused = true; }
void SimulationManager::resume() { isPaused = false; }
bool SimulationManager::getPaused() const { return isPaused; }
void SimulationManager::setControlledApe(EntityID id) { controlledApeID = id; }
EntityID SimulationManager::getControlledApe() const { return controlledApeID; }
SimulationRegistry& SimulationManager::getRegistry() { return registry; }
WorldClock& SimulationManager::getClock() { return clock; }

}