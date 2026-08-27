#pragma once
#include <cstdint>

class SeedManager {
public:
    static uint32_t getChunkSeed(uint32_t worldSeed, int chunkX);
    static float getRandomFloat(uint32_t& seed, float min, float max);
    static int getRandomInt(uint32_t& seed, int min, int max);

    // ---- Deterministic spatial noise -------------------------------------
    // Everything below is a pure function of (worldX, worldSeed). No state,
    // no per-frame work, safe to call from the async chunk-generation thread.
    // Used to give the environment large-scale spatial coherence (groves,
    // clearings, undergrowth patches) instead of independent random dots.

    // Hash a lattice coordinate. Exposed because callers sometimes want a
    // raw per-position roll that is independent of the noise field.
    static uint32_t hashPosition(int32_t gridPos, uint32_t seed);

    // Smooth 1D value noise in [0,1]. `frequency` is in units of 1/worldpixel,
    // so 0.001f gives features roughly 1000px across.
    static float valueNoise(float x, uint32_t seed, float frequency);

    // Fractal sum of valueNoise, normalised to [0,1]. 2-3 octaves is plenty
    // for what the environment needs.
    static float fbm(float x, uint32_t seed, float baseFrequency, int octaves);
};