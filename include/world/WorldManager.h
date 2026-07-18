#pragma once
#include <SFML/Graphics.hpp>
#include "world/ChunkManager.h"
#include <memory>

class WorldManager {
private:
    std::unique_ptr<ChunkManager> chunkManager;

public:
    WorldManager();
    void update(float dt, float playerX);
    void draw(sf::RenderWindow& window) const;
    
    float getTerrainHeight(float x) const;
    bool checkOneWayCollision(const sf::FloatRect& bounds, const sf::Vector2f& velocity, float dt, sf::FloatRect& outPlatformBounds) const;
    bool checkTrunkCollision(const sf::FloatRect& bounds, float& outTrunkCenter) const;
    bool checkHangCollision(const sf::FloatRect& bounds, sf::FloatRect& outBranchBounds) const;
};