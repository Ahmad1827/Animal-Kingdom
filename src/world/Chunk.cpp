#include "world/Chunk.h"
#include "world/TerrainGenerator.h"
#include "world/ForestGenerator.h"

Chunk::Chunk(float startX, float width) : startX(startX), width(width) {
    terrainMesh = TerrainGenerator::generateTerrainMesh(startX, width, 50.f);
    trees = ForestGenerator::generateTrees(startX, width);
    decorations = DecorationGenerator::generateDecorations(startX, width);
}

void Chunk::draw(sf::RenderWindow& window) const {
    for (const auto& bush : decorations.bushes) window.draw(bush);
    for (const auto& log : decorations.logs) window.draw(log);
    for (const auto& tree : trees) tree.draw(window);
    for (const auto& rock : decorations.rocks) window.draw(rock);
    window.draw(terrainMesh);
}

const std::vector<Tree>& Chunk::getTrees() const {
    return trees;
}