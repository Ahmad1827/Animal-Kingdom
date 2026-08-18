#include "world/Chunk.h"
#include "world/WorldGenerator.h"
#include "world/SeedManager.h"
#include <cmath>
#include <iostream>

static constexpr float FLAT_GROUND_Y = 500.0f;
static constexpr float DIRT_DEPTH = 1200.0f;

Chunk::Chunk(ChunkPos pos, float width, float height, uint32_t worldSeed, sf::Texture& decorTex) : pos(pos) {
    bounds = sf::FloatRect(pos.x * width, pos.y * height, width, height);
    uint32_t chunkSeed = SeedManager::getChunkSeed(worldSeed, pos.x) + pos.y;
    
    // Core Biome identity
    regionType = Biome::determineRegion(pos.x, worldSeed);
    BiomeProperties props = Biome::getProperties(regionType);

    undergroundMesh.setPrimitiveType(sf::Triangles);
    undergroundMesh.clear();

    float x1 = bounds.left;
    float x2 = bounds.left + bounds.width;

    float yGrassBase = FLAT_GROUND_Y - 4.0f;
    float yGrassBot  = FLAT_GROUND_Y + 8.0f;
    float yDirt1     = FLAT_GROUND_Y + 24.0f;
    float yDirt2     = FLAT_GROUND_Y + 80.0f;
    float yDirtDeep  = FLAT_GROUND_Y + DIRT_DEPTH;

    sf::Color cGrassBase = props.groundColor;
    sf::Color cGrassBot(static_cast<sf::Uint8>(props.groundColor.r * 0.7f),
                        static_cast<sf::Uint8>(props.groundColor.g * 0.7f),
                        static_cast<sf::Uint8>(props.groundColor.b * 0.7f));
    sf::Color cDirt1 = props.undergroundColor;
    sf::Color cDirt2(static_cast<sf::Uint8>(props.undergroundColor.r * 0.5f),
                     static_cast<sf::Uint8>(props.undergroundColor.g * 0.5f),
                     static_cast<sf::Uint8>(props.undergroundColor.b * 0.5f));
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

        float hash = std::fmod(std::abs(xL) * 37.1f, 7.f);
        float bladeHeight = 3.f + hash;

        terrainMesh.append(sf::Vertex(sf::Vector2f(xL + step / 2.f, yGrassBase - bladeHeight), props.grassTipColor));
        terrainMesh.append(sf::Vertex(sf::Vector2f(xR, yGrassBase), props.groundColor));
        terrainMesh.append(sf::Vertex(sf::Vector2f(xL, yGrassBase), props.groundColor));
    }

    waterMesh.setPrimitiveType(sf::Quads);
    waterMesh.clear();

    // Generate Environment
    trees.reserve(40);
    std::vector<Tree> candidateTrees = WorldGenerator::generateTrees(bounds.left, bounds.width, chunkSeed, worldSeed, props, decorTex);
    for (auto& tree : candidateTrees) {
        trees.push_back(std::move(tree));
    }

    if (regionType == BiomeType::Jungle) {
        std::cout << "[CHUNK TREE STORAGE] Chunk=" << pos.x << " TreesStored=" << trees.size() << "\n";
    }

    decorations.reserve(60);
    std::vector<Decoration> candidateDecs = WorldGenerator::generateDecorations(bounds.left, bounds.width, chunkSeed, worldSeed, props, decorTex);
    for (auto& dec : candidateDecs) {
        decorations.push_back(std::move(dec));
    }
}

void Chunk::updateSway(float globalTime, const sf::FloatRect& viewBounds, const sf::Vector2f& windVector) {
    if (!bounds.intersects(viewBounds)) return;
    for (auto& tree : trees) {
        if (tree.getBounds().intersects(viewBounds)) {
            tree.updateSway(globalTime, windVector);
        }
    }
}

const std::vector<Tree>& Chunk::getTrees() const { return trees; }
BiomeType Chunk::getRegionType() const { return regionType; }
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

    int renderedCount = 0;
    for (auto& tree : const_cast<std::vector<Tree>&>(trees)) {
        tree.update(1.0f / 60.0f);
        if (tree.getBounds().intersects(viewBounds) ||
            tree.getHarvestState() == TreeHarvestState::Falling ||
            tree.getHarvestState() == TreeHarvestState::Fading) {
            tree.drawGeometry(target, viewBounds, profiler);
            profiler.visibleTrees++;
            renderedCount++;
        } else {
            profiler.objectsCulled++;
        }
    }

    static sf::Clock renderDebugClock;
    if (regionType == BiomeType::Jungle && renderDebugClock.getElapsedTime().asSeconds() > 2.0f) {
        std::cout << "[CHUNK RENDER] Chunk=" << pos.x << " TreesRendered=" << renderedCount << "\n";
        renderDebugClock.restart();
    }
}