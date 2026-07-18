#pragma once
#include "world/Chunk.h"
#include <unordered_map>
#include <memory>

class ChunkManager {
private:
    float chunkWidth;
    std::unordered_map<int, std::unique_ptr<Chunk>> chunks;

public:
    ChunkManager();
    void update(float playerX);
    void draw(sf::RenderWindow& window) const;
    const std::unordered_map<int, std::unique_ptr<Chunk>>& getActiveChunks() const;
    float getTerrainHeight(float x) const;
};