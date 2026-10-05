#pragma once
// ---------------------------------------------------------------------------
// DynastyUI - the clan hall: one modal screen with four tabs that all look at
// the same "followed" ape.
//
//   C  Ape         who the followed ape is
//   F  Bloodline   their parents, mates, siblings and children
//   U  Succession  who inherits, and who would rather it were someone else
//   O  Council     the four seats, and seating the followed ape on one
//
// Picking an ape anywhere (roster, tree, heir list, council seat) follows them,
// so the tabs are four views of one selection rather than four separate screens.
// ---------------------------------------------------------------------------
#include <SFML/Graphics.hpp>
#include <functional>
#include <unordered_map>
#include <vector>
#include <string>
#include "dynasty/Character.h"
#include "dynasty/Dynasty.h"
#include "dynasty/Clan.h"
#include "dynasty/Faction.h"
#include "ui/UIKit.h"

namespace sim {

enum class DynastyUIMode {
    CLOSED,
    CHARACTER_VIEW,
    FAMILY_TREE_VIEW,
    SUCCESSION_VIEW,
    COUNCIL_VIEW
};

class DynastyUI {
public:
    DynastyUI();
    void init(const sf::Font& font);

    // Opens on that tab, switches to it, or closes if it is already showing.
    void toggle(DynastyUIMode mode);
    void close();
    bool isOpen() const { return currentMode != DynastyUIMode::CLOSED; }
    DynastyUIMode getMode() const { return currentMode; }

    // The hotkey that opens a tab from the world, or CLOSED if the key is not one.
    static DynastyUIMode modeForKey(sf::Keyboard::Key key);

    // mouseUi is the cursor in the 1280x720 UI space. Returns true if the event was used.
    bool handleEvent(const sf::Event& event, sf::Vector2f mouseUi);

    // Expects the UI view to be set on the window.
    void render(
        sf::RenderWindow& window,
        const Dynasty& dynasty,
        Clan& clan,
        const std::unordered_map<Character::ID, Character>& registry,
        const std::vector<Faction>& factions,
        Character::ID currentAlphaId
    );

private:
    using Registry = std::unordered_map<Character::ID, Character>;

    struct Hit {
        sf::FloatRect bounds;
        std::function<void()> action;
    };

    // Everything a tab needs for one frame.
    struct Frame {
        ui::Canvas&                 c;
        sf::Vector2f                mouse;
        const Dynasty&              dynasty;
        const Clan&                 clan;
        const Registry&             registry;
        const std::vector<Faction>& factions;
        Character::ID               alphaId;
        Character::ID               heirId;
        const Character*            focus;
    };

    const sf::Font* font = nullptr;
    DynastyUIMode currentMode = DynastyUIMode::CLOSED;
    Character::ID focusId = Character::INVALID_ID;      // the followed ape; falls back to the Alpha
    std::vector<Character::ID> roster;                  // kin shown last frame, in list order
    int rosterScroll = 0;
    bool revealFocus = true;                            // scroll the roster to the followed ape
    std::vector<Hit> hits;                              // rebuilt every frame by render

    // A council change asked for by a click, applied at the start of the next frame.
    bool            seatChangePending = false;
    CouncilPosition seatChangePos = CouncilPosition::NONE;
    Character::ID   seatChangeHolder = Character::INVALID_ID;

    bool        hasTip = false;
    ui::Tooltip tip;
    sf::Vector2f tipAnchor;

    void follow(Character::ID id);
    void stepFocus(int delta);
    void stepTab(int delta);
    void onClick(sf::FloatRect bounds, std::function<void()> action);
    void showTip(const ui::Tooltip& t, sf::Vector2f anchor);
    void applySeatChange(Clan& clan);

    std::string rankOf(const Frame& f, const Character& ch) const;
    float section(ui::Canvas& c, sf::FloatRect r, const std::string& title, const std::string& aside = "");

    void drawRoster(const Frame& f, sf::FloatRect area);
    void drawApeTab(const Frame& f, sf::FloatRect area);
    void drawBloodlineTab(const Frame& f, sf::FloatRect area);
    void drawSuccessionTab(const Frame& f, sf::FloatRect area);
    void drawCouncilTab(const Frame& f, sf::FloatRect area);
};

}
