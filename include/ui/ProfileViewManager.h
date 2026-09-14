#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <functional>
#include "simulation/SimulationRegistry.h"

class ProfileViewManager {
public:
    enum class ViewType {
        None,
        Character,
        Village,
        Kingdom
    };

    struct HistoryEntry {
        ViewType type = ViewType::None;
        uint32_t id = 0;
    };

    struct ClickableButton {
        sf::FloatRect bounds;
        std::function<void()> onClick;
        bool isHovered = false;
    };

    ProfileViewManager();

    void init(const sf::Font& font);
    bool handleEvent(const sf::Event& event, const sf::RenderWindow& window, const sf::View& letterboxView, sim::SimulationRegistry& reg, sim::EntityID controlledApeId);
    void draw(sf::RenderWindow& window, sim::SimulationRegistry& reg, sim::EntityID controlledApeId);

    void inspectCharacter(sim::EntityID id, bool recordHistory = true);
    void inspectVillage(sim::VillageID id, bool recordHistory = true);
    void inspectKingdom(sim::KingdomID id, bool recordHistory = true);
    void close();

    bool isInspecting() const { return currentView != ViewType::None; }
    sim::EntityID getInspectedApeId() const { return inspectedApeId; }

private:
    const sf::Font* font;
    ViewType currentView;
    sim::EntityID inspectedApeId;
    sim::VillageID selectedVillageId;
    sim::KingdomID selectedKingdomId;

    std::vector<HistoryEntry> navHistory;
    std::vector<ClickableButton> interactiveButtons;

    sf::Vector2f profilePanelPos;
    float panelWidth;
    float panelHeight;

    struct FamilyInfo {
        sim::EntityID liegeId = 0;
        std::string liegeTitle;
        sim::EntityID fatherId = 0;
        sim::EntityID motherId = 0;
        std::vector<sim::EntityID> spouseIds;
        std::vector<sim::EntityID> childrenIds;
        std::vector<sim::EntityID> siblingIds;
    };

    FamilyInfo resolveFamily(sim::EntityID apeId, sim::SimulationRegistry& reg);

    void drawCharacterProfile(sf::RenderWindow& window, sim::EntityID apeId, sim::SimulationRegistry& reg, sim::EntityID controlledApeId);
    void drawVillageProfile(sf::RenderWindow& window, sim::VillageID vId, sim::SimulationRegistry& reg);
    void drawKingdomProfile(sf::RenderWindow& window, sim::KingdomID kId, sim::SimulationRegistry& reg, sim::EntityID controlledApeId);

    void drawCloseButton(sf::RenderWindow& window, float x, float y);
    void drawBackButton(sf::RenderWindow& window, float x, float y);
    void drawWrappedText(sf::RenderWindow& window, const std::string& text, float x, float& y, float maxW, unsigned int size, sf::Color col, bool bold = false);
    void registerButton(sf::FloatRect bounds, const std::function<void()>& action);
};