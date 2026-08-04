#pragma once
#include "simulation/WorldClock.h"
#include "simulation/SimulationRegistry.h"
#include "simulation/EntityID.h"

namespace sim {

class SimulationManager {
private:
    WorldClock clock;
    SimulationRegistry registry;
    bool isPaused;
    EntityID controlledApeID;

    void simulateAging();
    void simulateHunger();
    void simulatePregnancies();
    void simulateResourceProduction();
    void simulateDiplomacy();
    void simulateAI();

public:
    SimulationManager();

    void update(float dt);
    void tick();
    
    void pause();
    void resume();
    bool getPaused() const;

    void setControlledApe(EntityID id);
    EntityID getControlledApe() const;

    SimulationRegistry& getRegistry();
    WorldClock& getClock();
};

}