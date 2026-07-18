#pragma once
#include <SFML/Graphics.hpp>
#include "world/ChunkManager.h"
#include <memory>
#include <cstdint>

class WorldManager {
private:
    std::unique_ptr<ChunkManager> chunkManager;

public:
    WorldManager(uint32_t seed);
    void update(float dt, float playerX);
    void draw(sf::RenderWindow& window, const sf::FloatRect& viewBounds) const;
    void drawBackground(sf::RenderWindow& window, const sf::FloatRect& viewBounds, bool showFoliage) const;
    void drawGeometry(sf::RenderWindow& window, const sf::FloatRect& viewBounds) const;
    void drawDebug(sf::RenderWindow& window, const sf::FloatRect& viewBounds, bool b1, bool b2, bool b3) const;
    
    float getTerrainHeight(float x) const;
    bool checkOneWayCollision(const sf::FloatRect& bounds, const sf::Vector2f& velocity, float dt, sf::FloatRect& outPlatformBounds) const;
    bool checkTrunkCollision(const sf::FloatRect& bounds, float& outTrunkCenter) const;
    bool checkHangCollision(const sf::FloatRect& bounds, sf::FloatRect& outBranchBounds) const;
    
    ChunkManager* getChunkManager() const;
};