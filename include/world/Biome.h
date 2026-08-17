#pragma once
#include <string>
#include <SFML/Graphics.hpp>

enum class RegionType {
    OldGrowth,
    DenseJungle,
    RiverBasin,
    YoungForest,
    RockyArea,
    Clearing
};

struct BiomeProperties {
    std::string name;
    RegionType type;
    sf::Color groundColor;
    sf::Color undergroundColor;
    sf::Color debugColor;
    
    float minTreeSpacing;
    float maxTreeSpacing;
    float treeScaleMin;
    float treeScaleMax;
    float treeWidthBase;
    int branchCountMin;
    int branchCountMax;
    float branchVerticalSpacing;
    float canopyBaseRadius;
    int decorationDensity;
    int primaryDecorType;
    int secondaryDecorType;
};

class Biome {
public:
    static RegionType determineRegion(int chunkX, uint32_t worldSeed);
    static RegionType determineRegionAtWorldX(float worldX, uint32_t worldSeed);
    static BiomeProperties getProperties(RegionType type);
};