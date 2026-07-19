#pragma once

struct ProfilerStats {
    float fps = 0.f;
    float frameTime = 0.f;
    float updateTime = 0.f;
    float renderTime = 0.f;
    float collisionTime = 0.f;
    float asyncLoadTime = 0.f;
    float particleTime = 0.f;
    
    float lastChunkGenTime = 0.f;
    float lastTerrainGenTime = 0.f;
    float lastTreeGenTime = 0.f;
    
    int objectsRendered = 0;
    int objectsCulled = 0;
    int chunksLoaded = 0;
    int chunksGenerating = 0;
    int chunksCached = 0;
    int chunksQueuedForInsertion = 0;
    
    std::string weatherString = "";
    std::string seasonString = "";
    size_t particleCount = 0;
    float timeOfDay = 0.f;

    void resetPerFrame() {
        objectsRendered = 0;
        objectsCulled = 0;
        chunksLoaded = 0;
        chunksGenerating = 0;
        chunksCached = 0;
        chunksQueuedForInsertion = 0;
    }
};