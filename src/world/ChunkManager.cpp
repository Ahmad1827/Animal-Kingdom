#include "world/ChunkManager.h"
#include "world/TerrainGenerator.h"
#include <cmath>

ChunkManager::ChunkManager() : chunkWidth(2000.f) {}

void ChunkManager::update(float playerX) {
    int currentChunkIdx = static_cast<int>(std::floor(playerX / chunkWidth));

    // Load active and adjacent chunks
    for (int i = currentChunkIdx - 1; i <= currentChunkIdx + 1; ++i) {
        if (chunks.find(i) == chunks.end()) {
            chunks[i] = std::make_unique<Chunk>(i * chunkWidth, chunkWidth);
        }
    }

    // Unload distant chunks
    for (auto it = chunks.begin(); it != chunks.end();) {
        if (std::abs(it->first - currentChunkIdx) > 2) {
            it = chunks.erase(it);
        } else {
            ++it;
        }
    }
}

void ChunkManager::draw(sf::RenderWindow& window) const {
    for (const auto& pair : chunks) {
        pair.second->draw(window);
    }
}

const std::unordered_map<int, std::unique_ptr<Chunk>>& ChunkManager::getActiveChunks() const {
    return chunks;
}

float ChunkManager::getTerrainHeight(float x) const {
    return TerrainGenerator::getTerrainHeight(x);
}