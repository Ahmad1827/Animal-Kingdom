#include "world/WorldGenerator.h"
#include "world/SeedManager.h"
#include "world/SettlementLayout.h"
#include <cmath>
#include <algorithm>
#include <mutex>
#include <unordered_map>
#include <iostream>

static constexpr float FLAT_GROUND_Y = 500.0f;
static constexpr float MIN_TREE_GAP = 60.0f;

static constexpr uint32_t SALT_OPENNESS   = 0x7A17u;
static constexpr uint32_t SALT_TREE_CLUMP = 0x33C1u;
static constexpr uint32_t SALT_VEG_REGION = 0x0A53u;
static constexpr uint32_t SALT_VEG_CLUMP  = 0x01D7u;

static uint32_t hashCoord(uint32_t worldSeed, int gridPos, uint32_t salt = 0) {
    uint32_t h = worldSeed ^ static_cast<uint32_t>(gridPos * 73856093) ^ (salt * 19349663);
    h = (h ^ (h >> 13)) * 0x5bd1e995;
    h = (h ^ (h >> 15)) * 0x1b873593;
    h = h ^ (h >> 16);
    return h;
}

static int pickTreeVariant(uint32_t seed, float& outTrunkWidth, float& outTotalWidth) {
    int roll = seed % 100;
    if (roll < 40) {
        outTrunkWidth = 35.0f;
        outTotalWidth = 80.0f;
        return 1;
    } else if (roll < 70) {
        outTrunkWidth = 48.0f;
        outTotalWidth = 120.0f;
        return 2;
    } else if (roll < 88) {
        outTrunkWidth = 68.0f;
        outTotalWidth = 170.0f;
        return 3;
    } else if (roll < 97) {
        outTrunkWidth = 95.0f;
        outTotalWidth = 240.0f;
        return 4;
    } else {
        outTrunkWidth = 140.0f;
        outTotalWidth = 330.0f;
        return 5;
    }
}

static std::mutex g_zoneMutex;
static std::unordered_map<uint32_t, std::vector<WorldClearanceZone>> g_zoneCache;

static const std::vector<WorldClearanceZone>& cachedZones(uint32_t worldSeed) {
    std::lock_guard<std::mutex> lock(g_zoneMutex);
    auto it = g_zoneCache.find(worldSeed);
    if (it == g_zoneCache.end()) {
        it = g_zoneCache.emplace(worldSeed, WorldGenerator::getClearanceZones(worldSeed)).first;
    }
    return it->second;
}

std::vector<WorldClearanceZone> WorldGenerator::getClearanceZones(uint32_t worldSeed) {
    std::vector<WorldClearanceZone> zones;

    struct SettlementBounds {
        float centerX;
        float borderMinX;
        float borderMaxX;
    };

    std::vector<SettlementBounds> settlements;

    float pC = SettlementLayout::getPlayerCenterX();
    float pR = SettlementLayout::getPlayerTerritoryRadius();
    settlements.push_back({pC, pC - pR, pC + pR});

    float vR = SettlementLayout::getVillageTerritoryRadius();
    for (float cX : SettlementLayout::getVillageCenters(worldSeed)) {
        settlements.push_back({cX, cX - vR, cX + vR});
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
    const auto& zones = cachedZones(worldSeed);
    for (const auto& z : zones) {
        if (worldX >= z.minX && worldX <= z.maxX) return true;
    }
    return false;
}

static float sampleWeightedClearGap(uint32_t seed, int variant, const EnvironmentProfile& env, float openness, float clump) {
    int spacingRoll = (seed >> 8) % 100;
    float minRange = 60.0f;
    float maxRange = 110.0f;

    if (spacingRoll < 38) {
        minRange = 60.0f;
        maxRange = 100.0f;
    } else if (spacingRoll < 75) {
        minRange = 100.0f;
        maxRange = 170.0f;
    } else if (spacingRoll < 93) {
        minRange = 170.0f;
        maxRange = 260.0f;
    } else {
        minRange = 260.0f;
        maxRange = 380.0f;
    }

    float frac = static_cast<float>((seed >> 16) % 1000) / 1000.0f;
    float chosenClearGap = minRange + frac * (maxRange - minRange);

    if (variant == 1) chosenClearGap -= 5.0f;
    else if (variant == 3) chosenClearGap += 15.0f;
    else if (variant == 4) chosenClearGap += 30.0f;
    else if (variant == 5) chosenClearGap += 60.0f;

    float densityDilation = 1.0f / std::clamp(env.treeDensity, 0.05f, 1.0f);
    chosenClearGap *= densityDilation;

    float clusterFactor = 1.0f - (env.clusterTendency * clump * 0.35f);
    chosenClearGap *= std::clamp(clusterFactor, 0.7f, 1.0f);

    if (openness < env.clearingTendency) {
        chosenClearGap *= 2.2f;
    }

    return std::max(MIN_TREE_GAP, chosenClearGap);
}

std::vector<Tree> WorldGenerator::generateTrees(float startX, float width, uint32_t, uint32_t worldSeed, const BiomeProperties&, sf::Texture& decorTex) {
    std::vector<Tree> result;
    result.reserve(60);

    float prevTreeX = -999999.0f;
    float prevTotalWidth = 120.0f;
    int treeCounter = 1;

    float simX = startX - 800.0f;
    while (simX < startX) {
        if (isPositionClear(simX, worldSeed)) {
            simX += 30.0f;
            continue;
        }

        EnvironmentProfile env = Biome::getBlendedEnvironment(simX);
        if (env.treeDensity < 0.02f) {
            simX += 200.0f;
            continue;
        }

        float openness = SeedManager::fbm(simX, worldSeed ^ SALT_OPENNESS, 0.0022f, 3);
        float clump    = SeedManager::fbm(simX, worldSeed ^ SALT_TREE_CLUMP, 0.0055f, 2);

        int gridCoord = static_cast<int>(std::floor(simX / 15.0f));
        uint32_t seed = hashCoord(worldSeed, gridCoord, treeCounter);
        float trunkW = 35.0f;
        float totalW = 80.0f;
        int variant = pickTreeVariant(seed, trunkW, totalW);

        float candidateTreeX = simX;
        if (!isPositionClear(candidateTreeX, worldSeed)) {
            if (prevTreeX > -900000.0f) {
                float requiredCenterDist = (prevTotalWidth * 0.5f) + (totalW * 0.5f) + MIN_TREE_GAP;
                if (candidateTreeX - prevTreeX < requiredCenterDist) {
                    candidateTreeX = prevTreeX + requiredCenterDist;
                }
            }
            prevTreeX = candidateTreeX;
            prevTotalWidth = totalW;
            float clearGap = sampleWeightedClearGap(seed, variant, env, openness, clump);
            simX = candidateTreeX + (totalW * 0.5f) + clearGap;
            treeCounter++;
        } else {
            simX += 30.0f;
        }
    }

    float currentX = startX + 20.0f;
    if (prevTreeX > -900000.0f) {
        currentX = std::max(currentX, prevTreeX + (prevTotalWidth * 0.5f) + MIN_TREE_GAP);
    }
    float endX = startX + width;

    while (currentX < endX) {
        if (isPositionClear(currentX, worldSeed)) {
            currentX += 30.0f;
            continue;
        }

        EnvironmentProfile env = Biome::getBlendedEnvironment(currentX);
        if (env.treeDensity < 0.02f) {
            currentX += 200.0f;
            continue;
        }

        float openness = SeedManager::fbm(currentX, worldSeed ^ SALT_OPENNESS, 0.0022f, 3);
        float clump    = SeedManager::fbm(currentX, worldSeed ^ SALT_TREE_CLUMP, 0.0055f, 2);

        int gridCoord = static_cast<int>(std::floor(currentX / 15.0f));
        uint32_t seed = hashCoord(worldSeed, gridCoord, treeCounter);
        float trunkW = 35.0f;
        float totalW = 80.0f;
        int variant = pickTreeVariant(seed, trunkW, totalW);

        float treeX = currentX;
        if (prevTreeX > -900000.0f) {
            float requiredCenterDist = (prevTotalWidth * 0.5f) + (totalW * 0.5f) + MIN_TREE_GAP;
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
        prevTotalWidth = totalW;

        float clearGap = sampleWeightedClearGap(seed, variant, env, openness, clump);
        currentX = treeX + (totalW * 0.5f) + clearGap;
    }

    return result;
}

std::vector<Decoration> WorldGenerator::generateDecorations(float startX, float width, uint32_t chunkSeed, uint32_t worldSeed, const BiomeProperties&, sf::Texture& decorTex) {
    std::vector<Decoration> decors;
    decors.reserve(96);

    const size_t MAX_DECOR_PER_CHUNK = 420;

    float currentX = startX + 20.0f;
    float endX = startX + width;
    int decorIndex = 0;

    while (currentX < endX && decors.size() < MAX_DECOR_PER_CHUNK) {
        if (isPositionClear(currentX, worldSeed)) {
            currentX += 45.0f;
            continue;
        }

        EnvironmentProfile env = Biome::getBlendedEnvironment(currentX);

        uint32_t itemSeed = chunkSeed + decorIndex * 53 + static_cast<uint32_t>(std::abs(currentX));
        itemSeed = (itemSeed ^ (itemSeed >> 13)) * 0x5bd1e995;
        float roll = static_cast<float>(itemSeed % 1000) / 1000.0f;
        float roll2 = static_cast<float>((itemSeed >> 10) % 1000) / 1000.0f;

        float region = SeedManager::fbm(currentX, worldSeed ^ SALT_VEG_REGION, 0.0030f, 3);
        float clump  = SeedManager::fbm(currentX, worldSeed ^ SALT_VEG_CLUMP,  0.0110f, 2);

        bool inClearing = (region < env.clearingTendency);
        float local = (0.35f + 0.65f * clump) * (inClearing ? 0.18f : 1.0f);

        float wUnder  = env.undergrowthDensity;
        float wFlower = env.flowerDensity;
        float wStone  = env.stoneDensity;
        float wLitter = env.litterDensity;
        float total = wUnder + wFlower + wStone + wLitter;

        if (total <= 0.001f) {
            currentX += std::max(60.0f, env.decorBaseStep);
            continue;
        }

        int type = DecorType::GenericBush;
        float pick = roll * total;

        if (pick < wUnder) {
            type = (local > 0.55f) ? DecorType::JungleUndergrowth : DecorType::SmallBush;
        } else if (pick < wUnder + wFlower) {
            type = DecorType::Flower;
        } else if (pick < wUnder + wFlower + wStone) {
            type = (roll2 < 0.55f) ? DecorType::Rock : DecorType::SmallRock;
        } else {
            type = DecorType::LeafLitter;
        }

        if (roll2 > 0.82f && env.grassDensity > 0.5f && local > 0.5f) {
            type = DecorType::GrassTuft;
        }

        decors.emplace_back(currentX, FLAT_GROUND_Y, type, itemSeed, decorTex);

        float step = env.decorBaseStep * (1.5f - std::clamp(local, 0.0f, 1.0f));
        if (inClearing) step *= 3.0f;
        step += static_cast<float>(itemSeed % 17);
        step = std::max(18.0f, step);

        currentX += step;
        decorIndex++;
    }

    return decors;
}