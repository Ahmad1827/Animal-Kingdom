#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <memory>
#include <algorithm>
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
    sf::VertexArray undergroundMesh;

    float terrainGenTime;
    float treeGenTime;
    float totalGenTime;

public:
    Chunk(ChunkPos pos, float width, float height, uint32_t worldSeed, sf::Texture& decorTex);
    void drawBackground(sf::RenderTarget& target, const sf::FloatRect& viewBounds, bool showFoliage, ProfilerStats& profiler, sf::Texture& tileset) const;
    void drawGeometry(sf::RenderTarget& target, const sf::FloatRect& viewBounds, ProfilerStats& profiler) const;
    void updateSway(float globalTime, const sf::FloatRect& viewBounds, const sf::Vector2f& windVector);
    const std::vector<Decoration>& getDecorations() const { return decorations; }
    const std::vector<Tree>& getTrees() const;
    std::vector<Tree>& getMutableTrees() { return trees; }
    bool removeTree(int treeId, float worldX = -999999.f) {
        auto it = std::remove_if(trees.begin(), trees.end(), [treeId, worldX](const Tree& t) {
            bool idMatch = (treeId != 0 && t.getId() == treeId);
            bool harvestedState = (t.getHarvestState() == TreeHarvestState::Harvested);
            bool posMatch = (worldX > -900000.f && std::abs(t.getTrunkCenter() - worldX) <= 120.f);
            return idMatch || harvestedState || posMatch;
        });
        if (it != trees.end()) {
            trees.erase(it, trees.end());
            return true;
        }
        return false;
    }
    bool removeTreeNear(float worldX, float radius) {
        auto it = std::remove_if(trees.begin(), trees.end(), [worldX, radius](const Tree& t) {
            return std::abs(t.getTrunkCenter() - worldX) <= radius || t.getHarvestState() == TreeHarvestState::Harvested;
        });
        if (it != trees.end()) {
            trees.erase(it, trees.end());
            return true;
        }
        return false;
    }
    RegionType getRegionType() const;
    ChunkPos getPos() const;
    sf::FloatRect getBounds() const;
    
    float getTerrainGenTime() const;
    float getTreeGenTime() const;
    float getTotalGenTime() const;
};