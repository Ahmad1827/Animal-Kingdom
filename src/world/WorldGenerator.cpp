#include "world/WorldGenerator.h"
#include "world/SeedManager.h"
#include <cmath>
#include <algorithm>
#include <iostream>

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
        float cX = offset * 2000.0f + 1000.0f;
        settlements.push_back({cX, cX - 2500.0f, cX + 2500.0f});
    }

    std::sort(settlements.begin(), settlements.end(), [](const auto& a, const auto& b) {
        return a.centerX < b.centerX;
    });

    for (const auto& s : settlements) {
        zones.push_back({s.centerX - 350.0f, s.centerX + 350.0f, ClearanceType::Base, "VillageBase"});
    }

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
        if (worldX >= z.minX && worldX <= z.maxX) return true;
    }
    return false;
}

std::vector<Tree> WorldGenerator::generateTrees(float startX, float width, uint32_t, uint32_t worldSeed, const BiomeProperties&, sf::Texture& decorTex) {
    std::vector<Tree> result;
    result.reserve(40);

    float currentX = startX + 30.0f;
    float endX = startX + width;
    int treeCounter = 1;

    while (currentX < endX) {
        if (isPositionClear(currentX, worldSeed)) {
            currentX += 30.0f;
            continue;
        }

        BiomeTransitionInfo trans = Biome::getTransitionInfo(currentX);

        if (trans.jungleWeight < 0.10f) {
            currentX += 150.0f;
            continue;
        }

        float baseSpacing = 135.0f;
        float currentSpacing = baseSpacing / std::clamp(trans.jungleWeight, 0.12f, 1.0f);

        float tWidth = 85.0f;
        float tHeight = 280.0f;
        sf::Color tColor(95, 62, 30);
        int uniqueTreeId = static_cast<int>(std::abs(currentX)) * 100 + (treeCounter++);

        Tree newTree(currentX, FLAT_GROUND_Y - 3.0f, tWidth, tHeight, tColor, decorTex, uniqueTreeId);

        uint32_t detailSeed = uniqueTreeId;
        newTree.buildCanopy(detailSeed, 100.0f, 180.0f, sf::Color(40, 140, 45), 3);
        newTree.addBranch(90.0f, 40.0f, true, sf::Color(70, 100, 50), decorTex);
        newTree.addBranch(150.0f, 40.0f, false, sf::Color(70, 100, 50), decorTex);

        if (trans.jungleWeight > 0.40f) {
            newTree.addVine(tWidth * 0.3f, tHeight * 0.5f, 120.0f);
        }

        newTree.initDynamicMesh();

        result.push_back(std::move(newTree));

        currentX += currentSpacing;
    }

    return result;
}

std::vector<Decoration> WorldGenerator::generateDecorations(float startX, float width, uint32_t chunkSeed, uint32_t worldSeed, const BiomeProperties&, sf::Texture& decorTex) {
    std::vector<Decoration> decors;
    decors.reserve(70);

    float currentX = startX + 25.0f;
    float endX = startX + width;
    int decorIndex = 0;

    while (currentX < endX) {
        if (isPositionClear(currentX, worldSeed)) {
            currentX += 40.0f;
            continue;
        }

        BiomeTransitionInfo trans = Biome::getTransitionInfo(currentX);

        uint32_t itemSeed = chunkSeed + decorIndex * 53 + static_cast<int>(std::abs(currentX));
        itemSeed = (itemSeed ^ (itemSeed >> 13)) * 0x5bd1e995;
        float roll = static_cast<float>(itemSeed % 1000) / 1000.0f;

        int type = 0;
        if (roll < trans.jungleWeight) {
            type = 2;
        } else if (roll < trans.jungleWeight + trans.fieldWeight) {
            type = ((itemSeed >> 4) % 100 < 70) ? 1 : 0;
        } else {
            type = ((itemSeed >> 4) % 100 < 60) ? 3 : 4;
        }

        float step = 100.0f;
        if (trans.desertWeight > 0.70f) {
            step = 220.0f;
        } else if (trans.fieldWeight > 0.60f) {
            step = 75.0f;
        } else if (trans.jungleWeight > 0.60f) {
            step = 90.0f;
        }

        decors.emplace_back(currentX, FLAT_GROUND_Y, type, itemSeed, decorTex);

        currentX += step;
        decorIndex++;
    }

    return decors;
}