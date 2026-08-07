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
    
    // --- PHYSICAL REPRESENTATIVE TRAVEL OVERRIDE ---
    ApeData* player = registry.getApe(controlledApeID);
    if (player && player->isWaitingForAudience && player->summonedRepId != 0) {
        ApeData* rep = registry.getApe(player->summonedRepId);
        if (rep) {
            float dist = player->meetingX - rep->worldX;
            if (std::abs(dist) > 150.f) {
                rep->currentJob = Job::March; // Temporarily override autonomous AI
                rep->worldX += (dist > 0 ? 1.0f : -1.0f) * 6.0f; // Walk 180 units/sec physically
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
        
        // If an ape is summoned for diplomacy, override normal wandering 
        // and physically move them through the world.
        if (ape.alive && ape.hasTravelDestination) {
            float dist = ape.travelDestinationX - ape.worldX;
            
            if (std::abs(dist) > 5.0f) {
                // Move 6 units per tick (approx 180 units per second)
                // NPCManager will naturally detect this movement, update the sprite position, 
                // flip the facing direction, and play the walking animation.
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