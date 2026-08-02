#include "world/WorldManager.h"
#include <set>
#include <cmath>

WorldManager::WorldManager(uint32_t seed, sf::Texture& decorTex) : swayTime(0.f) {
    chunkManager = std::make_unique<ChunkManager>(seed, decorTex);
}

void WorldManager::update(float dt, const sf::FloatRect& preloadBounds, const sf::FloatRect& unloadBounds, ProfilerStats& profiler) {
    chunkManager->update(preloadBounds, unloadBounds, profiler);

    std::set<uint64_t> currentActive;
    
    for (const auto& pair : chunkManager->getActiveChunks()) {
        uint64_t coord = pair.first;
        currentActive.insert(coord);
        
        if (activePhysicalVines.find(coord) == activePhysicalVines.end()) {
            std::vector<VinePhysics> chunkVines;
            for (const auto& tree : pair.second->getTrees()) {
                for (const auto& staticVine : tree.getVines()) {
                    int numSegments = static_cast<int>(staticVine.length / 10.f);
                    if (numSegments < 2) numSegments = 2;
                    float damp = 0.98f + (std::rand() % 10) / 1000.f; 
                    chunkVines.emplace_back(staticVine.origin, numSegments, 10.f, damp);
                }
            }
            activePhysicalVines[coord] = chunkVines;
        }
    }

    for (auto it = activePhysicalVines.begin(); it != activePhysicalVines.end(); ) {
        if (currentActive.find(it->first) == currentActive.end()) {
            it = activePhysicalVines.erase(it);
        } else {
            for (auto& vine : it->second) {
                vine.update(dt);
            }
            ++it;
        }
    }
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

    // --- DRAW THICK VINES INSTEAD OF 1-PIXEL LINES ---
    float vineThickness = 5.0f;
    sf::RectangleShape vineRect;
    vineRect.setFillColor(sf::Color(34, 139, 34)); // Forest green

    for (const auto& pair : activePhysicalVines) {
        for (const auto& vine : pair.second) {
            for (int i = 0; i < vine.getSegmentCount() - 1; ++i) {
                sf::Vector2f p1 = vine.getSegmentPosition(i);
                sf::Vector2f p2 = vine.getSegmentPosition(i + 1);
                
                // Calculate distance and angle between the two segments
                sf::Vector2f diff = p2 - p1;
                float length = std::sqrt(diff.x * diff.x + diff.y * diff.y);
                float angle = std::atan2(diff.y, diff.x) * 180.f / 3.14159265f;
                
                // Stretch and rotate the rectangle to bridge the points perfectly
                vineRect.setSize(sf::Vector2f(length, vineThickness));
                vineRect.setOrigin(0.f, vineThickness / 2.f);
                vineRect.setPosition(p1);
                vineRect.setRotation(angle);
                
                window.draw(vineRect);
            }
        }
    }
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
    for (const auto& pair : activePhysicalVines) {
        for (const auto& vine : pair.second) {
            for (int i = 0; i < vine.getSegmentCount(); ++i) {
                sf::Vector2f pos = vine.getSegmentPosition(i);
                sf::FloatRect vBounds(pos.x - 6.f, pos.y, 12.f, 10.f); 
                
                if (bounds.intersects(vBounds)) {
                    outVineCenter = pos.x; 
                    return true;
                }
            }
        }
    }
    return false;
}

void WorldManager::updateSway(float dt, const sf::FloatRect& viewBounds, const sf::Vector2f& windVector) {
    swayTime += dt;
    if (swayTime > 1000.f) swayTime -= 1000.f; 
    
    for (const auto& pair : chunkManager->getActiveChunks()) {
        pair.second->updateSway(swayTime, viewBounds, windVector); 
    }

    for (auto& pair : activePhysicalVines) {
        for (auto& vine : pair.second) {
            for (int i = 1; i < vine.getSegmentCount(); ++i) {
                vine.applyForce(i, sf::Vector2f(-windVector.x * dt * 2.f, 0.f));
            }
        }
    }
}

void WorldManager::disturbEnvironment(const sf::FloatRect& bounds, float velocityX) {
    int cX = chunkManager->getChunkXAt(bounds.left + bounds.width / 2.f);
    int cY = chunkManager->getChunkYAt(bounds.top + bounds.height / 2.f);
    for (int x = cX - 1; x <= cX + 1; ++x) {
        for (int y = cY - 1; y <= cY + 1; ++y) {
            Chunk* chunk = chunkManager->getChunk(x, y);
            if (!chunk) continue;
            for (auto& tree : const_cast<std::vector<Tree>&>(chunk->getTrees())) {
                tree.disturbVines(bounds, velocityX);
            }
        }
    }

    for (auto& pair : activePhysicalVines) {
        for (auto& vine : pair.second) {
            for (int i = 1; i < vine.getSegmentCount(); ++i) {
                sf::Vector2f pos = vine.getSegmentPosition(i);
                sf::FloatRect segBounds(pos.x - 12.f, pos.y - 12.f, 24.f, 24.f);
                if (bounds.intersects(segBounds)) {
                    vine.applyForce(i, sf::Vector2f(-velocityX * 0.015f, 0.f));
                }
            }
        }
    }
}

ChunkManager* WorldManager::getChunkManager() const {
    return chunkManager.get();
}