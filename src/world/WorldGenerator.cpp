#include "world/WorldGenerator.h"
#include "world/SeedManager.h"
#include <random>
#include <cmath>
#include <algorithm>

static constexpr float FLAT_GROUND_Y = 500.0f;

static bool isInsideSettlementClearing(float x) {
    if (std::abs(x - 1000.0f) < 520.0f) {
        return true;
    }
    for (int i = 1; i <= 6; ++i) {
        float rivalPos1 = (i * 3 * 2000.f) + 1000.f;
        float rivalPos2 = (-i * 3 * 2000.f) + 1000.f;
        if (std::abs(x - rivalPos1) < 520.0f || std::abs(x - rivalPos2) < 520.0f) {
            return true;
        }
    }
    return false;
}

static float getContinuousDensityNoise(float x, uint32_t seed) {
    float s = static_cast<float>(seed % 5000);
    float n1 = std::sin(x * 0.0012f + s) * 0.5f;
    float n2 = std::sin(x * 0.0035f + s * 1.7f) * 0.3f;
    float n3 = std::cos(x * 0.008f + s * 0.4f) * 0.2f;
    return (n1 + n2 + n3 + 1.0f) * 0.5f;
}

std::vector<Tree> WorldGenerator::generateTrees(float startX, float width, uint32_t seed, uint32_t worldSeed, const BiomeProperties& props, sf::Texture& decorTex) {
    std::vector<Tree> result;
    result.reserve(30);

    float currentX = startX;
    float endX = startX + width;
    int treeCounter = 1;

    float stepSize = 45.f;
    float nextAllowedX = startX;

    for (float sampleX = startX; sampleX < endX; sampleX += stepSize) {
        if (sampleX < nextAllowedX) continue;
        if (isInsideSettlementClearing(sampleX)) {
            nextAllowedX = sampleX + 80.f;
            continue;
        }

        uint32_t pointSeed = seed ^ static_cast<uint32_t>(std::abs(std::floor(sampleX)));
        pointSeed = (pointSeed ^ (pointSeed >> 16)) * 0x45d9f3b;
        pointSeed = (pointSeed ^ (pointSeed >> 16)) * 0x45d9f3b;
        pointSeed = pointSeed ^ (pointSeed >> 16);

        float density = getContinuousDensityNoise(sampleX, worldSeed);
        float localRoll = static_cast<float>(pointSeed % 1000) / 1000.f;

        float spawnThreshold = 0.55f;
        float baseSpacing = 220.f;

        RegionType currentRegion = Biome::determineRegion(static_cast<int>(std::floor(sampleX / 2000.f)), worldSeed);
        BiomeProperties localProps = Biome::getProperties(currentRegion);

        switch (currentRegion) {
            case RegionType::DenseJungle:
                spawnThreshold = 0.32f;
                baseSpacing = 110.f;
                break;
            case RegionType::OldGrowth:
                spawnThreshold = 0.65f;
                baseSpacing = 340.f;
                break;
            case RegionType::YoungForest:
                spawnThreshold = 0.50f;
                baseSpacing = 190.f;
                break;
            case RegionType::Clearing:
                spawnThreshold = 0.92f;
                baseSpacing = 700.f;
                break;
            case RegionType::RockyArea:
                spawnThreshold = 0.78f;
                baseSpacing = 450.f;
                break;
        }

        if (density < 0.25f) {
            spawnThreshold += 0.35f;
            baseSpacing += 300.f;
        } else if (density > 0.75f) {
            spawnThreshold -= 0.18f;
            baseSpacing *= 0.7f;
        }

        if (localRoll > (1.0f - spawnThreshold)) {
            float jitter = ((pointSeed % 41) - 20.f);
            float finalX = sampleX + jitter;

            if (isInsideSettlementClearing(finalX)) continue;

            float sizeRoll = ((pointSeed >> 8) % 100) / 100.f;
            float scale = (0.7f + sizeRoll * 0.6f) * localProps.treeSizeMultiplier;

            if (currentRegion == RegionType::OldGrowth && (pointSeed % 100) < 35) {
                scale *= 1.35f;
            }

            float treeWidth = localProps.treeWidthBase * scale;
            float treeHeight = 280.f * scale;
            sf::Color trunkColor(101, 67, 33);

            float yOffset = static_cast<float>((pointSeed % 7) - 3);
            int uniqueTreeId = static_cast<int>(std::abs(std::round(finalX))) * 100 + (treeCounter++);

            result.emplace_back(finalX, FLAT_GROUND_Y + yOffset, treeWidth, treeHeight, trunkColor, decorTex, uniqueTreeId);

            float clusterBonus = ((pointSeed >> 4) % 100 < 30 && currentRegion == RegionType::DenseJungle) ? 0.55f : 1.0f;
            nextAllowedX = finalX + (baseSpacing * clusterBonus) + (pointSeed % 60);
        }
    }

    return result;
}

std::vector<Decoration> WorldGenerator::generateDecorations(float startX, float width, uint32_t chunkSeed, uint32_t worldSeed, const BiomeProperties& props, sf::Texture& decorTex) {
    std::vector<Decoration> decors;
    decors.reserve(40);

    RegionType currentRegion = Biome::determineRegion(static_cast<int>(std::floor(startX / 2000.f)), worldSeed);
    BiomeProperties localProps = Biome::getProperties(currentRegion);

    int count = localProps.decorationDensity + static_cast<int>((chunkSeed % 9));
    float step = width / static_cast<float>(count);

    for (int i = 0; i < count; ++i) {
        uint32_t itemSeed = chunkSeed + i * 37 + (worldSeed % 100);
        itemSeed = (itemSeed ^ (itemSeed >> 13)) * 0x5bd1e995;

        float x = startX + (i * step) + static_cast<float>(itemSeed % static_cast<int>(step * 0.8f + 1.f));
        float y = FLAT_GROUND_Y;

        if (isInsideSettlementClearing(x)) {
            continue;
        }

        int type = 0;
        int roll = itemSeed % 100;

        if (currentRegion == RegionType::RockyArea) {
            type = (roll < 70) ? 3 : 0;
        } else if (currentRegion == RegionType::Clearing) {
            type = (roll < 45) ? 1 : ((roll < 75) ? 0 : 3);
        } else if (currentRegion == RegionType::DenseJungle) {
            type = (roll < 55) ? 2 : ((roll < 85) ? 0 : 4);
        } else if (currentRegion == RegionType::OldGrowth) {
            type = (roll < 50) ? 4 : ((roll < 80) ? 2 : 3);
        } else {
            type = (roll < 60) ? 0 : ((roll < 85) ? 2 : 3);
        }

        decors.emplace_back(x, y, type, itemSeed, decorTex);
    }

    return decors;
}