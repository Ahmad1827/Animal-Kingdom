#pragma once
#include <SFML/Graphics.hpp>
#include "core/Profiler.h"

class DebugOverlay {
private:
    sf::Font font;
    sf::Text text;
    bool visible = false;
    bool showBorders = false;
    bool showRegions = false;
    bool showHeatmaps = false;
    bool showFoliage = true;
    bool showProfiler = false;
    bool showEngineInternals = false;
    bool showGenerationDebug = false;
    bool showKinematicsDebug = false; // F11 Toggle

public:
    DebugOverlay();
    void toggle();
    void toggleBorders();
    void toggleRegions();
    void toggleHeatmaps();
    void toggleFoliage();
    void toggleProfiler();
    void toggleEngineInternals();
    void toggleGenerationDebug();
    void toggleKinematicsDebug(); // F11 Toggle

    bool getVisible() const;
    bool getShowBorders() const;
    bool getShowRegions() const;
    bool getShowHeatmaps() const;
    bool getShowFoliage() const;
    bool getShowGenerationDebug() const;
    bool getShowKinematicsDebug() const;

    void updateInfo(float dt, int chunkIdx, float px, float py, uint32_t seed, const std::string& region, const ProfilerStats& profiler);
    void draw(sf::RenderWindow& window) const;
};