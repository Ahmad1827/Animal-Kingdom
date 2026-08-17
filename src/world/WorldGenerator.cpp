#include "world/WorldGenerator.h"
#include "world/SeedManager.h"
#include <cmath>
#include <algorithm>

static constexpr float FLAT_GROUND_Y = 500.0f;

std::vector<WorldClearanceZone> WorldGenerator::getClearanceZones(uint32_t worldSeed) {
    std::vector<WorldClearanceZone> zones;

    struct SettlementBounds {
        float centerX;
        float borderMinX;
        float borderMaxX;
    };

    std::vector<SettlementBounds> settlements;
    settlements.push_back({1000.0f, 1000.0f - 3000.0f, 1000.0f + 3000.0f});

    uint32_t popSeed = worldSeed;
    int numVillages = 3 + (popSeed % 3);

    for (int v = 0; v < numVillages; ++v) {
        int offset = (v % 2 == 0 ? 1 : -1) * (v + 1) * 3;
        float cX = offset * 2000.f + 1000.f;
        settlements.push_back({cX, cX - 2500.0f, cX + 2500.0f});
    }

    std::sort(settlements.begin(), settlements.end(), [](const auto& a, const auto& b) {
        return a.centerX < b.centerX;
    });

    for (const auto& s : settlements) {
        zones.push_back({s.centerX - 460.0f, s.centerX + 460.0f, ClearanceType::Base, "VillageBase"});
    }

    for (size_t i = 0; i + 1 < settlements.size(); ++i) {
        float rightBorderA = settlements[i].borderMaxX;
        float leftBorderB = settlements[i + 1].borderMinX;

        if (rightBorderA < leftBorderB) {
            float midX = (rightBorderA + leftBorderB) * 0.5f;
            zones.push_back({midX - 180.0f, midX + 180.0f, ClearanceType::MeetingGround, "MeetingGround"});
        } else {
            float midX = (settlements[i].centerX + settlements[i + 1].centerX) * 0.5f;
            zones.push_back({midX - 180.0f, midX + 180.0f, ClearanceType::MeetingGround, "MeetingGround"});
        }
    }

    return zones;
}

bool WorldGenerator::isPositionClear(float worldX, uint32_t worldSeed) {
    auto zones = getClearanceZones(worldSeed);
    for (const auto& z : zones) {
        if (worldX >= z.minX && worldX <= z.maxX) {
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
    result.reserve(45);

    float currentX = startX + 25.f;
    float endX = startX + width - 25.f;
    int treeCounter = 1;

    while (currentX < endX) {
        if (isPositionClear(currentX, worldSeed)) {
            currentX += 50.f;
            continue;
        }

        BiomeProperties bProps = Biome::getBlendedProperties(currentX, worldSeed);
        int gridCoord = static_cast<int>(std::floor(currentX / 25.f));
        uint32_t pointHash = hashCoord(worldSeed, gridCoord);

        float spawnRoll = static_cast<float>(pointHash % 1000) / 1000.f;

        if (spawnRoll > bProps.treeDensity) {
            currentX += 45.f;
            continue;
        }

        float jitter = static_cast<float>((pointHash % 25) - 12);
        float finalX = currentX + jitter;

        if (isPositionClear(finalX, worldSeed)) {
            currentX += 50.f;
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

        float spacingRange = bProps.maxTreeSpacing - bProps.minTreeSpacing;
        float chosenSpacing = bProps.minTreeSpacing + (static_cast<float>(pointHash % 100) / 100.f) * spacingRange;

        bool formCluster = ((pointHash >> 4) % 100 < static_cast<int>(bProps.clusterProbability * 100.f));
        if (formCluster) {
            chosenSpacing *= 0.65f;
        }

        currentX = finalX + chosenSpacing;
    }

    return result;
}

std::vector<Decoration> WorldGenerator::generateDecorations(float startX, float width, uint32_t chunkSeed, uint32_t worldSeed, const BiomeProperties& props, sf::Texture& decorTex) {
    std::vector<Decoration> decors;
    decors.reserve(45);

    BiomeProperties bProps = Biome::getBlendedProperties(startX + width * 0.5f, worldSeed);

    int count = bProps.decorationDensity + static_cast<int>(chunkSeed % 7);
    float step = width / static_cast<float>(std::max(1, count));

    for (int i = 0; i < count; ++i) {
        uint32_t itemSeed = chunkSeed + i * 47 + (worldSeed % 256);
        itemSeed = (itemSeed ^ (itemSeed >> 13)) * 0x5bd1e995;

        float x = startX + (i * step) + static_cast<float>(itemSeed % static_cast<int>(step * 0.85f + 1.f));
        float y = FLAT_GROUND_Y;

        int type = 0;
        int roll = itemSeed % 100;

        if (roll < 60) {
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