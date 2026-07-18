#include "world/WorldManager.h"

WorldManager::WorldManager(uint32_t seed) {
    chunkManager = std::make_unique<ChunkManager>(seed);
}

void WorldManager::update(float dt, float playerX) {
    chunkManager->update(playerX);
}

void WorldManager::draw(sf::RenderWindow& window, const sf::FloatRect& viewBounds) const {
    chunkManager->draw(window, viewBounds);
}

void WorldManager::drawBackground(sf::RenderWindow& window, const sf::FloatRect& viewBounds, bool showFoliage) const {
    chunkManager->drawBackground(window, viewBounds, showFoliage);
}

void WorldManager::drawGeometry(sf::RenderWindow& window, const sf::FloatRect& viewBounds) const {
    chunkManager->drawGeometry(window, viewBounds);
}

void WorldManager::drawDebug(sf::RenderWindow& window, const sf::FloatRect& viewBounds, bool b1, bool b2, bool b3) const {
    chunkManager->drawDebug(window, viewBounds, b1, b2, b3);
}

float WorldManager::getTerrainHeight(float x) const {
    return chunkManager->getTerrainHeight(x);
}

bool WorldManager::checkOneWayCollision(const sf::FloatRect& bounds, const sf::Vector2f& velocity, float dt, sf::FloatRect& outPlatformBounds) const {
    if (velocity.y <= 0) return false;
    float bottomY = bounds.top + bounds.height;
    float previousBottomY = bottomY - (velocity.y * dt);

    for (const auto& pair : chunkManager->getActiveChunks()) {
        for (const auto& tree : pair.second->getTrees()) {
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
    for (const auto& pair : chunkManager->getActiveChunks()) {
        for (const auto& tree : pair.second->getTrees()) {
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

    for (const auto& pair : chunkManager->getActiveChunks()) {
        for (const auto& tree : pair.second->getTrees()) {
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

ChunkManager* WorldManager::getChunkManager() const {
    return chunkManager.get();
}