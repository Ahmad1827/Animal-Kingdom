#include "world/Biome.h"
#include <cmath>

BiomeType Biome::determineRegionAtWorldX(float worldX, uint32_t) {
    float shiftedX = worldX + 5000.f;
    float localX = std::fmod(shiftedX, 19000.f);
    if (localX < 0.f) localX += 19000.f;

    if (localX < 7500.f) {
        return BiomeType::Jungle;
    } else if (localX < 13000.f) {
        return BiomeType::Field;
    } else {
        return BiomeType::Desert;
    }
}

BiomeType Biome::determineRegion(int chunkX, uint32_t worldSeed) {
    return determineRegionAtWorldX(chunkX * 2000.f + 1000.f, worldSeed);
}

BiomeProperties Biome::getProperties(BiomeType type) {
    BiomeProperties p;
    p.type = type;

    switch (type) {
        case BiomeType::Jungle:
            p.name = "Tropical Jungle";
            p.vegetationMode = "JUNGLE TREES";
            p.groundColor = sf::Color(18, 62, 22);
            p.undergroundColor = sf::Color(24, 15, 8);
            p.grassTipColor = sf::Color(36, 105, 30);
            p.debugColor = sf::Color(0, 140, 45, 100);
            p.minTreeSpacing = 140.f;
            p.maxTreeSpacing = 140.f;
            break;

        case BiomeType::Field:
            p.name = "Flower Field";
            p.vegetationMode = "FIELD FLOWERS";
            p.groundColor = sf::Color(72, 140, 42);
            p.undergroundColor = sf::Color(60, 44, 22);
            p.grassTipColor = sf::Color(110, 180, 60);
            p.debugColor = sf::Color(210, 235, 120, 100);
            p.minTreeSpacing = 600.f;
            p.maxTreeSpacing = 600.f;
            break;

        case BiomeType::Desert:
        default:
            p.name = "Arid Desert";
            p.vegetationMode = "YELLOW SAND";
            p.groundColor = sf::Color(210, 180, 95);
            p.undergroundColor = sf::Color(160, 130, 65);
            p.grassTipColor = sf::Color(230, 200, 115);
            p.debugColor = sf::Color(230, 175, 60, 120);
            p.minTreeSpacing = 1200.f;
            p.maxTreeSpacing = 1200.f;
            break;
    }
    return p;
}