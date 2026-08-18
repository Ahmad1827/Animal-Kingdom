#include "core/DebugOverlay.h"
#include "world/Biome.h"
#include <sstream>
#include <iomanip>
#include <cmath>
#include <iostream>

DebugOverlay::DebugOverlay() : isVisible(false), showBorders(false), showRegions(false), showHeatmaps(false), showFoliage(false), showGenerationDebug(false), showProfiler(false), showEngineInternals(false), showKinematicsDebug(false), showVillageDebug(false), showKingdomDebug(false), showWarfareDebug(false) {
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

void DebugOverlay::updateInfo(float, int chunkIdx, float px, float py, uint32_t, const std::string&, const ProfilerStats& profiler) {
    BiomeTransitionInfo t = Biome::getTransitionInfo(px);
    BiomeProperties bp = Biome::getProperties(t.currentBiome);
    BiomeProperties nextBp = Biome::getProperties(t.nextBiome);

    std::ostringstream ss;
    ss << std::fixed << std::setprecision(1);
    ss << "=== BIOME TRANSITION DEBUG ===\n";
    ss << "CURRENT BIOME: " << bp.name << "\n";
    if (t.currentBiome != t.nextBiome && t.blendFactor > 0.01f) {
        ss << "NEXT BIOME:    " << nextBp.name << " (" << static_cast<int>(t.blendFactor * 100.0f) << "%)\n";
    } else {
        ss << "STATUS:        CORE REGION (100%)\n";
    }
    ss << "WEIGHTS -> Jungle: " << (t.jungleWeight * 100.0f) << "% | Field: " << (t.fieldWeight * 100.0f) << "% | Desert: " << (t.desertWeight * 100.0f) << "%\n\n";
    ss << "--- POSITION ---\n";
    ss << "Player X: " << static_cast<int>(px) << " | Y: " << static_cast<int>(py) << "\n";
    ss << "Chunk: " << chunkIdx << " | FPS: " << static_cast<int>(profiler.fps) << "\n";

    debugText.setString(ss.str());
}

void DebugOverlay::updateSimStats(int simApes, int loadedNPCs, int loadedChunks, uint64_t, int hour, int min, int day, int season, int year, int activeEvents) {
    if (!isVisible) return;
    std::string sName = (season == 0) ? "Spring" : (season == 1) ? "Summer" : (season == 2) ? "Autumn" : "Winter";
    char timeStr[256];
    std::snprintf(timeStr, sizeof(timeStr), "--- DYNASTY SIMULATION ---\nApes: %d | NPCs: %d\nTime: %02d:%02d | %s Y%d D%d\nActive Events: %d", simApes, loadedNPCs, hour, min, sName.c_str(), year, day, activeEvents);
    simText.setString(std::string(timeStr));
}

void DebugOverlay::updateDynastyStats(const std::string& name, float, float health, const std::string& dynName, uint64_t, uint64_t, int) {
    if (!isVisible) return;
    std::string info = "--- CONTROLLED ENTITY ---\nName: " + name + "\nDynasty: " + dynName + "\nHealth: " + std::to_string(static_cast<int>(health));
    dynText.setString(info);
}

void DebugOverlay::updateVillageStats(const std::string& vName, int pop, int food, int wood, int stone, int idle, int working, int builders, int sleep, int queueSize, int knownTribes, float radius, int, bool migrating) {
    if (!showVillageDebug || !isVisible) { villText.setString(""); return; }
    std::string info = "--- VILLAGE OVERLAY (V) ---\n";
    info += "Name: " + vName + (migrating ? " [MIGRATING]" : "") + " | Pop: " + std::to_string(pop) + " | Radius: " + std::to_string(static_cast<int>(radius)) + "\n";
    info += "Storage -> Food: " + std::to_string(food) + " | Wood: " + std::to_string(wood) + " | Stone: " + std::to_string(stone) + "\n";
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
    sf::View currentView = target.getView();
    target.setView(target.getDefaultView());

    if (showRegions) {
        float centerX = currentView.getCenter().x;
        BiomeTransitionInfo t = Biome::getTransitionInfo(centerX);
        BiomeProperties bp = Biome::getProperties(t.currentBiome);
        BiomeProperties nextBp = Biome::getProperties(t.nextBiome);

        sf::Color cJ = Biome::getProperties(BiomeType::Jungle).debugColor;
        sf::Color cF = Biome::getProperties(BiomeType::Field).debugColor;
        sf::Color cD = Biome::getProperties(BiomeType::Desert).debugColor;

        float r = cJ.r * t.jungleWeight + cF.r * t.fieldWeight + cD.r * t.desertWeight;
        float g = cJ.g * t.jungleWeight + cF.g * t.fieldWeight + cD.g * t.desertWeight;
        float b = cJ.b * t.jungleWeight + cF.b * t.fieldWeight + cD.b * t.desertWeight;

        sf::RectangleShape screenShade(sf::Vector2f(static_cast<float>(target.getSize().x), static_cast<float>(target.getSize().y)));
        screenShade.setPosition(0.0f, 0.0f);
        screenShade.setFillColor(sf::Color(static_cast<sf::Uint8>(r), static_cast<sf::Uint8>(g), static_cast<sf::Uint8>(b), 80));
        target.draw(screenShade);

        std::string labelText = "[" + bp.name + "]";
        if (t.currentBiome != t.nextBiome && t.blendFactor > 0.01f) {
            labelText += " -> [" + nextBp.name + " " + std::to_string(static_cast<int>(t.blendFactor * 100.0f)) + "%]";
        }

        sf::Text banner(labelText, font, 24);
        banner.setFillColor(sf::Color::White);
        banner.setOutlineColor(sf::Color::Black);
        banner.setOutlineThickness(2.0f);
        sf::FloatRect bounds = banner.getLocalBounds();
        banner.setOrigin(bounds.left + bounds.width / 2.0f, 0.0f);
        banner.setPosition(target.getSize().x / 2.0f, 20.0f);
        target.draw(banner);
    }

    if (isVisible) {
        target.draw(debugText);
        target.draw(simText);
        target.draw(dynText);
        target.draw(historyText);
        if (showVillageDebug) target.draw(villText);
        if (showKingdomDebug) target.draw(kingdomText);
        if (showWarfareDebug) target.draw(warfareText);
    }

    target.setView(currentView);
}

void DebugOverlay::toggleWarfareDebug() { showWarfareDebug = !showWarfareDebug; }
bool DebugOverlay::getShowWarfareDebug() const { return showWarfareDebug; }

void DebugOverlay::updateWarfareStats(const std::string& info) {
    if (!showWarfareDebug || !isVisible) { warfareText.setString(""); return; }
    warfareText.setString(info);
}