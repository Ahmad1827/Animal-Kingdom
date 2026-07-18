#include "world/Chunk.h"
#include "world/TerrainGenerator.h"
#include "world/WorldGenerator.h"
#include "world/SeedManager.h"

Chunk::Chunk(int chunkIndex, float width, uint32_t worldSeed) : chunkIndex(chunkIndex), width(width) {
    startX = chunkIndex * width;
    uint32_t chunkSeed = SeedManager::getChunkSeed(worldSeed, chunkIndex);
    
    regionType = Biome::determineRegion(chunkIndex, worldSeed);
    BiomeProperties props = Biome::getProperties(regionType);
    
    terrainMesh = TerrainGenerator::generateTerrainMesh(startX, width, 50.f, worldSeed, props.groundColor, props.undergroundColor);
    trees = WorldGenerator::generateTrees(startX, width, chunkSeed, worldSeed, props);
    decorations = WorldGenerator::generateDecorations(startX, width, chunkSeed, worldSeed, props);
}

void Chunk::draw(sf::RenderWindow& window, const sf::FloatRect& viewBounds) const {
    drawBackground(window, viewBounds, true);
    drawGeometry(window, viewBounds);
}

void Chunk::drawBackground(sf::RenderWindow& window, const sf::FloatRect& viewBounds, bool showFoliage) const {
    if (startX + width < viewBounds.left || startX > viewBounds.left + viewBounds.width) return; 

    window.draw(terrainMesh);
    
    for (const auto& dec : decorations) {
        if (dec.getBounds().intersects(viewBounds)) {
            dec.draw(window);
        }
    }
    
    if (showFoliage) {
        for (const auto& tree : trees) {
            if (tree.getBounds().intersects(viewBounds)) {
                tree.drawCanopy(window);
            }
        }
    }
}

void Chunk::drawGeometry(sf::RenderWindow& window, const sf::FloatRect& viewBounds) const {
    if (startX + width < viewBounds.left || startX > viewBounds.left + viewBounds.width) return; 

    for (const auto& tree : trees) {
        if (tree.getBounds().intersects(viewBounds)) {
            tree.drawGeometry(window);
        }
    }
}

const std::vector<Tree>& Chunk::getTrees() const {
    return trees;
}

RegionType Chunk::getRegionType() const {
    return regionType;
}

int Chunk::getIndex() const {
    return chunkIndex;
}