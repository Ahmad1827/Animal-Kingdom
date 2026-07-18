#pragma once
#include "world/Chunk.h"
#include "world/Biome.h"
#include <unordered_map>
#include <memory>
#include <cstdint>

class ChunkManager {
private:
    float chunkWidth;
    uint32_t worldSeed;
    std::unordered_map<int, std::unique_ptr<Chunk>> chunks;
    int currentChunkIdx;

public:
    ChunkManager(uint32_t seed);
    void update(float playerX);
    void draw(sf::RenderWindow& window, const sf::FloatRect& viewBounds) const;
    void drawBackground(sf::RenderWindow& window, const sf::FloatRect& viewBounds, bool showFoliage) const;
    void drawGeometry(sf::RenderWindow& window, const sf::FloatRect& viewBounds) const;
    void drawDebug(sf::RenderWindow& window, const sf::FloatRect& viewBounds, bool showBorders, bool showRegions, bool showHeatmaps) const;
    const std::unordered_map<int, std::unique_ptr<Chunk>>& getActiveChunks() const;
    float getTerrainHeight(float x) const;
    int getCurrentChunkIndex() const;
    RegionType getCurrentRegion(float playerX) const;
};