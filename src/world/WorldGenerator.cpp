#include "world/WorldGenerator.h"
#include "world/SeedManager.h"
#include "core/VisualConfig.h"
#include "world/SettlementLayout.h"
#include <cmath>
#include <algorithm>
#include <iostream>

static constexpr float FLAT_GROUND_Y = 500.0f;
static constexpr float MIN_APE_BODY_GAP = 90.0f;

static uint32_t hashCoord(uint32_t worldSeed, int gridPos, uint32_t salt = 0) {
    uint32_t h = worldSeed ^ static_cast<uint32_t>(gridPos * 73856093) ^ (salt * 19349663);
    h = (h ^ (h >> 13)) * 0x5bd1e995;
    h = (h ^ (h >> 15)) * 0x1b873593;
    h = h ^ (h >> 16);
    return h;
}

static int pickTreeVariant(uint32_t seed, float& outVisualWidth, float& outTrunkWidth) {
    int roll = seed % 100;
    if (roll < 42) {
        outVisualWidth = 90.0f;
        outTrunkWidth = 35.0f;
        return 1;
    } else if (roll < 72) {
        outVisualWidth = 135.0f;
        outTrunkWidth = 48.0f;
        return 2;
    } else if (roll < 89) {
        outVisualWidth = 190.0f;
        outTrunkWidth = 68.0f;
        return 3;
    } else if (roll < 97) {
        outVisualWidth = 270.0f;
        outTrunkWidth = 95.0f;
        return 4;
    } else {
        outVisualWidth = 370.0f;
        outTrunkWidth = 140.0f;
        return 5;
    }
}

static float sampleWeightedClearGap(uint32_t seed, int variant, int prevVariant, float jungleWeight) {
    int spacingRoll = (seed >> 8) % 100;
    float minRange = 120.0f;
    float maxRange = 220.0f;

    if (spacingRoll < 35) {
        minRange = 110.0f;
        maxRange = 190.0f;
    } else if (spacingRoll < 72) {
        minRange = 190.0f;
        maxRange = 320.0f;
    } else if (spacingRoll < 90) {
        minRange = 320.0f;
        maxRange = 480.0f;
    } else {
        minRange = 480.0f;
        maxRange = 680.0f;
    }

    float frac = static_cast<float>((seed >> 16) % 1000) / 1000.0f;
    float chosenClearGap = minRange + frac * (maxRange - minRange);

    if (variant >= 4) {
        chosenClearGap += (variant == 5) ? 180.0f : 110.0f;
    }
    if (prevVariant >= 4) {
        chosenClearGap += (prevVariant == 5) ? 180.0f : 110.0f;
    }
    if (variant >= 4 && prevVariant >= 4) {
        chosenClearGap += 220.0f;
    }

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
    settlements.push_back({SettlementLayout::getPlayerCenterX(), SettlementLayout::getPlayerCenterX() - SettlementLayout::getPlayerTerritoryRadius(), SettlementLayout::getPlayerCenterX() + SettlementLayout::getPlayerTerritoryRadius()});

    std::vector<float> aiCenters = SettlementLayout::getVillageCenters(worldSeed);
    for (float cX : aiCenters) {
        settlements.push_back({cX, cX - SettlementLayout::getVillageTerritoryRadius(), cX + SettlementLayout::getVillageTerritoryRadius()});
    }

    std::sort(settlements.begin(), settlements.end(), [](const auto& a, const auto& b) {
        return a.centerX < b.centerX;
    });

    const float VILLAGE_CLEARANCE_RADIUS = 1050.0f;

    for (const auto& s : settlements) {
        zones.push_back({s.centerX - VILLAGE_CLEARANCE_RADIUS, s.centerX + VILLAGE_CLEARANCE_RADIUS, ClearanceType::Base, "VillageBase"});
    }

    for (size_t i = 0; i + 1 < settlements.size(); ++i) {
        float rightBorderA = settlements[i].borderMaxX;
        float leftBorderB = settlements[i + 1].borderMinX;

        float midX = (rightBorderA < leftBorderB) ? ((rightBorderA + leftBorderB) * 0.5f) : ((settlements[i].centerX + settlements[i + 1].centerX) * 0.5f);
        zones.push_back({midX - 220.0f, midX + 220.0f, ClearanceType::MeetingGround, "MeetingGround"});
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
    float prevVisualWidth = 100.0f;
    int prevVariant = 1;
    int treeCounter = 1;

    float simX = startX - 1000.0f;
    while (simX < startX) {
        if (isPositionClear(simX, worldSeed)) {
            simX += 50.0f;
            continue;
        }

        BiomeTransitionInfo trans = Biome::getTransitionInfo(simX);
        if (trans.jungleWeight < 0.10f) {
            simX += 180.0f;
            continue;
        }

        int gridCoord = static_cast<int>(std::floor(simX / 15.0f));
        uint32_t seed = hashCoord(worldSeed, gridCoord, treeCounter);
        float visualW = 100.0f;
        float trunkW = 35.0f;
        int variant = pickTreeVariant(seed, visualW, trunkW);

        float candidateTreeX = simX;
        if (!isPositionClear(candidateTreeX, worldSeed)) {
            if (prevTreeX > -900000.0f) {
                float requiredCenterDist = (prevVisualWidth * 0.5f) + (visualW * 0.5f) + MIN_APE_BODY_GAP;
                if (candidateTreeX - prevTreeX < requiredCenterDist) {
                    candidateTreeX = prevTreeX + requiredCenterDist;
                }
            }
            float clearGap = sampleWeightedClearGap(seed, variant, prevVariant, trans.jungleWeight);
            simX = candidateTreeX + (visualW * 0.5f) + clearGap;
            prevTreeX = candidateTreeX;
            prevVisualWidth = visualW;
            prevVariant = variant;
            treeCounter++;
        } else {
            simX += 50.0f;
        }
    }

    float currentX = startX + 35.0f;
    if (prevTreeX > -900000.0f) {
        currentX = std::max(currentX, prevTreeX + (prevVisualWidth * 0.5f) + MIN_APE_BODY_GAP);
    }
    float endX = startX + width;

    while (currentX < endX) {
        if (isPositionClear(currentX, worldSeed)) {
            currentX += 50.0f;
            continue;
        }

        BiomeTransitionInfo trans = Biome::getTransitionInfo(currentX);

        if (trans.jungleWeight < 0.10f) {
            currentX += 180.0f;
            continue;
        }

        int gridCoord = static_cast<int>(std::floor(currentX / 15.0f));
        uint32_t seed = hashCoord(worldSeed, gridCoord, treeCounter);
        float visualW = 100.0f;
        float trunkW = 35.0f;
        int variant = pickTreeVariant(seed, visualW, trunkW);

        float treeX = currentX;
        if (prevTreeX > -900000.0f) {
            float requiredCenterDist = (prevVisualWidth * 0.5f) + (visualW * 0.5f) + MIN_APE_BODY_GAP;
            if (treeX - prevTreeX < requiredCenterDist) {
                treeX = prevTreeX + requiredCenterDist;
            }
        }

        if (isPositionClear(treeX, worldSeed)) {
            currentX += 50.0f;
            continue;
        }

        int uniqueTreeId = static_cast<int>(std::abs(treeX)) * 100 + (treeCounter++);
        result.emplace_back(treeX, FLAT_GROUND_Y, variant, decorTex, uniqueTreeId);

        float clearGap = sampleWeightedClearGap(seed, variant, prevVariant, trans.jungleWeight);
        currentX = treeX + (visualW * 0.5f) + clearGap;
        prevTreeX = treeX;
        prevVisualWidth = visualW;
        prevVariant = variant;
    }

    return result;
}

std::vector<Decoration> WorldGenerator::generateDecorations(float startX, float width, uint32_t chunkSeed, uint32_t worldSeed, const BiomeProperties&, sf::Texture& decorTex) {
    std::vector<Decoration> decors;
    decors.reserve(100);

    float currentX = startX + 10.0f;
    float endX = startX + width;
    int decorIndex = 0;

    sf::Texture& jungleGroundTex = Tree::getJungleGroundTexture(decorTex);

    while (currentX < endX) {
        if (isPositionClear(currentX, worldSeed)) {
            currentX += 50.0f;
            continue;
        }

        BiomeTransitionInfo trans = Biome::getTransitionInfo(currentX);

        uint32_t itemSeed = chunkSeed + decorIndex * 53 + static_cast<int>(std::abs(currentX));
        itemSeed = (itemSeed ^ (itemSeed >> 13)) * 0x5bd1e995;

        float step = 70.0f;

        if (trans.jungleWeight > 0.35f) {
            float densityNoise = (std::sin(currentX * 0.012f + worldSeed * 0.001f) * 0.5f +
                                  std::cos(currentX * 0.031f) * 0.5f + 1.0f) * 0.5f;

            if (densityNoise > 0.60f) {
                step = 28.0f + (itemSeed % 18);
            } else if (densityNoise > 0.28f) {
                step = 55.0f + (itemSeed % 28);
            } else {
                step = 110.0f + (itemSeed % 50);
            }

            int roll = itemSeed % 100;
            if (roll < 38) {
                sf::IntRect fernRect = VisualConfig::JUNGLE_FERN_01;
                int fRoll = (itemSeed >> 4) % 3;
                if (fRoll == 1) fernRect = VisualConfig::JUNGLE_FERN_02;
                else if (fRoll == 2) fernRect = VisualConfig::JUNGLE_FERN_04;
                decors.emplace_back(currentX, FLAT_GROUND_Y, fernRect, itemSeed, jungleGroundTex, 0.48f, 2.0f);
            } else if (roll < 70) {
                sf::IntRect plantRect = VisualConfig::JUNGLE_PLANT_01;
                int pRoll = (itemSeed >> 4) % 4;
                if (pRoll == 1) plantRect = VisualConfig::JUNGLE_PLANT_02;
                else if (pRoll == 2) plantRect = VisualConfig::JUNGLE_PLANT_03;
                else if (pRoll == 3) plantRect = VisualConfig::JUNGLE_PLANT_04;
                decors.emplace_back(currentX, FLAT_GROUND_Y, plantRect, itemSeed, jungleGroundTex, 0.45f, 2.0f);
            } else if (roll < 86) {
                sf::IntRect leafRect = VisualConfig::JUNGLE_LEAVES_01;
                int lRoll = (itemSeed >> 4) % 3;
                if (lRoll == 1) leafRect = VisualConfig::JUNGLE_LEAVES_02;
                else if (lRoll == 2) leafRect = VisualConfig::JUNGLE_LEAVES_03;
                decors.emplace_back(currentX, FLAT_GROUND_Y, leafRect, itemSeed, jungleGroundTex, 0.42f, 4.0f);
            } else {
                int rockRoll = (itemSeed >> 4) % 100;
                sf::IntRect rockRect = VisualConfig::JUNGLE_ROCK_01;
                float rockScale = 0.35f;

                if (rockRoll < 60) {
                    rockRect = VisualConfig::JUNGLE_ROCK_01;
                    rockScale = 0.35f;
                } else if (rockRoll < 90) {
                    rockRect = VisualConfig::JUNGLE_ROCK_02;
                    rockScale = 0.40f;
                } else {
                    rockRect = VisualConfig::JUNGLE_ROCK_03;
                    rockScale = 0.45f;
                }
                decors.emplace_back(currentX, FLAT_GROUND_Y, rockRect, itemSeed, jungleGroundTex, rockScale, 2.0f);
            }
        } else if (trans.desertWeight > 0.70f) {
            step = 260.0f;
            int type = (itemSeed % 100 < 70) ? 3 : 4;
            decors.emplace_back(currentX, FLAT_GROUND_Y, type, itemSeed, decorTex);
        } else if (trans.fieldWeight > 0.60f) {
            step = 95.0f + (itemSeed % 40);
            int type = (itemSeed % 100 < 75) ? 1 : 0;
            decors.emplace_back(currentX, FLAT_GROUND_Y, type, itemSeed, decorTex);
        } else {
            step = 110.0f + (itemSeed % 45);
            int type = (itemSeed % 100 < 50) ? 2 : 1;
            decors.emplace_back(currentX, FLAT_GROUND_Y, type, itemSeed, decorTex);
        }

        currentX += step;
        decorIndex++;
    }

    return decors;
}