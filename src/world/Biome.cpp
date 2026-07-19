#include "world/Biome.h"
#include "world/SeedManager.h"
#include <cmath>

RegionType Biome::determineRegion(int chunkX, uint32_t worldSeed) {
    // Smooth coherent noise to ensure gradual region transitions
    float noise = std::sin(chunkX * 0.1f + (worldSeed % 100)) + std::sin(chunkX * 0.05f);
    
    if (noise > 1.2f) return RegionType::OldGrowth;
    if (noise > 0.5f) return RegionType::DenseJungle;
    if (noise > -0.2f) return RegionType::YoungForest;
    if (noise > -1.0f) return RegionType::Clearing;
    return RegionType::RockyArea;
}

BiomeProperties Biome::getProperties(RegionType type) {
    BiomeProperties props;
    props.groundColor = sf::Color(34, 139, 34);
    props.undergroundColor = sf::Color(70, 50, 30);

    switch (type) {
        case RegionType::OldGrowth:
            props.name = "Old Growth";
            props.debugColor = sf::Color(0, 100, 0, 100);
            props.minTreeSpacing = 600.f;
            props.maxTreeSpacing = 1200.f;
            props.treeSizeMultiplier = 2.5f;
            props.treeWidthBase = 120.f;
            props.branchCountMin = 6;
            props.branchCountMax = 10;
            props.branchVerticalSpacing = 220.f;
            props.canopyBaseRadius = 150.f;
            props.decorationDensity = 3;
            props.preferredDecorationType = 4; // Logs
            break;
        case RegionType::DenseJungle:
            props.name = "Dense Jungle";
            props.debugColor = sf::Color(0, 150, 50, 100);
            props.minTreeSpacing = 150.f;
            props.maxTreeSpacing = 350.f;
            props.treeSizeMultiplier = 1.2f;
            props.treeWidthBase = 50.f;
            props.branchCountMin = 4;
            props.branchCountMax = 7;
            props.branchVerticalSpacing = 150.f;
            props.canopyBaseRadius = 90.f;
            props.decorationDensity = 8;
            props.preferredDecorationType = 2; // Ferns
            break;
        case RegionType::YoungForest:
            props.name = "Young Forest";
            props.debugColor = sf::Color(100, 200, 100, 100);
            props.minTreeSpacing = 200.f;
            props.maxTreeSpacing = 400.f;
            props.treeSizeMultiplier = 0.8f;
            props.treeWidthBase = 30.f;
            props.branchCountMin = 2;
            props.branchCountMax = 4;
            props.branchVerticalSpacing = 120.f;
            props.canopyBaseRadius = 60.f;
            props.decorationDensity = 5;
            props.preferredDecorationType = 0; // Grass
            break;
        case RegionType::Clearing:
            props.name = "Clearing";
            props.debugColor = sf::Color(200, 255, 150, 100);
            props.minTreeSpacing = 800.f;
            props.maxTreeSpacing = 2000.f;
            props.treeSizeMultiplier = 0.6f;
            props.treeWidthBase = 20.f;
            props.branchCountMin = 1;
            props.branchCountMax = 2;
            props.branchVerticalSpacing = 100.f;
            props.canopyBaseRadius = 40.f;
            props.decorationDensity = 10;
            props.preferredDecorationType = 1; // Flowers
            break;
        case RegionType::RockyArea:
            props.name = "Rocky Area";
            props.groundColor = sf::Color(100, 100, 100);
            props.debugColor = sf::Color(150, 150, 150, 100);
            props.minTreeSpacing = 500.f;
            props.maxTreeSpacing = 1000.f;
            props.treeSizeMultiplier = 1.0f;
            props.treeWidthBase = 40.f;
            props.branchCountMin = 2;
            props.branchCountMax = 5;
            props.branchVerticalSpacing = 160.f;
            props.canopyBaseRadius = 70.f;
            props.decorationDensity = 6;
            props.preferredDecorationType = 3; // Rocks
            break;
    }
    return props;
}