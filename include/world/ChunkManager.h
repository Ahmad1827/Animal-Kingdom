#pragma once
#include "world/Chunk.h"
#include "world/Biome.h"
#include <unordered_map>
#include <memory>
#include <cstdint>
#include <future>
#include <queue>

class ChunkManager {
private:
    float chunkWidth;
    float chunkHeight;
    uint32_t worldSeed;
    
    std::unordered_map<uint64_t, std::unique_ptr<Chunk>> chunks;
    std::unordered_map<uint64_t, std::unique_ptr<Chunk>> cachedChunks;
    std::unordered_map<uint64_t, std::future<std::unique_ptr<Chunk>>> pendingChunks;
    std::queue<std::unique_ptr<Chunk>> insertionQueue;
    
    int currentChunkIdx;
    
    uint64_t getChunkKey(int x, int y) const;

public:
    ChunkManager(uint32_t seed);
    void update(const sf::FloatRect& preloadBounds, const sf::FloatRect& unloadBounds, ProfilerStats& profiler);
    void drawBackground(sf::RenderWindow& window, const sf::FloatRect& viewBounds, bool showFoliage, ProfilerStats& profiler, sf::Texture& tileset) const;
    void drawGeometry(sf::RenderWindow& window, const sf::FloatRect& viewBounds, ProfilerStats& profiler) const;
    void drawDebug(sf::RenderWindow& window, const sf::FloatRect& viewBounds, const sf::FloatRect& preloadBounds, const sf::FloatRect& unloadBounds, class DebugOverlay* debugOverlay) const;

    void updateSway(float globalTime, const sf::FloatRect& viewBounds, const sf::Vector2f& windVector);
    
    int getChunkXAt(float x) const;
    int getChunkYAt(float y) const;
    Chunk* getChunk(int cx, int cy) const;
    
    const std::unordered_map<uint64_t, std::unique_ptr<Chunk>>& getActiveChunks() const;
    float getTerrainHeight(float x) const;
    int getCurrentChunkIndex() const;
    RegionType getCurrentRegion(float playerX) const;
};