#include "world/WorldGenerator.h"
#include "world/SeedManager.h"
#include <cmath>
#include <algorithm>
#include <iostream>

static constexpr float FLAT_GROUND_Y = 500.0f;

enum class TreeCategory {
    Young,
    Mature,
    Large,
    Ancient
};

static uint32_t hashCoord(uint32_t worldSeed, int gridPos, uint32_t salt = 0) {
    uint32_t h = worldSeed ^ static_cast<uint32_t>(gridPos * 73856093) ^ (salt * 19349663);
    h = (h ^ (h >> 13)) * 0x5bd1e995;
    h = (h ^ (h >> 15)) * 0x1b873593;
    h = h ^ (h >> 16);
    return h;
}

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
    result.reserve(60);

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

        int gridCoord = static_cast<int>(std::floor(currentX / 15.0f));
        uint32_t seed = hashCoord(worldSeed, gridCoord, treeCounter);

        TreeCategory cat;
        int catRoll = seed % 100;
        bool isLandmark = (std::abs(std::fmod(currentX, 4000.0f)) < 180.0f);

        if (isLandmark || catRoll < 4) {
            cat = TreeCategory::Ancient;
        } else if (catRoll < 26) {
            cat = TreeCategory::Large;
        } else if (catRoll < 76) {
            cat = TreeCategory::Mature;
        } else {
            cat = TreeCategory::Young;
        }

        float scale = 1.0f;
        float tWidth = 85.0f;
        float tHeight = 280.0f;
        float canopyRadius = 100.0f;
        float canopyHeight = 180.0f;
        int clusters = 3;
        int branchCount = 2;
        int vineCount = 0;

        switch (cat) {
            case TreeCategory::Young: {
                scale = 0.70f + ((seed >> 4) % 15) * 0.01f;
                tWidth = 58.0f * scale;
                tHeight = 200.0f * scale;
                canopyRadius = 75.0f * scale;
                canopyHeight = 130.0f * scale;
                clusters = 2;
                branchCount = 1;
                vineCount = 0;
                break;
            }
            case TreeCategory::Mature: {
                scale = 0.90f + ((seed >> 4) % 25) * 0.01f;
                tWidth = 85.0f * scale;
                tHeight = 270.0f * scale;
                canopyRadius = 100.0f * scale;
                canopyHeight = 175.0f * scale;
                clusters = 3;
                branchCount = 2;
                vineCount = ((seed >> 8) % 10 < 3) ? 1 : 0;
                break;
            }
            case TreeCategory::Large: {
                scale = 1.20f + ((seed >> 4) % 25) * 0.01f;
                tWidth = 110.0f * scale;
                tHeight = 330.0f * scale;
                canopyRadius = 130.0f * scale;
                canopyHeight = 220.0f * scale;
                clusters = 4;
                branchCount = 3;
                vineCount = 1 + ((seed >> 8) % 2);
                break;
            }
            case TreeCategory::Ancient: {
                scale = 1.55f + ((seed >> 4) % 30) * 0.01f;
                tWidth = 140.0f * scale;
                tHeight = 400.0f * scale;
                canopyRadius = 170.0f * scale;
                canopyHeight = 270.0f * scale;
                clusters = 5;
                branchCount = 4;
                vineCount = 2 + ((seed >> 8) % 2);
                break;
            }
        }

        sf::Color tColor(95, 62, 30);
        int uniqueTreeId = static_cast<int>(std::abs(currentX)) * 100 + (treeCounter++);

        Tree newTree(currentX, FLAT_GROUND_Y - 3.0f, tWidth, tHeight, tColor, decorTex, uniqueTreeId);

        uint32_t detailSeed = uniqueTreeId;
        newTree.buildCanopy(detailSeed, canopyRadius, canopyHeight, sf::Color(40, 140, 45), clusters);

        for (int b = 0; b < branchCount; ++b) {
            float branchY = (70.0f + b * 55.0f) * scale;
            bool rightSide = (b % 2 == 0);
            newTree.addBranch(branchY, 40.0f * scale, rightSide, sf::Color(70, 100, 50), decorTex);
        }

        if (trans.jungleWeight > 0.30f) {
            for (int v = 0; v < vineCount; ++v) {
                float vSide = (v % 2 == 0) ? -1.0f : 1.0f;
                float vXOff = vSide * (tWidth * (0.25f + v * 0.08f));
                float vYOff = tHeight * (0.45f + v * 0.10f);
                float vLen = (80.0f + ((seed + v * 23) % 40)) * scale;
                newTree.addVine(vXOff, vYOff, vLen);
            }
        }

        newTree.initDynamicMesh();
        result.push_back(std::move(newTree));

        // Weighted spacing distribution
        int spacingRoll = (seed >> 8) % 100;
        const char* categoryName = "NORMAL";
        float minRange = 90.0f;
        float maxRange = 140.0f;

        if (spacingRoll < 38) {
            categoryName = "VERY_CLOSE";
            minRange = 55.0f;
            maxRange = 90.0f;
        } else if (spacingRoll < 75) {
            categoryName = "NORMAL";
            minRange = 90.0f;
            maxRange = 140.0f;
        } else if (spacingRoll < 93) {
            categoryName = "MODERATE_GAP";
            minRange = 140.0f;
            maxRange = 210.0f;
        } else {
            categoryName = "LARGE_GAP";
            minRange = 210.0f;
            maxRange = 310.0f;
        }

        float frac = static_cast<float>((seed >> 16) % 1000) / 1000.0f;
        float chosenSpacing = minRange + frac * (maxRange - minRange);

        if (cat == TreeCategory::Young) chosenSpacing -= 15.0f;
        else if (cat == TreeCategory::Large) chosenSpacing += 20.0f;
        else if (cat == TreeCategory::Ancient) chosenSpacing += 45.0f;

        chosenSpacing /= std::clamp(trans.jungleWeight, 0.12f, 1.0f);
        chosenSpacing = std::max(45.0f * scale, chosenSpacing);

        std::cout << "[JUNGLE SPACING]\n"
                  << "PreviousTree=" << uniqueTreeId << "\n"
                  << "Category=" << categoryName << "\n"
                  << "Distance=" << static_cast<int>(chosenSpacing) << "\n\n";

        currentX += chosenSpacing;
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
            step = 70.0f + (itemSeed % 40);
        }

        decors.emplace_back(currentX, FLAT_GROUND_Y, type, itemSeed, decorTex);

        currentX += step;
        decorIndex++;
    }

    return decors;
}