#pragma once
#include <SFML/Graphics.hpp>
#include "core/Profiler.h"
#include <string>

class DebugOverlay {
private:
    sf::Font font;
    sf::Text debugText;
    sf::Text simText;
    sf::Text dynText; 
    bool isVisible;
    bool showBorders;
    bool showRegions;
    bool showHeatmaps;
    bool showFoliage;
    bool showGenerationDebug;
    bool showProfiler;
    bool showEngineInternals;
    bool showKinematicsDebug;

public:
    DebugOverlay();
    
    void updateInfo(float dt, int chunkIdx, float playerX, float playerY, uint32_t seed, const std::string& regionName, const ProfilerStats& profiler);
    void updateSimStats(int simApes, int loadedNPCs, int loadedChunks, uint64_t simTick, int hour, int min, int day, int season, int year);
    void updateDynastyStats(const std::string& name, float age, float health, const std::string& dynName, uint64_t id, uint64_t heirId, int livingCount);
    
    void draw(sf::RenderTarget& target) const;

    void toggle();
    void toggleBorders();
    void toggleRegions();
    void toggleHeatmaps();
    void toggleFoliage();
    void toggleGenerationDebug();
    void toggleProfiler();
    void toggleEngineInternals();
    void toggleKinematicsDebug();

    bool getVisible() const;
    bool getShowBorders() const;
    bool getShowRegions() const;
    bool getShowHeatmaps() const;
    bool getShowFoliage() const;
    bool getShowGenerationDebug() const;
    bool getShowProfiler() const;
    bool getShowEngineInternals() const;
    bool getShowKinematicsDebug() const;
};