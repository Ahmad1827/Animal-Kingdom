#include "world/WorldManager.h"

WorldManager::WorldManager(uint32_t seed) {
    chunkManager = std::make_unique<ChunkManager>(seed);
}

void WorldManager::update(float dt, const sf::FloatRect& preloadBounds, const sf::FloatRect& unloadBounds, ProfilerStats& profiler) {
    chunkManager->update(preloadBounds, unloadBounds, profiler);
}

void WorldManager::draw(sf::RenderWindow& window, const sf::FloatRect& viewBounds) const {
    chunkManager->draw(window, viewBounds);
}

void WorldManager::drawBackground(sf::RenderWindow& window, const sf::FloatRect& viewBounds, bool showFoliage, ProfilerStats& profiler) const {
    chunkManager->drawBackground(window, viewBounds, showFoliage, profiler);
}

void WorldManager::drawGeometry(sf::RenderWindow& window, const sf::FloatRect& viewBounds, ProfilerStats& profiler) const {
    chunkManager->drawGeometry(window, viewBounds, profiler);
}

void WorldManager::drawDebug(sf::RenderWindow& window, const sf::FloatRect& viewBounds, const sf::FloatRect& preloadBounds, const sf::FloatRect& unloadBounds, bool b1, bool b2, bool b3) const {
    chunkManager->drawDebug(window, viewBounds, preloadBounds, unloadBounds, b1, b2, b3);
}

float WorldManager::getTerrainHeight(float x) const {
    return chunkManager->getTerrainHeight(x);
}

bool WorldManager::checkOneWayCollision(const sf::FloatRect& bounds, const sf::Vector2f& velocity, float dt, sf::FloatRect& outPlatformBounds) const {
    if (velocity.y <= 0) return false;
    float bottomY = bounds.top + bounds.height;
    float previousBottomY = bottomY - (velocity.y * dt);

    // Spatial Chunk Query (O(1) Optimization)
    int chunkIdx = chunkManager->getChunkIndexAt(bounds.left + bounds.width / 2.f);
    for (int i = chunkIdx - 1; i <= chunkIdx + 1; ++i) {
        Chunk* chunk = chunkManager->getChunk(i);
        if (!chunk) continue;
        
        for (const auto& tree : chunk->getTrees()) {
            for (const auto& branch : tree.getBranches()) {
                sf::FloatRect branchBounds = branch.getGlobalBounds();
                if (bounds.intersects(branchBounds)) {
                    if (previousBottomY <= branchBounds.top + 5.f) {
                        outPlatformBounds = branchBounds;
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

bool WorldManager::checkTrunkCollision(const sf::FloatRect& bounds, float& outTrunkCenter) const {
    int chunkIdx = chunkManager->getChunkIndexAt(bounds.left + bounds.width / 2.f);
    for (int i = chunkIdx - 1; i <= chunkIdx + 1; ++i) {
        Chunk* chunk = chunkManager->getChunk(i);
        if (!chunk) continue;
        
        for (const auto& tree : chunk->getTrees()) {
            if (bounds.intersects(tree.getTrunkBounds())) {
                outTrunkCenter = tree.getTrunkCenter();
                return true;
            }
        }
    }
    return false;
}

bool WorldManager::checkHangCollision(const sf::FloatRect& bounds, sf::FloatRect& outBranchBounds) const {
    sf::FloatRect upperBounds = bounds;
    upperBounds.height = 10.f; 

    int chunkIdx = chunkManager->getChunkIndexAt(bounds.left + bounds.width / 2.f);
    for (int i = chunkIdx - 1; i <= chunkIdx + 1; ++i) {
        Chunk* chunk = chunkManager->getChunk(i);
        if (!chunk) continue;
        
        for (const auto& tree : chunk->getTrees()) {
            for (const auto& branch : tree.getBranches()) {
                if (upperBounds.intersects(branch.getGlobalBounds())) {
                    outBranchBounds = branch.getGlobalBounds();
                    return true;
                }
            }
        }
    }
    return false;
}

bool WorldManager::checkVineCollision(const sf::FloatRect& bounds, float& outVineCenter) const {
    int chunkIdx = chunkManager->getChunkIndexAt(bounds.left + bounds.width / 2.f);
    for (int i = chunkIdx - 1; i <= chunkIdx + 1; ++i) {
        Chunk* chunk = chunkManager->getChunk(i);
        if (!chunk) continue;
        
        for (const auto& tree : chunk->getTrees()) {
            for (const auto& vine : tree.getVines()) {
                if (bounds.intersects(vine.getGlobalBounds())) {
                    outVineCenter = vine.getPosition().x;
                    return true;
                }
            }
        }
    }
    return false;
}

void WorldManager::updateSway(float dt) {
    // Trees are stored by value in chunks, we must cast away constness to update sway safely in this architecture without rewriting the chunk container
    for (auto& pair : const_cast<std::unordered_map<int, std::unique_ptr<Chunk>>&>(chunkManager->getActiveChunks())) {
        for (auto& tree : const_cast<std::vector<Tree>&>(pair.second->getTrees())) {
            tree.update(dt);
        }
    }
}

ChunkManager* WorldManager::getChunkManager() const {
    return chunkManager.get();
}