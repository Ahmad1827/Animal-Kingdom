#include "world/WorldManager.h"

WorldManager::WorldManager(uint32_t seed) : swayTime(0.f) {
    chunkManager = std::make_unique<ChunkManager>(seed);
}

void WorldManager::update(float dt, const sf::FloatRect& preloadBounds, const sf::FloatRect& unloadBounds, ProfilerStats& profiler) {
    chunkManager->update(preloadBounds, unloadBounds, profiler);
}

void WorldManager::draw(sf::RenderWindow& window, const sf::FloatRect& viewBounds) const {
    ProfilerStats dummyProfiler;
    static sf::Texture dummyTilesetTex;
    if (dummyTilesetTex.getSize().x == 0) {
        dummyTilesetTex.create(32, 32);
    }
    chunkManager->drawBackground(window, viewBounds, true, dummyProfiler, dummyTilesetTex);
    chunkManager->drawGeometry(window, viewBounds, dummyProfiler);
}

void WorldManager::drawBackground(sf::RenderWindow& window, const sf::FloatRect& viewBounds, bool showFoliage, ProfilerStats& profiler, sf::Texture& tileset) const {
    chunkManager->drawBackground(window, viewBounds, showFoliage, profiler, tileset);
}

void WorldManager::drawGeometry(sf::RenderWindow& window, const sf::FloatRect& viewBounds, ProfilerStats& profiler) const {
    chunkManager->drawGeometry(window, viewBounds, profiler);
}

void WorldManager::drawDebug(sf::RenderWindow& window, const sf::FloatRect& viewBounds, const sf::FloatRect& preloadBounds, const sf::FloatRect& unloadBounds, DebugOverlay* debugOverlay) const {
    chunkManager->drawDebug(window, viewBounds, preloadBounds, unloadBounds, debugOverlay);
}

float WorldManager::getTerrainHeight(float x) const {
    return chunkManager->getTerrainHeight(x);
}

bool WorldManager::checkOneWayCollision(const sf::FloatRect& bounds, const sf::Vector2f& velocity, float dt, sf::FloatRect& outPlatformBounds) const {
    if (velocity.y <= 0) return false;
    float bottomY = bounds.top + bounds.height;
    float previousBottomY = bottomY - (velocity.y * dt);

    int cX = chunkManager->getChunkXAt(bounds.left + bounds.width / 2.f);
    int cY = chunkManager->getChunkYAt(bounds.top + bounds.height / 2.f);
    
    // Spatial 2D Query
    for (int x = cX - 1; x <= cX + 1; ++x) {
        for (int y = cY - 1; y <= cY + 1; ++y) {
            Chunk* chunk = chunkManager->getChunk(x, y);
            if (!chunk) continue;
            for (const auto& tree : chunk->getTrees()) {
                for (const auto& branch : tree.getBranches()) {
                    if (bounds.intersects(branch.bounds)) {
                        if (previousBottomY <= branch.bounds.top + 5.f) {
                            outPlatformBounds = branch.bounds;
                            return true;
                        }
                    }
                }
            }
        }
    }
    return false;
}

bool WorldManager::checkTrunkCollision(const sf::FloatRect& bounds, float& outTrunkCenter) const {
    int cX = chunkManager->getChunkXAt(bounds.left + bounds.width / 2.f);
    int cY = chunkManager->getChunkYAt(bounds.top + bounds.height / 2.f);
    for (int x = cX - 1; x <= cX + 1; ++x) {
        for (int y = cY - 1; y <= cY + 1; ++y) {
            Chunk* chunk = chunkManager->getChunk(x, y);
            if (!chunk) continue;
            for (const auto& tree : chunk->getTrees()) {
                if (bounds.intersects(tree.getTrunkBounds())) {
                    outTrunkCenter = tree.getTrunkCenter();
                    return true;
                }
            }
        }
    }
    return false;
}

bool WorldManager::checkHangCollision(const sf::FloatRect& bounds, sf::FloatRect& outBranchBounds) const {
    sf::FloatRect upperBounds = bounds;
    upperBounds.height = 10.f; 
    int cX = chunkManager->getChunkXAt(bounds.left + bounds.width / 2.f);
    int cY = chunkManager->getChunkYAt(bounds.top + bounds.height / 2.f);
    for (int x = cX - 1; x <= cX + 1; ++x) {
        for (int y = cY - 1; y <= cY + 1; ++y) {
            Chunk* chunk = chunkManager->getChunk(x, y);
            if (!chunk) continue;
            for (const auto& tree : chunk->getTrees()) {
                for (const auto& branch : tree.getBranches()) {
                    if (upperBounds.intersects(branch.bounds)) {
                        outBranchBounds = branch.bounds;
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

bool WorldManager::checkVineCollision(const sf::FloatRect& bounds, float& outVineCenter) const {
    int cX = chunkManager->getChunkXAt(bounds.left + bounds.width / 2.f);
    int cY = chunkManager->getChunkYAt(bounds.top + bounds.height / 2.f);
    for (int x = cX - 1; x <= cX + 1; ++x) {
        for (int y = cY - 1; y <= cY + 1; ++y) {
            Chunk* chunk = chunkManager->getChunk(x, y);
            if (!chunk) continue;
            for (const auto& tree : chunk->getTrees()) {
                for (const auto& vine : tree.getVines()) {
                    sf::FloatRect vBounds(vine.origin.x - 3.f, vine.origin.y, 6.f, vine.length);
                    if (bounds.intersects(vBounds)) {
                        outVineCenter = vine.origin.x;
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

void WorldManager::updateSway(float dt, const sf::FloatRect& viewBounds, const sf::Vector2f& windVector) {
    swayTime += dt;
    if (swayTime > 1000.f) swayTime -= 1000.f; 
    
    // Pass windVector to chunks
    for (const auto& pair : chunkManager->getActiveChunks()) {
        pair.second->updateSway(swayTime, viewBounds, windVector); // We will update chunk logic next
    }
}

void WorldManager::disturbEnvironment(const sf::FloatRect& bounds, float velocityX) {
    int cX = chunkManager->getChunkXAt(bounds.left + bounds.width / 2.f);
    int cY = chunkManager->getChunkYAt(bounds.top + bounds.height / 2.f);
    for (int x = cX - 1; x <= cX + 1; ++x) {
        for (int y = cY - 1; y <= cY + 1; ++y) {
            Chunk* chunk = chunkManager->getChunk(x, y);
            if (!chunk) continue;
            // The tree container is const from getTrees(), we circumvent safely here for interaction
            for (auto& tree : const_cast<std::vector<Tree>&>(chunk->getTrees())) {
                tree.disturbVines(bounds, velocityX);
            }
        }
    }
}

ChunkManager* WorldManager::getChunkManager() const {
    return chunkManager.get();
}