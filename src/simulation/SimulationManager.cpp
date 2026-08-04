#include "simulation/SimulationManager.h"
#include "simulation/JobSystem.h"

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
}

void SimulationManager::simulateAging() {}
void SimulationManager::simulateHunger() {} 
void SimulationManager::simulatePregnancies() {}
void SimulationManager::simulateResourceProduction() {}
void SimulationManager::simulateDiplomacy() {}
void SimulationManager::simulateAI() {}

void SimulationManager::pause() { isPaused = true; }
void SimulationManager::resume() { isPaused = false; }
bool SimulationManager::getPaused() const { return isPaused; }
void SimulationManager::setControlledApe(EntityID id) { controlledApeID = id; }
EntityID SimulationManager::getControlledApe() const { return controlledApeID; }
SimulationRegistry& SimulationManager::getRegistry() { return registry; }
WorldClock& SimulationManager::getClock() { return clock; }

}