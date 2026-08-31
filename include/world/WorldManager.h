#pragma once
#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>
#include <unordered_map>
#include "world/ChunkManager.h"
#include "world/VinePhysics.h"

namespace sim {
    class SimulationRegistry;
}

class DebugOverlay;
class Tree;

struct ActiveVine {
    VinePhysics physics;
    int ownerTreeId = -1;
    bool active = true;
};

class WorldManager {
private:
    std::unique_ptr<ChunkManager> chunkManager;
    std::unordered_map<uint64_t, std::vector<ActiveVine>> activePhysicalVines;
    float swayTime = 0.f;
    const sf::Texture* villageTexture = nullptr;

    const sf::IntRect rectBorderMonument = sf::IntRect(1070, 1423, 224, 666);
    const sf::IntRect rectLookpost       = sf::IntRect(1832, 1430, 275, 659);

    void syncVineOwnership();

public:
    WorldManager(uint32_t seed, sf::Texture& decorTex);

    void setVillageTexture(const sf::Texture& tex) { villageTexture = &tex; }
    const sf::Texture* getTexture() const { return villageTexture; }

    void update(float dt, const sf::FloatRect& preloadBounds, const sf::FloatRect& unloadBounds, ProfilerStats& profiler);
    void updateSway(float dt, const sf::FloatRect& viewBounds, const sf::Vector2f& windVector);

    void draw(sf::RenderTarget& target, const sf::FloatRect& viewBounds) const;
    void drawBackground(sf::RenderTarget& target, const sf::FloatRect& viewBounds, bool showFoliage, ProfilerStats& profiler, sf::Texture& tileset) const;
    void drawGeometry(sf::RenderTarget& target, const sf::FloatRect& viewBounds, ProfilerStats& profiler) const;
    void drawTerritoryMarkers(sf::RenderTarget& target, sim::SimulationRegistry& registry, const sf::FloatRect& viewBounds) const;
    void drawDebug(sf::RenderTarget& target, const sf::FloatRect& viewBounds, const sf::FloatRect& preloadBounds, const sf::FloatRect& unloadBounds, DebugOverlay* debugOverlay) const;

    float getTerrainHeight(float worldX) const;
    bool checkOneWayCollision(const sf::FloatRect& bounds, const sf::Vector2f& velocity, float dt, sf::FloatRect& outPlatformBounds) const;
    bool checkTrunkCollision(const sf::FloatRect& bounds, float& outTrunkCenter, float& outTrunkTop) const;
    bool checkHangCollision(const sf::FloatRect& bounds, sf::FloatRect& outBranchBounds) const;

    bool checkVineCollision(const sf::FloatRect& bounds, uint64_t& outChunk, int& outVine, int& outSeg) const;
    sf::Vector2f getVineSegmentPosition(uint64_t chunk, int vine, int seg) const;
    sf::Vector2f getVineSegmentVelocity(uint64_t chunk, int vine, int seg, float dt) const;
    void applyVineForce(uint64_t chunk, int vine, int seg, const sf::Vector2f& force);
    int getVineSegmentCount(uint64_t chunk, int vine) const;

    void disturbEnvironment(const sf::FloatRect& bounds, float velocityX);

    ChunkManager* getChunkManager() const;
    std::vector<Tree*> getNearbyTrees(float centerX, float radius);
    bool harvestTree(int treeId);
    bool harvestTreeNear(float worldX, float radius);
};