#pragma once
#include "entities/Ape.h"
#include "simulation/ApeData.h"
#include "simulation/SimulationRegistry.h"
#include "world/WorldManager.h"
#include <SFML/Graphics.hpp>
#include <string>

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

    float pauseTimer;
    float baseSpeedMultiplier;
    float personalOffset;
    float workTimer;
    float woodcutLogTimer;

    void determineNextAction(sim::ApeData* data, float timeOfDay, sim::SimulationRegistry& registry, sim::EntityID playerId);
    void applyPhysics(float dt, WorldManager* worldManager);
    bool hasTrait(sim::ApeData* data, sim::Trait trait);
    void fireAudioHook(const std::string& soundEvent);

public:
    NPCApe(sim::EntityID id, float x, float y, sf::Texture& texture);
    void update(float dt, sim::ApeData* data, WorldManager* worldManager, float timeOfDay, sim::SimulationRegistry& registry, sim::EntityID playerId);
    void draw(sf::RenderTarget& target);
    
    sim::EntityID getId() const { return simId; }
    sf::FloatRect getBounds() const { return physicalApe.getBounds(); }

    void setVisualEquipment(sim::ToolType tool, sim::ResourceType res, int amount, bool isKing) {
        physicalApe.setVisualEquipment(tool, res, amount, isKing);
    }
};