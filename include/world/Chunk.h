#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <memory>
#include "entities/Tree.h"
#include "world/Decoration.h"
#include "world/Biome.h"
#include "core/Profiler.h"

struct ChunkPos {
    int x, y;
    bool operator==(const ChunkPos& o) const { return x == o.x && y == o.y; }
};

class Chunk {
private:
    ChunkPos pos;
    sf::FloatRect bounds;
    sf::VertexArray terrainMesh;
    sf::VertexArray waterMesh;
    std::vector<Tree> trees;
    std::vector<Decoration> decorations;
    RegionType regionType;

    float terrainGenTime;
    float treeGenTime;
    float totalGenTime;

public:
    Chunk(ChunkPos pos, float width, float height, uint32_t worldSeed);
    void drawBackground(sf::RenderWindow& window, const sf::FloatRect& viewBounds, bool showFoliage, ProfilerStats& profiler) const;
    void drawGeometry(sf::RenderWindow& window, const sf::FloatRect& viewBounds, ProfilerStats& profiler) const;
    void updateSway(float globalTime, const sf::FloatRect& viewBounds, const sf::Vector2f& windVector);
    const std::vector<Decoration>& getDecorations() const { return decorations; }
    const std::vector<Tree>& getTrees() const;
    RegionType getRegionType() const;
    ChunkPos getPos() const;
    sf::FloatRect getBounds() const;
    
    float getTerrainGenTime() const;
    float getTreeGenTime() const;
    float getTotalGenTime() const;
};