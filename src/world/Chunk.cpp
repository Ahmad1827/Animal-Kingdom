#include "world/Chunk.h"
#include "world/TerrainGenerator.h"
#include "world/WorldGenerator.h"
#include "world/SeedManager.h"
#include <cmath>

static constexpr float FLAT_GROUND_Y = 500.0f;
static constexpr float DIRT_DEPTH = 2000.0f;

Chunk::Chunk(ChunkPos pos, float width, float height, uint32_t worldSeed, sf::Texture& decorTex) : pos(pos) {
    sf::Clock totalClock;
    
    bounds = sf::FloatRect(pos.x * width, pos.y * height, width, height);
    uint32_t chunkSeed = SeedManager::getChunkSeed(worldSeed, pos.x) + pos.y;
    
    regionType = Biome::determineRegion(pos.x, worldSeed);
    BiomeProperties props = Biome::getProperties(regionType);
    
    sf::Clock stepClock;

    float step = 50.f;
    int segments = static_cast<int>(std::ceil(bounds.width / step));

    terrainMesh.setPrimitiveType(sf::Triangles);
    terrainMesh.clear();

    undergroundMesh.setPrimitiveType(sf::Triangles);
    undergroundMesh.clear();

    for (int i = 0; i < segments; ++i) {
        float x1 = bounds.left + (i * step);
        float x2 = std::min(x1 + step, bounds.left + bounds.width);

        float yTop = FLAT_GROUND_Y;
        float yBottom = FLAT_GROUND_Y + DIRT_DEPTH;

        sf::Color dirtColor = props.undergroundColor;
        sf::Color deepDirtColor(
            static_cast<sf::Uint8>(props.undergroundColor.r * 0.6f),
            static_cast<sf::Uint8>(props.undergroundColor.g * 0.6f),
            static_cast<sf::Uint8>(props.undergroundColor.b * 0.6f)
        );

        undergroundMesh.append(sf::Vertex(sf::Vector2f(x1, yTop), dirtColor));
        undergroundMesh.append(sf::Vertex(sf::Vector2f(x2, yTop), dirtColor));
        undergroundMesh.append(sf::Vertex(sf::Vector2f(x1, yBottom), deepDirtColor));

        undergroundMesh.append(sf::Vertex(sf::Vector2f(x2, yTop), dirtColor));
        undergroundMesh.append(sf::Vertex(sf::Vector2f(x2, yBottom), deepDirtColor));
        undergroundMesh.append(sf::Vertex(sf::Vector2f(x1, yBottom), deepDirtColor));

        float grassHeight = 16.0f;
        sf::Color grassTopColor(85, 160, 45);
        sf::Color grassBottomColor(50, 110, 25);

        terrainMesh.append(sf::Vertex(sf::Vector2f(x1, yTop - grassHeight), grassTopColor, sf::Vector2f(0.f, 0.f)));
        terrainMesh.append(sf::Vertex(sf::Vector2f(x2, yTop - grassHeight), grassTopColor, sf::Vector2f(16.f, 0.f)));
        terrainMesh.append(sf::Vertex(sf::Vector2f(x1, yTop), grassBottomColor, sf::Vector2f(0.f, 16.f)));

        terrainMesh.append(sf::Vertex(sf::Vector2f(x2, yTop - grassHeight), grassTopColor, sf::Vector2f(16.f, 0.f)));
        terrainMesh.append(sf::Vertex(sf::Vector2f(x2, yTop), grassBottomColor, sf::Vector2f(16.f, 16.f)));
        terrainMesh.append(sf::Vertex(sf::Vector2f(x1, yTop), grassBottomColor, sf::Vector2f(0.f, 16.f)));
    }
    
    waterMesh.setPrimitiveType(sf::Quads);
    waterMesh.clear();
    
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

void Chunk::drawBackground(sf::RenderTarget& target, const sf::FloatRect& viewBounds, bool showFoliage, ProfilerStats& profiler, sf::Texture& tileset) const {
    if (!bounds.intersects(viewBounds)) return;

    if (undergroundMesh.getVertexCount() > 0) {
        target.draw(undergroundMesh);
        profiler.drawCalls++;
        profiler.objectsRendered++;
    }

    if (terrainMesh.getVertexCount() > 0) {
        sf::RenderStates states;
        states.texture = &tileset;
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

    for (const auto& tree : trees) {
        if (tree.getBounds().intersects(viewBounds)) {
            tree.drawGeometry(target, viewBounds, profiler);
            profiler.visibleTrees++;
        } else profiler.objectsCulled++;
    }
}