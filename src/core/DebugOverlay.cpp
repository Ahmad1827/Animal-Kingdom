#include "core/DebugOverlay.h"
#include "world/Biome.h"
#include <sstream>
#include <iomanip>
#include <cmath>
#include <iostream>

DebugOverlay::DebugOverlay() : isVisible(false), showBorders(false), showRegions(false), showHeatmaps(false), showFoliage(false), showGenerationDebug(false), showProfiler(false), showEngineInternals(false), showKinematicsDebug(false), showVillageDebug(false), showKingdomDebug(false) {
    if (!font.loadFromFile("assets/fonts/arial.ttf")) {
        if (!font.loadFromFile("../assets/fonts/arial.ttf")) {
            std::cout << "ERROR: Could not load arial.ttf font for DebugOverlay!\n";
        }
    }
    debugText.setFont(font); debugText.setCharacterSize(14); debugText.setFillColor(sf::Color::White); debugText.setOutlineColor(sf::Color::Black); debugText.setOutlineThickness(1.f); debugText.setPosition(10.f, 10.f);
    simText.setFont(font); simText.setCharacterSize(14); simText.setFillColor(sf::Color::Yellow); simText.setOutlineColor(sf::Color::Black); simText.setOutlineThickness(1.f); simText.setPosition(10.f, 400.f);
    dynText.setFont(font); dynText.setCharacterSize(14); dynText.setFillColor(sf::Color::Cyan); dynText.setOutlineColor(sf::Color::Black); dynText.setOutlineThickness(1.f); dynText.setPosition(10.f, 200.f);
    villText.setFont(font); villText.setCharacterSize(14); villText.setFillColor(sf::Color::Green); villText.setOutlineColor(sf::Color::Black); villText.setOutlineThickness(1.f); villText.setPosition(10.f, 500.f);
    historyText.setFont(font); historyText.setCharacterSize(14); historyText.setFillColor(sf::Color::Magenta); historyText.setOutlineColor(sf::Color::Black); historyText.setOutlineThickness(1.f); historyText.setPosition(800.f, 10.f);
    kingdomText.setFont(font); kingdomText.setCharacterSize(14); kingdomText.setFillColor(sf::Color::Red); kingdomText.setOutlineColor(sf::Color::Black); kingdomText.setOutlineThickness(1.f); kingdomText.setPosition(800.f, 200.f);
    warfareText.setFont(font); warfareText.setCharacterSize(14); warfareText.setFillColor(sf::Color::Yellow); warfareText.setOutlineColor(sf::Color::Black); warfareText.setOutlineThickness(1.f); warfareText.setPosition(400.f, 200.f);
}

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
void DebugOverlay::toggleKingdomDebug() { showKingdomDebug = !showKingdomDebug; }
bool DebugOverlay::getVisible() const { return isVisible; }
bool DebugOverlay::getShowBorders() const { return showBorders; }
bool DebugOverlay::getShowRegions() const { return showRegions; }
bool DebugOverlay::getShowHeatmaps() const { return showHeatmaps; }
bool DebugOverlay::getShowFoliage() const { return showFoliage; }
bool DebugOverlay::getShowGenerationDebug() const { return showGenerationDebug; }
bool DebugOverlay::getShowKinematicsDebug() const { return showKinematicsDebug; }
bool DebugOverlay::getShowVillageDebug() const { return showVillageDebug; }
bool DebugOverlay::getShowKingdomDebug() const { return showKingdomDebug; }

void DebugOverlay::updateInfo(float dt, int chunkIdx, float px, float py, uint32_t seed, const std::string& region, const ProfilerStats& profiler) {
    EnvironmentalMetrics m = Biome::getMetrics(px, seed);
    BiomeWeights bw = Biome::getWeights(px, seed);
    BiomeProperties bp = Biome::getProperties(bw.getDominantBiome());

    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2);
    ss << "=== BIOME DEBUG ===\n";
    ss << "Current Biome: " << bp.name << " (ID: " << static_cast<int>(bw.getDominantBiome()) << ")\n";
    ss << "Temperature:   " << m.temperature << "\n";
    ss << "Moisture:      " << m.moisture << "\n";
    ss << "Elevation:     " << m.elevation << "\n";
    ss << "Fertility:     " << m.fertility << "\n\n";
    ss << "--- INFLUENCE WEIGHTS ---\n";
    ss << "Jungle: " << bw.get(BiomeType::Jungle) * 100.f << "%\n";
    ss << "Field:  " << bw.get(BiomeType::Field) * 100.f << "%\n";
    ss << "Desert: " << bw.get(BiomeType::Desert) * 100.f << "%\n";
    ss << "Hills:  " << bw.get(BiomeType::Hills) * 100.f << "%\n";
    ss << "Mount:  " << bw.get(BiomeType::Mountain) * 100.f << "%\n\n";
    ss << "FPS: " << static_cast<int>(profiler.fps) << " | PosX: " << static_cast<int>(px) << "\n";
    
    debugText.setString(ss.str());
}

void DebugOverlay::updateSimStats(int simApes, int loadedNPCs, int loadedChunks, uint64_t simTick, int hour, int min, int day, int season, int year, int activeEvents) {
    if (!isVisible) return;
    std::string sName = (season == 0) ? "Spring" : (season == 1) ? "Summer" : (season == 2) ? "Autumn" : "Winter";
    char timeStr[256];
    std::snprintf(timeStr, sizeof(timeStr), "--- DYNASTY SIMULATION ---\nApes: %d | NPCs: %d\nTime: %02d:%02d | %s Y%d D%d\nActive Events: %d", simApes, loadedNPCs, hour, min, sName.c_str(), year, day, activeEvents);
    simText.setString(std::string(timeStr));
}

void DebugOverlay::updateDynastyStats(const std::string& name, float age, float health, const std::string& dynName, uint64_t id, uint64_t heirId, int livingCount) {
    if (!isVisible) return;
    std::string info = "--- CONTROLLED ENTITY ---\nName: " + name + "\nDynasty: " + dynName + "\nHealth: " + std::to_string(static_cast<int>(health));
    dynText.setString(info);
}

void DebugOverlay::updateVillageStats(const std::string& vName, int pop, int food, int wood, int stone, int idle, int working, int builders, int sleep, int queueSize, int knownTribes, float radius, int tools, bool migrating) {
    if (!showVillageDebug || !isVisible) { villText.setString(""); return; }
    std::string info = "--- VILLAGE OVERLAY (V) ---\n";
    info += "Name: " + vName + (migrating ? " [MIGRATING]" : "") + " | Pop: " + std::to_string(pop) + " | Radius: " + std::to_string(static_cast<int>(radius)) + "\n";
    info += "Storage -> Food: " + std::to_string(food) + " | Wood: " + std::to_string(wood) + " | Stone: " + std::to_string(stone) + " | Tools: " + std::to_string(tools) + "\n";
    info += "Activity -> Work: " + std::to_string(working) + " | Build: " + std::to_string(builders) + " | Idle: " + std::to_string(idle) + " | Sleep: " + std::to_string(sleep) + "\n";
    info += "Projects: " + std::to_string(queueSize) + " | Known Tribes: " + std::to_string(knownTribes) + "\n";
    villText.setString(info);
}

void DebugOverlay::updateHistory(const std::string& recentHistory) {
    if (!isVisible) return;
    historyText.setString("--- WORLD HISTORY ---\n" + recentHistory);
}

void DebugOverlay::updateKingdomStats(const std::string& kName, const std::string& kingName, const std::string& dynName, int villages, int pop, int treasury, float influence, int military, int knownK) {
    if (!showKingdomDebug || !isVisible) { kingdomText.setString(""); return; }
    std::string info = "--- KINGDOM OVERLAY (K) ---\n";
    info += "Kingdom: " + kName + "\n";
    info += "Ruler: " + kingName + " of " + dynName + "\n";
    info += "Villages: " + std::to_string(villages) + " | Pop: " + std::to_string(pop) + "\n";
    info += "Treasury: " + std::to_string(treasury) + " | Military: " + std::to_string(military) + "\n";
    info += "Influence: " + std::to_string(static_cast<int>(influence)) + " | Known Kingdoms: " + std::to_string(knownK) + "\n";
    kingdomText.setString(info);
}

void DebugOverlay::draw(sf::RenderTarget& target) const {
    if (isVisible) {
        sf::View currentView = target.getView();
        target.setView(target.getDefaultView());
        target.draw(debugText);
        target.draw(simText);
        target.draw(dynText);
        target.draw(historyText);
        if (showVillageDebug) target.draw(villText);
        if (showKingdomDebug) target.draw(kingdomText);
        if (showWarfareDebug) target.draw(warfareText);
        target.setView(currentView);
    }
}

void DebugOverlay::toggleWarfareDebug() { showWarfareDebug = !showWarfareDebug; }
bool DebugOverlay::getShowWarfareDebug() const { return showWarfareDebug; }

void DebugOverlay::updateWarfareStats(const std::string& info) {
    if (!showWarfareDebug || !isVisible) { warfareText.setString(""); return; }
    warfareText.setString(info);
}