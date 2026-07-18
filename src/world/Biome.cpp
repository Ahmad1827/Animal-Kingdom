#include "world/Biome.h"
#include <cmath>

RegionType Biome::determineRegion(int chunkX, uint32_t worldSeed) {
    // Low frequency noise for larger coherent regions
    float noise = std::sin(chunkX * 0.15f + (worldSeed % 100)) + std::cos(chunkX * 0.08f);
    
    if (noise > 1.2f) return RegionType::AncientGrove;
    if (noise > 0.4f) return RegionType::DenseJungle;
    if (noise > -0.3f) return RegionType::OldGrowth;
    if (noise > -1.0f) return RegionType::YoungForest;
    return RegionType::Clearing;
}

BiomeProperties Biome::getProperties(RegionType type) {
    BiomeProperties props;
    props.trunkColorMin = sf::Color(45, 30, 20);
    props.trunkColorMax = sf::Color(65, 45, 25);
    props.leafColorMin = sf::Color(20, 70, 30);
    props.leafColorMax = sf::Color(40, 100, 40);
    props.groundColor = sf::Color(25, 60, 25);
    props.undergroundColor = sf::Color(10, 25, 15);

    if (type == RegionType::AncientGrove) {
        props.name = "Ancient Grove";
        props.minTrees = 2; props.maxTrees = 4;
        props.minDecorations = 15; props.maxDecorations = 30;
        props.treeSizeMultiplier = 2.5f;
        props.branchCountModifier = -2; // Few, but massive branches
        props.canopyDensity = 1.5f;
        props.debugColor = sf::Color(150, 0, 150, 100);
    } else if (type == RegionType::Clearing) {
        props.name = "Clearing";
        props.minTrees = 0; props.maxTrees = 2;
        props.minDecorations = 40; props.maxDecorations = 70; // High ground cover
        props.treeSizeMultiplier = 0.8f;
        props.branchCountModifier = 0;
        props.canopyDensity = 0.5f;
        props.groundColor = sf::Color(45, 90, 35);
        props.leafColorMin = sf::Color(60, 130, 40);
        props.debugColor = sf::Color(200, 200, 0, 100);
    } else if (type == RegionType::YoungForest) {
        props.name = "Young Forest";
        props.minTrees = 15; props.maxTrees = 25;
        props.minDecorations = 10; props.maxDecorations = 20;
        props.treeSizeMultiplier = 0.6f;
        props.branchCountModifier = -1;
        props.canopyDensity = 0.6f;
        props.leafColorMin = sf::Color(40, 120, 50);
        props.debugColor = sf::Color(0, 200, 0, 100);
    } else if (type == RegionType::DenseJungle) {
        props.name = "Dense Jungle";
        props.minTrees = 12; props.maxTrees = 18;
        props.minDecorations = 30; props.maxDecorations = 50;
        props.treeSizeMultiplier = 1.2f;
        props.branchCountModifier = 2;
        props.canopyDensity = 1.2f;
        props.debugColor = sf::Color(0, 100, 0, 100);
    } else {
        props.name = "Old Growth";
        props.minTrees = 6; props.maxTrees = 10;
        props.minDecorations = 20; props.maxDecorations = 35;
        props.treeSizeMultiplier = 1.5f;
        props.branchCountModifier = 1;
        props.canopyDensity = 1.0f;
        props.debugColor = sf::Color(0, 50, 150, 100);
    }
    return props;
}