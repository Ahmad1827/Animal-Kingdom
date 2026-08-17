#include "world/Biome.h"
#include "world/SeedManager.h"
#include <cmath>

static float sampleSmoothNoise(float x, uint32_t seed) {
    float s = static_cast<float>(seed % 10000);
    return std::sin(x * 0.00035f + s) * 0.6f + 
           std::sin(x * 0.00095f + s * 1.3f) * 0.3f + 
           std::sin(x * 0.0025f + s * 0.7f) * 0.1f;
}

RegionType Biome::determineRegion(int chunkX, uint32_t worldSeed) {
    float worldX = chunkX * 2000.f + 1000.f;
    float n = sampleSmoothNoise(worldX, worldSeed);
    
    if (n > 0.45f) return RegionType::OldGrowth;
    if (n > 0.15f) return RegionType::DenseJungle;
    if (n > -0.15f) return RegionType::YoungForest;
    if (n > -0.45f) return RegionType::Clearing;
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
            props.minTreeSpacing = 320.f;
            props.maxTreeSpacing = 750.f;
            props.treeSizeMultiplier = 1.45f;
            props.treeWidthBase = 95.f;
            props.branchCountMin = 6;
            props.branchCountMax = 10;
            props.branchVerticalSpacing = 220.f;
            props.canopyBaseRadius = 150.f;
            props.decorationDensity = 18;
            props.preferredDecorationType = 4;
            break;

        case RegionType::DenseJungle:
            props.name = "Dense Jungle";
            props.debugColor = sf::Color(0, 150, 50, 100);
            props.minTreeSpacing = 95.f;
            props.maxTreeSpacing = 240.f;
            props.treeSizeMultiplier = 1.1f;
            props.treeWidthBase = 75.f;
            props.branchCountMin = 4;
            props.branchCountMax = 7;
            props.branchVerticalSpacing = 150.f;
            props.canopyBaseRadius = 90.f;
            props.decorationDensity = 24;
            props.preferredDecorationType = 2;
            break;

        case RegionType::YoungForest:
            props.name = "Young Forest";
            props.debugColor = sf::Color(100, 200, 100, 100);
            props.minTreeSpacing = 180.f;
            props.maxTreeSpacing = 420.f;
            props.treeSizeMultiplier = 0.9f;
            props.treeWidthBase = 65.f;
            props.branchCountMin = 2;
            props.branchCountMax = 5;
            props.branchVerticalSpacing = 120.f;
            props.canopyBaseRadius = 65.f;
            props.decorationDensity = 14;
            props.preferredDecorationType = 0;
            break;

        case RegionType::Clearing:
            props.name = "Clearing";
            props.debugColor = sf::Color(200, 255, 150, 100);
            props.minTreeSpacing = 650.f;
            props.maxTreeSpacing = 1500.f;
            props.treeSizeMultiplier = 0.75f;
            props.treeWidthBase = 55.f;
            props.branchCountMin = 1;
            props.branchCountMax = 3;
            props.branchVerticalSpacing = 100.f;
            props.canopyBaseRadius = 45.f;
            props.decorationDensity = 16;
            props.preferredDecorationType = 1;
            break;

        case RegionType::RockyArea:
            props.name = "Rocky Area";
            props.groundColor = sf::Color(100, 100, 100);
            props.debugColor = sf::Color(150, 150, 150, 100);
            props.minTreeSpacing = 400.f;
            props.maxTreeSpacing = 900.f;
            props.treeSizeMultiplier = 0.85f;
            props.treeWidthBase = 60.f;
            props.branchCountMin = 2;
            props.branchCountMax = 4;
            props.branchVerticalSpacing = 160.f;
            props.canopyBaseRadius = 60.f;
            props.decorationDensity = 20;
            props.preferredDecorationType = 3;
            break;
    }
    return props;
}