#include "core/DebugOverlay.h"
#include <sstream>
#include <iomanip>
#include <cmath>
#include <iostream>

DebugOverlay::DebugOverlay() : isVisible(false), showBorders(false), showRegions(false), showHeatmaps(false), showFoliage(false), showGenerationDebug(false), showProfiler(false), showEngineInternals(false), showKinematicsDebug(false), showVillageDebug(false) {
    if (!font.loadFromFile("assets/fonts/arial.ttf")) {
        if (!font.loadFromFile("../assets/fonts/arial.ttf")) {
            std::cout << "ERROR: Could not load arial.ttf font for DebugOverlay!\n";
        }
    }
    
    debugText.setFont(font); debugText.setCharacterSize(14); debugText.setFillColor(sf::Color::White); debugText.setOutlineColor(sf::Color::Black); debugText.setOutlineThickness(1.f); debugText.setPosition(10.f, 10.f);
    simText.setFont(font); simText.setCharacterSize(14); simText.setFillColor(sf::Color::Yellow); simText.setOutlineColor(sf::Color::Black); simText.setOutlineThickness(1.f); simText.setPosition(10.f, 400.f);
    dynText.setFont(font); dynText.setCharacterSize(14); dynText.setFillColor(sf::Color::Cyan); dynText.setOutlineColor(sf::Color::Black); dynText.setOutlineThickness(1.f); dynText.setPosition(10.f, 200.f);
    villText.setFont(font); villText.setCharacterSize(14); villText.setFillColor(sf::Color::Green); villText.setOutlineColor(sf::Color::Black); villText.setOutlineThickness(1.f); villText.setPosition(10.f, 500.f);
}

// ... toggles and getters remain same ...
void DebugOverlay::toggle() { isVisible = !isVisible; }
void DebugOverlay::toggleBorders() { showBorders = !showBorders; }
void DebugOverlay::toggleRegions() { showRegions = !showRegions; }
void DebugOverlay::toggleHeatmaps() { showHeatmaps = !showHeatmaps; }
void DebugOverlay::toggleFoliage() { showFoliage = !showFoliage; }
void DebugOverlay::toggleProfiler() { showProfiler = !showProfiler; }
void DebugOverlay::toggleEngineInternals() { showEngineInternals = !showEngineInternals; }
void DebugOverlay::toggleGenerationDebug() { showGenerationDebug = !showGenerationDebug; }
void DebugOverlay::toggleKinematicsDebug() { showKinematicsDebug = !showKinematicsDebug; }
void DebugOverlay::toggleVillageDebug() { showVillageDebug = !showVillageDebug; }
bool DebugOverlay::getVisible() const { return isVisible; }
bool DebugOverlay::getShowBorders() const { return showBorders; }
bool DebugOverlay::getShowRegions() const { return showRegions; }
bool DebugOverlay::getShowHeatmaps() const { return showHeatmaps; }
bool DebugOverlay::getShowFoliage() const { return showFoliage; }
bool DebugOverlay::getShowGenerationDebug() const { return showGenerationDebug; }
bool DebugOverlay::getShowKinematicsDebug() const { return showKinematicsDebug; }
bool DebugOverlay::getShowVillageDebug() const { return showVillageDebug; }

void DebugOverlay::updateInfo(float dt, int chunkIdx, float px, float py, uint32_t seed, const std::string& region, const ProfilerStats& profiler) {
    std::ostringstream ss;
    if (showKinematicsDebug) {
        ss << std::fixed << std::setprecision(2);
        ss << "--- KINEMATICS & PHYSICS DEBUG (F11) ---\nPhys Pos: (" << profiler.playerPos.x << ", " << profiler.playerPos.y << ")\n";
    } else if (showProfiler) {
        ss << std::fixed << std::setprecision(2);
        ss << "--- PERFORMANCE PROFILER (F10) ---\nFPS: " << profiler.fps << " | Frame: " << profiler.frameTime << " ms\n";
    } else {
        ss << "FPS: " << static_cast<int>(profiler.fps) << "\nPos: " << static_cast<int>(px) << ", " << static_cast<int>(py) << "\nChunk: " << chunkIdx << " | Region: " << region << "\n";
    }
    debugText.setString(ss.str());
}

void DebugOverlay::updateSimStats(int simApes, int loadedNPCs, int loadedChunks, uint64_t simTick, int hour, int min, int day, int season, int year) {
    if (!isVisible) return;
    char timeStr[256];
    std::snprintf(timeStr, sizeof(timeStr), "--- DYNASTY SIMULATION ---\nApes: %d | NPCs: %d\nTime: %02d:%02d | Day: %d", simApes, loadedNPCs, hour, min, day);
    simText.setString(std::string(timeStr));
}

void DebugOverlay::updateDynastyStats(const std::string& name, float age, float health, const std::string& dynName, uint64_t id, uint64_t heirId, int livingCount) {
    if (!isVisible) return;
    std::string info = "--- CONTROLLED ENTITY ---\nName: " + name + "\nDynasty: " + dynName + "\nHealth: " + std::to_string(static_cast<int>(health));
    dynText.setString(info);
}

void DebugOverlay::updateVillageStats(const std::string& vName, int pop, int food, int wood, int stone, int idle, int working, int builders, int sleep, int queueSize) {
    if (!showVillageDebug || !isVisible) { villText.setString(""); return; }
    std::string info = "--- VILLAGE OVERLAY (V) ---\n";
    info += "Name: " + vName + " | Pop: " + std::to_string(pop) + "\n";
    info += "Storage -> Food: " + std::to_string(food) + " | Wood: " + std::to_string(wood) + " | Stone: " + std::to_string(stone) + "\n";
    info += "Activity -> Work: " + std::to_string(working) + " | Build: " + std::to_string(builders) + " | Idle: " + std::to_string(idle) + " | Sleep: " + std::to_string(sleep) + "\n";
    info += "Projects: " + std::to_string(queueSize) + "\n";
    villText.setString(info);
}

void DebugOverlay::draw(sf::RenderTarget& target) const {
    if (isVisible) {
        sf::View currentView = target.getView();
        target.setView(target.getDefaultView());
        target.draw(debugText);
        target.draw(simText);
        target.draw(dynText);
        if (showVillageDebug) target.draw(villText);
        target.setView(currentView);
    }
}