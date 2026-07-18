#include "world/WorldManager.h"

WorldManager::WorldManager() {
    chunkManager = std::make_unique<ChunkManager>();
}

void WorldManager::update(float dt, float playerX) {
    chunkManager->update(playerX);
}

void WorldManager::draw(sf::RenderWindow& window) const {
    chunkManager->draw(window);
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