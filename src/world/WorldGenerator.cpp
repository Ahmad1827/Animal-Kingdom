#include "world/WorldGenerator.h"
#include "world/SeedManager.h"
#include <cmath>
#include <algorithm>
#include <iostream>

static constexpr float FLAT_GROUND_Y = 500.0f;
static constexpr float MIN_APE_BODY_GAP = 42.0f;

enum class TreeCategory {
    Young,
    Mature,
    Large,
    Ancient
};

struct TreeArchetype {
    float scale;
    float trunkWidth;
    float trunkHeight;
    float canopyRadius;
    float canopyHeight;
    int clusters;
    int branchCount;
    int vineCount;
};

static uint32_t hashCoord(uint32_t worldSeed, int gridPos, uint32_t salt = 0) {
    uint32_t h = worldSeed ^ static_cast<uint32_t>(gridPos * 73856093) ^ (salt * 19349663);
    h = (h ^ (h >> 13)) * 0x5bd1e995;
    h = (h ^ (h >> 15)) * 0x1b873593;
    h = h ^ (h >> 16);
    return h;
}

static TreeArchetype getTreeArchetype(TreeCategory cat, uint32_t seed) {
    TreeArchetype arch;
    switch (cat) {
        case TreeCategory::Young: {
            arch.scale = 0.70f + ((seed >> 4) % 15) * 0.01f;
            arch.trunkWidth = 58.0f * arch.scale;
            arch.trunkHeight = 200.0f * arch.scale;
            arch.canopyRadius = 75.0f * arch.scale;
            arch.canopyHeight = 130.0f * arch.scale;
            arch.clusters = 2;
            arch.branchCount = 1;
            arch.vineCount = 0;
            break;
        }
        case TreeCategory::Mature: {
            arch.scale = 0.90f + ((seed >> 4) % 25) * 0.01f;
            arch.trunkWidth = 85.0f * arch.scale;
            arch.trunkHeight = 270.0f * arch.scale;
            arch.canopyRadius = 100.0f * arch.scale;
            arch.canopyHeight = 175.0f * arch.scale;
            arch.clusters = 3;
            arch.branchCount = 2;
            arch.vineCount = ((seed >> 8) % 10 < 3) ? 1 : 0;
            break;
        }
        case TreeCategory::Large: {
            arch.scale = 1.20f + ((seed >> 4) % 25) * 0.01f;
            arch.trunkWidth = 110.0f * arch.scale;
            arch.trunkHeight = 330.0f * arch.scale;
            arch.canopyRadius = 130.0f * arch.scale;
            arch.canopyHeight = 220.0f * arch.scale;
            arch.clusters = 4;
            arch.branchCount = 3;
            arch.vineCount = 1 + ((seed >> 8) % 2);
            break;
        }
        case TreeCategory::Ancient: {
            arch.scale = 1.55f + ((seed >> 4) % 30) * 0.01f;
            arch.trunkWidth = 140.0f * arch.scale;
            arch.trunkHeight = 400.0f * arch.scale;
            arch.canopyRadius = 170.0f * arch.scale;
            arch.canopyHeight = 270.0f * arch.scale;
            arch.clusters = 5;
            arch.branchCount = 4;
            arch.vineCount = 2 + ((seed >> 8) % 2);
            break;
        }
    }
    return arch;
}

static TreeCategory determineCategory(uint32_t seed, float worldX) {
    int catRoll = seed % 100;
    bool isLandmark = (std::abs(std::fmod(worldX, 4000.0f)) < 180.0f);

    if (isLandmark || catRoll < 4) {
        return TreeCategory::Ancient;
    } else if (catRoll < 26) {
        return TreeCategory::Large;
    } else if (catRoll < 76) {
        return TreeCategory::Mature;
    } else {
        return TreeCategory::Young;
    }
}

static float sampleWeightedClearGap(uint32_t seed, TreeCategory cat, float jungleWeight) {
    int spacingRoll = (seed >> 8) % 100;
    float minRange = 45.0f;
    float maxRange = 85.0f;

    if (spacingRoll < 38) {
        minRange = 42.0f;
        maxRange = 75.0f;
    } else if (spacingRoll < 75) {
        minRange = 75.0f;
        maxRange = 130.0f;
    } else if (spacingRoll < 93) {
        minRange = 130.0f;
        maxRange = 200.0f;
    } else {
        minRange = 200.0f;
        maxRange = 300.0f;
    }

    float frac = static_cast<float>((seed >> 16) % 1000) / 1000.0f;
    float chosenClearGap = minRange + frac * (maxRange - minRange);

    if (cat == TreeCategory::Young) chosenClearGap -= 6.0f;
    else if (cat == TreeCategory::Large) chosenClearGap += 12.0f;
    else if (cat == TreeCategory::Ancient) chosenClearGap += 25.0f;

    float transitionDilation = 1.0f / std::clamp(jungleWeight, 0.12f, 1.0f);
    chosenClearGap *= transitionDilation;

    return std::max(MIN_APE_BODY_GAP, chosenClearGap);
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

    float prevTreeX = -999999.0f;
    float prevTrunkWidth = 80.0f;
    int treeCounter = 1;

    float simX = startX - 450.0f;
    while (simX < startX) {
        if (isPositionClear(simX, worldSeed)) {
            simX += 30.0f;
            continue;
        }

        BiomeTransitionInfo trans = Biome::getTransitionInfo(simX);
        if (trans.jungleWeight < 0.10f) {
            simX += 150.0f;
            continue;
        }

        int gridCoord = static_cast<int>(std::floor(simX / 15.0f));
        uint32_t seed = hashCoord(worldSeed, gridCoord, treeCounter);
        TreeCategory cat = determineCategory(seed, simX);
        TreeArchetype arch = getTreeArchetype(cat, seed);

        float candidateTreeX = simX;
        if (!isPositionClear(candidateTreeX, worldSeed)) {
            if (prevTreeX > -900000.0f) {
                float requiredCenterDist = (prevTrunkWidth * 0.5f) + (arch.trunkWidth * 0.5f) + MIN_APE_BODY_GAP;
                if (candidateTreeX - prevTreeX < requiredCenterDist) {
                    candidateTreeX = prevTreeX + requiredCenterDist;
                }
            }
            prevTreeX = candidateTreeX;
            prevTrunkWidth = arch.trunkWidth;
            float clearGap = sampleWeightedClearGap(seed, cat, trans.jungleWeight);
            simX = candidateTreeX + (arch.trunkWidth * 0.5f) + clearGap;
            treeCounter++;
        } else {
            simX += 30.0f;
        }
    }

    float currentX = startX + 20.0f;
    if (prevTreeX > -900000.0f) {
        currentX = std::max(currentX, prevTreeX + (prevTrunkWidth * 0.5f) + MIN_APE_BODY_GAP);
    }
    float endX = startX + width;

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

        TreeCategory cat = determineCategory(seed, currentX);
        TreeArchetype arch = getTreeArchetype(cat, seed);

        float treeX = currentX;
        if (prevTreeX > -900000.0f) {
            float requiredCenterDist = (prevTrunkWidth * 0.5f) + (arch.trunkWidth * 0.5f) + MIN_APE_BODY_GAP;
            if (treeX - prevTreeX < requiredCenterDist) {
                treeX = prevTreeX + requiredCenterDist;
            }
        }

        if (isPositionClear(treeX, worldSeed)) {
            currentX += 30.0f;
            continue;
        }

        sf::Color tColor(95, 62, 30);
        int uniqueTreeId = static_cast<int>(std::abs(treeX)) * 100 + (treeCounter++);

        Tree newTree(treeX, FLAT_GROUND_Y - 3.0f, arch.trunkWidth, arch.trunkHeight, tColor, decorTex, uniqueTreeId);

        uint32_t detailSeed = uniqueTreeId;
        newTree.buildCanopy(detailSeed, arch.canopyRadius, arch.canopyHeight, sf::Color(40, 140, 45), arch.clusters);

        for (int b = 0; b < arch.branchCount; ++b) {
            float branchY = (70.0f + b * 55.0f) * arch.scale;
            bool rightSide = (b % 2 == 0);
            newTree.addBranch(branchY, 40.0f * arch.scale, rightSide, sf::Color(70, 100, 50), decorTex);
        }

        if (trans.jungleWeight > 0.30f) {
            for (int v = 0; v < arch.vineCount; ++v) {
                float vSide = (v % 2 == 0) ? -1.0f : 1.0f;
                float vXOff = vSide * (arch.trunkWidth * (0.25f + v * 0.08f));
                float vYOff = arch.trunkHeight * (0.45f + v * 0.10f);
                float vLen = (80.0f + ((seed + v * 23) % 40)) * arch.scale;
                newTree.addVine(vXOff, vYOff, vLen);
            }
        }

        newTree.initDynamicMesh();
        result.push_back(std::move(newTree));

        float actualGap = (prevTreeX > -900000.0f) ? ((treeX - prevTreeX) - (prevTrunkWidth * 0.5f + arch.trunkWidth * 0.5f)) : 999.0f;
        std::cout << "[TREE SPACING]\n"
                  << "PreviousX=" << static_cast<int>(prevTreeX) << "\n"
                  << "CurrentX=" << static_cast<int>(treeX) << "\n"
                  << "ActualGap=" << static_cast<int>(actualGap) << "\n"
                  << "MinimumAllowed=" << static_cast<int>(MIN_APE_BODY_GAP) << "\n"
                  << "VALID\n\n";

        prevTreeX = treeX;
        prevTrunkWidth = arch.trunkWidth;

        float clearGap = sampleWeightedClearGap(seed, cat, trans.jungleWeight);
        currentX = treeX + (arch.trunkWidth * 0.5f) + clearGap;
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