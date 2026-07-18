#pragma once
#include "world/Chunk.h"
#include "world/Biome.h"
#include <unordered_map>
#include <memory>
#include <cstdint>
#include <future>

class ChunkManager {
private:
    float chunkWidth;
    uint32_t worldSeed;
    std::unordered_map<int, std::unique_ptr<Chunk>> chunks;
    std::unordered_map<int, std::unique_ptr<Chunk>> cachedChunks;
    std::unordered_map<int, std::future<std::unique_ptr<Chunk>>> pendingChunks;
    int currentChunkIdx;

public:
    ChunkManager(uint32_t seed);
    void update(const sf::FloatRect& preloadBounds, const sf::FloatRect& unloadBounds, ProfilerStats& profiler);
    void draw(sf::RenderWindow& window, const sf::FloatRect& viewBounds) const;
    void drawBackground(sf::RenderWindow& window, const sf::FloatRect& viewBounds, bool showFoliage, ProfilerStats& profiler) const;
    void drawGeometry(sf::RenderWindow& window, const sf::FloatRect& viewBounds, ProfilerStats& profiler) const;
    void drawDebug(sf::RenderWindow& window, const sf::FloatRect& viewBounds, const sf::FloatRect& preloadBounds, const sf::FloatRect& unloadBounds, bool showBorders, bool showRegions, bool showHeatmaps) const;
    
    int getChunkIndexAt(float x) const;
    Chunk* getChunk(int index) const;
    const std::unordered_map<int, std::unique_ptr<Chunk>>& getActiveChunks() const;
    float getTerrainHeight(float x) const;
    int getCurrentChunkIndex() const;
    RegionType getCurrentRegion(float playerX) const;
};