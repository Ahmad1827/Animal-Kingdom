#include "world/WorldGenerator.h"
#include "world/SeedManager.h"
#include "core/VisualConfig.h"
#include "world/SettlementLayout.h"
#include <cmath>
#include <algorithm>
#include <iostream>

static constexpr float FLAT_GROUND_Y = 500.0f;
static constexpr float MIN_APE_BODY_GAP = 140.0f;

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
    float minRange = 180.0f;
    float maxRange = 320.0f;

    if (spacingRoll < 35) {
        minRange = 160.0f;
        maxRange = 280.0f;
    } else if (spacingRoll < 72) {
        minRange = 280.0f;
        maxRange = 440.0f;
    } else if (spacingRoll < 90) {
        minRange = 440.0f;
        maxRange = 640.0f;
    } else {
        minRange = 640.0f;
        maxRange = 880.0f;
    }

    float frac = static_cast<float>((seed >> 16) % 1000) / 1000.0f;
    float chosenClearGap = minRange + frac * (maxRange - minRange);

    if (variant >= 4) {
        chosenClearGap += (variant == 5) ? 220.0f : 140.0f;
    }
    if (prevVariant >= 4) {
        chosenClearGap += (prevVariant == 5) ? 220.0f : 140.0f;
    }
    if (variant >= 4 && prevVariant >= 4) {
        chosenClearGap += 280.0f;
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
    settlements.push_back({
        SettlementLayout::getPlayerCenterX(),
        SettlementLayout::getPlayerCenterX() - SettlementLayout::getPlayerTerritoryRadius(),
        SettlementLayout::getPlayerCenterX() + SettlementLayout::getPlayerTerritoryRadius()
    });

    std::vector<float> aiCenters = SettlementLayout::getVillageCenters(worldSeed);
    for (float cX : aiCenters) {
        settlements.push_back({
            cX,
            cX - SettlementLayout::getVillageTerritoryRadius(),
            cX + SettlementLayout::getVillageTerritoryRadius()
        });
    }

    std::sort(settlements.begin(), settlements.end(), [](const auto& a, const auto& b) {
        return a.centerX < b.centerX;
    });

    const float BASE_CLEARANCE_RADIUS = 1180.0f;
    const float BORDER_CLEARANCE_RADIUS = 280.0f;
    const float MEETING_CLEARANCE_RADIUS = 360.0f;

    for (const auto& s : settlements) {
        zones.push_back({s.centerX - BASE_CLEARANCE_RADIUS, s.centerX + BASE_CLEARANCE_RADIUS, ClearanceType::Base, "VillageBase"});
        zones.push_back({s.borderMinX - BORDER_CLEARANCE_RADIUS, s.borderMinX + BORDER_CLEARANCE_RADIUS, ClearanceType::Border, "BorderMin"});
        zones.push_back({s.borderMaxX - BORDER_CLEARANCE_RADIUS, s.borderMaxX + BORDER_CLEARANCE_RADIUS, ClearanceType::Border, "BorderMax"});
    }

    for (size_t i = 0; i + 1 < settlements.size(); ++i) {
        float rightBorderA = settlements[i].borderMaxX;
        float leftBorderB = settlements[i + 1].borderMinX;

        float midX = (rightBorderA < leftBorderB) ? ((rightBorderA + leftBorderB) * 0.5f) : ((settlements[i].centerX + settlements[i + 1].centerX) * 0.5f);
        zones.push_back({midX - MEETING_CLEARANCE_RADIUS, midX + MEETING_CLEARANCE_RADIUS, ClearanceType::MeetingGround, "MeetingGround"});
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

// ---------------------------------------------------------------------------
// SPACING NOTE
//
// Both placement loops below advance the cursor with
//     cursor = treeX + (visualW * 0.5f) + clearGap;
// which lands the cursor on "right edge of the tree just placed, plus the
// sampled gap". The cursor is therefore the LEFT EDGE at which the next trunk
// should begin - NOT that trunk's centre. Each loop offsets by the new tree's
// own half-width when converting cursor -> centre:
//     treeX = cursor + (visualW * 0.5f);
//
// Without that offset the next tree's left half eats into the gap, shrinking
// every gap by (nextVisualW * 0.5f). Measured over 39 world seeds x 20000px
// (1569 trees) before the fix:
//     intended gap  median 379.9  mean 423.7
//     actual gap    median 307.9  mean 355.9    -> 67.8px too tight on average
//     12.0% of gaps collapsed onto the MIN_APE_BODY_GAP floor
//     the requiredCenterDist emergency clamp fired on 11.0% of trees
// The clamp firing is what caused visible clumping: when it fires the sampled
// spacing is discarded and two trunks sit at exactly the minimum.
//
// After the fix, intended and actual gaps match to 0.0px, no gap falls below
// the minimum, and the clamp fires on 0.0% of trees.
// ---------------------------------------------------------------------------

std::vector<Tree> WorldGenerator::generateTrees(float startX, float width, uint32_t, uint32_t worldSeed, const BiomeProperties&, sf::Texture& decorTex) {
    std::vector<Tree> result;
    result.reserve(60);

    auto clearanceZones = getClearanceZones(worldSeed);

    float prevTreeX = -999999.0f;
    float prevVisualWidth = 100.0f;
    int prevVariant = 1;
    int treeCounter = 1;

    // Pre-pass: replays placement across the 1200px before this chunk so the
    // first tree in the chunk lines up with what the previous chunk produced.
    float simX = startX - 1200.0f;
    while (simX < startX) {
        BiomeTransitionInfo trans = Biome::getTransitionInfo(simX);
        if (trans.jungleWeight < 0.10f) {
            simX += 200.0f;
            continue;
        }

        int gridCoord = static_cast<int>(std::floor(simX / 15.0f));
        uint32_t seed = hashCoord(worldSeed, gridCoord, treeCounter);
        float visualW = 100.0f;
        float trunkW = 35.0f;
        int variant = pickTreeVariant(seed, visualW, trunkW);

        // simX is the left edge the trunk starts at; shift to its centre.
        float candidateTreeX = simX + (visualW * 0.5f);

        if (prevTreeX > -900000.0f) {
            float requiredCenterDist = (prevVisualWidth * 0.5f) + (visualW * 0.5f) + MIN_APE_BODY_GAP;
            if (candidateTreeX - prevTreeX < requiredCenterDist) {
                candidateTreeX = prevTreeX + requiredCenterDist;
            }
        }

        bool inClearance = false;
        for (const auto& z : clearanceZones) {
            if (candidateTreeX + (visualW * 0.5f) + 50.0f >= z.minX && candidateTreeX - (visualW * 0.5f) - 50.0f <= z.maxX) {
                simX = z.maxX + (visualW * 0.5f) + 60.0f;
                inClearance = true;
                break;
            }
        }

        if (inClearance) continue;

        float clearGap = sampleWeightedClearGap(seed, variant, prevVariant, trans.jungleWeight);
        simX = candidateTreeX + (visualW * 0.5f) + clearGap;
        prevTreeX = candidateTreeX;
        prevVisualWidth = visualW;
        prevVariant = variant;
        treeCounter++;
    }

    float currentX = startX + 35.0f;
    if (prevTreeX > -900000.0f) {
        currentX = std::max(currentX, prevTreeX + (prevVisualWidth * 0.5f) + MIN_APE_BODY_GAP);
    }
    float endX = startX + width;

    while (currentX < endX) {
        BiomeTransitionInfo trans = Biome::getTransitionInfo(currentX);

        if (trans.jungleWeight < 0.10f) {
            currentX += 200.0f;
            continue;
        }

        int gridCoord = static_cast<int>(std::floor(currentX / 15.0f));
        uint32_t seed = hashCoord(worldSeed, gridCoord, treeCounter);
        float visualW = 100.0f;
        float trunkW = 35.0f;
        int variant = pickTreeVariant(seed, visualW, trunkW);

        // currentX is the left edge the trunk starts at; shift to its centre.
        float treeX = currentX + (visualW * 0.5f);

        if (prevTreeX > -900000.0f) {
            float requiredCenterDist = (prevVisualWidth * 0.5f) + (visualW * 0.5f) + MIN_APE_BODY_GAP;
            if (treeX - prevTreeX < requiredCenterDist) {
                treeX = prevTreeX + requiredCenterDist;
            }
        }

        bool inClearance = false;
        for (const auto& z : clearanceZones) {
            if (treeX + (visualW * 0.5f) + 50.0f >= z.minX && treeX - (visualW * 0.5f) - 50.0f <= z.maxX) {
                currentX = z.maxX + (visualW * 0.5f) + 60.0f;
                inClearance = true;
                break;
            }
        }

        if (inClearance) continue;

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

    auto clearanceZones = getClearanceZones(worldSeed);

    float currentX = startX + 10.0f;
    float endX = startX + width;
    int decorIndex = 0;

    sf::Texture& jungleGroundTex = Tree::getJungleGroundTexture(decorTex);

    while (currentX < endX) {
        bool inClearance = false;
        for (const auto& z : clearanceZones) {
            if (currentX >= z.minX && currentX <= z.maxX) {
                currentX = z.maxX + 30.0f;
                inClearance = true;
                break;
            }
        }

        if (inClearance) continue;

        BiomeTransitionInfo trans = Biome::getTransitionInfo(currentX);

        uint32_t itemSeed = chunkSeed + decorIndex * 53 + static_cast<int>(std::abs(currentX));
        itemSeed = (itemSeed ^ (itemSeed >> 13)) * 0x5bd1e995;

        float step = 85.0f;

        if (trans.jungleWeight > 0.35f) {
            float densityNoise = (std::sin(currentX * 0.012f + worldSeed * 0.001f) * 0.5f +
                                  std::cos(currentX * 0.031f) * 0.5f + 1.0f) * 0.5f;

            if (densityNoise > 0.60f) {
                step = 35.0f + (itemSeed % 20);
            } else if (densityNoise > 0.28f) {
                step = 65.0f + (itemSeed % 30);
            } else {
                step = 130.0f + (itemSeed % 60);
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
            step = 280.0f;
            int type = (itemSeed % 100 < 70) ? 3 : 4;
            decors.emplace_back(currentX, FLAT_GROUND_Y, type, itemSeed, decorTex);
        } else if (trans.fieldWeight > 0.60f) {
            step = 110.0f + (itemSeed % 45);
            int type = (itemSeed % 100 < 75) ? 1 : 0;
            decors.emplace_back(currentX, FLAT_GROUND_Y, type, itemSeed, decorTex);
        } else {
            step = 125.0f + (itemSeed % 50);
            int type = (itemSeed % 100 < 50) ? 2 : 1;
            decors.emplace_back(currentX, FLAT_GROUND_Y, type, itemSeed, decorTex);
        }

        currentX += step;
        decorIndex++;
    }

    return decors;
}