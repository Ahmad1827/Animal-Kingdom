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
    // Player village center at X = 1000
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

    // 1. BASE CLEARANCE: [centerX - 350, centerX + 350] -> zero trees inside the settlement
    for (const auto& s : settlements) {
        zones.push_back({s.centerX - 350.0f, s.centerX + 350.0f, ClearanceType::Base, "VillageBase"});
    }

    // 2. MEETING GROUND CLEARANCE: Exact midpoint between neighboring borders
    for (size_t i = 0; i + 1 < settlements.size(); ++i) {
        float rightBorderA = settlements[i].borderMaxX;
        float leftBorderB = settlements[i + 1].borderMinX;

        if (rightBorderA < leftBorderB) {
            float midX = (rightBorderA + leftBorderB) * 0.5f;
            zones.push_back({midX - 140.0f, midX + 140.0f, ClearanceType::MeetingGround, "MeetingGround"});
        } else {
            float midX = (settlements[i].centerX + settlements[i + 1].centerX) * 0.5f;
            zones.push_back({midX - 140.0f, midX + 140.0f, ClearanceType::MeetingGround, "MeetingGround"});
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
    result.reserve(60);

    float currentX = startX + 15.f;
    float endX = startX + width - 15.f;
    int treeCounter = 1;

    while (currentX < endX) {
        if (isPositionClear(currentX, worldSeed)) {
            currentX += 25.f;
            continue;
        }

        BiomeType region = Biome::determineRegionAtWorldX(currentX, worldSeed);
        BiomeProperties bProps = Biome::getBlendedProperties(currentX, worldSeed);

        int gridCoord = static_cast<int>(std::floor(currentX / 20.f));
        uint32_t pointHash = hashCoord(worldSeed, gridCoord);

        float minSpacing = 80.f;
        float maxSpacing = 135.f;
        float scaleMin = 0.95f;
        float scaleMax = 1.55f;
        float baseWidth = 85.f;

        if (region == BiomeType::Field) {
            minSpacing = 450.f;
            maxSpacing = 850.f;
            scaleMin = 0.70f;
            scaleMax = 1.00f;
            baseWidth = 55.f;
        } else if (region == BiomeType::Desert) {
            minSpacing = 1000.f;
            maxSpacing = 2000.f;
            scaleMin = 0.55f;
            scaleMax = 0.80f;
            baseWidth = 45.f;
        } else if (region == BiomeType::Hills || region == BiomeType::Mountain) {
            minSpacing = 150.f;
            maxSpacing = 300.f;
            scaleMin = 0.80f;
            scaleMax = 1.20f;
            baseWidth = 65.f;
        }

        float jitter = static_cast<float>((pointHash % 17) - 8);
        float treeX = currentX + jitter;

        if (isPositionClear(treeX, worldSeed)) {
            currentX += 25.f;
            continue;
        }

        float scaleRatio = static_cast<float>((pointHash >> 4) % 100) / 100.f;
        float scale = scaleMin + scaleRatio * (scaleMax - scaleMin);
        float tWidth = baseWidth * scale;
        float tHeight = 280.f * scale;
        sf::Color tColor(95, 62, 30);
        float yOff = static_cast<float>((pointHash % 7) - 3);
        int uniqueTreeId = static_cast<int>(std::abs(std::round(treeX))) * 100 + (treeCounter++);

        result.emplace_back(treeX, FLAT_GROUND_Y + yOff, tWidth, tHeight, tColor, decorTex, uniqueTreeId);

        float spacingRange = maxSpacing - minSpacing;
        float chosenSpacing = minSpacing + (static_cast<float>(pointHash % 100) / 100.f) * spacingRange;

        currentX = treeX + chosenSpacing;
    }

    return result;
}

std::vector<Decoration> WorldGenerator::generateDecorations(float startX, float width, uint32_t chunkSeed, uint32_t worldSeed, const BiomeProperties& props, sf::Texture& decorTex) {
    std::vector<Decoration> decors;
    decors.reserve(70);

    BiomeProperties bProps = Biome::getBlendedProperties(startX + width * 0.5f, worldSeed);
    int count = bProps.decorationDensity + static_cast<int>(chunkSeed % 12);
    float step = width / static_cast<float>(std::max(1, count));

    for (int i = 0; i < count; ++i) {
        uint32_t itemSeed = chunkSeed + i * 47 + (worldSeed % 256);
        itemSeed = (itemSeed ^ (itemSeed >> 13)) * 0x5bd1e995;

        float x = startX + (i * step) + static_cast<float>(itemSeed % static_cast<int>(step * 0.85f + 1.f));
        if (isPositionClear(x, worldSeed)) continue;

        int type = 0;
        int roll = itemSeed % 100;

        if (bProps.type == BiomeType::Jungle) {
            type = (roll < 70) ? 2 : 0;
        } else if (bProps.type == BiomeType::Field) {
            type = (roll < 75) ? 1 : 0;
        } else if (bProps.type == BiomeType::Desert) {
            type = (roll < 60) ? 3 : 4;
        } else {
            type = (roll < 60) ? bProps.primaryDecorType : bProps.secondaryDecorType;
        }

        decors.emplace_back(x, FLAT_GROUND_Y, type, itemSeed, decorTex);
    }

    return decors;
}