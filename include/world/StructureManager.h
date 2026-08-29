#pragma once
#include "simulation/SimulationRegistry.h"
#include "world/WorldManager.h"
#include <SFML/Graphics.hpp>

class StructureManager {
public:
    StructureManager();
    ~StructureManager() = default;

    void update(float dt, sim::SimulationRegistry& registry);
    void draw(sf::RenderTarget& target, sim::SimulationRegistry& registry, WorldManager* world, const sf::FloatRect& viewBounds);
    void drawForeground(sf::RenderTarget& target, sim::SimulationRegistry& registry, WorldManager* world, const sf::FloatRect& viewBounds);

private:
    void drawSettlementFootprint(sf::RenderTarget& target, const sim::VillageData& village, float groundY);
    void drawVillageCenter(sf::RenderTarget& target, const sim::StructureData& s, const sim::VillageData& village, float groundY);
    void drawNest(sf::RenderTarget& target, const sim::StructureData& s, const sim::VillageData& village, float groundY);
    void drawStorageHut(sf::RenderTarget& target, const sim::StructureData& s, const sim::VillageData& village, float groundY);
    void drawWatchPlatform(sf::RenderTarget& target, const sim::StructureData& s, const sim::VillageData& village, float groundY);
    void drawBuilderHut(sf::RenderTarget& target, const sim::StructureData& s, const sim::VillageData& village, float groundY);
    void drawBonfire(sf::RenderTarget& target, const sim::StructureData& s, const sim::VillageData& village, float groundY);
    void drawToolRack(sf::RenderTarget& target, const sim::StructureData& s, const sim::VillageData& village, float groundY);
    void drawSimpleBarrier(sf::RenderTarget& target, const sim::StructureData& s, const sim::VillageData& village, float groundY);
    void drawStockpileProps(sf::RenderTarget& target, const sim::StructureData& s, const sim::VillageData& village, float groundY);
    void drawConstructionSite(sf::RenderTarget& target, const sim::StructureData& s, float groundY);
};