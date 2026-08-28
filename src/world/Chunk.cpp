#include "world/Chunk.h"
#include "world/WorldGenerator.h"
#include "world/SeedManager.h"
#include "core/VisualConfig.h"
#include <cmath>
#include <algorithm>
#include <iostream>

static constexpr float FLAT_GROUND_Y = 500.0f;
static constexpr float VISIBLE_GROUND_DEPTH = 55.0f;

static uint32_t hashCoord(uint32_t worldSeed, int gridPos, uint32_t salt = 0) {
    uint32_t h = worldSeed ^ static_cast<uint32_t>(gridPos * 73856093) ^ (salt * 19349663);
    h = (h ^ (h >> 13)) * 0x5bd1e995;
    h = (h ^ (h >> 15)) * 0x1b873593;
    h = h ^ (h >> 16);
    return h;
}

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
        sf::Texture& jungleGroundTex = Tree::getJungleGroundTexture(decorTex);
        BiomeTransitionInfo centerTrans = Biome::getTransitionInfo(bounds.left + bounds.width * 0.5f);
        isJungleTerrain = (centerTrans.jungleWeight > 0.15f);

        if (!isJungleTerrain) {
            const float subStep = 16.0f;
            int slices = static_cast<int>(std::ceil(bounds.width / subStep));

            for (int s = 0; s < slices; ++s) {
                float x1 = bounds.left + s * subStep;
                float x2 = std::min(x1 + subStep, bounds.left + bounds.width);

                sf::Color cGround1 = Biome::getBlendedGroundColor(x1, worldSeed);
                sf::Color cGround2 = Biome::getBlendedGroundColor(x2, worldSeed);

                sf::Color cUnder1 = Biome::getBlendedUndergroundColor(x1, worldSeed);
                sf::Color cUnder2 = Biome::getBlendedUndergroundColor(x2, worldSeed);

                sf::Color cSub1 = Biome::getBlendedSubsoilColor(x1, worldSeed);
                sf::Color cSub2 = Biome::getBlendedSubsoilColor(x2, worldSeed);

                sf::Color cHumus1(static_cast<sf::Uint8>(cGround1.r * 0.55f + cUnder1.r * 0.45f),
                                  static_cast<sf::Uint8>(cGround1.g * 0.55f + cUnder1.g * 0.45f),
                                  static_cast<sf::Uint8>(cGround1.b * 0.55f + cUnder1.b * 0.45f));
                sf::Color cHumus2(static_cast<sf::Uint8>(cGround2.r * 0.55f + cUnder2.r * 0.45f),
                                  static_cast<sf::Uint8>(cGround2.g * 0.55f + cUnder2.g * 0.45f),
                                  static_cast<sf::Uint8>(cGround2.b * 0.55f + cUnder2.b * 0.45f));

                sf::Color cDeep1(static_cast<sf::Uint8>(cSub1.r * 0.35f),
                                 static_cast<sf::Uint8>(cSub1.g * 0.35f),
                                 static_cast<sf::Uint8>(cSub1.b * 0.35f));
                sf::Color cDeep2(static_cast<sf::Uint8>(cSub2.r * 0.35f),
                                 static_cast<sf::Uint8>(cSub2.g * 0.35f),
                                 static_cast<sf::Uint8>(cSub2.b * 0.35f));

                float y0 = FLAT_GROUND_Y - 4.0f;
                float y1 = FLAT_GROUND_Y + 12.0f;
                float y2 = FLAT_GROUND_Y + 30.0f;
                float yDeep = FLAT_GROUND_Y + VISIBLE_GROUND_DEPTH;

                auto addQuad = [&](float tY1, float tY2, float bY1, float bY2, sf::Color tc1, sf::Color tc2, sf::Color bc1, sf::Color bc2) {
                    undergroundMesh.append(sf::Vertex(sf::Vector2f(x1, tY1), tc1));
                    undergroundMesh.append(sf::Vertex(sf::Vector2f(x2, tY2), tc2));
                    undergroundMesh.append(sf::Vertex(sf::Vector2f(x1, bY1), bc1));

                    undergroundMesh.append(sf::Vertex(sf::Vector2f(x2, tY2), tc2));
                    undergroundMesh.append(sf::Vertex(sf::Vector2f(x2, bY2), bc2));
                    undergroundMesh.append(sf::Vertex(sf::Vector2f(x1, bY1), bc1));
                };

                addQuad(y0, y0, y1, y1, cGround1, cGround2, cHumus1, cHumus2);
                addQuad(y1, y1, y2, y2, cHumus1, cHumus2, cUnder1, cUnder2);
                addQuad(y2, y2, yDeep, yDeep, cUnder1, cUnder2, cSub1, cSub2);
            }

            const float step = 4.0f;
            int segments = static_cast<int>(std::ceil(bounds.width / step));
            for (int i = 0; i < segments; ++i) {
                float xL = bounds.left + i * step;
                float xR = std::min(xL + step, bounds.left + bounds.width);
                float midX = (xL + xR) * 0.5f;

                float microHash = std::fmod(std::abs(std::sin(midX * 12.9898f + worldSeed * 0.05f)) * 43758.5453f, 1.0f);
                float bladeHeight = 2.0f + microHash * 3.0f;

                sf::Color tipColor = Biome::getBlendedGrassTipColor(midX, worldSeed);
                sf::Color baseColor = Biome::getBlendedGrassBaseColor(midX, worldSeed);

                float tipX = midX + (microHash - 0.5f) * (step * 0.6f);
                float groundBaseY = FLAT_GROUND_Y - 4.0f;

                terrainMesh.append(sf::Vertex(sf::Vector2f(tipX, groundBaseY - bladeHeight), tipColor));
                terrainMesh.append(sf::Vertex(sf::Vector2f(xR, groundBaseY), baseColor));
                terrainMesh.append(sf::Vertex(sf::Vector2f(xL, groundBaseY), baseColor));
            }
        }

        if (isJungleTerrain) {
            float grassBottomY = FLAT_GROUND_Y + 28.0f;

            float soilTileW = 486.0f;
            float soilStepX = soilTileW - 2.0f;
            float startSoilX = std::floor((bounds.left - 50.0f) / soilStepX) * soilStepX;
            float endSoilX = bounds.left + bounds.width + 50.0f;

            for (float curX = startSoilX; curX < endSoilX; curX += soilStepX) {
                int tileIndex = static_cast<int>(std::floor(curX / soilStepX));
                uint32_t sSeed = hashCoord(worldSeed, tileIndex, 42);

                sf::IntRect sRect = VisualConfig::JUNGLE_SOIL_01;
                int sRoll = sSeed % 3;
                if (sRoll == 1) sRect = VisualConfig::JUNGLE_SOIL_02;
                else if (sRoll == 2) sRect = VisualConfig::JUNGLE_SOIL_03;

                sf::Sprite soilSpr(jungleGroundTex);
                soilSpr.setTextureRect(sRect);
                soilSpr.setPosition(curX, grassBottomY);

                bool mirrored = ((tileIndex & 1) == 0);
                if (mirrored) {
                    soilSpr.setOrigin(static_cast<float>(sRect.width), 0.0f);
                    soilSpr.setScale(-1.0f, 1.0f);
                } else {
                    soilSpr.setOrigin(0.0f, 0.0f);
                    soilSpr.setScale(1.0f, 1.0f);
                }

                soilSpr.setColor(sf::Color(255, 255, 255, 255));
                jungleSoilSprites.push_back(soilSpr);
            }

            float topTileW = 1470.0f;
            float topStepX = topTileW - 2.0f;
            float startTopX = std::floor((bounds.left - 100.0f) / topStepX) * topStepX;
            float endTopX = bounds.left + bounds.width + 100.0f;

            for (float curX = startTopX; curX < endTopX; curX += topStepX) {
                int tileIndex = static_cast<int>(std::floor(curX / topStepX));
                uint32_t tSeed = hashCoord(worldSeed, tileIndex, 19);

                sf::IntRect topRect = ((tSeed % 2) == 0) ? VisualConfig::JUNGLE_GROUND_TOP_01 : VisualConfig::JUNGLE_GROUND_TOP_02;

                sf::Sprite topSpr(jungleGroundTex);
                topSpr.setTextureRect(topRect);
                topSpr.setPosition(curX, grassBottomY);

                bool mirrored = ((tileIndex & 1) == 0);
                if (mirrored) {
                    topSpr.setOrigin(static_cast<float>(topRect.width), static_cast<float>(topRect.height));
                    topSpr.setScale(-1.0f, 1.0f);
                } else {
                    topSpr.setOrigin(0.0f, static_cast<float>(topRect.height));
                    topSpr.setScale(1.0f, 1.0f);
                }

                topSpr.setColor(sf::Color(255, 255, 255, 255));
                jungleSurfaceSprites.push_back(topSpr);
            }
        }

        trees.reserve(40);
        std::vector<Tree> candidateTrees = WorldGenerator::generateTrees(bounds.left, bounds.width, chunkSeed, worldSeed, props, decorTex);
        for (auto& tree : candidateTrees) {
            trees.push_back(std::move(tree));
        }

        decorations.reserve(100);
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

    if (!isJungleTerrain && undergroundMesh.getVertexCount() > 0) {
        target.draw(undergroundMesh);
        profiler.drawCalls++;
        profiler.objectsRendered++;
    }

    if (isJungleTerrain) {
        for (const auto& s : jungleSoilSprites) {
            if (s.getGlobalBounds().intersects(viewBounds)) {
                target.draw(s);
                profiler.drawCalls++;
                profiler.objectsRendered++;
            }
        }
    }

    if (!isJungleTerrain && terrainMesh.getVertexCount() > 0) {
        sf::RenderStates states;
        states.texture = nullptr;
        target.draw(terrainMesh, states);
        profiler.drawCalls++;
        profiler.objectsRendered++;
    }

    if (isJungleTerrain) {
        for (const auto& s : jungleSurfaceSprites) {
            if (s.getGlobalBounds().intersects(viewBounds)) {
                target.draw(s);
                profiler.drawCalls++;
                profiler.objectsRendered++;
            }
        }
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