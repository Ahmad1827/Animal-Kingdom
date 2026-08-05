#pragma once
#include "entities/Ape.h"
#include "simulation/ApeData.h"
#include "simulation/SimulationRegistry.h"
#include "world/WorldManager.h"
#include <SFML/Graphics.hpp>

class NPCApe {
private:
    sim::EntityID simId;
    Ape physicalApe;
    float stateTimer;
    float intendedMoveX;
    bool isDroppingToHang;
    sf::FloatRect activeBranch;

    uint64_t grabbedChunk;
    int grabbedVine;
    int grabbedSeg;

    void determineNextAction(sim::ApeData* data, float timeOfDay, sim::SimulationRegistry& registry);
    void applyPhysics(float dt, WorldManager* worldManager);
    bool hasTrait(sim::ApeData* data, sim::Trait trait);

public:
    NPCApe(sim::EntityID id, float x, float y, sf::Texture& texture);
    void update(float dt, sim::ApeData* data, WorldManager* worldManager, float timeOfDay, sim::SimulationRegistry& registry);
    void draw(sf::RenderTarget& target);
    
    sim::EntityID getId() const { return simId; }
    sf::FloatRect getBounds() const { return physicalApe.getBounds(); }

    template<typename T>
    void setVisualEquipment(T equippedTool, sim::ResourceType carriedType, int carriedAmount, bool isKing) {
        if (carriedType == sim::ResourceType::Food) physicalApe.setCarriedItem(1);
        else if (carriedType == sim::ResourceType::Wood) physicalApe.setCarriedItem(2);
        else if (carriedType == sim::ResourceType::Stone) physicalApe.setCarriedItem(3);
        else physicalApe.setCarriedItem(0);
    }
};