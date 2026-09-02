#include "world/WorldManager.h"
#include "simulation/SimulationRegistry.h"
#include <set>
#include <unordered_set>
#include <cmath>
#include <algorithm>

static constexpr float FLAT_GROUND_Y = 500.0f;

static inline bool treeIsStanding(TreeHarvestState s) {
    return s == TreeHarvestState::Untouched ||
           s == TreeHarvestState::Targeted ||
           s == TreeHarvestState::BeingHarvested;
}

WorldManager::WorldManager(uint32_t seed, sf::Texture& decorTex) : swayTime(0.f) {
    chunkManager = std::make_unique<ChunkManager>(seed, decorTex);
}

void WorldManager::syncVineOwnership() {
    for (const auto& chunkPair : chunkManager->getActiveChunks()) {
        auto vIt = activePhysicalVines.find(chunkPair.first);
        if (vIt == activePhysicalVines.end()) continue;

        bool anyActive = false;
        for (const auto& av : vIt->second) {
            if (av.active) { anyActive = true; break; }
        }
        if (!anyActive) continue;

        std::unordered_set<int> standingIds;
        standingIds.reserve(chunkPair.second->getTrees().size() * 2);
        for (const auto& tree : chunkPair.second->getTrees()) {
            if (treeIsStanding(tree.getHarvestState())) {
                standingIds.insert(tree.getId());
            }
        }

        for (auto& av : vIt->second) {
            if (av.active && standingIds.find(av.ownerTreeId) == standingIds.end()) {
                av.active = false;
            }
        }
    }
}

void WorldManager::update(float dt, const sf::FloatRect& preloadBounds, const sf::FloatRect& unloadBounds, ProfilerStats& profiler) {
    chunkManager->update(preloadBounds, unloadBounds, profiler);

    std::set<uint64_t> currentActive;

    for (const auto& pair : chunkManager->getActiveChunks()) {
        uint64_t coord = pair.first;
        currentActive.insert(coord);

        if (activePhysicalVines.find(coord) == activePhysicalVines.end()) {
            std::vector<ActiveVine> chunkVines;
            for (const auto& tree : pair.second->getTrees()) {
                if (!treeIsStanding(tree.getHarvestState())) continue;

                for (const auto& staticVine : tree.getVines()) {
                    int numSegments = static_cast<int>(staticVine.length / 10.f);
                    if (numSegments < 2) numSegments = 2;
                    float damp = 0.98f + (std::rand() % 10) / 1000.f;
                    chunkVines.push_back(ActiveVine{
                        VinePhysics(staticVine.origin, numSegments, 10.f, damp),
                        tree.getId(),
                        true
                    });
                }
            }
            activePhysicalVines[coord] = std::move(chunkVines);
        }
    }

    syncVineOwnership();

    for (auto it = activePhysicalVines.begin(); it != activePhysicalVines.end(); ) {
        if (currentActive.find(it->first) == currentActive.end()) {
            it = activePhysicalVines.erase(it);
        } else {
            for (auto& av : it->second) {
                if (av.active) av.physics.update(dt);
            }
            ++it;
        }
    }
}

void WorldManager::draw(sf::RenderTarget& target, const sf::FloatRect& viewBounds) const {
    ProfilerStats dummyProfiler;
    static sf::Texture dummyTilesetTex;
    if (dummyTilesetTex.getSize().x == 0) {
        dummyTilesetTex.create(32, 32);
    }
    chunkManager->drawBackground(target, viewBounds, true, dummyProfiler, dummyTilesetTex);
    chunkManager->drawGeometry(target, viewBounds, dummyProfiler);
}

void WorldManager::drawBackground(sf::RenderTarget& target, const sf::FloatRect& viewBounds, bool showFoliage, ProfilerStats& profiler, sf::Texture& tileset) const {
    chunkManager->drawBackground(target, viewBounds, showFoliage, profiler, tileset);
}

void WorldManager::drawGeometry(sf::RenderTarget& target, const sf::FloatRect& viewBounds, ProfilerStats& profiler) const {
    chunkManager->drawGeometry(target, viewBounds, profiler);

    float vineThickness = 5.0f;
    sf::RectangleShape vineRect;
    vineRect.setFillColor(sf::Color(34, 139, 34));

    for (const auto& pair : activePhysicalVines) {
        for (const auto& av : pair.second) {
            if (!av.active) continue;
            const VinePhysics& vine = av.physics;
            for (int i = 0; i < vine.getSegmentCount() - 1; ++i) {
                sf::Vector2f p1 = vine.getSegmentPosition(i);
                sf::Vector2f p2 = vine.getSegmentPosition(i + 1);

                sf::Vector2f diff = p2 - p1;
                float length = std::sqrt(diff.x * diff.x + diff.y * diff.y);
                float angle = std::atan2(diff.y, diff.x) * 180.f / 3.14159265f;

                vineRect.setSize(sf::Vector2f(length, vineThickness));
                vineRect.setOrigin(0.f, vineThickness / 2.f);
                vineRect.setPosition(p1);
                vineRect.setRotation(angle);

                target.draw(vineRect);
            }
        }
    }
}

void WorldManager::drawDebug(sf::RenderTarget& target, const sf::FloatRect& viewBounds, const sf::FloatRect& preloadBounds, const sf::FloatRect& unloadBounds, DebugOverlay* debugOverlay) const {
    chunkManager->drawDebug(target, viewBounds, preloadBounds, unloadBounds, debugOverlay);
}

float WorldManager::getTerrainHeight(float) const {
    return FLAT_GROUND_Y;
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
                if (!treeIsStanding(tree.getHarvestState())) continue;
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

bool WorldManager::checkTrunkCollision(const sf::FloatRect& bounds, float& outTrunkCenter, float& outTrunkTop) const {
    int cX = chunkManager->getChunkXAt(bounds.left + bounds.width / 2.f);
    int cY = chunkManager->getChunkYAt(bounds.top + bounds.height / 2.f);
    for (int x = cX - 1; x <= cX + 1; ++x) {
        for (int y = cY - 1; y <= cY + 1; ++y) {
            Chunk* chunk = chunkManager->getChunk(x, y);
            if (!chunk) continue;
            for (const auto& tree : chunk->getTrees()) {
                if (!treeIsStanding(tree.getHarvestState())) continue;

                sf::FloatRect tBounds = tree.getTrunkBounds();

                if (bounds.intersects(tBounds)) {
                    outTrunkCenter = tree.getTrunkCenter();

                    float highestVisBranchY = tBounds.top + tBounds.height;
                    bool hasBranch = false;
                    for (const auto& branch : tree.getBranches()) {
                        if (!hasBranch || branch.bounds.top < highestVisBranchY) {
                            highestVisBranchY = branch.bounds.top;
                            hasBranch = true;
                        }
                    }

                    outTrunkTop = hasBranch ? highestVisBranchY - 30.f : tBounds.top + (tBounds.height * 0.2f);
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
                if (!treeIsStanding(tree.getHarvestState())) continue;
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

bool WorldManager::checkVineCollision(const sf::FloatRect& bounds, uint64_t& outChunk, int& outVine, int& outSeg) const {
    for (const auto& pair : activePhysicalVines) {
        int vineIdx = 0;
        for (const auto& av : pair.second) {
            if (!av.active) { vineIdx++; continue; }
            const VinePhysics& vine = av.physics;
            for (int i = 0; i < vine.getSegmentCount(); ++i) {
                sf::Vector2f pos = vine.getSegmentPosition(i);
                sf::FloatRect vBounds(pos.x - 10.f, pos.y, 20.f, 20.f);

                if (bounds.intersects(vBounds)) {
                    outChunk = pair.first;
                    outVine = vineIdx;
                    outSeg = i;
                    return true;
                }
            }
            vineIdx++;
        }
    }
    return false;
}

sf::Vector2f WorldManager::getVineSegmentPosition(uint64_t chunk, int vine, int seg) const {
    auto it = activePhysicalVines.find(chunk);
    if (it != activePhysicalVines.end() && vine >= 0 && vine < static_cast<int>(it->second.size())) {
        const auto& av = it->second[vine];
        if (!av.active) return sf::Vector2f(0.f, 0.f);
        return av.physics.getSegmentPosition(std::clamp(seg, 0, av.physics.getSegmentCount() - 1));
    }
    return sf::Vector2f(0.f, 0.f);
}

sf::Vector2f WorldManager::getVineSegmentVelocity(uint64_t chunk, int vine, int seg, float dt) const {
    auto it = activePhysicalVines.find(chunk);
    if (it != activePhysicalVines.end() && vine >= 0 && vine < static_cast<int>(it->second.size())) {
        const auto& av = it->second[vine];
        if (!av.active) return sf::Vector2f(0.f, 0.f);
        return av.physics.getSegmentVelocity(std::clamp(seg, 0, av.physics.getSegmentCount() - 1), dt);
    }
    return sf::Vector2f(0.f, 0.f);
}

void WorldManager::applyVineForce(uint64_t chunk, int vine, int seg, const sf::Vector2f& force) {
    auto it = activePhysicalVines.find(chunk);
    if (it != activePhysicalVines.end() && vine >= 0 && vine < static_cast<int>(it->second.size())) {
        auto& av = it->second[vine];
        if (!av.active) return;
        av.physics.applyForce(std::clamp(seg, 0, av.physics.getSegmentCount() - 1), force);
    }
}

int WorldManager::getVineSegmentCount(uint64_t chunk, int vine) const {
    auto it = activePhysicalVines.find(chunk);
    if (it != activePhysicalVines.end() && vine >= 0 && vine < static_cast<int>(it->second.size())) {
        if (!it->second[vine].active) return 0;
        return it->second[vine].physics.getSegmentCount();
    }
    return 0;
}

void WorldManager::updateSway(float dt, const sf::FloatRect& viewBounds, const sf::Vector2f& windVector) {
    swayTime += dt;
    if (swayTime > 1000.f) swayTime -= 1000.f;

    for (const auto& pair : chunkManager->getActiveChunks()) {
        pair.second->updateSway(swayTime, viewBounds, windVector);
    }

    for (auto& pair : activePhysicalVines) {
        for (auto& av : pair.second) {
            if (!av.active) continue;
            for (int i = 1; i < av.physics.getSegmentCount(); ++i) {
                av.physics.applyForce(i, sf::Vector2f(-windVector.x * dt * 2.f, 0.f));
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
                if (!treeIsStanding(tree.getHarvestState())) continue;
                tree.disturbVines(bounds, velocityX);
            }
        }
    }

    for (auto& pair : activePhysicalVines) {
        for (auto& av : pair.second) {
            if (!av.active) continue;
            for (int i = 1; i < av.physics.getSegmentCount(); ++i) {
                sf::Vector2f pos = av.physics.getSegmentPosition(i);
                sf::FloatRect segBounds(pos.x - 12.f, pos.y - 12.f, 24.f, 24.f);
                if (bounds.intersects(segBounds)) {
                    av.physics.applyForce(i, sf::Vector2f(-velocityX * 0.015f, 0.f));
                }
            }
        }
    }
}

ChunkManager* WorldManager::getChunkManager() const {
    return chunkManager.get();
}

void WorldManager::drawTerritoryMarkers(sf::RenderTarget& target, sim::SimulationRegistry& registry, const sf::FloatRect& viewBounds) const {
    for (const auto& pair : registry.getAllVillages()) {
        const sim::VillageData& v = pair.second;

        float leftEdge = v.borderMinX;
        float rightEdge = v.borderMaxX;

        if (std::abs(leftEdge - v.centerX) < 500.f || std::abs(rightEdge - v.centerX) < 500.f) {
            continue;
        }

        bool hideLeft = false;
        bool hideRight = false;
        if (v.isExpandingBorder && v.borderMoverApe != 0) {
            sim::ApeData* mover = registry.getApe(v.borderMoverApe);
            if (mover && mover->isCarryingBorder) {
                if (v.expandingSideRight) {
                    hideRight = true;
                } else {
                    hideLeft = true;
                }
            }
        }

        sf::Color kingdomColor = sf::Color(200, 50, 50);
        if (v.kingdomId % 3 == 1) kingdomColor = sf::Color(50, 100, 200);
        else if (v.kingdomId % 3 == 2) kingdomColor = sf::Color(200, 150, 20);

        if (!hideLeft && leftEdge >= viewBounds.left - 500.f && leftEdge <= viewBounds.left + viewBounds.width + 500.f) {
            float groundY = getTerrainHeight(leftEdge);

            if (villageTexture && villageTexture->getSize().x > 0) {
                sf::Sprite lookpost(*villageTexture, rectLookpost);
                lookpost.setOrigin(static_cast<float>(rectLookpost.width) * 0.5f, static_cast<float>(rectLookpost.height));
                lookpost.setPosition(leftEdge + 360.f, groundY);
                lookpost.setScale(0.82f, 0.82f);
                target.draw(lookpost);

                sf::Sprite totem(*villageTexture, rectBorderMonument);
                totem.setOrigin(static_cast<float>(rectBorderMonument.width) * 0.5f, static_cast<float>(rectBorderMonument.height));
                totem.setPosition(leftEdge, groundY);
                totem.setScale(0.48f, 0.48f);
                target.draw(totem);
            } else {
                sf::RectangleShape pole(sf::Vector2f(6.f, 120.f));
                pole.setOrigin(3.f, 120.f);
                pole.setPosition(leftEdge, groundY);
                pole.setFillColor(sf::Color(101, 67, 33));
                target.draw(pole);

                sf::ConvexShape flag(3);
                flag.setPoint(0, sf::Vector2f(0.f, 0.f));
                flag.setPoint(1, sf::Vector2f(40.f, 15.f));
                flag.setPoint(2, sf::Vector2f(0.f, 30.f));
                flag.setPosition(leftEdge + 3.f, groundY - 110.f);
                flag.setFillColor(kingdomColor);
                target.draw(flag);

                sf::CircleShape skull(12.f);
                skull.setOrigin(12.f, 12.f);
                skull.setPosition(leftEdge, groundY - 120.f);
                skull.setFillColor(sf::Color(220, 220, 220));
                target.draw(skull);
            }
        }

        if (!hideRight && rightEdge >= viewBounds.left - 500.f && rightEdge <= viewBounds.left + viewBounds.width + 500.f) {
            float groundY = getTerrainHeight(rightEdge);

            if (villageTexture && villageTexture->getSize().x > 0) {
                sf::Sprite lookpost(*villageTexture, rectLookpost);
                lookpost.setOrigin(static_cast<float>(rectLookpost.width) * 0.5f, static_cast<float>(rectLookpost.height));
                lookpost.setPosition(rightEdge - 360.f, groundY);
                lookpost.setScale(0.82f, 0.82f);
                target.draw(lookpost);

                sf::Sprite totem(*villageTexture, rectBorderMonument);
                totem.setOrigin(static_cast<float>(rectBorderMonument.width) * 0.5f, static_cast<float>(rectBorderMonument.height));
                totem.setPosition(rightEdge, groundY);
                totem.setScale(0.48f, 0.48f);
                target.draw(totem);
            } else {
                sf::RectangleShape pole(sf::Vector2f(6.f, 120.f));
                pole.setOrigin(3.f, 120.f);
                pole.setPosition(rightEdge, groundY);
                pole.setFillColor(sf::Color(101, 67, 33));
                target.draw(pole);

                sf::ConvexShape flag(3);
                flag.setPoint(0, sf::Vector2f(0.f, 0.f));
                flag.setPoint(1, sf::Vector2f(-40.f, 15.f));
                flag.setPoint(2, sf::Vector2f(0.f, 30.f));
                flag.setPosition(rightEdge - 3.f, groundY - 110.f);
                flag.setFillColor(kingdomColor);
                target.draw(flag);

                sf::CircleShape skull(12.f);
                skull.setOrigin(12.f, 12.f);
                skull.setPosition(rightEdge, groundY - 120.f);
                skull.setFillColor(sf::Color(220, 220, 220));
                target.draw(skull);
            }
        }
    }
}

std::vector<Tree*> WorldManager::getNearbyTrees(float centerX, float radius) {
    return chunkManager ? chunkManager->getNearbyTrees(centerX, radius) : std::vector<Tree*>{};
}

bool WorldManager::harvestTree(int treeId) {
    return chunkManager ? chunkManager->harvestTree(treeId) : false;
}

bool WorldManager::harvestTreeNear(float worldX, float radius) {
    return chunkManager ? chunkManager->harvestTreeNear(worldX, radius) : false;
}