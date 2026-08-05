#include "world/StructureManager.h"
#include <SFML/Graphics.hpp>

StructureManager::StructureManager() {}

void StructureManager::update(float dt, sim::SimulationRegistry& registry) {}

void StructureManager::draw(sf::RenderTarget& target, sim::SimulationRegistry& registry, WorldManager* world, const sf::FloatRect& viewBounds) {
    for (auto& pair : registry.getAllStructures()) {
        sim::StructureData& s = pair.second;
        
        if (s.worldX < viewBounds.left - 200.f || s.worldX > viewBounds.left + viewBounds.width + 200.f) continue;

        float groundY = world->getTerrainHeight(s.worldX);

        if (!s.isFinished) {
            float progressRatio = s.progress / s.maxProgress;
            if (progressRatio < 0.1f) progressRatio = 0.1f; 
            
            float height = 50.f + (50.f * progressRatio);
            
            sf::RectangleShape scaffold(sf::Vector2f(80.f, height));
            scaffold.setOrigin(40.f, height);
            scaffold.setPosition(s.worldX, groundY);
            scaffold.setFillColor(sf::Color(0, 0, 0, 0));
            scaffold.setOutlineColor(sf::Color(139, 69, 19, 180));
            scaffold.setOutlineThickness(3.f);
            target.draw(scaffold);
            
            int planks = static_cast<int>(progressRatio * 4.0f);
            for (int i = 0; i < planks; ++i) {
                sf::RectangleShape plank(sf::Vector2f(90.f, 6.f));
                plank.setOrigin(45.f, 3.f);
                plank.setPosition(s.worldX, groundY - (height * ((float)(i+1)/4.0f)));
                plank.setFillColor(sf::Color(160, 82, 45));
                target.draw(plank);
            }
        } else {
            if (s.type == sim::StructureType::StorageHut) {
                sf::RectangleShape hutBase(sf::Vector2f(120.f, 90.f));
                hutBase.setOrigin(60.f, 90.f);
                hutBase.setPosition(s.worldX, groundY);
                hutBase.setFillColor(sf::Color(101, 67, 33));
                target.draw(hutBase);
                
                sf::ConvexShape roof(3);
                roof.setPoint(0, sf::Vector2f(0.f, -50.f));
                roof.setPoint(1, sf::Vector2f(-75.f, 0.f));
                roof.setPoint(2, sf::Vector2f(75.f, 0.f));
                roof.setPosition(s.worldX, groundY - 90.f);
                roof.setFillColor(sf::Color(139, 69, 19));
                target.draw(roof);
                
                sf::RectangleShape door(sf::Vector2f(30.f, 45.f));
                door.setOrigin(15.f, 45.f);
                door.setPosition(s.worldX, groundY);
                door.setFillColor(sf::Color(50, 30, 15));
                target.draw(door);
                
            } else if (s.type == sim::StructureType::Nest) {
                sf::CircleShape nest(40.f);
                nest.setOrigin(40.f, 40.f);
                nest.setPosition(s.worldX, groundY - 10.f);
                nest.setFillColor(sf::Color(218, 165, 32));
                nest.setScale(1.5f, 0.4f);
                target.draw(nest);
                
                sf::CircleShape inner(25.f);
                inner.setOrigin(25.f, 25.f);
                inner.setPosition(s.worldX, groundY - 15.f);
                inner.setFillColor(sf::Color(184, 134, 11));
                inner.setScale(1.5f, 0.4f);
                target.draw(inner);

            } else if (s.type == sim::StructureType::WatchPlatform) {
                sf::RectangleShape poleLeft(sf::Vector2f(8.f, 180.f));
                poleLeft.setOrigin(4.f, 180.f);
                poleLeft.setPosition(s.worldX - 25.f, groundY);
                poleLeft.setFillColor(sf::Color(101, 67, 33));
                target.draw(poleLeft);
                
                sf::RectangleShape poleRight(sf::Vector2f(8.f, 180.f));
                poleRight.setOrigin(4.f, 180.f);
                poleRight.setPosition(s.worldX + 25.f, groundY);
                poleRight.setFillColor(sf::Color(101, 67, 33));
                target.draw(poleRight);
                
                sf::RectangleShape plat(sf::Vector2f(80.f, 12.f));
                plat.setOrigin(40.f, 6.f);
                plat.setPosition(s.worldX, groundY - 170.f);
                plat.setFillColor(sf::Color(160, 82, 45));
                target.draw(plat);
            }
        }
    }
}