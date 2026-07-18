#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <memory>
#include "entities/Tree.h"
#include "world/Decoration.h"
#include "world/Biome.h"

class Chunk {
private:
    float startX;
    float width;
    int chunkIndex;
    sf::VertexArray terrainMesh;
    std::vector<Tree> trees;
    std::vector<Decoration> decorations;
    RegionType regionType;

public:
   Chunk(int chunkIndex, float width, uint32_t worldSeed);
    void draw(sf::RenderWindow& window, const sf::FloatRect& viewBounds) const;
    void drawBackground(sf::RenderWindow& window, const sf::FloatRect& viewBounds, bool showFoliage) const;
    void drawGeometry(sf::RenderWindow& window, const sf::FloatRect& viewBounds) const;
    const std::vector<Tree>& getTrees() const;
    RegionType getRegionType() const;
    int getIndex() const;
};