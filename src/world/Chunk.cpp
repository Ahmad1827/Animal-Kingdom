#include "world/Chunk.h"
#include "world/TerrainGenerator.h"
#include "world/WorldGenerator.h"
#include "world/SeedManager.h"

Chunk::Chunk(ChunkPos pos, float width, float height, uint32_t worldSeed) : pos(pos) {
    sf::Clock totalClock;
    
    bounds = sf::FloatRect(pos.x * width, pos.y * height, width, height);
    uint32_t chunkSeed = SeedManager::getChunkSeed(worldSeed, pos.x) + pos.y; // Mix Y for local variance
    
    regionType = Biome::determineRegion(pos.x, worldSeed);
    BiomeProperties props = Biome::getProperties(regionType);
    
    sf::Clock stepClock;
    terrainMesh = TerrainGenerator::generateSurfaceMesh(bounds, 50.f, worldSeed);
    undergroundMesh = TerrainGenerator::generateUndergroundMesh(bounds, 50.f, worldSeed, props.undergroundColor);
    
    // Procedural Water Body Generation (Water level at Y = 750)
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
    
    // Expand generation area horizontally to catch trees that bleed into this chunk from neighbors
    float genStartX = bounds.left - 1000.f;
    float genWidth = bounds.width + 2000.f;
    
    std::vector<Tree> candidateTrees = WorldGenerator::generateTrees(genStartX, genWidth, SeedManager::getChunkSeed(worldSeed, pos.x), worldSeed, props);
    for (auto& tree : candidateTrees) {
        if (tree.getBounds().intersects(bounds)) {
            trees.push_back(std::move(tree));
        }
    }
    treeGenTime = stepClock.restart().asSeconds();
    
    std::vector<Decoration> candidateDecs = WorldGenerator::generateDecorations(bounds.left, bounds.width, chunkSeed, worldSeed, props);
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
        window.draw(undergroundMesh); // no texture -> flat color, always seamless
        profiler.objectsRendered++;
    }

    if (terrainMesh.getVertexCount() > 0) {
        sf::RenderStates states;
        states.texture = &tileset;
        window.draw(terrainMesh, states);
        profiler.objectsRendered++;
    }

    if (waterMesh.getVertexCount() > 0) {
        sf::RenderStates waterStates;
        waterStates.texture = nullptr;
        window.draw(waterMesh, waterStates);
        profiler.objectsRendered++;
    }
}

void Chunk::drawGeometry(sf::RenderWindow& window, const sf::FloatRect& viewBounds, ProfilerStats& profiler) const {
    if (!bounds.intersects(viewBounds)) return; 

    for (const auto& tree : trees) {
        if (tree.getBounds().intersects(viewBounds)) {
            tree.drawGeometry(window, viewBounds, profiler);
        } else profiler.objectsCulled++;
    }
}