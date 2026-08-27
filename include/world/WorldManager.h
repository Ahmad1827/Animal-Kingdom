#pragma once
#include <SFML/Graphics.hpp>
#include "world/ChunkManager.h"
#include "world/VinePhysics.h"
#include <memory>
#include <cstdint>
#include <map>
#include <vector>
#include "simulation/SimulationRegistry.h"

class WorldManager {
private:
    std::unique_ptr<ChunkManager> chunkManager;
    float swayTime;

    // A simulated vine plus the identity of the tree it hangs from.
    //
    // `ownerTreeId` is what makes tree -> vine removal work: when the parent
    // tree stops existing (harvested, or mid-fall), only THAT tree's vines get
    // switched off. Everything else keeps swinging.
    //
    // `active` exists instead of erasing from the vector because PlayState
    // holds a vine *index* across frames while the player is swinging
    // (grabbedVine). Erasing would silently re-point a swinging player onto a
    // different vine. Deactivating in place keeps every index stable for the
    // lifetime of the chunk, and getVineSegmentCount() returns 0 for a dead
    // vine, which trips the release path PlayState already has.
    struct ActiveVine {
        VinePhysics physics;
        int ownerTreeId;
        bool active;
    };

    std::map<uint64_t, std::vector<ActiveVine>> activePhysicalVines;

    // Switches off vines whose parent tree is gone or no longer standing.
    void syncVineOwnership();

public:
    WorldManager(uint32_t seed, sf::Texture& decorTex);
    void update(float dt, const sf::FloatRect& preloadBounds, const sf::FloatRect& unloadBounds, ProfilerStats& profiler);
    void draw(sf::RenderTarget& target, const sf::FloatRect& viewBounds) const;
    void drawBackground(sf::RenderTarget& target, const sf::FloatRect& viewBounds, bool showFoliage, ProfilerStats& profiler, sf::Texture& tileset) const;
    void drawGeometry(sf::RenderTarget& target, const sf::FloatRect& viewBounds, ProfilerStats& profiler) const;
    void drawDebug(sf::RenderTarget& target, const sf::FloatRect& viewBounds, const sf::FloatRect& preloadBounds, const sf::FloatRect& unloadBounds, class DebugOverlay* debugOverlay) const;
    void drawTerritoryMarkers(sf::RenderTarget& target, sim::SimulationRegistry& registry, const sf::FloatRect& viewBounds) const;

    float getTerrainHeight(float x) const;
    bool checkOneWayCollision(const sf::FloatRect& bounds, const sf::Vector2f& velocity, float dt, sf::FloatRect& outPlatformBounds) const;
    bool checkTrunkCollision(const sf::FloatRect& bounds, float& outTrunkCenter, float& outTrunkTop) const;
    bool checkHangCollision(const sf::FloatRect& bounds, sf::FloatRect& outBranchBounds) const;
    bool checkVineCollision(const sf::FloatRect& bounds, uint64_t& outChunk, int& outVine, int& outSeg) const;

    sf::Vector2f getVineSegmentPosition(uint64_t chunk, int vine, int seg) const;
    sf::Vector2f getVineSegmentVelocity(uint64_t chunk, int vine, int seg, float dt) const;
    void applyVineForce(uint64_t chunk, int vine, int seg, const sf::Vector2f& force);
    int getVineSegmentCount(uint64_t chunk, int vine) const;

    void updateSway(float dt, const sf::FloatRect& viewBounds, const sf::Vector2f& windVector);
    void disturbEnvironment(const sf::FloatRect& bounds, float velocityX);

    ChunkManager* getChunkManager() const;
    std::vector<Tree*> getNearbyTrees(float centerX, float radius);
    bool harvestTree(int treeId);
    bool harvestTreeNear(float worldX, float radius);
};