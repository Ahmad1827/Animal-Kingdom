#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <memory>
#include "entities/Tree.h"
#include "world/Decoration.h"
#include "world/Biome.h"
#include "core/Profiler.h"

class Chunk {
private:
    float startX;
    float width;
    int chunkIndex;
    sf::VertexArray terrainMesh;
    std::vector<Tree> trees;
    std::vector<Decoration> decorations;
    RegionType regionType;

    float terrainGenTime;
    float treeGenTime;
    float totalGenTime;

public:
    Chunk(int chunkIndex, float width, uint32_t worldSeed);
    void draw(sf::RenderWindow& window, const sf::FloatRect& viewBounds) const;
    void drawBackground(sf::RenderWindow& window, const sf::FloatRect& viewBounds, bool showFoliage, ProfilerStats& profiler) const;
    void drawGeometry(sf::RenderWindow& window, const sf::FloatRect& viewBounds, ProfilerStats& profiler) const;
    
    const std::vector<Tree>& getTrees() const;
    float getTerrainGenTime() const;
    float getTreeGenTime() const;
    float getTotalGenTime() const;
    RegionType getRegionType() const;
    int getIndex() const;
};