#include "world/WorldGenerator.h"
#include "world/SeedManager.h"
#include <cmath>
#include <algorithm>
#include <iostream>

static constexpr float FLAT_GROUND_Y = 500.0f;

static std::vector<float> getDeterministicSettlementCenters(uint32_t worldSeed) {
    std::vector<float> centers;
    centers.push_back(1000.0f);

    uint32_t popSeed = worldSeed;
    int numVillages = 3 + (popSeed % 3);

    for (int v = 0; v < numVillages; ++v) {
        int offset = (v % 2 == 0 ? 1 : -1) * (v + 1) * 3;
        float cX = offset * 2000.f + 1000.f;
        centers.push_back(cX);
    }
    return centers;
}

bool WorldGenerator::isSettlementCore(float worldX, uint32_t worldSeed) {
    auto centers = getDeterministicSettlementCenters(worldSeed);
    for (float cX : centers) {
        if (std::abs(worldX - cX) < 160.0f) {
            return true;
        }
    }
    return false;
}

bool WorldGenerator::isMeetingGroundClearing(float worldX, uint32_t worldSeed) {
    auto centers = getDeterministicSettlementCenters(worldSeed);
    std::sort(centers.begin(), centers.end());

    for (size_t i = 0; i + 1 < centers.size(); ++i) {
        float midX = (centers[i] + centers[i + 1]) * 0.5f;
        if (std::abs(worldX - midX) < 80.0f) {
            return true;
        }
    }
    return false;
}

static uint32_t hashCoord(uint32_t worldSeed, int gridPos) {
    uint32_t h = worldSeed ^ static_cast<uint32_t>(gridPos * 73856093);
    h = (h ^ (h >> 13)) * 0x5bd1e995;
    h = (h ^ (h >> 15)) * 0x1b873593;
    h = h ^ (h >> 16);
    return h;
}

std::vector<Tree> WorldGenerator::generateTrees(float startX, float width, uint32_t chunkSeed, uint32_t worldSeed, const BiomeProperties& props, sf::Texture& decorTex) {
    std::vector<Tree> result;
    result.reserve(40);

    float currentX = startX + 30.f;
    float endX = startX + width - 30.f;
    int treeCounter = 1;

    int chunkIndex = static_cast<int>(std::floor((startX + width * 0.5f) / 2000.f));
    RegionType centerRegion = Biome::determineRegionAtWorldX(startX + width * 0.5f, worldSeed);
    BiomeProperties centerProps = Biome::getProperties(centerRegion);

    int totalCandidates = 0;
    int acceptedCount = 0;
    int rejectedSettlement = 0;

    while (currentX < endX) {
        totalCandidates++;

        RegionType region = Biome::determineRegionAtWorldX(currentX, worldSeed);
        BiomeProperties bProps = Biome::getProperties(region);

        if (isSettlementCore(currentX, worldSeed)) {
            rejectedSettlement++;
            std::cout << "[TREE SKIPPED] Reason=InsideSettlementCore | WorldX=" << currentX << std::endl;
            currentX += 80.f;
            continue;
        }

        if (isMeetingGroundClearing(currentX, worldSeed)) {
            rejectedSettlement++;
            std::cout << "[TREE SKIPPED] Reason=InsideMeetingGround | WorldX=" << currentX << std::endl;
            currentX += 80.f;
            continue;
        }

        int gridCoord = static_cast<int>(std::floor(currentX / 25.f));
        uint32_t pointHash = hashCoord(worldSeed, gridCoord);

        float jitter = static_cast<float>((pointHash % 25) - 12);
        float finalX = currentX + jitter;

        if (isSettlementCore(finalX, worldSeed) || isMeetingGroundClearing(finalX, worldSeed)) {
            rejectedSettlement++;
            currentX += 70.f;
            continue;
        }

        float scaleRatio = static_cast<float>((pointHash >> 8) % 100) / 100.f;
        float scale = bProps.treeScaleMin + scaleRatio * (bProps.treeScaleMax - bProps.treeScaleMin);

        float treeWidth = bProps.treeWidthBase * scale;
        float treeHeight = 280.f * scale;
        sf::Color trunkColor(95, 62, 30);

        float yOffset = static_cast<float>((pointHash % 7) - 3);
        int uniqueTreeId = static_cast<int>(std::abs(std::round(finalX))) * 100 + (treeCounter++);

        result.emplace_back(finalX, FLAT_GROUND_Y + yOffset, treeWidth, treeHeight, trunkColor, decorTex, uniqueTreeId);
        acceptedCount++;

        std::cout << "[TREE GEN] Biome=" << bProps.name 
                  << " | TreeSpawned=true | TreeID=" << uniqueTreeId 
                  << " | TreeX=" << finalX << " | TreeY=" << (FLAT_GROUND_Y + yOffset) 
                  << std::endl;

        float spacingRange = bProps.maxTreeSpacing - bProps.minTreeSpacing;
        float chosenSpacing = bProps.minTreeSpacing + (static_cast<float>(pointHash % 100) / 100.f) * spacingRange;

        currentX = finalX + chosenSpacing;
    }

    std::cout << "[CHUNK " << chunkIndex << " TREE STORAGE] Biome=" << centerProps.name
              << " | TotalCandidates=" << totalCandidates
              << " | StoredTrees=" << acceptedCount
              << " | RejectedSettlement=" << rejectedSettlement
              << std::endl << std::flush;

    return result;
}

std::vector<Decoration> WorldGenerator::generateDecorations(float startX, float width, uint32_t chunkSeed, uint32_t worldSeed, const BiomeProperties& props, sf::Texture& decorTex) {
    std::vector<Decoration> decors;
    decors.reserve(45);

    RegionType region = Biome::determineRegionAtWorldX(startX + width * 0.5f, worldSeed);
    BiomeProperties bProps = Biome::getProperties(region);

    int count = bProps.decorationDensity + static_cast<int>(chunkSeed % 7);
    float step = width / static_cast<float>(std::max(1, count));

    for (int i = 0; i < count; ++i) {
        uint32_t itemSeed = chunkSeed + i * 47 + (worldSeed % 256);
        itemSeed = (itemSeed ^ (itemSeed >> 13)) * 0x5bd1e995;

        float x = startX + (i * step) + static_cast<float>(itemSeed % static_cast<int>(step * 0.85f + 1.f));
        float y = FLAT_GROUND_Y;

        if (isSettlementCore(x, worldSeed)) {
            continue;
        }

        int type = 0;
        int roll = itemSeed % 100;

        if (isMeetingGroundClearing(x, worldSeed)) {
            type = (roll < 60) ? 3 : 0;
        } else if (roll < 60) {
            type = bProps.primaryDecorType;
        } else if (roll < 90) {
            type = bProps.secondaryDecorType;
        } else {
            type = (itemSeed >> 4) % 5;
        }

        decors.emplace_back(x, y, type, itemSeed, decorTex);
    }

    return decors;
}