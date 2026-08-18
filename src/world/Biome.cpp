#include "world/Biome.h"
#include <cmath>
#include <algorithm>

static float smoothstep(float edge0, float edge1, float x) {
    float t = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

BiomeTransitionInfo Biome::getTransitionInfo(float worldX) {
    const float cycleLength = 19000.0f;
    float shiftedX = worldX + 5000.0f;
    float localX = std::fmod(shiftedX, cycleLength);
    if (localX < 0.0f) localX += cycleLength;

    BiomeTransitionInfo info;
    info.jungleWeight = 0.0f;
    info.fieldWeight = 0.0f;
    info.desertWeight = 0.0f;

    if (localX < 5500.0f) {
        info.currentBiome = BiomeType::Jungle;
        info.nextBiome = BiomeType::Jungle;
        info.blendFactor = 0.0f;
        info.jungleWeight = 1.0f;
    } else if (localX < 7500.0f) {
        float t = smoothstep(5500.0f, 7500.0f, localX);
        info.currentBiome = (t < 0.5f) ? BiomeType::Jungle : BiomeType::Field;
        info.nextBiome = (t < 0.5f) ? BiomeType::Field : BiomeType::Jungle;
        info.blendFactor = (t < 0.5f) ? t : (1.0f - t);
        info.jungleWeight = 1.0f - t;
        info.fieldWeight = t;
    } else if (localX < 11000.0f) {
        info.currentBiome = BiomeType::Field;
        info.nextBiome = BiomeType::Field;
        info.blendFactor = 0.0f;
        info.fieldWeight = 1.0f;
    } else if (localX < 13000.0f) {
        float t = smoothstep(11000.0f, 13000.0f, localX);
        info.currentBiome = (t < 0.5f) ? BiomeType::Field : BiomeType::Desert;
        info.nextBiome = (t < 0.5f) ? BiomeType::Desert : BiomeType::Field;
        info.blendFactor = (t < 0.5f) ? t : (1.0f - t);
        info.fieldWeight = 1.0f - t;
        info.desertWeight = t;
    } else if (localX < 17000.0f) {
        info.currentBiome = BiomeType::Desert;
        info.nextBiome = BiomeType::Desert;
        info.blendFactor = 0.0f;
        info.desertWeight = 1.0f;
    } else {
        float t = smoothstep(17000.0f, 19000.0f, localX);
        info.currentBiome = (t < 0.5f) ? BiomeType::Desert : BiomeType::Jungle;
        info.nextBiome = (t < 0.5f) ? BiomeType::Jungle : BiomeType::Desert;
        info.blendFactor = (t < 0.5f) ? t : (1.0f - t);
        info.desertWeight = 1.0f - t;
        info.jungleWeight = t;
    }

    return info;
}

BiomeType Biome::determineRegionAtWorldX(float worldX, uint32_t) {
    return getTransitionInfo(worldX).currentBiome;
}

BiomeType Biome::determineRegion(int chunkX, uint32_t worldSeed) {
    return determineRegionAtWorldX(chunkX * 2000.0f + 1000.0f, worldSeed);
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
            p.minTreeSpacing = 140.0f;
            p.maxTreeSpacing = 140.0f;
            break;

        case BiomeType::Field:
            p.name = "Flower Field";
            p.vegetationMode = "FIELD FLOWERS";
            p.groundColor = sf::Color(72, 140, 42);
            p.undergroundColor = sf::Color(60, 44, 22);
            p.grassTipColor = sf::Color(110, 180, 60);
            p.debugColor = sf::Color(210, 235, 120, 100);
            p.minTreeSpacing = 600.0f;
            p.maxTreeSpacing = 600.0f;
            break;

        case BiomeType::Desert:
        default:
            p.name = "Arid Desert";
            p.vegetationMode = "YELLOW SAND";
            p.groundColor = sf::Color(210, 180, 95);
            p.undergroundColor = sf::Color(160, 130, 65);
            p.grassTipColor = sf::Color(230, 200, 115);
            p.debugColor = sf::Color(230, 175, 60, 120);
            p.minTreeSpacing = 1200.0f;
            p.maxTreeSpacing = 1200.0f;
            break;
    }
    return p;
}

static sf::Color blendColors(const sf::Color& c1, const sf::Color& c2, const sf::Color& c3, float w1, float w2, float w3) {
    float r = c1.r * w1 + c2.r * w2 + c3.r * w3;
    float g = c1.g * w1 + c2.g * w2 + c3.g * w3;
    float b = c1.b * w1 + c2.b * w2 + c3.b * w3;
    float a = c1.a * w1 + c2.a * w2 + c3.a * w3;
    return sf::Color(
        static_cast<sf::Uint8>(std::clamp(r, 0.0f, 255.0f)),
        static_cast<sf::Uint8>(std::clamp(g, 0.0f, 255.0f)),
        static_cast<sf::Uint8>(std::clamp(b, 0.0f, 255.0f)),
        static_cast<sf::Uint8>(std::clamp(a, 0.0f, 255.0f))
    );
}

sf::Color Biome::getBlendedGroundColor(float worldX, uint32_t) {
    BiomeTransitionInfo t = getTransitionInfo(worldX);
    sf::Color cJ = getProperties(BiomeType::Jungle).groundColor;
    sf::Color cF = getProperties(BiomeType::Field).groundColor;
    sf::Color cD = getProperties(BiomeType::Desert).groundColor;
    return blendColors(cJ, cF, cD, t.jungleWeight, t.fieldWeight, t.desertWeight);
}

sf::Color Biome::getBlendedUndergroundColor(float worldX, uint32_t) {
    BiomeTransitionInfo t = getTransitionInfo(worldX);
    sf::Color cJ = getProperties(BiomeType::Jungle).undergroundColor;
    sf::Color cF = getProperties(BiomeType::Field).undergroundColor;
    sf::Color cD = getProperties(BiomeType::Desert).undergroundColor;
    return blendColors(cJ, cF, cD, t.jungleWeight, t.fieldWeight, t.desertWeight);
}

sf::Color Biome::getBlendedGrassTipColor(float worldX, uint32_t) {
    BiomeTransitionInfo t = getTransitionInfo(worldX);
    sf::Color cJ = getProperties(BiomeType::Jungle).grassTipColor;
    sf::Color cF = getProperties(BiomeType::Field).grassTipColor;
    sf::Color cD = getProperties(BiomeType::Desert).grassTipColor;
    return blendColors(cJ, cF, cD, t.jungleWeight, t.fieldWeight, t.desertWeight);
}