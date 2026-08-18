#include "world/WorldGenerator.h"
#include <cmath>
#include <iostream>

static constexpr float FLAT_GROUND_Y = 500.0f;

std::vector<WorldClearanceZone> WorldGenerator::getClearanceZones(uint32_t /*worldSeed*/) {
    std::vector<WorldClearanceZone> zones;
    
    // Player Base at X = 1000 (Clearance: 650 to 1350)
    zones.push_back({1000.f - 350.f, 1000.f + 350.f, ClearanceType::Base, "PlayerBase"});
    
    // Hardcoded neighboring clearings for simplicity
    zones.push_back({-5000.f - 350.f, -5000.f + 350.f, ClearanceType::Base, "WestBase"});
    zones.push_back({7000.f - 350.f, 7000.f + 350.f, ClearanceType::Base, "EastBase"});

    // Meeting Grounds exactly halfway between
    zones.push_back({-2000.f - 150.f, -2000.f + 150.f, ClearanceType::MeetingGround, "WestMeeting"});
    zones.push_back({4000.f - 150.f, 4000.f + 150.f, ClearanceType::MeetingGround, "EastMeeting"});

    return zones;
}

bool WorldGenerator::isPositionClear(float worldX, uint32_t worldSeed) {
    auto zones = getClearanceZones(worldSeed);
    for (const auto& z : zones) {
        if (worldX >= z.minX && worldX <= z.maxX) return true;
    }
    return false;
}

std::vector<Tree> WorldGenerator::generateTrees(float startX, float width, uint32_t chunkSeed, uint32_t worldSeed, const BiomeProperties& props, sf::Texture& decorTex) {
    std::vector<Tree> result;

    if (props.type != BiomeType::Jungle) {
        return result; // Explicitly NO JUNGLE TREES outside of Jungle
    }

    std::cout << "[JUNGLE GENERATION] Chunk X=" << startX << " Biome=TropicalJungle Generating trees...\n";

    float currentX = startX + 50.f;
    float endX = startX + width;
    int treeCounter = 1;

    // Deterministic loop. Guaranteed to place a tree every 140 pixels if clear.
    while (currentX < endX) {
        if (isPositionClear(currentX, worldSeed)) {
            currentX += 30.f; // Small step through clearance to resume exactly after
            continue;
        }

        float tWidth = 85.f;
        float tHeight = 280.f;
        sf::Color tColor(95, 62, 30);
        int uniqueTreeId = static_cast<int>(std::abs(currentX)) * 100 + (treeCounter++);

        Tree newTree(currentX, FLAT_GROUND_Y - 3.f, tWidth, tHeight, tColor, decorTex, uniqueTreeId);

        // Build guaranteed dense canopy and branches directly
        uint32_t detailSeed = uniqueTreeId;
        newTree.buildCanopy(detailSeed, 100.f, 180.f, sf::Color(40, 140, 45), 3);
        newTree.addBranch(90.f, 40.f, true, sf::Color(70, 100, 50), decorTex);
        newTree.addBranch(150.f, 40.f, false, sf::Color(70, 100, 50), decorTex);
        newTree.addVine(tWidth * 0.3f, tHeight * 0.5f, 120.f);
        
        newTree.initDynamicMesh();

        std::cout << "[TREE CREATED] ID=" << uniqueTreeId << " WorldX=" << currentX << " WorldY=" << FLAT_GROUND_Y << "\n";

        result.push_back(std::move(newTree));

        currentX += 140.f; // Strict 140px spacing
    }

    return result;
}

std::vector<Decoration> WorldGenerator::generateDecorations(float startX, float width, uint32_t chunkSeed, uint32_t worldSeed, const BiomeProperties& props, sf::Texture& decorTex) {
    std::vector<Decoration> decors;
    
    float currentX = startX + 50.f;
    float endX = startX + width;

    while (currentX < endX) {
        if (isPositionClear(currentX, worldSeed)) {
            currentX += 50.f;
            continue;
        }

        int type = 0;
        if (props.type == BiomeType::Jungle) type = 2; // Bushes
        else if (props.type == BiomeType::Field) type = 1; // Flowers
        else if (props.type == BiomeType::Desert) type = 3; // Rocks

        decors.emplace_back(currentX, FLAT_GROUND_Y, type, chunkSeed + static_cast<int>(currentX), decorTex);

        currentX += 180.f;
    }

    return decors;
}