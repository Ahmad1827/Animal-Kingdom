#include "simulation/SimulationManager.h"

namespace sim {

SimulationManager::SimulationManager() : isPaused(false) {}

void SimulationManager::update(float dt) {
    if (isPaused) {
        return;
    }

    clock.update(dt);
    while (clock.consumeTick()) {
        tick();
    }
}

void SimulationManager::tick() {
    simulateAging();
    simulateHunger();
    simulatePregnancies();
    simulateResourceProduction();
    simulateDiplomacy();
    simulateAI();
}

void SimulationManager::simulateAging() {}
void SimulationManager::simulateHunger() {}
void SimulationManager::simulatePregnancies() {}
void SimulationManager::simulateResourceProduction() {}
void SimulationManager::simulateDiplomacy() {}
void SimulationManager::simulateAI() {}

void SimulationManager::pause() {
    isPaused = true;
}

void SimulationManager::resume() {
    isPaused = false;
}

bool SimulationManager::getPaused() const {
    return isPaused;
}

SimulationRegistry& SimulationManager::getRegistry() {
    return registry;
}

WorldClock& SimulationManager::getClock() {
    return clock;
}

}