#include "world/WorldGenerator.h"
#include "world/SeedManager.h"
#include <cmath>
#include <algorithm>
#include <iostream>

static constexpr float FLAT_GROUND_Y = 500.0f;
static constexpr float MIN_APE_BODY_GAP = 42.0f;

static uint32_t hashCoord(uint32_t worldSeed, int gridPos, uint32_t salt = 0) {
    uint32_t h = worldSeed ^ static_cast<uint32_t>(gridPos * 73856093) ^ (salt * 19349663);
    h = (h ^ (h >> 13)) * 0x5bd1e995;
    h = (h ^ (h >> 15)) * 0x1b873593;
    h = h ^ (h >> 16);
    return h;
}

static int pickTreeVariant(uint32_t seed, float& outTrunkWidth) {
    int roll = seed % 100;
    if (roll < 40) {
        outTrunkWidth = 35.0f;
        return 1;
    } else if (roll < 70) {
        outTrunkWidth = 48.0f;
        return 2;
    } else if (roll < 88) {
        outTrunkWidth = 68.0f;
        return 3;
    } else if (roll < 97) {
        outTrunkWidth = 95.0f;
        return 4;
    } else {
        outTrunkWidth = 140.0f;
        return 5;
    }
}

static float sampleWeightedClearGap(uint32_t seed, int variant, float jungleWeight) {
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

    if (variant == 1) chosenClearGap -= 6.0f;
    else if (variant == 2) chosenClearGap += 0.0f;
    else if (variant == 3) chosenClearGap += 12.0f;
    else if (variant == 4) chosenClearGap += 25.0f;
    else if (variant == 5) chosenClearGap += 50.0f;

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

        float midX = (rightBorderA < leftBorderB) ? ((rightBorderA + leftBorderB) * 0.5f) : ((settlements[i].centerX + settlements[i + 1].centerX) * 0.5f);
        zones.push_back({midX - 180.0f, midX + 180.0f, ClearanceType::MeetingGround, "MeetingGround"});
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
    float prevTrunkWidth = 48.0f;
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
        float trunkW = 35.0f;
        int variant = pickTreeVariant(seed, trunkW);

        float candidateTreeX = simX;
        if (!isPositionClear(candidateTreeX, worldSeed)) {
            if (prevTreeX > -900000.0f) {
                float requiredCenterDist = (prevTrunkWidth * 0.5f) + (trunkW * 0.5f) + MIN_APE_BODY_GAP;
                if (candidateTreeX - prevTreeX < requiredCenterDist) {
                    candidateTreeX = prevTreeX + requiredCenterDist;
                }
            }
            prevTreeX = candidateTreeX;
            prevTrunkWidth = trunkW;
            float clearGap = sampleWeightedClearGap(seed, variant, trans.jungleWeight);
            simX = candidateTreeX + (trunkW * 0.5f) + clearGap;
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
        float trunkW = 35.0f;
        int variant = pickTreeVariant(seed, trunkW);

        float treeX = currentX;
        if (prevTreeX > -900000.0f) {
            float requiredCenterDist = (prevTrunkWidth * 0.5f) + (trunkW * 0.5f) + MIN_APE_BODY_GAP;
            if (treeX - prevTreeX < requiredCenterDist) {
                treeX = prevTreeX + requiredCenterDist;
            }
        }

        if (isPositionClear(treeX, worldSeed)) {
            currentX += 30.0f;
            continue;
        }

        int uniqueTreeId = static_cast<int>(std::abs(treeX)) * 100 + (treeCounter++);
        result.emplace_back(treeX, FLAT_GROUND_Y, variant, decorTex, uniqueTreeId);

        prevTreeX = treeX;
        prevTrunkWidth = trunkW;

        float clearGap = sampleWeightedClearGap(seed, variant, trans.jungleWeight);
        currentX = treeX + (trunkW * 0.5f) + clearGap;
    }

    return result;
}

std::vector<Decoration> WorldGenerator::generateDecorations(float startX, float width, uint32_t chunkSeed, uint32_t worldSeed, const BiomeProperties&, sf::Texture& decorTex) {
    std::vector<Decoration> decors;
    decors.reserve(80);

    float currentX = startX + 20.0f;
    float endX = startX + width;
    int decorIndex = 0;

    while (currentX < endX) {
        if (isPositionClear(currentX, worldSeed)) {
            currentX += 45.0f;
            continue;
        }

        BiomeTransitionInfo trans = Biome::getTransitionInfo(currentX);

        uint32_t itemSeed = chunkSeed + decorIndex * 53 + static_cast<int>(std::abs(currentX));
        itemSeed = (itemSeed ^ (itemSeed >> 13)) * 0x5bd1e995;
        float roll = static_cast<float>(itemSeed % 1000) / 1000.0f;

        int type = 0;
        float step = 100.0f;

        if (trans.jungleWeight > 0.35f) {
            float densityNoise = (std::sin(currentX * 0.008f + worldSeed * 0.001f) * 0.5f +
                                  std::cos(currentX * 0.022f) * 0.5f + 1.0f) * 0.5f;

            if (densityNoise > 0.65f) {
                step = 24.0f + (itemSeed % 18);
            } else if (densityNoise > 0.30f) {
                step = 42.0f + (itemSeed % 26);
            } else {
                step = 85.0f + (itemSeed % 40);
            }

            if (roll < 0.72f) {
                type = 2;
            } else if (roll < 0.88f) {
                type = 3;
            } else {
                type = 0;
            }
        } else if (trans.desertWeight > 0.70f) {
            step = 220.0f;
            type = (roll < 0.7f) ? 3 : 4;
        } else if (trans.fieldWeight > 0.60f) {
            step = 65.0f + (itemSeed % 30);
            type = (roll < 0.75f) ? 1 : 0;
        } else {
            step = 80.0f + (itemSeed % 35);
            type = (roll < 0.5f) ? 2 : 1;
        }

        decors.emplace_back(currentX, FLAT_GROUND_Y, type, itemSeed, decorTex);

        currentX += step;
        decorIndex++;
    }

    return decors;
}