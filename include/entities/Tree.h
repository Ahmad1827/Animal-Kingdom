#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <cstdint>
#include "world/WorldObject.h"
#include "core/Profiler.h"

struct BranchData {
    sf::FloatRect bounds;
};

struct VineData {
    sf::Vector2f origin;
    float length;
    float disturbance;
};

struct CanopyData {
    sf::Vector2f center;
    float radius;
    sf::Color color;
};

enum class TreeHarvestState {
    Untouched,
    Targeted,
    BeingHarvested,
    Falling,
    Fading,
    Harvested
};

class Tree : public WorldObject {
private:
    int treeId = 0;
    sf::FloatRect totalBounds;
    sf::FloatRect trunkBounds;

    sf::Sprite trunkSprite;
    std::vector<sf::Sprite> branchSprites;
    
    std::vector<BranchData> branchData;
    std::vector<VineData> vineData;
    std::vector<CanopyData> canopyData;

    sf::VertexArray staticMesh;
    sf::VertexArray dynamicMesh;

    TreeHarvestState harvestState = TreeHarvestState::Untouched;
    float harvestProgress = 0.0f;
    static constexpr float maxHarvestProgress = 10.0f;
    uint64_t assignedWorkerId = 0;

    float fallAngle = 0.0f;
    float fallAngularVelocity = 0.0f;
    float fallDirection = 1.0f;
    float fadeTimer = 0.0f;
    static constexpr float maxFadeTime = 1.5f;

    void appendQuad(sf::VertexArray& mesh, const sf::FloatRect& rect, sf::Color color);
    void appendOctagon(sf::VertexArray& mesh, const sf::Vector2f& center, float radius, sf::Color color);

public:
    Tree(float x, float y, float width, float height, sf::Color trunkColor, sf::Texture& decorTexture, int id = 0);
    void addBranch(float yOffset, float width, bool rightSide, sf::Color color, sf::Texture& decorTexture);
    void addVine(float xOffset, float yOffset, float length);
    void buildCanopy(uint32_t& seed, float baseRadius, float yOffset, sf::Color color, int clusterCount);
    void initDynamicMesh();
    
    void update(float dt) override;
    void updateSway(float globalTime, const sf::Vector2f& windVector);
    void disturbVines(const sf::FloatRect& bounds, float velocityX);
    void draw(sf::RenderTarget& target) const;
    void draw(sf::RenderWindow& window) const override { draw(static_cast<sf::RenderTarget&>(window)); }
    void drawCanopy(sf::RenderTarget& target, const sf::FloatRect& viewBounds, ProfilerStats& profiler) const;
    void drawGeometry(sf::RenderTarget& target, const sf::FloatRect& viewBounds, ProfilerStats& profiler) const;
    
    sf::FloatRect getBounds() const override;
    sf::FloatRect getTrunkBounds() const;
    float getTrunkCenter() const;
    const std::vector<BranchData>& getBranches() const;
    const std::vector<VineData>& getVines() const;

    int getId() const { return treeId; }
    void setId(int id) { treeId = id; }
    TreeHarvestState getHarvestState() const { return harvestState; }
    void setHarvestState(TreeHarvestState state) { harvestState = state; }
    uint64_t getAssignedWorkerId() const { return assignedWorkerId; }
    void setAssignedWorkerId(uint64_t workerId) { assignedWorkerId = workerId; }
    float getHarvestProgress() const { return harvestProgress; }
    float getMaxHarvestProgress() const { return maxHarvestProgress; }
    
    void startFalling(float direction = 1.0f) {
        harvestState = TreeHarvestState::Falling;
        fallDirection = (direction >= 0.0f) ? 1.0f : -1.0f;
        fallAngularVelocity = 15.0f * fallDirection;
        fallAngle = 0.0f;
        fadeTimer = 0.0f;
    }
    
    bool advanceHarvest(float dt) {
        harvestProgress += dt;
        return harvestProgress >= maxHarvestProgress;
    }
    void resetHarvest() {
        harvestProgress = 0.0f;
        harvestState = TreeHarvestState::Untouched;
        assignedWorkerId = 0;
        fallAngle = 0.0f;
        fallAngularVelocity = 0.0f;
        fadeTimer = 0.0f;
    }
};