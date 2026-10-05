#pragma once
// ---------------------------------------------------------------------------
// ProfileViewManager - the inspect panel. Click an ape in the world, or a
// county or realm on the map, and it opens on that subject.
//
// Every subject has three tabs (keys 1-3):
//   Ape      Ape / Kin / Standing
//   County   Holding / Apes / Stores
//   Realm    Realm / Counties / Diplomacy
//
// Names on the panel are links: following one pushes the current subject and
// tab onto the history, so Back returns to exactly where you were. Each kind of
// subject remembers its own tab, so walking a family through Kin stays in Kin.
// ---------------------------------------------------------------------------
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <functional>
#include "simulation/SimulationRegistry.h"
#include "ui/UIKit.h"

class ProfileViewManager {
public:
    enum class ViewType {
        None,
        Character,
        Village,
        Kingdom
    };

    ProfileViewManager();

    void init(const sf::Font& font);
    bool handleEvent(const sf::Event& event, const sf::RenderWindow& window, const sf::View& letterboxView, sim::SimulationRegistry& reg, sim::EntityID controlledApeId);
    // Expects the UI view to be set on the window.
    void draw(sf::RenderWindow& window, sim::SimulationRegistry& reg, sim::EntityID controlledApeId);

    void inspectCharacter(sim::EntityID id, bool recordHistory = true);
    void inspectVillage(sim::VillageID id, bool recordHistory = true);
    void inspectKingdom(sim::KingdomID id, bool recordHistory = true);
    void close();

    bool isInspecting() const { return currentView != ViewType::None; }
    sim::EntityID getInspectedApeId() const { return currentView == ViewType::Character ? subjectId : 0; }

private:
    static const int TAB_COUNT = 3;

    struct HistoryEntry {
        ViewType type = ViewType::None;
        uint64_t id = 0;
        int      tab = 0;
    };

    struct ClickableButton {
        sf::FloatRect bounds;
        std::function<void()> onClick;
    };

    // One line of a scrolling list. Rows with an action are links.
    struct ListRow {
        std::string title;
        std::string detail;                     // muted line under the title
        std::string tag;                        // right-aligned, optional
        sf::Color   tagColor = ui::theme::TextMuted;
        std::function<void()> action;
        bool        dim = false;                // unknown or dead: drawn muted
    };

    struct FamilyInfo {
        sim::EntityID liegeId = 0;
        std::string liegeTitle;
        sim::EntityID fatherId = 0;
        sim::EntityID motherId = 0;
        std::vector<sim::EntityID> spouseIds;
        std::vector<sim::EntityID> childrenIds;
        std::vector<sim::EntityID> siblingIds;
    };

    const sf::Font* font;
    ViewType currentView;
    uint64_t subjectId;                         // ape, village or kingdom id, by currentView
    int tabByView[4];                           // last tab used for each ViewType
    int listScroll;                             // first visible row of the open list

    bool isDragging;
    sf::Vector2f dragOffset;
    sf::Vector2f profilePanelPos;
    float panelWidth;
    float panelHeight;

    std::vector<HistoryEntry> navHistory;
    std::vector<ClickableButton> interactiveButtons;    // rebuilt every frame by draw

    sf::Vector2f mouse;                         // cursor in UI space, this frame
    bool        hasTip = false;
    ui::Tooltip tip;
    sf::Vector2f tipAnchor;

    int& activeTab() { return tabByView[static_cast<int>(currentView)]; }
    void open(ViewType type, uint64_t id, bool recordHistory);
    void goBack();
    void setTab(int tab);

    FamilyInfo resolveFamily(sim::EntityID apeId, sim::SimulationRegistry& reg);
    void appointToCouncil(sim::SimulationRegistry& reg, sim::EntityID apeId, sim::CouncilRole role);

    void drawCharacterProfile(ui::Canvas& c, sim::EntityID apeId, sim::SimulationRegistry& reg, sim::EntityID controlledApeId);
    void drawVillageProfile(ui::Canvas& c, sim::VillageID vId, sim::SimulationRegistry& reg, sim::EntityID controlledApeId);
    void drawKingdomProfile(ui::Canvas& c, sim::KingdomID kId, sim::SimulationRegistry& reg, sim::EntityID controlledApeId);

    // Panel chrome. Returns the content area under the tab strip.
    sf::FloatRect drawChrome(ui::Canvas& c, const std::string& title, const std::string& subtitle,
                             const char* const tabNames[TAB_COUNT], sf::Color banner);
    float heading(ui::Canvas& c, sf::FloatRect area, float y, const std::string& title, const std::string& aside = "");
    float fact(ui::Canvas& c, sf::FloatRect area, float y, const std::string& label, const std::string& value,
               sf::Color valueColor = ui::theme::Text);
    float linkCard(ui::Canvas& c, sf::FloatRect area, float y, const std::string& label, const std::string& title,
                   const std::string& detail, const std::function<void()>& action);
    void drawList(ui::Canvas& c, sf::FloatRect area, float y, const std::vector<ListRow>& rows, const std::string& emptyText);
    void showTip(const ui::Tooltip& t, sf::Vector2f anchor);
    void registerButton(sf::FloatRect bounds, const std::function<void()>& action);
};
