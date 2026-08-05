#include "world/StructureManager.h"
#include <SFML/Graphics.hpp>

StructureManager::StructureManager() {}

void StructureManager::update(float dt, sim::SimulationRegistry& registry) {}

void StructureManager::draw(sf::RenderTarget& target, sim::SimulationRegistry& registry, WorldManager* world, const sf::FloatRect& viewBounds) {
    for (auto& pair : registry.getAllStructures()) {
        sim::StructureData& s = pair.second;
        
        if (s.worldX < viewBounds.left - 300.f || s.worldX > viewBounds.left + viewBounds.width + 300.f) continue;

        float groundY = world->getTerrainHeight(s.worldX);

        if (!s.isFinished) {
            float progressRatio = s.progress / s.maxProgress;
            
            // Phase 1: Foundation (0% - 33%)
            sf::RectangleShape foundation(sf::Vector2f(100.f, 10.f));
            foundation.setOrigin(50.f, 10.f);
            foundation.setPosition(s.worldX, groundY);
            foundation.setFillColor(sf::Color(105, 105, 105)); 
            target.draw(foundation);

            // Phase 2: Scaffolding (33% - 66%)
            if (progressRatio >= 0.33f) {
                float height = 40.f;
                sf::RectangleShape scaffold(sf::Vector2f(80.f, height));
                scaffold.setOrigin(40.f, height);
                scaffold.setPosition(s.worldX, groundY - 10.f);
                scaffold.setFillColor(sf::Color::Transparent);
                scaffold.setOutlineColor(sf::Color(139, 69, 19, 200));
                scaffold.setOutlineThickness(2.f);
                target.draw(scaffold);
            }

            // Phase 3: Half-built (66% - 99%)
            if (progressRatio >= 0.66f) {
                sf::RectangleShape walls(sf::Vector2f(90.f, 30.f));
                walls.setOrigin(45.f, 30.f);
                walls.setPosition(s.worldX, groundY - 10.f);
                walls.setFillColor(sf::Color(160, 82, 45, 150)); 
                target.draw(walls);
            }
        } else {
            // Finished Structures
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
                sf::RectangleShape pole(sf::Vector2f(12.f, 180.f));
                pole.setOrigin(6.f, 180.f);
                pole.setPosition(s.worldX, groundY);
                pole.setFillColor(sf::Color(101, 67, 33));
                target.draw(pole);
                
                sf::RectangleShape plat(sf::Vector2f(80.f, 12.f));
                plat.setOrigin(40.f, 6.f);
                plat.setPosition(s.worldX, groundY - 150.f);
                plat.setFillColor(sf::Color(160, 82, 45));
                target.draw(plat);

            } else if (s.type == sim::StructureType::VillageCenter) {
                sf::RectangleShape base(sf::Vector2f(160.f, 120.f));
                base.setOrigin(80.f, 120.f);
                base.setPosition(s.worldX, groundY);
                base.setFillColor(sf::Color(80, 50, 20));
                target.draw(base);

                sf::RectangleShape flagPole(sf::Vector2f(6.f, 200.f));
                flagPole.setOrigin(3.f, 200.f);
                flagPole.setPosition(s.worldX - 50.f, groundY);
                flagPole.setFillColor(sf::Color(50, 50, 50));
                target.draw(flagPole);

            } else if (s.type == sim::StructureType::BuilderHut) {
                sf::RectangleShape hutBase(sf::Vector2f(100.f, 70.f));
                hutBase.setOrigin(50.f, 70.f);
                hutBase.setPosition(s.worldX, groundY);
                hutBase.setFillColor(sf::Color(120, 80, 40));
                target.draw(hutBase);

                sf::RectangleShape roof(sf::Vector2f(110.f, 10.f));
                roof.setOrigin(55.f, 10.f);
                roof.setPosition(s.worldX, groundY - 70.f);
                roof.setFillColor(sf::Color(90, 60, 30));
                target.draw(roof);

            } else if (s.type == sim::StructureType::Bonfire) {
                sf::RectangleShape log1(sf::Vector2f(40.f, 10.f));
                log1.setOrigin(20.f, 5.f);
                log1.setPosition(s.worldX, groundY - 5.f);
                log1.setFillColor(sf::Color(60, 30, 10));
                target.draw(log1);
                
                sf::CircleShape fire(15.f, 3); 
                fire.setOrigin(15.f, 30.f);
                fire.setPosition(s.worldX, groundY - 10.f);
                fire.setFillColor(sf::Color(255, 100, 0, 200));
                target.draw(fire);
            }
        }
    }
}