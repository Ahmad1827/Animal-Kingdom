#pragma once
#include "entities/Ape.h"
#include "simulation/ApeData.h"
#include "world/WorldManager.h"
#include <SFML/Graphics.hpp>

enum class AIState {
    Idle,
    Wandering,
    Climbing,
    ReturningHome
};

class NPCApe {
private:
    sim::EntityID simId;
    Ape physicalApe;
    AIState currentState;
    float stateTimer;
    float intendedMoveX;
    bool isDroppingToHang;
    sf::FloatRect activeBranch;

    uint64_t grabbedChunk;
    int grabbedVine;
    int grabbedSeg;

    void determineNextAction(sim::ApeData* data, float timeOfDay);
    void applyPhysics(float dt, WorldManager* worldManager);
    bool hasTrait(sim::ApeData* data, sim::Trait trait);

public:
    NPCApe(sim::EntityID id, float x, float y, sf::Texture& texture);
    
    void update(float dt, sim::ApeData* data, WorldManager* worldManager, float timeOfDay);
    void draw(sf::RenderTarget& target);
    
    sim::EntityID getId() const { return simId; }
    sf::FloatRect getBounds() const { return physicalApe.getBounds(); }
};