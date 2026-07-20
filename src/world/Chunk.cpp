#include "world/Chunk.h"
#include "world/TerrainGenerator.h"
#include "world/WorldGenerator.h"
#include "world/SeedManager.h"

Chunk::Chunk(ChunkPos pos, float width, float height, uint32_t worldSeed, sf::Texture& decorTex) : pos(pos) {
    sf::Clock totalClock;
    
    bounds = sf::FloatRect(pos.x * width, pos.y * height, width, height);
    uint32_t chunkSeed = SeedManager::getChunkSeed(worldSeed, pos.x) + pos.y;
    
    regionType = Biome::determineRegion(pos.x, worldSeed);
    BiomeProperties props = Biome::getProperties(regionType);
    
    sf::Clock stepClock;
    terrainMesh = TerrainGenerator::generateSurfaceMesh(bounds, 50.f, worldSeed);
    undergroundMesh = TerrainGenerator::generateUndergroundMesh(bounds, 50.f, worldSeed, props.undergroundColor);
    
    waterMesh.setPrimitiveType(sf::Quads);
    const float WATER_LEVEL = 750.f;
    float res = 50.f;
    int points = static_cast<int>(bounds.width / res);
    
    for (int i = 0; i < points; ++i) {
        float x1 = bounds.left + (i * res);
        float x2 = x1 + res;
        float y1 = TerrainGenerator::getTerrainHeight(x1, worldSeed);
        float y2 = TerrainGenerator::getTerrainHeight(x2, worldSeed);
        
        if (y1 > WATER_LEVEL || y2 > WATER_LEVEL) {
            sf::Color wc(20, 90, 140, 160);
            waterMesh.append(sf::Vertex(sf::Vector2f(x1, WATER_LEVEL), wc));
            waterMesh.append(sf::Vertex(sf::Vector2f(x2, WATER_LEVEL), wc));
            waterMesh.append(sf::Vertex(sf::Vector2f(x2, std::max(y1, y2) + 100.f), wc));
            waterMesh.append(sf::Vertex(sf::Vector2f(x1, std::max(y1, y2) + 100.f), wc));
        }
    }
    
    terrainGenTime = stepClock.restart().asSeconds();
    
    float genStartX = bounds.left - 1000.f;
    float genWidth = bounds.width + 2000.f;
    
    trees.reserve(20);
    std::vector<Tree> candidateTrees = WorldGenerator::generateTrees(genStartX, genWidth, SeedManager::getChunkSeed(worldSeed, pos.x), worldSeed, props, decorTex);
    for (auto& tree : candidateTrees) {
        if (tree.getBounds().intersects(bounds)) {
            trees.push_back(std::move(tree));
        }
    }
    treeGenTime = stepClock.restart().asSeconds();
    
    decorations.reserve(30);
    std::vector<Decoration> candidateDecs = WorldGenerator::generateDecorations(bounds.left, bounds.width, chunkSeed, worldSeed, props, decorTex);
    for (auto& dec : candidateDecs) {
        if (dec.getBounds().intersects(bounds)) decorations.push_back(std::move(dec));
    }
    
    totalGenTime = totalClock.getElapsedTime().asSeconds();
}

void Chunk::updateSway(float globalTime, const sf::FloatRect& viewBounds, const sf::Vector2f& windVector) {
    if (!bounds.intersects(viewBounds)) return; 
    for (auto& tree : trees) {
        if (tree.getBounds().intersects(viewBounds)) {
            tree.updateSway(globalTime, windVector);
        }
    }
}

float Chunk::getTerrainGenTime() const { return terrainGenTime; }
float Chunk::getTreeGenTime() const { return treeGenTime; }
float Chunk::getTotalGenTime() const { return totalGenTime; }
const std::vector<Tree>& Chunk::getTrees() const { return trees; }
RegionType Chunk::getRegionType() const { return regionType; }
ChunkPos Chunk::getPos() const { return pos; }
sf::FloatRect Chunk::getBounds() const { return bounds; }

void Chunk::drawBackground(sf::RenderWindow& window, const sf::FloatRect& viewBounds, bool showFoliage, ProfilerStats& profiler, sf::Texture& tileset) const {
    if (!bounds.intersects(viewBounds)) return;

    if (undergroundMesh.getVertexCount() > 0) {
        window.draw(undergroundMesh);
        profiler.drawCalls++;
        profiler.objectsRendered++;
    }

    if (terrainMesh.getVertexCount() > 0) {
        sf::RenderStates states;
        states.texture = &tileset;
        window.draw(terrainMesh, states);
        profiler.drawCalls++;
        profiler.objectsRendered++;
    }

    if (waterMesh.getVertexCount() > 0) {
        sf::RenderStates waterStates;
        waterStates.texture = nullptr;
        window.draw(waterMesh, waterStates);
        profiler.drawCalls++;
        profiler.objectsRendered++;
    }
}

void Chunk::drawGeometry(sf::RenderWindow& window, const sf::FloatRect& viewBounds, ProfilerStats& profiler) const {
    if (!bounds.intersects(viewBounds)) return; 

    for (const auto& dec : decorations) {
        if (dec.getBounds().intersects(viewBounds)) {
            dec.draw(window);
            profiler.drawCalls++;
            profiler.visibleDecorations++;
        }
    }

    for (const auto& tree : trees) {
        if (tree.getBounds().intersects(viewBounds)) {
            tree.drawGeometry(window, viewBounds, profiler);
            profiler.visibleTrees++;
        } else profiler.objectsCulled++;
    }
}