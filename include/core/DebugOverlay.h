#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include "core/Profiler.h"

class DebugOverlay {
private:
    sf::Font font;
    sf::Text text;
    bool isVisible;
    bool fontLoaded;
    float updateTimer;

    bool showChunkBorders;
    bool showRegions;
    bool showHeatmaps;
    bool showFoliage;
    bool showProfiler;
    bool showEngineInternals;

public:
    DebugOverlay();
    void toggleEngineInternals();
    void toggle();
    void toggleBorders();
    void toggleRegions();
    void toggleHeatmaps();
    void toggleFoliage();
    void toggleProfiler();
    
    void updateInfo(float dt, int chunkX, float pX, float pY, uint32_t seed, const std::string& biomeName, const ProfilerStats& profiler);
    void draw(sf::RenderWindow& window) const;
    bool getVisible() const;
    
    bool getShowBorders() const;
    bool getShowRegions() const;
    bool getShowHeatmaps() const;
    bool getShowFoliage() const;
};