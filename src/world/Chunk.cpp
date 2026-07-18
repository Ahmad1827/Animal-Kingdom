#include "world/Chunk.h"
#include "world/TerrainGenerator.h"
#include "world/WorldGenerator.h"
#include "world/SeedManager.h"

Chunk::Chunk(int chunkIndex, float width, uint32_t worldSeed) : chunkIndex(chunkIndex), width(width) {
    sf::Clock totalClock;
    
    startX = chunkIndex * width;
    uint32_t chunkSeed = SeedManager::getChunkSeed(worldSeed, chunkIndex);
    regionType = Biome::determineRegion(chunkIndex, worldSeed);
    BiomeProperties props = Biome::getProperties(regionType);
    
    sf::Clock stepClock;
    terrainMesh = TerrainGenerator::generateTerrainMesh(startX, width, 50.f, worldSeed, props.groundColor, props.undergroundColor);
    terrainGenTime = stepClock.restart().asSeconds();
    
    trees = WorldGenerator::generateTrees(startX, width, chunkSeed, worldSeed, props);
    treeGenTime = stepClock.restart().asSeconds();
    
    decorations = WorldGenerator::generateDecorations(startX, width, chunkSeed, worldSeed, props);
    
    totalGenTime = totalClock.getElapsedTime().asSeconds();
}

float Chunk::getTerrainGenTime() const { return terrainGenTime; }
float Chunk::getTreeGenTime() const { return treeGenTime; }
float Chunk::getTotalGenTime() const { return totalGenTime; }

void Chunk::draw(sf::RenderWindow& window, const sf::FloatRect& viewBounds) const {
    ProfilerStats dummyProfiler;
    drawBackground(window, viewBounds, true, dummyProfiler);
    drawGeometry(window, viewBounds, dummyProfiler);
}

void Chunk::drawBackground(sf::RenderWindow& window, const sf::FloatRect& viewBounds, bool showFoliage, ProfilerStats& profiler) const {
    if (startX + width < viewBounds.left || startX > viewBounds.left + viewBounds.width) return; 

    window.draw(terrainMesh);
    profiler.objectsRendered++;
    
    for (const auto& dec : decorations) {
        if (dec.getBounds().intersects(viewBounds)) {
            dec.draw(window);
            profiler.objectsRendered++;
        } else profiler.objectsCulled++;
    }
    
    if (showFoliage) {
        for (const auto& tree : trees) {
            if (tree.getBounds().intersects(viewBounds)) {
                tree.drawCanopy(window, viewBounds, profiler);
            } else {
                profiler.objectsCulled += tree.getCanopy().size();
            }
        }
    }
}

void Chunk::drawGeometry(sf::RenderWindow& window, const sf::FloatRect& viewBounds, ProfilerStats& profiler) const {
    if (startX + width < viewBounds.left || startX > viewBounds.left + viewBounds.width) return; 

    for (auto tree : trees) {
        if (tree.getBounds().intersects(viewBounds)) {
            tree.drawGeometry(window, viewBounds, profiler);
        } else {
            profiler.objectsCulled += 1 + tree.getBranches().size() + tree.getVines().size();
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