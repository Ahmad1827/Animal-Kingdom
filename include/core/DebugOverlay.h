#pragma once
#include <SFML/Graphics.hpp>
#include <string>

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

public:
    DebugOverlay();
    void toggle();
    void toggleBorders();
    void toggleRegions();
    void toggleHeatmaps();
    void toggleFoliage();
    
    void updateInfo(float dt, float fps, int chunkX, float pX, float pY, uint32_t seed, const std::string& biomeName, size_t chunks);
    void draw(sf::RenderWindow& window) const;
    bool getVisible() const;
    
    bool getShowBorders() const;
    bool getShowRegions() const;
    bool getShowHeatmaps() const;
    bool getShowFoliage() const;
};