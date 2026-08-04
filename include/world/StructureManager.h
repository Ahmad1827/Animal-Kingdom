#pragma once
#include <SFML/Graphics.hpp>
#include "simulation/SimulationRegistry.h"
#include "world/WorldManager.h"

class StructureManager {
private:
    sf::RectangleShape shape;
public:
    StructureManager();
    void draw(sf::RenderTarget& target, sim::SimulationRegistry& registry, WorldManager* worldManager, const sf::FloatRect& viewBounds);
};