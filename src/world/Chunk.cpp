#include "world/Chunk.h"
#include "world/TerrainGenerator.h"
#include "world/WorldGenerator.h"
#include "world/SeedManager.h"
#include <cmath>
#include <iostream>

static constexpr float FLAT_GROUND_Y = 500.0f;
static constexpr float DIRT_DEPTH = 1200.0f;

Chunk::Chunk(ChunkPos pos, float width, float height, uint32_t worldSeed, sf::Texture& decorTex) : pos(pos) {
    sf::Clock totalClock;
    
    bounds = sf::FloatRect(pos.x * width, pos.y * height, width, height);
    uint32_t chunkSeed = SeedManager::getChunkSeed(worldSeed, pos.x) + pos.y;
    regionType = Biome::determineRegion(pos.x, worldSeed);
    BiomeProperties props = Biome::getProperties(regionType);
    
    std::cout << "[CHUNK] Chunk generated at X=" << pos.x << " Y=" << pos.y 
              << " | Biome=" << props.name 
              << " | Bounds=[" << bounds.left << ", " << (bounds.left + bounds.width) << "]" 
              << std::endl;

    sf::Clock stepClock;

    undergroundMesh.setPrimitiveType(sf::Triangles);
    undergroundMesh.clear();

    float x1 = bounds.left;
    float x2 = bounds.left + bounds.width;

    float yGrassBase = FLAT_GROUND_Y - 4.0f; 
    float yGrassBot  = FLAT_GROUND_Y + 8.0f;
    float yDirt1     = FLAT_GROUND_Y + 24.0f;
    float yDirt2     = FLAT_GROUND_Y + 80.0f;
    float yDirtDeep  = FLAT_GROUND_Y + DIRT_DEPTH;

    sf::Color cGrassBase(45, 100, 35);
    sf::Color cGrassBot(30, 70, 25);
    sf::Color cDirt1(22, 16, 12);
    sf::Color cDirt2(12, 8, 6);
    sf::Color cDirtDeep(0, 0, 0);

    auto addQuad = [&](float topY, float botY, sf::Color topC, sf::Color botC) {
        undergroundMesh.append(sf::Vertex(sf::Vector2f(x1, topY), topC));
        undergroundMesh.append(sf::Vertex(sf::Vector2f(x2, topY), topC));
        undergroundMesh.append(sf::Vertex(sf::Vector2f(x1, botY), botC));

        undergroundMesh.append(sf::Vertex(sf::Vector2f(x2, topY), topC));
        undergroundMesh.append(sf::Vertex(sf::Vector2f(x2, botY), botC));
        undergroundMesh.append(sf::Vertex(sf::Vector2f(x1, botY), botC));
    };

    addQuad(yGrassBase, yGrassBot, cGrassBase, cGrassBot);
    addQuad(yGrassBot, yDirt1, cGrassBot, cDirt1);
    addQuad(yDirt1, yDirt2, cDirt1, cDirt2);
    addQuad(yDirt2, yDirtDeep, cDirt2, cDirtDeep);

    terrainMesh.setPrimitiveType(sf::Triangles);
    terrainMesh.clear();
    
    float step = 8.f; 
    int segments = static_cast<int>(std::ceil(bounds.width / step));
    for (int i = 0; i < segments; ++i) {
        float xL = bounds.left + i * step;
        float xR = std::min(xL + step, bounds.left + bounds.width);
        
        float hash = std::fmod(xL * 37.1f, 7.f);
        float bladeHeight = 3.f + hash;
        
        sf::Color grassTip(65, 130, 45);
        
        terrainMesh.append(sf::Vertex(sf::Vector2f(xL + step/2.f, yGrassBase - bladeHeight), grassTip)); 
        terrainMesh.append(sf::Vertex(sf::Vector2f(xR, yGrassBase), cGrassBase));
        terrainMesh.append(sf::Vertex(sf::Vector2f(xL, yGrassBase), cGrassBase));
    }

    waterMesh.setPrimitiveType(sf::Quads);
    waterMesh.clear();
    
    terrainGenTime = stepClock.restart().asSeconds();
    
    trees.reserve(25);
    std::vector<Tree> candidateTrees = WorldGenerator::generateTrees(bounds.left, bounds.width, chunkSeed, worldSeed, props, decorTex);
    for (auto& tree : candidateTrees) {
        trees.push_back(std::move(tree));
    }
    treeGenTime = stepClock.restart().asSeconds();
    
    decorations.reserve(30);
    std::vector<Decoration> candidateDecs = WorldGenerator::generateDecorations(bounds.left, bounds.width, chunkSeed, worldSeed, props, decorTex);
    for (auto& dec : candidateDecs) {
        decorations.push_back(std::move(dec));
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

void Chunk::drawBackground(sf::RenderTarget& target, const sf::FloatRect& viewBounds, bool showFoliage, ProfilerStats& profiler, sf::Texture& tileset) const {
    if (!bounds.intersects(viewBounds)) return;

    if (undergroundMesh.getVertexCount() > 0) {
        target.draw(undergroundMesh);
        profiler.drawCalls++;
        profiler.objectsRendered++;
    }

    if (terrainMesh.getVertexCount() > 0) {
        sf::RenderStates states;
        states.texture = nullptr; 
        target.draw(terrainMesh, states);
        profiler.drawCalls++;
        profiler.objectsRendered++;
    }

    if (waterMesh.getVertexCount() > 0) {
        sf::RenderStates waterStates;
        waterStates.texture = nullptr;
        target.draw(waterMesh, waterStates);
        profiler.drawCalls++;
        profiler.objectsRendered++;
    }
}

void Chunk::drawGeometry(sf::RenderTarget& target, const sf::FloatRect& viewBounds, ProfilerStats& profiler) const {
    if (!bounds.intersects(viewBounds)) return; 

    for (const auto& dec : decorations) {
        if (dec.getBounds().intersects(viewBounds)) {
            dec.draw(target);
            profiler.drawCalls++;
            profiler.visibleDecorations++;
        }
    }

    for (auto& tree : const_cast<std::vector<Tree>&>(trees)) {
        tree.update(1.0f / 60.0f);
        if (tree.getBounds().intersects(viewBounds) || 
            tree.getHarvestState() == TreeHarvestState::Falling || 
            tree.getHarvestState() == TreeHarvestState::Fading) {
            tree.drawGeometry(target, viewBounds, profiler);
            profiler.visibleTrees++;
        } else {
            profiler.objectsCulled++;
        }
    }
}