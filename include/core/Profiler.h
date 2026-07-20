#pragma once
#include <string>
#include <SFML/System/Vector2.hpp>

struct ProfilerStats {
    float fps = 0.f;
    float frameTime = 0.f;
    float updateTime = 0.f;
    float renderTime = 0.f;
    float physicsTime = 0.f;
    float chunkGenTime = 0.f;
    float cameraTime = 0.f;
    float particleTime = 0.f;
    float lastTerrainGenTime = 0.f;
    float lastTreeGenTime = 0.f;
    float lastChunkGenTime = 0.f;
    float asyncLoadTime = 0.f;
    float collisionTime = 0.f;
    
    int loadedChunks = 0;
    int chunksLoaded = 0; // Legacy support
    int visibleChunks = 0;
    int loadedTrees = 0;
    int visibleTrees = 0;
    int loadedDecorations = 0;
    int visibleDecorations = 0;
    int drawCalls = 0;
    
    int chunksGenerating = 0;
    int chunksQueuedForInsertion = 0;
    int chunksCached = 0;
    int objectsRendered = 0;
    int objectsCulled = 0;
    int particleCount = 0;
    
    std::string weatherString;
    std::string seasonString;
    float timeOfDay = 0.f;

    // Kinematic Debug Data
    sf::Vector2f playerPos;
    sf::Vector2f cameraPos;
    sf::Vector2f cameraTarget;
    float groundHeight = 0.f;
    float verticalVelocity = 0.f;
    bool isGrounded = false;

    void resetPerFrame() {
        objectsRendered = 0;
        objectsCulled = 0;
        visibleChunks = 0;
        visibleTrees = 0;
        visibleDecorations = 0;
        drawCalls = 0;
        loadedTrees = 0;
        loadedDecorations = 0;
        physicsTime = 0.f;
        cameraTime = 0.f;
    }
};