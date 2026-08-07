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

    // BUG #2 FIX: PHYSICAL REPRESENTATIVE TRAVEL OVERRIDE
    // Runs after normal AI to prevent the AI from canceling the movement
    for (auto& pair : registry.getAllApes()) {
        ApeData& ape = pair.second;
        if (ape.alive && ape.hasTravelDestination) {
            ape.currentJob = Job::March; // Lock AI to walking state
            float dist = ape.travelDestinationX - ape.worldX;
            
            if (std::abs(dist) > 5.0f) {
                // Physically move 180 units per second (6 units * 30 ticks)
                // NPCManager reads this worldX, flips the sprite, and animates it walking
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