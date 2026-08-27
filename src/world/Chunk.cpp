#include "world/Chunk.h"
#include "world/WorldGenerator.h"
#include "world/SeedManager.h"
#include <cmath>
#include <iostream>

static constexpr float FLAT_GROUND_Y = 500.0f;
static constexpr float DIRT_DEPTH = 1200.0f;

// Noise salts for the ground layer. Kept distinct from the ones in
// WorldGenerator so soil mottling doesn't correlate with tree placement.
static constexpr uint32_t SALT_SOIL   = 0x5011u;
static constexpr uint32_t SALT_GRASS  = 0x6A22u;

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
        const float subStep = 16.0f;
        int slices = static_cast<int>(std::ceil(bounds.width / subStep));

        // Large-scale soil mottling. Was three summed sines gated on
        // jungleWeight, so only the jungle ever got tonal variation. Now it's
        // proper low-frequency noise, and the *strength* comes from the biome
        // profile - which means desert gets sand-tone variation and field gets
        // a gentler version of the same treatment.
        auto sampleSoilVariation = [&](float x) {
            float n = SeedManager::fbm(x, worldSeed ^ SALT_SOIL, 0.0022f, 3);
            return (n - 0.5f) * 2.0f; // -1 .. 1
        };

        auto applySoilTint = [](sf::Color base, float factor) {
            float m = std::clamp(1.0f + factor * 0.22f, 0.75f, 1.25f);
            return sf::Color(
                static_cast<sf::Uint8>(std::clamp(base.r * m, 0.0f, 255.0f)),
                static_cast<sf::Uint8>(std::clamp(base.g * m, 0.0f, 255.0f)),
                static_cast<sf::Uint8>(std::clamp(base.b * m, 0.0f, 255.0f))
            );
        };

        for (int s = 0; s < slices; ++s) {
            float x1 = bounds.left + s * subStep;
            float x2 = std::min(x1 + subStep, bounds.left + bounds.width);

            BiomeTransitionInfo trans1 = Biome::getTransitionInfo(x1);
            BiomeTransitionInfo trans2 = Biome::getTransitionInfo(x2);

            EnvironmentProfile env1 = Biome::getBlendedEnvironment(x1);
            EnvironmentProfile env2 = Biome::getBlendedEnvironment(x2);

            sf::Color cGround1 = Biome::getBlendedGroundColor(x1, worldSeed);
            sf::Color cGround2 = Biome::getBlendedGroundColor(x2, worldSeed);

            sf::Color cUnder1 = Biome::getBlendedUndergroundColor(x1, worldSeed);
            sf::Color cUnder2 = Biome::getBlendedUndergroundColor(x2, worldSeed);

            sf::Color cSub1 = Biome::getBlendedSubsoilColor(x1, worldSeed);
            sf::Color cSub2 = Biome::getBlendedSubsoilColor(x2, worldSeed);

            // Strength now blends with the biome instead of being jungle-only.
            float soilVar1 = sampleSoilVariation(x1) * env1.soilVariation;
            float soilVar2 = sampleSoilVariation(x2) * env2.soilVariation;

            cGround1 = applySoilTint(cGround1, soilVar1);
            cGround2 = applySoilTint(cGround2, soilVar2);
            cUnder1 = applySoilTint(cUnder1, soilVar1 * 0.8f);
            cUnder2 = applySoilTint(cUnder2, soilVar2 * 0.8f);

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

            sf::Color cAbyss(0, 0, 0);

            float nY1 = std::sin(x1 * 0.05f) * 2.0f * trans1.jungleWeight;
            float nY2 = std::sin(x2 * 0.05f) * 2.0f * trans2.jungleWeight;

            float y0_1 = FLAT_GROUND_Y - 4.0f;
            float y0_2 = FLAT_GROUND_Y - 4.0f;
            float y1_1 = FLAT_GROUND_Y + 12.0f + nY1;
            float y1_2 = FLAT_GROUND_Y + 12.0f + nY2;
            float y2_1 = FLAT_GROUND_Y + 42.0f + nY1 * 1.5f;
            float y2_2 = FLAT_GROUND_Y + 42.0f + nY2 * 1.5f;
            float y3_1 = FLAT_GROUND_Y + 150.0f + nY1 * 2.0f;
            float y3_2 = FLAT_GROUND_Y + 150.0f + nY2 * 2.0f;
            float yDeep = FLAT_GROUND_Y + DIRT_DEPTH;

            auto addQuad = [&](float tY1, float tY2, float bY1, float bY2, sf::Color tc1, sf::Color tc2, sf::Color bc1, sf::Color bc2) {
                undergroundMesh.append(sf::Vertex(sf::Vector2f(x1, tY1), tc1));
                undergroundMesh.append(sf::Vertex(sf::Vector2f(x2, tY2), tc2));
                undergroundMesh.append(sf::Vertex(sf::Vector2f(x1, bY1), bc1));

                undergroundMesh.append(sf::Vertex(sf::Vector2f(x2, tY2), tc2));
                undergroundMesh.append(sf::Vertex(sf::Vector2f(x2, bY2), bc2));
                undergroundMesh.append(sf::Vertex(sf::Vector2f(x1, bY1), bc1));
            };

            addQuad(y0_1, y0_2, y1_1, y1_2, cGround1, cGround2, cHumus1, cHumus2);
            addQuad(y1_1, y1_2, y2_1, y2_2, cHumus1, cHumus2, cUnder1, cUnder2);
            addQuad(y2_1, y2_2, y3_1, y3_2, cUnder1, cUnder2, cSub1, cSub2);
            addQuad(y3_1, y3_2, yDeep, yDeep, cSub1, cSub2, cDeep1, cDeep2);
            addQuad(yDeep, yDeep, yDeep + 300.0f, yDeep + 300.0f, cDeep1, cDeep2, cAbyss, cAbyss);
        }

        // --- Surface layer: grass blades + leaf litter ----------------------
        const float step = 4.0f;
        int segments = static_cast<int>(std::ceil(bounds.width / step));
        for (int i = 0; i < segments; ++i) {
            float xL = bounds.left + i * step;
            float xR = std::min(xL + step, bounds.left + bounds.width);
            float midX = (xL + xR) * 0.5f;

            EnvironmentProfile env = Biome::getBlendedEnvironment(midX);

            // Two independent per-column hashes: one decides whether anything
            // grows here, the other jitters height/offset. Using one for both
            // correlated "tall" with "present" and produced visible banding.
            uint32_t h = SeedManager::hashPosition(static_cast<int32_t>(std::floor(midX)), worldSeed ^ SALT_GRASS);
            float microHash  = static_cast<float>(h & 0xFFFFu) / 65535.0f;
            float microHash2 = static_cast<float>((h >> 16) & 0xFFFFu) / 65535.0f;

            // Large-scale patchiness so grass thickens and thins in drifts.
            float patch = SeedManager::fbm(midX, worldSeed ^ SALT_GRASS, 0.0018f, 3);
            float localDensity = std::clamp(env.grassDensity * (0.55f + 0.9f * patch), 0.0f, 1.0f);

            if (microHash <= localDensity) {
                float bladeHeight = env.grassHeightBase
                                  + patch * env.grassHeightVar
                                  + microHash2 * env.grassHeightVar * 0.5f;

                sf::Color tipColor = Biome::getBlendedGrassTipColor(midX, worldSeed);
                sf::Color baseColor = Biome::getBlendedGrassBaseColor(midX, worldSeed);

                // Occasional lit tips, strongest where undergrowth is dense.
                if (env.undergrowthDensity > 0.3f) {
                    float highlightMod = SeedManager::valueNoise(midX, worldSeed ^ SALT_GRASS, 0.012f);
                    if (highlightMod > 0.75f && microHash2 > 0.5f) {
                        tipColor.r = static_cast<sf::Uint8>(std::min(255, tipColor.r + 14));
                        tipColor.g = static_cast<sf::Uint8>(std::min(255, tipColor.g + 26));
                        tipColor.b = static_cast<sf::Uint8>(std::min(255, tipColor.b + 8));
                    }
                }

                float tipX = midX + (microHash2 - 0.5f) * (step * 0.6f);
                float groundBaseY = FLAT_GROUND_Y - 4.0f;

                terrainMesh.append(sf::Vertex(sf::Vector2f(tipX, groundBaseY - bladeHeight), tipColor));
                terrainMesh.append(sf::Vertex(sf::Vector2f(xR, groundBaseY), baseColor));
                terrainMesh.append(sf::Vertex(sf::Vector2f(xL, groundBaseY), baseColor));
            }

            // Leaf litter: tiny flat flecks lying on the soil. Cheap (2 tris
            // worth of verts at most) and gives the jungle floor depth without
            // covering everything in sprite noise.
            if (env.litterDensity > 0.02f && microHash2 < env.litterDensity * 0.10f) {
                float groundBaseY = FLAT_GROUND_Y - 4.0f;
                float fleckW = 2.0f + microHash * 3.0f;
                float fleckY = groundBaseY + 1.0f + microHash * 2.0f;

                sf::Color litter = Biome::getBlendedUndergroundColor(midX, worldSeed);
                litter.r = static_cast<sf::Uint8>(std::min(255, litter.r + 26));
                litter.g = static_cast<sf::Uint8>(std::min(255, litter.g + 18));

                terrainMesh.append(sf::Vertex(sf::Vector2f(midX - fleckW, fleckY), litter));
                terrainMesh.append(sf::Vertex(sf::Vector2f(midX + fleckW, fleckY), litter));
                terrainMesh.append(sf::Vertex(sf::Vector2f(midX, fleckY + 1.6f), litter));
            }
        }

        trees.reserve(40);
        std::vector<Tree> candidateTrees = WorldGenerator::generateTrees(bounds.left, bounds.width, chunkSeed, worldSeed, props, decorTex);
        for (auto& tree : candidateTrees) {
            trees.push_back(std::move(tree));
        }

        decorations.reserve(96);
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