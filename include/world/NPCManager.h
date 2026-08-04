#pragma once
#include "entities/NPCApe.h"
#include "simulation/SimulationManager.h"
#include "world/WorldManager.h"
#include <unordered_map>
#include <memory>
#include <SFML/Graphics.hpp>

class NPCManager {
private:
    std::unordered_map<sim::EntityID, std::unique_ptr<NPCApe>> activeNPCs;
    sf::Texture& apeTexture;

public:
    NPCManager(sf::Texture& texture);
    void update(float dt, const sf::FloatRect& preloadBounds, const sf::FloatRect& unloadBounds, sim::SimulationManager& simManager, WorldManager* worldManager, float timeOfDay);
    void draw(sf::RenderTarget& target);
    void removeNPC(sim::EntityID id);
    int getLoadedNPCCount() const { return activeNPCs.size(); }
};