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
};