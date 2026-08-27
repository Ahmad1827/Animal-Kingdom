#pragma once
#include <string>
#include <SFML/Graphics.hpp>

enum class BiomeType {
    Jungle = 0,
    Field = 1,
    Desert = 2
};

using RegionType = BiomeType;

struct BiomeTransitionInfo {
    BiomeType currentBiome;
    BiomeType nextBiome;
    float blendFactor;
    float jungleWeight;
    float fieldWeight;
    float desertWeight;
};

struct BiomeProperties {
    std::string name;
    BiomeType type;
    std::string vegetationMode;
    sf::Color groundColor;
    sf::Color undergroundColor;
    sf::Color subsoilColor;
    sf::Color grassTipColor;
    sf::Color grassBaseColor;
    sf::Color debugColor;
    float minTreeSpacing;
    float maxTreeSpacing;
};

// ---------------------------------------------------------------------------
// EnvironmentProfile
//
// BIOME -> ENVIRONMENT PROFILE -> ground / vegetation / decoration / trees.
//
// Every field is a plain scalar so profiles blend the same way the ground
// colours already do: sample per world-column, weight by the biome weights
// from getTransitionInfo(), sum. That means biome transitions are gradual for
// *behaviour* (how many plants, how big the gaps between trees) and not just
// for colour, with no extra transition machinery.
// ---------------------------------------------------------------------------
struct EnvironmentProfile {
    // Ground surface
    float grassDensity     = 1.0f;  // 0..1 fraction of 4px columns that grow a blade
    float grassHeightBase  = 3.0f;  // px
    float grassHeightVar   = 6.0f;  // px added by large/small scale noise
    float soilVariation    = 1.0f;  // strength of large-scale soil mottling
    float litterDensity    = 0.0f;  // leaf litter flecks drawn into the ground mesh

    // Scattered decoration (Decoration objects)
    float undergrowthDensity = 1.0f; // ferns, bushes, roots
    float flowerDensity      = 0.0f;
    float stoneDensity       = 0.3f;
    float decorBaseStep      = 40.0f; // average px between decoration attempts

    // Trees
    float treeDensity      = 1.0f;  // 0 = none. Scales the gap between trunks.
    float clusterTendency  = 0.5f;  // how strongly trees pull into groves
    float clearingTendency = 0.3f;  // how much of the land reads as open
};

class Biome {
public:
    static BiomeTransitionInfo getTransitionInfo(float worldX);
    static BiomeType determineRegionAtWorldX(float worldX, uint32_t worldSeed = 0);
    static BiomeType determineRegion(int chunkX, uint32_t worldSeed = 0);
    static BiomeProperties getProperties(BiomeType type);
    static sf::Color getBlendedGroundColor(float worldX, uint32_t worldSeed = 0);
    static sf::Color getBlendedUndergroundColor(float worldX, uint32_t worldSeed = 0);
    static sf::Color getBlendedSubsoilColor(float worldX, uint32_t worldSeed = 0);
    static sf::Color getBlendedGrassTipColor(float worldX, uint32_t worldSeed = 0);
    static sf::Color getBlendedGrassBaseColor(float worldX, uint32_t worldSeed = 0);

    // Per-biome environmental parameters, and the blended value at a world X.
    static EnvironmentProfile getEnvironmentProfile(BiomeType type);
    static EnvironmentProfile getBlendedEnvironment(float worldX);
};