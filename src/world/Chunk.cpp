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
    
    regionType = Biome::determineRegion(pos.x, worldSeed);
    BiomeProperties props = Biome::getProperties(regionType);

    undergroundMesh.setPrimitiveType(sf::Triangles);
    undergroundMesh.clear();

    terrainMesh.setPrimitiveType(sf::Triangles);
    terrainMesh.clear();

    waterMesh.setPrimitiveType(sf::Quads);
    waterMesh.clear();

    bool isGroundChunk = (pos.y == 0) || (bounds.top <= FLAT_GROUND_Y && bounds.top + bounds.height > FLAT_GROUND_Y);

    if (isGroundChunk) {
        const float subStep = 50.0f;
        int slices = static_cast<int>(std::ceil(bounds.width / subStep));

        for (int s = 0; s < slices; ++s) {
            float x1 = bounds.left + s * subStep;
            float x2 = std::min(x1 + subStep, bounds.left + bounds.width);

            sf::Color cGrassBase1 = Biome::getBlendedGroundColor(x1, worldSeed);
            sf::Color cGrassBase2 = Biome::getBlendedGroundColor(x2, worldSeed);

            sf::Color cUnder1 = Biome::getBlendedUndergroundColor(x1, worldSeed);
            sf::Color cUnder2 = Biome::getBlendedUndergroundColor(x2, worldSeed);

            sf::Color cGrassBot1(static_cast<sf::Uint8>(cGrassBase1.r * 0.7f), static_cast<sf::Uint8>(cGrassBase1.g * 0.7f), static_cast<sf::Uint8>(cGrassBase1.b * 0.7f));
            sf::Color cGrassBot2(static_cast<sf::Uint8>(cGrassBase2.r * 0.7f), static_cast<sf::Uint8>(cGrassBase2.g * 0.7f), static_cast<sf::Uint8>(cGrassBase2.b * 0.7f));

            sf::Color cDirt2_1(static_cast<sf::Uint8>(cUnder1.r * 0.5f), static_cast<sf::Uint8>(cUnder1.g * 0.5f), static_cast<sf::Uint8>(cUnder1.b * 0.5f));
            sf::Color cDirt2_2(static_cast<sf::Uint8>(cUnder2.r * 0.5f), static_cast<sf::Uint8>(cUnder2.g * 0.5f), static_cast<sf::Uint8>(cUnder2.b * 0.5f));

            sf::Color cDirtDeep(0, 0, 0);

            float yGrassBase = FLAT_GROUND_Y - 4.0f;
            float yGrassBot  = FLAT_GROUND_Y + 8.0f;
            float yDirt1     = FLAT_GROUND_Y + 24.0f;
            float yDirt2     = FLAT_GROUND_Y + 80.0f;
            float yDirtDeep  = FLAT_GROUND_Y + DIRT_DEPTH;

            auto addBlendedQuad = [&](float topY, float botY, sf::Color topC1, sf::Color topC2, sf::Color botC1, sf::Color botC2) {
                undergroundMesh.append(sf::Vertex(sf::Vector2f(x1, topY), topC1));
                undergroundMesh.append(sf::Vertex(sf::Vector2f(x2, topY), topC2));
                undergroundMesh.append(sf::Vertex(sf::Vector2f(x1, botY), botC1));

                undergroundMesh.append(sf::Vertex(sf::Vector2f(x2, topY), topC2));
                undergroundMesh.append(sf::Vertex(sf::Vector2f(x2, botY), botC2));
                undergroundMesh.append(sf::Vertex(sf::Vector2f(x1, botY), botC1));
            };

            addBlendedQuad(yGrassBase, yGrassBot, cGrassBase1, cGrassBase2, cGrassBot1, cGrassBot2);
            addBlendedQuad(yGrassBot, yDirt1, cGrassBot1, cGrassBot2, cUnder1, cUnder2);
            addBlendedQuad(yDirt1, yDirt2, cUnder1, cUnder2, cDirt2_1, cDirt2_2);
            addBlendedQuad(yDirt2, yDirtDeep, cDirt2_1, cDirt2_2, cDirtDeep, cDirtDeep);
        }

        float step = 8.0f;
        int segments = static_cast<int>(std::ceil(bounds.width / step));
        for (int i = 0; i < segments; ++i) {
            float xL = bounds.left + i * step;
            float xR = std::min(xL + step, bounds.left + bounds.width);

            float hash = std::fmod(std::abs(xL) * 37.1f, 7.0f);
            float bladeHeight = 3.0f + hash;

            sf::Color tipColor = Biome::getBlendedGrassTipColor(xL + step * 0.5f, worldSeed);
            sf::Color baseColor = Biome::getBlendedGroundColor(xL + step * 0.5f, worldSeed);

            terrainMesh.append(sf::Vertex(sf::Vector2f(xL + step / 2.0f, FLAT_GROUND_Y - 4.0f - bladeHeight), tipColor));
            terrainMesh.append(sf::Vertex(sf::Vector2f(xR, FLAT_GROUND_Y - 4.0f), baseColor));
            terrainMesh.append(sf::Vertex(sf::Vector2f(xL, FLAT_GROUND_Y - 4.0f), baseColor));
        }

        trees.reserve(40);
        std::vector<Tree> candidateTrees = WorldGenerator::generateTrees(bounds.left, bounds.width, chunkSeed, worldSeed, props, decorTex);
        for (auto& tree : candidateTrees) {
            trees.push_back(std::move(tree));
        }

        decorations.reserve(60);
        std::vector<Decoration> candidateDecs = WorldGenerator::generateDecorations(bounds.left, bounds.width, chunkSeed, worldSeed, props, decorTex);
        for (auto& dec : candidateDecs) {
            decorations.push_back(std::move(dec));
        }
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

void Chunk::drawBackground(sf::RenderTarget& target, const sf::FloatRect& viewBounds, bool, ProfilerStats& profiler, sf::Texture&) const {
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