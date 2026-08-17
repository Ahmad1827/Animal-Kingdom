#include "world/Biome.h"
#include <cmath>
#include <algorithm>

static float sampleMacroNoise(float x, uint32_t seed) {
    float s = static_cast<float>(seed % 10000);
    float n1 = std::sin(x * 0.00006f + s * 0.31f) * 0.55f;
    float n2 = std::sin(x * 0.00018f + s * 0.73f) * 0.30f;
    float n3 = std::cos(x * 0.00045f + s * 1.17f) * 0.15f;
    return n1 + n2 + n3;
}

RegionType Biome::determineRegionAtWorldX(float worldX, uint32_t worldSeed) {
    float macro = sampleMacroNoise(worldX, worldSeed);

    if (macro > 0.45f) return RegionType::OldGrowth;
    if (macro > 0.15f) return RegionType::DenseJungle;
    if (macro > -0.10f) return RegionType::RiverBasin;
    if (macro > -0.35f) return RegionType::YoungForest;
    if (macro > -0.65f) return RegionType::Clearing;
    return RegionType::RockyArea;
}

RegionType Biome::determineRegion(int chunkX, uint32_t worldSeed) {
    float worldX = chunkX * 2000.f + 1000.f;
    return determineRegionAtWorldX(worldX, worldSeed);
}

BiomeProperties Biome::getProperties(RegionType type) {
    BiomeProperties props;
    props.type = type;
    props.groundColor = sf::Color(34, 139, 34);
    props.undergroundColor = sf::Color(70, 50, 30);

    switch (type) {
        case RegionType::OldGrowth:
            props.name = "Ancient Old Growth";
            props.groundColor = sf::Color(30, 85, 30);
            props.undergroundColor = sf::Color(55, 38, 22);
            props.debugColor = sf::Color(15, 70, 20, 100);
            props.minTreeSpacing = 140.f;
            props.maxTreeSpacing = 240.f;
            props.treeScaleMin = 1.25f;
            props.treeScaleMax = 1.80f;
            props.treeWidthBase = 95.f;
            props.branchCountMin = 6;
            props.branchCountMax = 10;
            props.branchVerticalSpacing = 220.f;
            props.canopyBaseRadius = 150.f;
            props.decorationDensity = 14;
            props.primaryDecorType = 4;
            props.secondaryDecorType = 2;
            break;

        case RegionType::DenseJungle:
            props.name = "Dense Jungle";
            props.groundColor = sf::Color(25, 110, 35);
            props.undergroundColor = sf::Color(45, 30, 18);
            props.debugColor = sf::Color(0, 140, 45, 100);
            props.minTreeSpacing = 80.f;
            props.maxTreeSpacing = 150.f;
            props.treeScaleMin = 0.90f;
            props.treeScaleMax = 1.30f;
            props.treeWidthBase = 75.f;
            props.branchCountMin = 4;
            props.branchCountMax = 8;
            props.branchVerticalSpacing = 140.f;
            props.canopyBaseRadius = 95.f;
            props.decorationDensity = 22;
            props.primaryDecorType = 2;
            props.secondaryDecorType = 0;
            break;

        case RegionType::RiverBasin:
            props.name = "River Basin";
            props.groundColor = sf::Color(45, 130, 50);
            props.undergroundColor = sf::Color(50, 40, 25);
            props.debugColor = sf::Color(30, 160, 140, 100);
            props.minTreeSpacing = 110.f;
            props.maxTreeSpacing = 200.f;
            props.treeScaleMin = 0.85f;
            props.treeScaleMax = 1.20f;
            props.treeWidthBase = 70.f;
            props.branchCountMin = 3;
            props.branchCountMax = 6;
            props.branchVerticalSpacing = 130.f;
            props.canopyBaseRadius = 85.f;
            props.decorationDensity = 18;
            props.primaryDecorType = 2;
            props.secondaryDecorType = 1;
            break;

        case RegionType::YoungForest:
            props.name = "Young Forest";
            props.groundColor = sf::Color(55, 145, 45);
            props.undergroundColor = sf::Color(65, 45, 28);
            props.debugColor = sf::Color(90, 190, 80, 100);
            props.minTreeSpacing = 120.f;
            props.maxTreeSpacing = 220.f;
            props.treeScaleMin = 0.80f;
            props.treeScaleMax = 1.15f;
            props.treeWidthBase = 65.f;
            props.branchCountMin = 2;
            props.branchCountMax = 5;
            props.branchVerticalSpacing = 120.f;
            props.canopyBaseRadius = 65.f;
            props.decorationDensity = 12;
            props.primaryDecorType = 0;
            props.secondaryDecorType = 1;
            break;

        case RegionType::RockyArea:
            props.name = "Rocky Area";
            props.groundColor = sf::Color(115, 115, 105);
            props.undergroundColor = sf::Color(65, 60, 55);
            props.debugColor = sf::Color(140, 140, 140, 100);
            props.minTreeSpacing = 200.f;
            props.maxTreeSpacing = 380.f;
            props.treeScaleMin = 0.75f;
            props.treeScaleMax = 1.05f;
            props.treeWidthBase = 60.f;
            props.branchCountMin = 2;
            props.branchCountMax = 4;
            props.branchVerticalSpacing = 150.f;
            props.canopyBaseRadius = 60.f;
            props.decorationDensity = 18;
            props.primaryDecorType = 3;
            props.secondaryDecorType = 4;
            break;

        case RegionType::Clearing:
            props.name = "Clearing";
            props.groundColor = sf::Color(85, 160, 55);
            props.undergroundColor = sf::Color(80, 60, 35);
            props.debugColor = sf::Color(210, 235, 120, 100);
            props.minTreeSpacing = 350.f;
            props.maxTreeSpacing = 600.f;
            props.treeScaleMin = 0.70f;
            props.treeScaleMax = 0.95f;
            props.treeWidthBase = 55.f;
            props.branchCountMin = 1;
            props.branchCountMax = 3;
            props.branchVerticalSpacing = 95.f;
            props.canopyBaseRadius = 50.f;
            props.decorationDensity = 16;
            props.primaryDecorType = 1;
            props.secondaryDecorType = 0;
            break;
    }
    return props;
}