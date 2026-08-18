#include "world/WorldGenerator.h"
#include "world/SeedManager.h"
#include <cmath>
#include <algorithm>
#include <iostream>

static constexpr float FLAT_GROUND_Y = 500.0f;

// ============================================================
// JUNGLE GENERATION PARAMETERS & DATA STRUCTURES
// ============================================================
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
    int canopyClusters;
    int branchCount;
    int vineCount;
    float baseSpacing;
};

// Procedural hash for deterministic tree generation
static uint32_t hashCoord(uint32_t worldSeed, int gridPos, uint32_t salt = 0) {
    uint32_t h = worldSeed ^ static_cast<uint32_t>(gridPos * 73856093) ^ (salt * 19349663);
    h = (h ^ (h >> 13)) * 0x5bd1e995;
    h = (h ^ (h >> 15)) * 0x1b873593;
    h = h ^ (h >> 16);
    return h;
}

// Generates a smooth local density field (0.0 to 1.0)
// This divides the jungle into Dense Groves, Normal Jungle, and Open Clearings
static float getJungleMacroDensity(float worldX, uint32_t worldSeed) {
    float s1 = static_cast<float>((worldSeed * 10139) % 10000);
    float s2 = static_cast<float>((worldSeed * 16645) % 10000);
    float s3 = static_cast<float>((worldSeed * 21401) % 10000);

    // Frequencies set so groves/clearings change every ~1000-2000 pixels
    float v1 = std::sin(worldX * 0.0015f + s1);
    float v2 = std::sin(worldX * 0.0027f + s2) * 0.5f;
    float v3 = std::cos(worldX * 0.0043f + s3) * 0.25f;

    float norm = (v1 + v2 + v3) / 1.75f; // Roughly -1.0 to 1.0
    return std::clamp((norm + 1.0f) * 0.5f, 0.0f, 1.0f);
}

// Determines the physical characteristics of a tree based on its category
static TreeArchetype getTreeArchetype(TreeCategory cat, uint32_t hash) {
    TreeArchetype arch;
    float var = static_cast<float>((hash >> 4) % 100) / 100.0f; // 0.0 to 1.0 variation

    switch (cat) {
        case TreeCategory::Young:
            arch.scale = 0.65f + var * 0.20f; // 0.65 - 0.85
            arch.trunkWidth = 55.f * arch.scale;
            arch.trunkHeight = 180.f * arch.scale;
            arch.canopyClusters = 2;
            arch.branchCount = 1 + (hash % 2);
            arch.vineCount = 0;
            arch.baseSpacing = 50.0f;
            break;

        case TreeCategory::Mature:
            arch.scale = 0.90f + var * 0.25f; // 0.90 - 1.15
            arch.trunkWidth = 80.f * arch.scale;
            arch.trunkHeight = 250.f * arch.scale;
            arch.canopyClusters = 3;
            arch.branchCount = 2 + (hash % 2);
            arch.vineCount = (hash % 10 < 3) ? 1 : 0; // 30% chance for 1 vine
            arch.baseSpacing = 80.0f;
            break;

        case TreeCategory::Large:
            arch.scale = 1.20f + var * 0.25f; // 1.20 - 1.45
            arch.trunkWidth = 110.f * arch.scale;
            arch.trunkHeight = 320.f * arch.scale;
            arch.canopyClusters = 4;
            arch.branchCount = 3 + (hash % 3);
            arch.vineCount = 1 + (hash % 3);
            arch.baseSpacing = 130.0f;
            break;

        case TreeCategory::Ancient:
            arch.scale = 1.50f + var * 0.40f; // 1.50 - 1.90
            arch.trunkWidth = 145.f * arch.scale;
            arch.trunkHeight = 410.f * arch.scale;
            arch.canopyClusters = 5;
            arch.branchCount = 5 + (hash % 3);
            arch.vineCount = 3 + (hash % 3);
            arch.baseSpacing = 190.0f;
            break;
    }
    return arch;
}

// ============================================================
// CLEARANCES
// ============================================================
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

// ============================================================
// ENVIRONMENT GENERATION
// ============================================================
std::vector<Tree> WorldGenerator::generateTrees(float startX, float width, uint32_t, uint32_t worldSeed, const BiomeProperties&, sf::Texture& decorTex) {
    std::vector<Tree> result;
    result.reserve(50);

    float currentX = startX + 30.0f;
    float endX = startX + width;
    int treeCounter = 1;

    while (currentX < endX) {
        // Skip base and meeting ground footprints entirely
        if (isPositionClear(currentX, worldSeed)) {
            currentX += 40.0f;
            continue;
        }

        // Get biome transition data to smoothly thin out trees near borders
        BiomeTransitionInfo trans = Biome::getTransitionInfo(currentX);

        // If we are deep into the Field/Desert (less than 10% Jungle), stop generating jungle trees.
        if (trans.jungleWeight < 0.10f) {
            currentX += 150.0f;
            continue;
        }

        // Get ecological macro-density (creates dense groves and open clearings)
        float macroDensity = getJungleMacroDensity(currentX, worldSeed);
        
        // Effective density naturally drops off as we transition to Flower Field
        float effectiveDensity = std::clamp(macroDensity * trans.jungleWeight, 0.0f, 1.0f);

        // Deterministic hashing for tree properties
        int gridCoord = static_cast<int>(std::floor(currentX / 10.0f));
        uint32_t pointHash = hashCoord(worldSeed, gridCoord, 1);

        float jitter = static_cast<float>((pointHash % 41) - 20);
        float treeX = currentX + jitter;

        // Double check jitter didn't push us into a base
        if (isPositionClear(treeX, worldSeed)) {
            currentX += 40.0f;
            continue;
        }

        // --- Select Tree Category based on Density ---
        TreeCategory cat;
        int roll = pointHash % 100;
        
        // Rare ancient landmark trees (1 in ~100 trees in dense areas)
        bool isLandmark = (pointHash % 1000) < 10 && effectiveDensity > 0.6f;

        if (isLandmark) {
            cat = TreeCategory::Ancient;
        } else if (effectiveDensity > 0.7f) { // Dense Grove
            if (roll < 30) cat = TreeCategory::Large;
            else if (roll < 75) cat = TreeCategory::Mature;
            else cat = TreeCategory::Young;
        } else if (effectiveDensity > 0.4f) { // Normal Jungle
            if (roll < 10) cat = TreeCategory::Large;
            else if (roll < 60) cat = TreeCategory::Mature;
            else cat = TreeCategory::Young;
        } else { // Open / Edge of Jungle
            if (roll < 5) cat = TreeCategory::Large;
            else if (roll < 40) cat = TreeCategory::Mature;
            else cat = TreeCategory::Young;
        }

        // Construct Tree Archetype
        TreeArchetype arch = getTreeArchetype(cat, pointHash);
        float yOff = static_cast<float>((pointHash % 9) - 4);
        int uniqueTreeId = static_cast<int>(std::abs(std::round(treeX))) * 100 + (treeCounter++);

        Tree newTree(treeX, FLAT_GROUND_Y + yOff, arch.trunkWidth, arch.trunkHeight, sf::Color(90, 60, 25), decorTex, uniqueTreeId);
        uint32_t detailSeed = pointHash ^ 0x9e3779b9u;

        // Visual Construction
        float canopyRadius = arch.trunkWidth * 1.5f;
        float canopyHeight = arch.trunkHeight * 0.6f;
        newTree.buildCanopy(detailSeed, canopyRadius, canopyHeight, sf::Color(35, 120, 40), arch.canopyClusters);

        float branchStartY = arch.trunkHeight * 0.35f;
        float branchVerticalSpacing = arch.trunkHeight * 0.15f;

        for (int b = 0; b < arch.branchCount; ++b) {
            float yOffset = branchStartY + b * branchVerticalSpacing;
            bool rightSide = (b % 2 == 0);
            newTree.addBranch(yOffset, arch.trunkWidth * 0.5f, rightSide, sf::Color(65, 95, 45), decorTex);
        }

        for (int v = 0; v < arch.vineCount; ++v) {
            float vSide = (v % 2 == 0) ? -1.0f : 1.0f;
            float vXOff = vSide * (arch.trunkWidth * (0.25f + v * 0.05f));
            float vYOff = arch.trunkHeight * (0.45f + (v * 0.1f));
            float vLen = (70.0f + static_cast<float>((pointHash + v * 31) % 90)) * arch.scale;
            newTree.addVine(vXOff, vYOff, vLen);
        }

        newTree.initDynamicMesh();
        result.push_back(std::move(newTree));

        // --- Calculate Dynamic Spacing ---
        // Spacing stretches out significantly when local density is low
        // multiplier = 1.0 at full density, 3.0 at zero density
        float densityMultiplier = 1.0f + (1.0f - effectiveDensity) * 2.0f; 
        float chosenSpacing = arch.baseSpacing * densityMultiplier;

        // Ensure we always step forward to prevent infinite loops
        currentX += std::max(40.0f, chosenSpacing);
    }

    return result;
}

std::vector<Decoration> WorldGenerator::generateDecorations(float startX, float width, uint32_t chunkSeed, uint32_t worldSeed, const BiomeProperties&, sf::Texture& decorTex) {
    std::vector<Decoration> decors;
    decors.reserve(80);

    float currentX = startX + 15.0f;
    float endX = startX + width;
    int decorIndex = 0;

    while (currentX < endX) {
        if (isPositionClear(currentX, worldSeed)) {
            currentX += 45.0f;
            continue;
        }

        BiomeTransitionInfo trans = Biome::getTransitionInfo(currentX);
        float macroDensity = getJungleMacroDensity(currentX, worldSeed);
        float effectiveJungle = std::clamp(macroDensity * trans.jungleWeight, 0.0f, 1.0f);

        uint32_t itemSeed = chunkSeed + decorIndex * 53 + static_cast<int>(std::abs(currentX));
        itemSeed = (itemSeed ^ (itemSeed >> 13)) * 0x5bd1e995;
        float roll = static_cast<float>(itemSeed % 1000) / 1000.0f;

        int type = 0;
        float step = 100.0f;

        // Decoration spawning driven directly by ecosystem density
        if (trans.jungleWeight > 0.5f) { 
            // Primarily Jungle Zone
            if (effectiveJungle > 0.6f) { 
                // Dense undergrowth
                type = 2; // Bushes/Ferns
                step = 40.0f + (itemSeed % 30); 
            } else if (effectiveJungle > 0.3f) { 
                // Normal undergrowth
                type = (roll < 0.8f) ? 2 : 0; // Bushes or low grass
                step = 70.0f + (itemSeed % 40);
            } else { 
                // Natural jungle clearing (mix of bushes and field flowers!)
                type = (roll < 0.5f) ? 2 : 1; 
                step = 120.0f + (itemSeed % 60);
            }
        } else if (trans.fieldWeight > 0.5f) { 
            // Primarily Flower Field Zone
            type = (roll < 0.85f) ? 1 : 0; // Heavy Flowers
            step = 55.0f + (itemSeed % 35); 
        } else if (trans.desertWeight > 0.5f) { 
            // Primarily Desert Zone
            type = (roll < 0.7f) ? 3 : 4; // Rocks
            step = 250.0f + (itemSeed % 100); 
        } else {
            // Mixed transition zone fallback
            type = (roll < 0.5f) ? 1 : 2;
            step = 90.0f;
        }

        // Double check clearance again after step calculation
        if (!isPositionClear(currentX, worldSeed)) {
            decors.emplace_back(currentX, FLAT_GROUND_Y, type, itemSeed, decorTex);
        }

        currentX += std::max(30.0f, step);
        decorIndex++;
    }

    return decors;
}