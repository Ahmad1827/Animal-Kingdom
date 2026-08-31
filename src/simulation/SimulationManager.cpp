#include "simulation/SimulationManager.h"
#include "simulation/JobSystem.h"
#include <cmath>

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

    for (auto& pair : registry.getAllApes()) {
        ApeData& ape = pair.second;
        if (ape.alive && ape.hasTravelDestination) {
            ape.currentJob = Job::March;
            float dist = ape.travelDestinationX - ape.worldX;
            
            if (std::abs(dist) > 5.0f) {
                ape.worldX += (dist > 0 ? 1.0f : -1.0f) * 6.0f; 
            } else {
                ape.worldX = ape.travelDestinationX;
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
    for (auto& pair : registry.getAllApes()) {
        ApeData& ape = pair.second;
        if (ape.alive && ape.hasTravelDestination) {
            float dist = ape.travelDestinationX - ape.worldX;
            
            if (std::abs(dist) > 5.0f) {
                ape.worldX += (dist > 0 ? 1.0f : -1.0f) * 6.0f; 
            } else {
                ape.worldX = ape.travelDestinationX;
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