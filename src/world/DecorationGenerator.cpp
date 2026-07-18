#include "world/DecorationGenerator.h"
#include "world/TerrainGenerator.h"
#include <cstdlib>

Decoration DecorationGenerator::generateDecorations(float startX, float width) {
    Decoration decs;
    std::srand(static_cast<unsigned>(startX) * 73856); // Deterministic based on chunk

    int numBushes = 5 + (std::rand() % 10);
    for (int i = 0; i < numBushes; ++i) {
        float dx = startX + (std::rand() % static_cast<int>(width));
        float dy = TerrainGenerator::getTerrainHeight(dx);
        
        sf::CircleShape bush(15.f + (std::rand() % 20));
        bush.setFillColor(sf::Color(20, 80, 20));
        bush.setOrigin(bush.getRadius(), bush.getRadius());
        bush.setPosition(dx, dy + 5.f);
        decs.bushes.push_back(bush);
    }

    int numRocks = 3 + (std::rand() % 6);
    for (int i = 0; i < numRocks; ++i) {
        float dx = startX + (std::rand() % static_cast<int>(width));
        float dy = TerrainGenerator::getTerrainHeight(dx);
        
        sf::ConvexShape rock;
        rock.setPointCount(5);
        rock.setPoint(0, sf::Vector2f(0.f, -10.f));
        rock.setPoint(1, sf::Vector2f(15.f, -5.f));
        rock.setPoint(2, sf::Vector2f(20.f, 10.f));
        rock.setPoint(3, sf::Vector2f(-15.f, 10.f));
        rock.setPoint(4, sf::Vector2f(-20.f, 0.f));
        rock.setFillColor(sf::Color(100, 100, 100));
        rock.setPosition(dx, dy);
        float scale = 0.5f + ((std::rand() % 100) / 100.f);
        rock.setScale(scale, scale);
        decs.rocks.push_back(rock);
    }

    return decs;
}