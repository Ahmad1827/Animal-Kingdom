#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <cstdint>
#include "world/WorldObject.h"
#include "core/Profiler.h"

struct BranchData { sf::FloatRect bounds; };
struct VineData { sf::Vector2f origin; float length; float disturbance; };
struct CanopyData { sf::Vector2f center; float radius; sf::Color color; };

class Tree : public WorldObject {
private:
    sf::FloatRect totalBounds;
    sf::FloatRect trunkBounds;

    sf::Sprite trunkSprite;
    std::vector<sf::Sprite> branchSprites;
    
    std::vector<BranchData> branchData;
    std::vector<VineData> vineData;
    std::vector<CanopyData> canopyData;

    sf::VertexArray staticMesh;  // Batched trunk + branches
    sf::VertexArray dynamicMesh; // Batched vines + leaves

    void appendQuad(sf::VertexArray& mesh, const sf::FloatRect& rect, sf::Color color);
    void appendOctagon(sf::VertexArray& mesh, const sf::Vector2f& center, float radius, sf::Color color);

public:
    Tree(float x, float y, float width, float height, sf::Color trunkColor, sf::Texture& decorTexture);
    void addBranch(float yOffset, float width, bool rightSide, sf::Color color, sf::Texture& decorTexture);
    void addVine(float xOffset, float yOffset, float length);
    void buildCanopy(uint32_t& seed, float baseRadius, float yOffset, sf::Color color, int clusterCount);
    void initDynamicMesh(); // Pre-allocates vertex arrays to prevent render-loop reallocations
    
    void update(float dt) override;
    void updateSway(float globalTime, const sf::Vector2f& windVector);
    void disturbVines(const sf::FloatRect& bounds, float velocityX);
    void draw(sf::RenderWindow& window) const override;
    void drawCanopy(sf::RenderWindow& window, const sf::FloatRect& viewBounds, ProfilerStats& profiler) const;
    void drawGeometry(sf::RenderWindow& window, const sf::FloatRect& viewBounds, ProfilerStats& profiler) const;
    
    sf::FloatRect getBounds() const override;
    sf::FloatRect getTrunkBounds() const;
    float getTrunkCenter() const;
    const std::vector<BranchData>& getBranches() const;
    const std::vector<VineData>& getVines() const;
};