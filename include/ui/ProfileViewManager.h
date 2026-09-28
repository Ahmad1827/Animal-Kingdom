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

    enum class CharacterTab {
        Overview = 0,
        Kinship = 1,
        Realm = 2
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
    CharacterTab activeTab;
    sim::EntityID inspectedApeId;
    sim::VillageID selectedVillageId;
    sim::KingdomID selectedKingdomId;

    bool isDragging;
    sf::Vector2f dragOffset;
    sf::Vector2f profilePanelPos;
    float panelWidth;
    float panelHeight;
    sf::FloatRect headerDragBounds;

    std::vector<HistoryEntry> navHistory;
    std::vector<ClickableButton> interactiveButtons;

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

    void drawHeader(sf::RenderWindow& window, const std::string& title, const std::string& subtitle, sf::Color titleColor = sf::Color(255, 225, 130));
    void drawTabs(sf::RenderWindow& window);
    void drawStatBox(sf::RenderWindow& window, float x, float y, float w, float h, const std::string& label, int val, sf::Color accent);
    void drawOpinionBar(sf::RenderWindow& window, float x, float y, float w, int opinion);
    void drawWrappedText(sf::RenderWindow& window, const std::string& text, float x, float& y, float maxW, unsigned int size, sf::Color col, bool bold = false);
    void registerButton(sf::FloatRect bounds, const std::function<void()>& action);
};