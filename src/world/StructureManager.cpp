#include "world/StructureManager.h"

StructureManager::StructureManager() {}

void StructureManager::draw(sf::RenderTarget& target, sim::SimulationRegistry& registry, WorldManager* worldManager, const sf::FloatRect& viewBounds) {
    for (const auto& pair : registry.getAllStructures()) {
        const sim::StructureData& s = pair.second;
        
        // Culling
        if (s.worldX < viewBounds.left - 500.f || s.worldX > viewBounds.left + viewBounds.width + 500.f) continue;

        float groundY = worldManager->getTerrainHeight(s.worldX);

        if (s.type == sim::StructureType::StorageHut) {
            shape.setSize(sf::Vector2f(200.f, 150.f));
            shape.setFillColor(sf::Color(100, 70, 40));
        } else if (s.type == sim::StructureType::Nest) {
            shape.setSize(sf::Vector2f(100.f, 50.f));
            shape.setFillColor(sf::Color(130, 150, 50));
        } else {
            shape.setSize(sf::Vector2f(80.f, 80.f));
            shape.setFillColor(sf::Color(150, 150, 150));
        }

        shape.setOrigin(shape.getSize().x / 2.f, shape.getSize().y);
        shape.setPosition(s.worldX, groundY);

        // Visual Construction Progress
        if (!s.isFinished) {
            float perc = s.progress / s.maxProgress;
            sf::Color c = shape.getFillColor();
            c.a = 50 + static_cast<sf::Uint8>(205.f * perc);
            shape.setFillColor(c);
            
            // Draw scaffolding outline
            shape.setOutlineColor(sf::Color::Yellow);
            shape.setOutlineThickness(2.f);
        } else {
            shape.setOutlineThickness(0.f);
        }

        target.draw(shape);
    }
}