#pragma once
#include <SFML/Graphics.hpp>
#include "world/ChunkManager.h"
#include <memory>
#include <cstdint>

class WorldManager {
private:
    std::unique_ptr<ChunkManager> chunkManager;
    float swayTime;

public:
    WorldManager(uint32_t seed);
    void update(float dt, const sf::FloatRect& preloadBounds, const sf::FloatRect& unloadBounds, ProfilerStats& profiler);
    void draw(sf::RenderWindow& window, const sf::FloatRect& viewBounds) const;
    void drawBackground(sf::RenderWindow& window, const sf::FloatRect& viewBounds, bool showFoliage, ProfilerStats& profiler) const;
    void drawGeometry(sf::RenderWindow& window, const sf::FloatRect& viewBounds, ProfilerStats& profiler) const;
    void drawDebug(sf::RenderWindow& window, const sf::FloatRect& viewBounds, const sf::FloatRect& preloadBounds, const sf::FloatRect& unloadBounds, class DebugOverlay* debugOverlay) const;

    float getTerrainHeight(float x) const;
    bool checkOneWayCollision(const sf::FloatRect& bounds, const sf::Vector2f& velocity, float dt, sf::FloatRect& outPlatformBounds) const;
    bool checkTrunkCollision(const sf::FloatRect& bounds, float& outTrunkCenter) const;
    bool checkHangCollision(const sf::FloatRect& bounds, sf::FloatRect& outBranchBounds) const;
    bool checkVineCollision(const sf::FloatRect& bounds, float& outVineCenter) const;
    void updateSway(float dt, const sf::FloatRect& viewBounds, const sf::Vector2f& windVector);
    void disturbEnvironment(const sf::FloatRect& bounds, float velocityX);    
    
    ChunkManager* getChunkManager() const;
};