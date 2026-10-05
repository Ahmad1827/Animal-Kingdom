#include "dynasty/DynastyUI.h"
#include "dynasty/Succession.h"
#include <algorithm>
#include <cmath>

namespace sim {

namespace {

using namespace ui;

// Screen layout in the 1280x720 UI space. Same frame as the strategic map, so
// the two full screens sit in the same place under the HUD top bar (y < 40).
const sf::FloatRect FRAME(40.f, 44.f, 1200.f, 664.f);
const float HEADER_H = 44.f;
const sf::FloatRect SIDE(50.f, 94.f, 250.f, 604.f);
const sf::FloatRect MAIN(308.f, 94.f, 922.f, 604.f);

struct TabDef {
    DynastyUIMode     mode;
    sf::Keyboard::Key key;
    const char*       keyLabel;
    const char*       name;
    const char*       blurb;
};
const int TAB_COUNT = 4;
const TabDef TABS[TAB_COUNT] = {
    { DynastyUIMode::CHARACTER_VIEW,   sf::Keyboard::C, "C", "Ape",        "Who this ape is, and what they want" },
    { DynastyUIMode::FAMILY_TREE_VIEW, sf::Keyboard::F, "F", "Bloodline",  "Parents, mates, siblings and young" },
    { DynastyUIMode::SUCCESSION_VIEW,  sf::Keyboard::U, "U", "Succession", "Who leads the clan when the Alpha falls" },
    { DynastyUIMode::COUNCIL_VIEW,     sf::Keyboard::O, "O", "Council",    "The four seats beside the Alpha" },
};

struct TraitInfo { const char* name; const char* effect; bool flaw; };
TraitInfo traitInfo(TraitID t) {
    switch (t) {
        case TraitID::SILVERBACK:     return { "Silverback",     "+6 Prowess, +4 Martial. Born to lead a troop.",  false };
        case TraitID::FIERCE_ROAR:    return { "Fierce Roar",    "+3 Prowess, +2 Martial. Cows rivals in a challenge.", false };
        case TraitID::SNEAKY_FORAGER: return { "Sneaky Forager", "+5 Intrigue, +2 Stewardship. Takes more than a share.", false };
        case TraitID::WISE_ELDER:     return { "Wise Elder",     "+6 Diplomacy, +2 Stewardship. Steadies the clan.", false };
        case TraitID::NATURAL_LEADER: return { "Natural Leader", "+4 Diplomacy, +2 Martial. Apes follow willingly.", false };
        case TraitID::AMBITIOUS:      return { "Ambitious",      "+3 Intrigue, +2 Martial. Wants power, gathers factions.", true };
        case TraitID::LOYAL:          return { "Loyal",          "+2 Diplomacy. Slow to turn on the Alpha.",        false };
        case TraitID::COWARD:         return { "Coward",         "-5 Prowess, -4 Martial. Flees from danger.",      true };
        case TraitID::FICKLE_GROOMER: return { "Fickle Groomer", "Loyalty and opinions swing without warning.",     true };
    }
    return { "Unknown", "", false };
}

struct SeatInfo {
    CouncilPosition pos;
    const char*     title;
    const char*     duty;
    const char*     bonus;      // what the seat adds to, lower case
};
const int SEAT_COUNT = 4;
const SeatInfo SEATS[SEAT_COUNT] = {
    { CouncilPosition::WAR_CHANTER,   "War-Chanter",   "Drums the troop to war and leads the hunt.",       "war morale" },
    { CouncilPosition::FORAGER_CHIEF, "Forager Chief", "Decides where the clan forages and what is stored.", "foraging" },
    { CouncilPosition::WISE_ELDER,    "Wise Elder",    "Settles quarrels before they become feuds.",        "stability" },
    { CouncilPosition::WHISPERER,     "Whisperer",     "Listens for plots against the Alpha.",              "plot watch" },
};

struct LawInfo { SuccessionLaw law; const char* name; const char* who; const char* note; };
const LawInfo LAWS[3] = {
    { SuccessionLaw::BLOODLINE_PRIMOGENITURE, "Bloodline", "Eldest child",  "The Alpha's own young come first, then siblings." },
    { SuccessionLaw::ELDER_SENIORITY,         "Elders",    "Oldest kin",    "The oldest living ape of the bloodline leads." },
    { SuccessionLaw::RIGHT_OF_THE_STRONGEST,  "Strongest", "Mightiest kin", "Prowess and martial skill decide who leads." },
};

std::string upper(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char ch) { return static_cast<char>(std::toupper(ch)); });
    return s;
}

std::string signedInt(int v) { return (v >= 0 ? "+" : "") + std::to_string(v); }

sf::Color opinionColor(int v) { return v > 0 ? theme::Good : (v < 0 ? theme::Bad : theme::TextMuted); }

sf::Color tensionColor(int t) { return t < 30 ? theme::Good : (t < 60 ? theme::Amber : theme::Bad); }
const char* tensionWord(int t) { return t < 30 ? "Calm" : (t < 60 ? "Uneasy" : (t < 85 ? "Restless" : "Breaking")); }
const char* tensionNote(int t) {
    if (t < 30) return "The clan grooms, eats and sleeps together. Few would back a challenger.";
    if (t < 60) return "Old grudges are being remembered. A slighted ape may start to gather friends.";
    if (t < 85) return "Apes are choosing sides. Factions grow quickly while the clan is like this.";
    return "The clan is close to splitting. A challenge to the Alpha could come any day.";
}

const char* factionGoal(FactionType t) {
    switch (t) {
        case FactionType::CLAIMANT_FOR_ALPHA:   return "Wants a new Alpha";
        case FactionType::CHANGE_LAW_SENIORITY: return "Wants the elders to inherit";
        case FactionType::CHANGE_LAW_STRONGEST: return "Wants the strongest to inherit";
        case FactionType::COUNCIL_MALCONTENT:   return "Wants a seat on the council";
    }
    return "";
}

const SeatInfo* seatHeldBy(const Clan& clan, Character::ID id) {
    if (id == Character::INVALID_ID) return nullptr;
    for (const auto& seat : SEATS) {
        if (clan.getCouncilMember(seat.pos) == id) return &seat;
    }
    return nullptr;
}

// The stat line and clan bonus a holder brings to a seat. Mirrors Clan::calculateModifiers.
float seatBonus(CouncilPosition pos, const CharacterStats& s, std::string& statLine) {
    switch (pos) {
        case CouncilPosition::WAR_CHANTER:
            statLine = "Martial " + std::to_string(s.martial) + ", Prowess " + std::to_string(s.prowess);
            return static_cast<float>(s.martial + s.prowess) * 0.02f;
        case CouncilPosition::FORAGER_CHIEF:
            statLine = "Stewardship " + std::to_string(s.stewardship);
            return static_cast<float>(s.stewardship) * 0.03f;
        case CouncilPosition::WISE_ELDER:
            statLine = "Diplomacy " + std::to_string(s.diplomacy);
            return static_cast<float>(s.diplomacy) * 0.025f;
        case CouncilPosition::WHISPERER:
            statLine = "Intrigue " + std::to_string(s.intrigue);
            return static_cast<float>(s.intrigue) * 0.04f;
        default:
            break;
    }
    statLine.clear();
    return 0.f;
}

std::string percent(float bonus) {
    return "+" + std::to_string(static_cast<int>(std::lround(bonus * 100.f))) + "%";
}

const Character* lookup(const std::unordered_map<Character::ID, Character>& registry, Character::ID id) {
    if (id == Character::INVALID_ID) return nullptr;
    auto it = registry.find(id);
    return it != registry.end() ? &it->second : nullptr;
}

} // namespace

DynastyUI::DynastyUI() = default;

void DynastyUI::init(const sf::Font& loadedFont) {
    font = &loadedFont;
}

void DynastyUI::toggle(DynastyUIMode mode) {
    currentMode = (currentMode == mode) ? DynastyUIMode::CLOSED : mode;
    revealFocus = true;
    hits.clear();
}

void DynastyUI::close() {
    currentMode = DynastyUIMode::CLOSED;
    hits.clear();
}

DynastyUIMode DynastyUI::modeForKey(sf::Keyboard::Key key) {
    for (const auto& tab : TABS) {
        if (tab.key == key) return tab.mode;
    }
    return DynastyUIMode::CLOSED;
}

void DynastyUI::follow(Character::ID id) {
    focusId = id;
    revealFocus = true;
}

void DynastyUI::stepFocus(int delta) {
    if (roster.empty()) return;
    const int n = static_cast<int>(roster.size());
    const auto it = std::find(roster.begin(), roster.end(), focusId);
    const int at = (it == roster.end()) ? 0 : static_cast<int>(it - roster.begin());
    follow(roster[static_cast<size_t>(((at + delta) % n + n) % n)]);
}

void DynastyUI::stepTab(int delta) {
    for (int i = 0; i < TAB_COUNT; ++i) {
        if (TABS[i].mode == currentMode) {
            currentMode = TABS[((i + delta) % TAB_COUNT + TAB_COUNT) % TAB_COUNT].mode;
            return;
        }
    }
}

void DynastyUI::onClick(sf::FloatRect bounds, std::function<void()> action) {
    hits.push_back({ bounds, std::move(action) });
}

void DynastyUI::showTip(const ui::Tooltip& t, sf::Vector2f anchor) {
    hasTip = true;
    tip = t;
    tipAnchor = anchor;
}

void DynastyUI::applySeatChange(Clan& clan) {
    if (!seatChangePending) return;
    seatChangePending = false;
    if (seatChangeHolder != Character::INVALID_ID) {
        // An ape sits on one seat at a time.
        for (const auto& seat : SEATS) {
            if (clan.getCouncilMember(seat.pos) == seatChangeHolder) clan.assignCouncil(seat.pos, Character::INVALID_ID);
        }
    }
    clan.assignCouncil(seatChangePos, seatChangeHolder);
}

bool DynastyUI::handleEvent(const sf::Event& event, sf::Vector2f mouseUi) {
    if (!isOpen()) return false;

    if (event.type == sf::Event::KeyPressed) {
        const sf::Keyboard::Key key = event.key.code;
        if (key == sf::Keyboard::Escape) { close(); return true; }
        const DynastyUIMode mode = modeForKey(key);
        if (mode != DynastyUIMode::CLOSED) { toggle(mode); return true; }
        if (key == sf::Keyboard::Left)  { stepTab(-1); return true; }
        if (key == sf::Keyboard::Right) { stepTab(1); return true; }
        if (key == sf::Keyboard::Up)    { stepFocus(-1); return true; }
        if (key == sf::Keyboard::Down)  { stepFocus(1); return true; }
        return false;
    }

    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        // Later hits are drawn on top, so they win.
        for (auto it = hits.rbegin(); it != hits.rend(); ++it) {
            if (it->bounds.contains(mouseUi)) {
                const std::function<void()> action = it->action;
                if (action) action();
                return true;
            }
        }
        if (!FRAME.contains(mouseUi)) close();
        return true;
    }

    if (event.type == sf::Event::MouseWheelScrolled && SIDE.contains(mouseUi)) {
        rosterScroll -= (event.mouseWheelScroll.delta > 0.f) ? 1 : -1;
        revealFocus = false;
        return true;
    }

    return false;
}

std::string DynastyUI::rankOf(const Frame& f, const Character& ch) const {
    if (!ch.isAlive) return "Deceased";
    if (ch.id == f.alphaId) return "Alpha";
    if (const SeatInfo* seat = seatHeldBy(f.clan, ch.id)) return seat->title;
    if (ch.id == f.heirId) return "Heir";
    return ch.age < 12 ? "Juvenile" : "Kin";
}

float DynastyUI::section(ui::Canvas& c, sf::FloatRect r, const std::string& title, const std::string& aside) {
    c.inset(r);
    // title band: a strip of warm light across the top of the box
    const float u = c.px(1);
    c.gradient({r.left + u, r.top + u, r.width - 2.f * u, 26.f - u}, sf::Color(255, 214, 140, 34), sf::Color(255, 214, 140, 6));
    c.heading(r.left + 10.f, r.top + 9.f, r.width - 20.f, title, aside);
    return r.top + 36.f;
}

void DynastyUI::render(
    sf::RenderWindow& window,
    const Dynasty& dynasty,
    Clan& clan,
    const Registry& registry,
    const std::vector<Faction>& factions,
    Character::ID currentAlphaId
) {
    if (currentMode == DynastyUIMode::CLOSED || !font) return;

    applySeatChange(clan);
    hits.clear();
    hasTip = false;

    // The Alpha heads the list, then the rest of the bloodline in birth order.
    roster.clear();
    if (registry.count(currentAlphaId)) roster.push_back(currentAlphaId);
    for (Character::ID id : dynasty.memberIds) {
        if (registry.count(id) && std::find(roster.begin(), roster.end(), id) == roster.end()) roster.push_back(id);
    }
    if (std::find(roster.begin(), roster.end(), focusId) == roster.end()) {
        focusId = roster.empty() ? Character::INVALID_ID : roster.front();
    }

    Canvas c(window, *font);
    const sf::Vector2f mouse = Canvas::mouse(window);

    // Dim the world but leave the HUD top bar readable.
    c.fill({c.left(), 40.f, c.right() - c.left(), c.bottom() - 40.f}, sf::Color(8, 7, 6, 190));
    c.panel(FRAME);

    // ---- header: clan name, tabs ----------------------------------------------
    const TabDef* current = &TABS[0];
    for (const auto& tab : TABS) if (tab.mode == currentMode) current = &tab;

    const float headMid = FRAME.top + HEADER_H * 0.5f + 3.f;
    c.text(c.fit(upper(clan.name), 244.f, 15, true), FRAME.left + 18.f, FRAME.top + 19.f, {15, theme::Gold, true});
    c.text(current->blurb, FRAME.left + 18.f, FRAME.top + 36.f, {10, theme::TextMuted, false, Align::Left, false, true});

    const bool unrest = !factions.empty() || clan.tension >= 60;
    float tx = FRAME.left + 270.f;
    const float tabW = 124.f, tabH = 28.f, tabY = headMid - tabH * 0.5f;
    for (const auto& tab : TABS) {
        const sf::FloatRect r(tx, tabY, tabW, tabH);
        const bool active = (tab.mode == currentMode);
        c.tab(r, tab.keyLabel, tab.name, active, r.contains(mouse), tab.mode == DynastyUIMode::SUCCESSION_VIEW && unrest);
        const DynastyUIMode mode = tab.mode;
        if (!active) onClick(r, [this, mode]() { currentMode = mode; });
        tx += tabW + 4.f;
    }
    c.text("ESC to close", FRAME.left + FRAME.width - 18.f, headMid, {10, theme::TextMuted, false, Align::Right});

    const Character* focus = lookup(registry, focusId);
    if (!focus) {
        c.inset({SIDE.left, SIDE.top, MAIN.left + MAIN.width - SIDE.left, SIDE.height});
        c.text("No kin are recorded for this bloodline yet.", FRAME.left + FRAME.width * 0.5f, FRAME.top + FRAME.height * 0.5f,
               {13, theme::TextMuted, false, Align::Center, false, true});
        return;
    }

    Character::ID heirId = Character::INVALID_ID;
    {
        const auto line = SuccessionSystem::evaluateSuccession(dynasty, registry, factions, clan.successionLaw);
        if (!line.empty()) heirId = line.front().characterId;
    }

    const Frame f{ c, mouse, dynasty, clan, registry, factions, currentAlphaId, heirId, focus };
    drawRoster(f, SIDE);
    switch (currentMode) {
        case DynastyUIMode::CHARACTER_VIEW:   drawApeTab(f, MAIN); break;
        case DynastyUIMode::FAMILY_TREE_VIEW: drawBloodlineTab(f, MAIN); break;
        case DynastyUIMode::SUCCESSION_VIEW:  drawSuccessionTab(f, MAIN); break;
        case DynastyUIMode::COUNCIL_VIEW:     drawCouncilTab(f, MAIN); break;
        default: break;
    }

    if (hasTip) c.tooltip(tipAnchor.x, tipAnchor.y, tip);
}

// ---------------------------------------------------------------------------
// Left column: the kin you can follow, and how the clan is holding together.
// ---------------------------------------------------------------------------
void DynastyUI::drawRoster(const Frame& f, sf::FloatRect area) {
    Canvas& c = f.c;
    float y = section(c, area, "THE BLOODLINE", c.fit(f.dynasty.name, 120.f, 10));

    const float statusH = 148.f;
    const float rowH = 40.f, rowStep = 44.f;
    const float listBottom = area.top + area.height - statusH - 8.f;
    const int visible = std::max(1, static_cast<int>((listBottom - y + (rowStep - rowH)) / rowStep));
    const int count = static_cast<int>(roster.size());
    const int maxScroll = std::max(0, count - visible);

    if (revealFocus) {
        const int at = static_cast<int>(std::find(roster.begin(), roster.end(), focusId) - roster.begin());
        if (at < rosterScroll) rosterScroll = at;
        if (at >= rosterScroll + visible) rosterScroll = at - visible + 1;
        revealFocus = false;
    }
    rosterScroll = std::clamp(rosterScroll, 0, maxScroll);

    for (int i = rosterScroll; i < std::min(count, rosterScroll + visible); ++i) {
        const Character& ch = f.registry.at(roster[static_cast<size_t>(i)]);
        const sf::FloatRect r(area.left + 8.f, y, area.width - 16.f, rowH);
        const bool focused = (ch.id == focusId);
        c.button(r, focused, r.contains(f.mouse));

        c.fill({r.left + 9.f, r.top + 10.f, 5.f, 5.f}, ch.isAlive ? theme::Good : theme::Bad);
        c.text(c.fit(ch.name, 116.f, 12, true), r.left + 21.f, r.top + 13.f,
               {12, focused ? theme::GoldBright : (ch.isAlive ? theme::Text : theme::TextMuted), true});
        c.text(ch.isAlive ? "Age " + std::to_string(ch.age) : "Deceased", r.left + 21.f, r.top + 28.f, {10, theme::TextMuted});

        const std::string rank = rankOf(f, ch);
        if (ch.isAlive && rank != "Kin") {
            const sf::Color tag = (ch.id == f.alphaId) ? theme::Gold : (ch.id == f.heirId ? theme::Prestige : theme::Text);
            c.text(upper(rank), r.left + r.width - 9.f, r.top + 13.f, {9, tag, true, Align::Right});
        }
        if (ch.id == f.heirId && rank != "Heir") {
            c.text("HEIR", r.left + r.width - 9.f, r.top + 28.f, {9, theme::Prestige, true, Align::Right});
        }

        const Character::ID id = ch.id;
        if (!focused) onClick(r, [this, id]() { follow(id); });
        y += rowStep;
    }
    if (maxScroll > 0) {
        const int below = count - (rosterScroll + visible);
        const std::string more = below > 0 ? std::to_string(below) + " more below" : "Scroll up for the rest";
        c.text(more, area.left + area.width * 0.5f, listBottom + 2.f, {9, theme::TextMuted, false, Align::Center, false, true});
    }

    // ---- clan status -----------------------------------------------------------
    const float left = area.left + 12.f, right = area.left + area.width - 12.f;
    float sy = area.top + area.height - statusH;
    c.fill({area.left + 10.f, sy, area.width - 20.f, c.px(1)}, theme::BronzeDim);

    sy += 16.f;
    const int tension = std::clamp(f.clan.tension, 0, 100);
    c.text("TENSION", left, sy, {9, theme::TextMuted, true});
    c.text(std::to_string(tension) + "%  " + tensionWord(tension), right, sy, {11, tensionColor(tension), true, Align::Right});
    c.meter({left, sy + 9.f, right - left, 8.f}, static_cast<float>(tension) / 100.f, tensionColor(tension));

    sy += 38.f;
    const LawInfo* law = &LAWS[0];
    for (const auto& l : LAWS) if (l.law == f.clan.successionLaw) law = &l;
    c.text("INHERITANCE", left, sy, {9, theme::TextMuted, true});
    c.text(law->name, right, sy, {11, theme::Text, true, Align::Right});

    sy += 20.f;
    const Character* heir = lookup(f.registry, f.heirId);
    c.text("HEIR", left, sy, {9, theme::TextMuted, true});
    c.text(heir ? c.fit(heir->name, 150.f, 11, true) : "None", right, sy, {11, heir ? theme::Prestige : theme::Bad, true, Align::Right});

    sy += 20.f;
    c.text("FACTIONS", left, sy, {9, theme::TextMuted, true});
    c.text(f.factions.empty() ? "None" : std::to_string(f.factions.size()), right, sy,
           {11, f.factions.empty() ? theme::Text : theme::Bad, true, Align::Right});

    c.text("Up / Down to choose kin", area.left + area.width * 0.5f, area.top + area.height - 14.f,
           {9, theme::TextMuted, false, Align::Center, false, true});
}

// ---------------------------------------------------------------------------
// Ape: identity, abilities, nature, where they stand with the Alpha, deeds.
// ---------------------------------------------------------------------------
void DynastyUI::drawApeTab(const Frame& f, sf::FloatRect area) {
    Canvas& c = f.c;
    const Character& ch = *f.focus;
    const bool isAlpha = (ch.id == f.alphaId);
    const CharacterStats stats = ch.getEffectiveStats();

    // ---- identity ---------------------------------------------------------------
    const sf::FloatRect idr(area.left, area.top, area.width, 112.f);
    c.inset(idr);

    const sf::FloatRect crest(idr.left + 14.f, idr.top + 14.f, 84.f, 84.f);
    c.fill(crest, theme::EdgeDark);
    const sf::FloatRect crestIn(crest.left + c.px(1), crest.top + c.px(1), crest.width - c.px(2), crest.height - c.px(2));
    if (isAlpha) c.surface(crestIn, theme::ActiveTop, theme::ActiveBottom); else c.surface(crestIn, theme::HoverTop, theme::ButtonBottom);
    c.frame(crestIn, isAlpha ? theme::Gold : theme::Bronze);
    c.fill({crestIn.left + c.px(1), crestIn.top + c.px(1), crestIn.width - c.px(2), c.px(1)}, sf::Color(255, 244, 210, 70));
    const float crestX = crest.left + crest.width * 0.5f;
    c.text(ch.name.empty() ? "?" : ch.name.substr(0, 1), crestX, crest.top + 36.f,
           {34, isAlpha ? theme::GoldBright : theme::Text, true, Align::Center});
    c.text(ch.sex == Sex::MALE ? "MALE" : "FEMALE", crestX, crest.top + 70.f, {9, theme::TextMuted, true, Align::Center});
    if (isAlpha) c.icon(Icon::Crown, crest.left + crest.width - 13.f, crest.top + 13.f);

    const std::string rank = rankOf(f, ch);
    std::string standing;
    if (!ch.isAlive) standing = "Deceased, of the " + f.dynasty.name + " bloodline";
    else if (rank == "Kin" || rank == "Juvenile") standing = rank + " of the " + f.dynasty.name + " bloodline";
    else if (rank == "Heir") standing = "Heir to " + f.clan.name;
    else standing = rank + " of " + f.clan.name;

    const float tx = idr.left + 114.f;
    c.text(c.fit(ch.name, 370.f, 20, true), tx, idr.top + 26.f, {20, theme::GoldBright, true});
    c.text(c.fit(standing + ", age " + std::to_string(ch.age), 380.f, 11), tx, idr.top + 50.f, {11, theme::Text});
    c.text("AMBITION", tx, idr.top + 74.f, {9, theme::TextMuted, true});
    c.text(c.fit(ch.ambition.getName(), 380.f, 12), tx, idr.top + 92.f, {12, theme::Text, false, Align::Left, false, true});

    struct Chip { const char* label; int value; sf::Color color; bool meter; const char* note; };
    const Chip chips[3] = {
        { "PRESTIGE", ch.prestige,      theme::Prestige, false, "Standing earned among the clans." },
        { "LOYALTY",  ch.loyalty,       ch.loyalty >= 60 ? theme::Good : (ch.loyalty >= 35 ? theme::Amber : theme::Bad), true,
          "How firmly this ape stands with the Alpha." },
        { "DRIVE",    ch.ambitionScore, theme::Amber,    true,  "How hard this ape pushes for more." },
    };
    const float chipW = 128.f, chipGap = 8.f;
    float cx = idr.left + idr.width - 14.f - chipW * 3.f - chipGap * 2.f;
    for (const auto& chip : chips) {
        const sf::FloatRect r(cx, idr.top + 14.f, chipW, 84.f);
        c.button(r, false, r.contains(f.mouse));
        c.text(chip.label, r.left + 10.f, r.top + 15.f, {9, theme::TextMuted, true});
        c.text(std::to_string(chip.value), r.left + 10.f, r.top + 40.f, {18, chip.color, true});
        if (chip.meter) c.meter({r.left + 10.f, r.top + 62.f, r.width - 20.f, 8.f}, static_cast<float>(chip.value) / 100.f, chip.color);
        else c.text("among the clans", r.left + 10.f, r.top + 66.f, {9, theme::TextMuted, false, Align::Left, false, true});
        if (r.contains(f.mouse)) {
            Tooltip t;
            t.title = chip.label;
            t.accent = chip.color;
            t.note = chip.note;
            showTip(t, {r.left + r.width * 0.5f, r.top + r.height + 4.f});
        }
        cx += chipW + chipGap;
    }

    // ---- abilities ----------------------------------------------------------------
    const float row2 = area.top + 120.f, row2H = 222.f;
    const float leftW = 380.f;
    const float rightX = area.left + leftW + 8.f, rightW = area.width - leftW - 8.f;

    struct Ability { const char* name; int value; int base; sf::Color color; const char* note; };
    const Ability abilities[5] = {
        { "Prowess",     stats.prowess,     ch.baseStats.prowess,     theme::Bad,      "Strength in a fight or a dominance challenge." },
        { "Martial",     stats.martial,     ch.baseStats.martial,     theme::Amber,    "Leading warriors and hunts." },
        { "Stewardship", stats.stewardship, ch.baseStats.stewardship, theme::Good,     "Foraging, stores and tools." },
        { "Intrigue",    stats.intrigue,    ch.baseStats.intrigue,    theme::Piety,    "Plotting, and seeing plots coming." },
        { "Diplomacy",   stats.diplomacy,   ch.baseStats.diplomacy,   theme::Prestige, "Keeping the clan together." },
    };
    {
        const sf::FloatRect box(area.left, row2, leftW, row2H);
        float y = section(c, box, "ABILITIES", "out of 25");
        for (const auto& a : abilities) {
            const sf::FloatRect r(box.left + 8.f, y, box.width - 16.f, 32.f);
            const bool hov = r.contains(f.mouse);
            if (hov) c.fill(r, theme::HoverFill);
            const float mid = r.top + r.height * 0.5f;
            c.fill({r.left + 6.f, mid - 8.f, 5.f, 16.f}, a.color);
            c.text(a.name, r.left + 20.f, mid, {12, theme::Text});
            c.meter({r.left + 142.f, mid - 5.f, 160.f, 10.f}, static_cast<float>(a.value) / 25.f, a.color);
            c.text(std::to_string(a.value), r.left + r.width - 12.f, mid, {14, theme::Text, true, Align::Right});
            if (hov) {
                Tooltip t;
                t.title = upper(a.name);
                t.accent = a.color;
                t.rows.push_back({ "Born with", std::to_string(a.base), theme::Text });
                if (a.value != a.base) t.rows.push_back({ "From traits", signedInt(a.value - a.base), opinionColor(a.value - a.base) });
                t.rows.push_back({ "Total", std::to_string(a.value), theme::Text, true });
                t.note = a.note;
                showTip(t, {r.left + r.width * 0.5f, r.top + r.height + 2.f});
            }
            y += 36.f;
        }
    }

    // ---- nature (traits) ----------------------------------------------------------
    {
        const sf::FloatRect box(rightX, row2, rightW, row2H);
        const float top = section(c, box, "NATURE", ch.traits.empty() ? "" : std::to_string(ch.traits.size()) + " traits");
        if (ch.traits.empty()) {
            c.text("Nothing sets this ape apart yet.", box.left + 12.f, top + 10.f, {11, theme::TextMuted, false, Align::Left, false, true});
        }
        const float cardW = (box.width - 16.f - 8.f) * 0.5f, cardH = 40.f;
        for (size_t i = 0; i < std::min<size_t>(ch.traits.size(), 8); ++i) {
            const TraitInfo info = traitInfo(ch.traits[i]);
            const sf::FloatRect r(box.left + 8.f + static_cast<float>(i % 2) * (cardW + 8.f),
                                  top + static_cast<float>(i / 2) * (cardH + 5.f), cardW, cardH);
            c.button(r, false, r.contains(f.mouse));
            c.fill({r.left, r.top, 4.f, r.height}, info.flaw ? theme::Bad : theme::Good);
            c.text(info.name, r.left + 13.f, r.top + 13.f, {12, theme::Text, true});
            c.text(c.fit(info.effect, r.width - 22.f, 10), r.left + 13.f, r.top + 28.f, {10, theme::TextMuted});
            if (r.contains(f.mouse)) {
                Tooltip t;
                t.title = upper(info.name);
                t.accent = info.flaw ? theme::Bad : theme::Good;
                t.note = info.effect;
                showTip(t, {r.left + r.width * 0.5f, r.top + r.height + 2.f});
            }
        }
    }

    // ---- opinion --------------------------------------------------------------------
    const float row3 = row2 + row2H + 8.f, row3H = area.top + area.height - row3;
    {
        const sf::FloatRect box(area.left, row3, leftW, row3H);
        const float left = box.left + 12.f, right = box.left + box.width - 12.f;
        if (isAlpha) {
            // The Alpha has no opinion of themself worth showing; show what the clan thinks instead.
            float y = section(c, box, "HOW THE CLAN SEES THE ALPHA");
            int shown = 0;
            for (Character::ID id : roster) {
                const Character& other = f.registry.at(id);
                if (id == ch.id || !other.isAlive) continue;
                if (shown == 6) break;
                const sf::FloatRect r(box.left + 8.f, y, box.width - 16.f, 30.f);
                const bool hov = r.contains(f.mouse);
                if (hov) c.fill(r, theme::HoverFill);
                const float mid = r.top + r.height * 0.5f;
                const int op = other.getOpinionOf(ch.id);
                c.text(c.fit(other.name, 120.f, 12), r.left + 6.f, mid, {12, hov ? theme::GoldBright : theme::Text});
                c.balance({r.left + 142.f, mid - 5.f, 160.f, 10.f}, static_cast<float>(op) / 100.f);
                c.text(signedInt(op), r.left + r.width - 12.f, mid, {13, opinionColor(op), true, Align::Right});
                onClick(r, [this, id]() { follow(id); });
                y += 32.f;
                ++shown;
            }
            if (shown == 0) c.text("There is no one else in the bloodline.", left, y + 10.f, {11, theme::TextMuted, false, Align::Left, false, true});
        } else {
            const OpinionMatrix* matrix = ch.getOpinionBreakdown(f.alphaId);
            const int total = matrix ? matrix->calculateTotal() : 0;
            float y = section(c, box, "OPINION OF THE ALPHA");
            if (!matrix || matrix->modifiers.empty()) {
                c.text("No strong feelings either way.", left, y + 10.f, {11, theme::TextMuted, false, Align::Left, false, true});
            } else {
                y += 10.f;
                const size_t maxRows = 6;
                for (size_t i = 0; i < std::min(matrix->modifiers.size(), maxRows); ++i) {
                    const OpinionModifier& mod = matrix->modifiers[i];
                    c.text(c.fit(mod.reason, 280.f, 11), left, y, {11, theme::Text});
                    c.text(signedInt(mod.value), right, y, {12, opinionColor(mod.value), true, Align::Right});
                    y += 22.f;
                }
                if (matrix->modifiers.size() > maxRows) {
                    c.text("and " + std::to_string(matrix->modifiers.size() - maxRows) + " more", left, y, {10, theme::TextMuted, false, Align::Left, false, true});
                }
            }
            const float ty = box.top + box.height - 26.f;
            c.fill({box.left + 10.f, ty - 16.f, box.width - 20.f, c.px(1)}, theme::BronzeDim);
            c.text("Total", left, ty, {12, theme::Text, true});
            c.balance({left + 130.f, ty - 5.f, 160.f, 10.f}, static_cast<float>(total) / 100.f);
            c.text(signedInt(total), right, ty, {14, opinionColor(total), true, Align::Right});
        }
    }

    // ---- chronicle --------------------------------------------------------------------
    {
        const sf::FloatRect box(rightX, row3, rightW, row3H);
        float y = section(c, box, "CHRONICLE") + 8.f;
        const float left = box.left + 12.f, bottom = box.top + box.height - 16.f;
        if (ch.history.empty()) {
            c.text("No deeds worth remembering. Yet.", left, y + 2.f, {11, theme::TextMuted, false, Align::Left, false, true});
        }
        // Newest first.
        for (auto it = ch.history.rbegin(); it != ch.history.rend() && y < bottom - 30.f; ++it) {
            c.text("YEAR " + std::to_string(it->year) + ", DAY " + std::to_string(it->day), left, y, {9, theme::Bronze, true});
            y = c.paragraph(it->description, left, y + 15.f, box.width - 24.f, {11, theme::Text}) + 8.f;
        }
    }
}

// ---------------------------------------------------------------------------
// Bloodline: three generations around the followed ape. Click any ape to
// follow them and walk the tree.
// ---------------------------------------------------------------------------
void DynastyUI::drawBloodlineTab(const Frame& f, sf::FloatRect area) {
    Canvas& c = f.c;
    const Character& ch = *f.focus;
    section(c, area, "BLOODLINE OF " + upper(ch.name), "Click an ape to follow their line");

    const float nodeW = 160.f, nodeH = 60.f, spacing = 180.f;
    const float midX = area.left + area.width * 0.5f;
    const float yParents = area.top + 70.f, yFocus = area.top + 240.f, yYoung = area.top + 410.f;
    const float t = c.px(2);

    auto hline = [&](float x1, float x2, float y, sf::Color col) {
        c.fill({std::min(x1, x2), y - t * 0.5f, std::abs(x2 - x1) + t, t}, col);
    };
    auto vline = [&](float x, float y1, float y2, sf::Color col) {
        c.fill({x - t * 0.5f, std::min(y1, y2), t, std::abs(y2 - y1)}, col);
    };

    // A known ape, or a ghost card for one the registry has no record of.
    auto node = [&](float cx, float top, const Character* who, const std::string& relation, const std::string& ghost) {
        const sf::FloatRect r(cx - nodeW * 0.5f, top, nodeW, nodeH);
        if (!who) {
            c.inset(r);
            c.text(ghost, cx, r.top + 22.f, {11, theme::TextMuted, false, Align::Center, false, true});
            c.text(upper(relation), cx, r.top + 44.f, {9, theme::BronzeDim, true, Align::Center});
            return;
        }
        const bool focused = (who->id == focusId);
        c.button(r, focused, r.contains(f.mouse));
        c.fill({r.left + 9.f, r.top + 11.f, 5.f, 5.f}, who->isAlive ? theme::Good : theme::Bad);
        c.text(c.fit(who->name, nodeW - 44.f, 12, true), r.left + 21.f, r.top + 14.f,
               {12, focused ? theme::GoldBright : (who->isAlive ? theme::Text : theme::TextMuted), true});
        if (who->id == f.alphaId) c.icon(Icon::Crown, r.left + r.width - 13.f, r.top + 14.f);
        const std::string rank = rankOf(f, *who);
        c.text(who->isAlive ? rank + ", age " + std::to_string(who->age) : rank, r.left + 10.f, r.top + 31.f, {10, theme::TextMuted});
        c.text(upper(relation), r.left + 10.f, r.top + 47.f, {9, focused ? theme::Gold : theme::Bronze, true});
        const Character::ID id = who->id;
        if (!focused) onClick(r, [this, id]() { follow(id); });
    };

    auto ghostFor = [](Character::ID id) { return id == Character::INVALID_ID ? "Unknown" : "Lost to memory"; };

    // ---- gather kin --------------------------------------------------------------
    const Character* father = lookup(f.registry, ch.fatherId);
    const Character* mother = lookup(f.registry, ch.motherId);

    std::vector<const Character*> siblings, mates, young;
    for (Character::ID id : roster) {
        const Character& other = f.registry.at(id);
        if (other.id == ch.id) continue;
        const bool sameFather = ch.fatherId != Character::INVALID_ID && other.fatherId == ch.fatherId;
        const bool sameMother = ch.motherId != Character::INVALID_ID && other.motherId == ch.motherId;
        if (sameFather || sameMother) siblings.push_back(&other);
    }
    for (Character::ID id : ch.spouseIds) if (const Character* m = lookup(f.registry, id)) mates.push_back(m);
    for (Character::ID id : ch.childrenIds) if (const Character* y = lookup(f.registry, id)) young.push_back(y);

    const size_t maxSide = 2, maxYoung = 5;
    const size_t shownSiblings = std::min(siblings.size(), maxSide);
    const size_t shownMates = std::min(mates.size(), maxSide);
    const size_t shownYoung = std::min(young.size(), maxYoung);

    // ---- lines first, cards on top -------------------------------------------------
    const bool knownParent = (father || mother);
    const sf::Color bloodLine = knownParent ? theme::Bronze : theme::BronzeDim;
    const float railSib = yFocus - 20.f;
    hline(midX - 95.f + nodeW * 0.5f, midX + 95.f - nodeW * 0.5f, yParents + nodeH * 0.5f, bloodLine);
    vline(midX, yParents + nodeH * 0.5f, yFocus, bloodLine);
    if (shownSiblings > 0) {
        hline(midX - spacing * static_cast<float>(shownSiblings), midX, railSib, theme::Bronze);
        for (size_t i = 0; i < shownSiblings; ++i) vline(midX - spacing * static_cast<float>(i + 1), railSib, yFocus, theme::Bronze);
    }
    if (shownMates > 0) {
        hline(midX, midX + spacing * static_cast<float>(shownMates), yFocus + nodeH * 0.5f, theme::Gold);
    }
    if (shownYoung > 0) {
        const float railYoung = yYoung - 20.f;
        const float first = midX - spacing * static_cast<float>(shownYoung - 1) * 0.5f;
        vline(midX, yFocus + nodeH, railYoung, theme::Bronze);
        if (shownYoung > 1) hline(first, first + spacing * static_cast<float>(shownYoung - 1), railYoung, theme::Bronze);
        for (size_t i = 0; i < shownYoung; ++i) vline(first + spacing * static_cast<float>(i), railYoung, yYoung, theme::Bronze);
    }

    // ---- cards -----------------------------------------------------------------------
    node(midX - 95.f, yParents, father, "Father", ghostFor(ch.fatherId));
    node(midX + 95.f, yParents, mother, "Mother", ghostFor(ch.motherId));

    for (size_t i = 0; i < shownSiblings; ++i) node(midX - spacing * static_cast<float>(i + 1), yFocus, siblings[i], "Sibling", "");
    node(midX, yFocus, &ch, "Following", "");
    for (size_t i = 0; i < shownMates; ++i) node(midX + spacing * static_cast<float>(i + 1), yFocus, mates[i], "Mate", "");

    if (shownYoung == 0) {
        c.text("No young yet.", midX, yYoung + nodeH * 0.5f, {11, theme::TextMuted, false, Align::Center, false, true});
    }
    const float firstYoung = midX - spacing * static_cast<float>(shownYoung > 0 ? shownYoung - 1 : 0) * 0.5f;
    for (size_t i = 0; i < shownYoung; ++i) node(firstYoung + spacing * static_cast<float>(i), yYoung, young[i], "Child", "");

    // ---- footer: counts, and the way home ---------------------------------------------
    const float fy = area.top + area.height - 28.f;
    c.fill({area.left + 10.f, fy - 22.f, area.width - 20.f, c.px(1)}, theme::BronzeDim);
    std::string counts = std::to_string(mates.size()) + (mates.size() == 1 ? " mate" : " mates") + ",  " +
                         std::to_string(young.size()) + " young,  " +
                         std::to_string(siblings.size()) + (siblings.size() == 1 ? " sibling" : " siblings");
    const size_t hidden = (siblings.size() - shownSiblings) + (mates.size() - shownMates) + (young.size() - shownYoung);
    if (hidden > 0) counts += "   (" + std::to_string(hidden) + " not shown, find them in the list)";
    c.text(counts, area.left + 14.f, fy, {10, theme::TextMuted});

    if (ch.id != f.alphaId && f.registry.count(f.alphaId)) {
        const sf::FloatRect r(area.left + area.width - 14.f - 150.f, fy - 13.f, 150.f, 26.f);
        const bool hov = r.contains(f.mouse);
        c.button(r, false, hov);
        c.text("Back to the Alpha", r.left + r.width * 0.5f, fy, {11, hov ? theme::GoldBright : theme::Text, true, Align::Center});
        const Character::ID alpha = f.alphaId;
        onClick(r, [this, alpha]() { follow(alpha); });
    }
}

// ---------------------------------------------------------------------------
// Succession: the law, the line it produces, and the unrest pulling at it.
// ---------------------------------------------------------------------------
void DynastyUI::drawSuccessionTab(const Frame& f, sf::FloatRect area) {
    Canvas& c = f.c;
    const float leftW = 540.f;

    // ---- the line ------------------------------------------------------------------
    {
        const sf::FloatRect box(area.left, area.top, leftW, area.height);
        float y = section(c, box, "LINE OF SUCCESSION", "Click an ape to read about them");

        const float lawW = (box.width - 24.f - 12.f) / 3.f;
        for (int i = 0; i < 3; ++i) {
            const LawInfo& law = LAWS[i];
            const sf::FloatRect r(box.left + 12.f + static_cast<float>(i) * (lawW + 6.f), y, lawW, 40.f);
            const bool active = (law.law == f.clan.successionLaw);
            c.button(r, active, false);
            c.text(law.name, r.left + r.width * 0.5f, r.top + 14.f, {11, active ? theme::GoldBright : theme::TextMuted, true, Align::Center});
            c.text(law.who, r.left + r.width * 0.5f, r.top + 29.f, {9, active ? theme::Text : theme::BronzeDim, false, Align::Center});
            if (r.contains(f.mouse)) {
                Tooltip t;
                t.title = upper(law.name);
                t.accent = active ? theme::Gold : theme::Bronze;
                t.rows.push_back({ "Status", active ? "Law of the clan" : "Not in force", active ? theme::Good : theme::TextMuted });
                t.note = law.note;
                showTip(t, {r.left + r.width * 0.5f, r.top + r.height + 2.f});
            }
        }
        y += 52.f;

        const auto line = SuccessionSystem::evaluateSuccession(f.dynasty, f.registry, f.factions, f.clan.successionLaw);
        if (line.empty()) {
            c.paragraph("No living kin can inherit. If the Alpha falls, the bloodline ends and the clan is anyone's to take.",
                        box.left + 14.f, y + 10.f, box.width - 28.f, {12, theme::Bad});
        }
        const float rowH = 56.f;
        const float bottom = box.top + box.height - 10.f;
        int rank = 1;
        for (const auto& cand : line) {
            const Character* ape = lookup(f.registry, cand.characterId);
            if (!ape) continue;
            if (y + rowH > bottom) break;
            const sf::FloatRect r(box.left + 8.f, y, box.width - 16.f, rowH);
            const bool first = (rank == 1);
            const bool hov = r.contains(f.mouse);
            c.button(r, false, hov);

            const sf::FloatRect badge(r.left + 10.f, r.top + 12.f, 32.f, 32.f);
            if (first) c.button(badge, true, false); else c.inset(badge);
            c.text(std::to_string(rank), badge.left + 16.f, badge.top + 16.f, {14, first ? theme::GoldBright : theme::TextMuted, true, Align::Center});

            const float nx = r.left + 54.f;
            const float nw = c.text(c.fit(ape->name, 250.f, 13, true), nx, r.top + 19.f, {13, first || hov ? theme::GoldBright : theme::Text, true});
            if (first) c.text("HEIR", nx + nw + 10.f, r.top + 19.f, {9, theme::Prestige, true});
            std::string why = "Age " + std::to_string(ape->age) + ", " + cand.rationale;
            c.text(c.fit(why, 330.f, 10), nx, r.top + 38.f, {10, theme::TextMuted});

            c.text("CLAIM", r.left + r.width - 12.f, r.top + 17.f, {9, theme::TextMuted, true, Align::Right});
            c.text(std::to_string(static_cast<int>(cand.score)), r.left + r.width - 12.f, r.top + 37.f, {14, theme::Text, true, Align::Right});
            if (cand.factionBackingPower > 0.f) {
                c.text("Faction backed", r.left + r.width - 70.f, r.top + 37.f, {9, theme::Bad, true, Align::Right});
            }

            const Character::ID id = ape->id;
            onClick(r, [this, id]() { follow(id); currentMode = DynastyUIMode::CHARACTER_VIEW; });
            y += rowH + 6.f;
            ++rank;
        }
    }

    // ---- unrest --------------------------------------------------------------------
    {
        const sf::FloatRect box(area.left + leftW + 8.f, area.top, area.width - leftW - 8.f, area.height);
        const float left = box.left + 14.f, right = box.left + box.width - 14.f;
        float y = section(c, box, "UNREST") + 10.f;

        const int tension = std::clamp(f.clan.tension, 0, 100);
        c.text("CLAN TENSION", left, y, {9, theme::TextMuted, true});
        c.text(tensionWord(tension), right, y, {11, tensionColor(tension), true, Align::Right});
        y += 24.f;
        c.text(std::to_string(tension) + "%", left, y, {20, tensionColor(tension), true});
        c.meter({left + 70.f, y - 5.f, right - left - 70.f, 10.f}, static_cast<float>(tension) / 100.f, tensionColor(tension));
        y += 26.f;
        y = c.paragraph(tensionNote(tension), left, y, right - left, {10, theme::TextMuted, false, Align::Left, false, true}) + 10.f;

        c.fill({box.left + 10.f, y - 4.f, box.width - 20.f, c.px(1)}, theme::BronzeDim);
        y += 12.f;
        c.text("FACTIONS", left, y, {10, theme::Gold, true});
        c.text(f.factions.empty() ? "none" : std::to_string(f.factions.size()), right, y, {10, f.factions.empty() ? theme::TextMuted : theme::Bad, true, Align::Right});
        y += 16.f;

        if (f.factions.empty()) {
            c.paragraph("The clan stands behind its Alpha. No faction is gathering against the line.",
                        left, y + 8.f, right - left, {11, theme::TextMuted, false, Align::Left, false, true});
        }
        const float cardH = 92.f;
        const float bottom = box.top + box.height - 10.f;
        for (const auto& faction : f.factions) {
            if (y + cardH > bottom) {
                c.text("More factions than fit here.", left, y + 8.f, {10, theme::TextMuted, false, Align::Left, false, true});
                break;
            }
            const sf::FloatRect r(box.left + 8.f, y, box.width - 16.f, cardH);
            const Character* leader = lookup(f.registry, faction.leaderId);
            const bool hov = leader && r.contains(f.mouse);
            c.button(r, false, hov);
            c.fill({r.left, r.top, 4.f, r.height}, theme::Bad);

            c.text(c.fit(faction.name, r.width - 26.f, 12, true), r.left + 13.f, r.top + 15.f, {12, theme::Bad, true});
            c.text(factionGoal(faction.type), r.left + 13.f, r.top + 33.f, {10, theme::Text});
            c.text("Led by " + (leader ? leader->name : std::string("an unknown ape")) + ", " +
                   std::to_string(faction.memberIds.size()) + (faction.memberIds.size() == 1 ? " member" : " members"),
                   r.left + 13.f, r.top + 50.f, {10, theme::TextMuted});
            c.text("STRENGTH", r.left + 13.f, r.top + 72.f, {9, theme::TextMuted, true});
            c.meter({r.left + 78.f, r.top + 67.f, r.width - 78.f - 52.f, 10.f}, faction.powerRating / 200.f, theme::Bad);
            c.text(std::to_string(static_cast<int>(faction.powerRating)), r.left + r.width - 12.f, r.top + 72.f, {12, theme::Bad, true, Align::Right});

            if (leader) {
                const Character::ID id = leader->id;
                onClick(r, [this, id]() { follow(id); currentMode = DynastyUIMode::CHARACTER_VIEW; });
            }
            y += cardH + 6.f;
        }
    }
}

// ---------------------------------------------------------------------------
// Council: four seats. The followed ape can be seated on any of them.
// ---------------------------------------------------------------------------
void DynastyUI::drawCouncilTab(const Frame& f, sf::FloatRect area) {
    Canvas& c = f.c;
    const Character& focus = *f.focus;

    // ---- who is being seated -------------------------------------------------------
    const sf::FloatRect intro(area.left, area.top, area.width, 64.f);
    c.inset(intro);
    c.text("THE ALPHA'S COUNCIL", intro.left + 14.f, intro.top + 20.f, {12, theme::Gold, true});
    c.text("Four seats share the work of leading. Choose an ape in the list, then seat them here.",
           intro.left + 14.f, intro.top + 42.f, {10, theme::TextMuted, false, Align::Left, false, true});
    c.text("CHOOSING FOR", intro.left + intro.width - 14.f, intro.top + 20.f, {9, theme::TextMuted, true, Align::Right});
    c.text(c.fit(focus.name, 220.f, 14, true) , intro.left + intro.width - 14.f, intro.top + 42.f, {14, theme::GoldBright, true, Align::Right});

    // Why the followed ape cannot be seated at all, if they cannot.
    std::string barred;
    if (!focus.isAlive) barred = focus.name + " is dead";
    else if (focus.id == f.alphaId) barred = "The Alpha leads the council and takes no seat";
    else if (focus.age < 12) barred = focus.name + " is too young to sit on the council";

    auto textButton = [&](sf::FloatRect r, const std::string& label, std::function<void()> action) {
        const bool hov = r.contains(f.mouse);
        c.button(r, false, hov);
        c.text(c.fit(label, r.width - 12.f, 11, true), r.left + r.width * 0.5f, r.top + r.height * 0.5f,
               {11, hov ? theme::GoldBright : theme::Text, true, Align::Center});
        onClick(r, std::move(action));
    };

    // ---- seats ---------------------------------------------------------------------
    const float gap = 8.f;
    const float cardW = (area.width - gap) * 0.5f, cardH = 200.f;
    const float gridTop = intro.top + intro.height + gap;
    float bonus[SEAT_COUNT] = { 0.f, 0.f, 0.f, 0.f };

    for (int i = 0; i < SEAT_COUNT; ++i) {
        const SeatInfo& seat = SEATS[i];
        const sf::FloatRect r(area.left + static_cast<float>(i % 2) * (cardW + gap),
                              gridTop + static_cast<float>(i / 2) * (cardH + gap), cardW, cardH);
        const Character::ID holderId = f.clan.getCouncilMember(seat.pos);
        const Character* holder = lookup(f.registry, holderId);
        if (holder && !holder->isAlive) holder = nullptr;

        const float top = section(c, r, upper(seat.title), holder ? "" : "empty");
        const float left = r.left + 14.f;
        c.text(seat.duty, left, top + 4.f, {10, theme::TextMuted, false, Align::Left, false, true});

        if (holder) {
            std::string statLine;
            bonus[i] = seatBonus(seat.pos, holder->getEffectiveStats(), statLine);
            const float nameW = c.textWidth(c.fit(holder->name, 300.f, 16, true), 16, true);
            const sf::FloatRect nameHit(left - 4.f, top + 22.f, nameW + 8.f, 26.f);
            const bool hov = nameHit.contains(f.mouse);
            c.text(c.fit(holder->name, 300.f, 16, true), left, top + 35.f, {16, hov || holder->id == focusId ? theme::GoldBright : theme::Text, true});
            c.text("Age " + std::to_string(holder->age) + ", " + statLine, left, top + 57.f, {10, theme::TextMuted});
            c.text(percent(bonus[i]) + " " + seat.bonus, left, top + 82.f, {13, theme::Good, true});
            const Character::ID id = holder->id;
            if (id != focusId) onClick(nameHit, [this, id]() { follow(id); });
        } else {
            c.text("No one sits here", left, top + 35.f, {14, theme::TextMuted, false, Align::Left, false, true});
            c.text("The clan gets nothing from an empty seat.", left, top + 57.f, {10, theme::TextMuted});
        }

        // ---- seat / dismiss ------------------------------------------------------
        const float by = r.top + r.height - 38.f;
        const CouncilPosition pos = seat.pos;
        if (holder && holder->id == focus.id) {
            c.text(focus.name + " holds this seat", left, by + 13.f, {10, theme::Gold, false, Align::Left, false, true});
        } else if (!barred.empty()) {
            c.text(c.fit(barred, r.width - 150.f, 10), left, by + 13.f, {10, theme::TextMuted, false, Align::Left, false, true});
        } else {
            std::string statLine;
            const float would = seatBonus(seat.pos, focus.getEffectiveStats(), statLine);
            const sf::FloatRect seatBtn(left - 2.f, by, 190.f, 26.f);
            const Character::ID id = focus.id;
            textButton(seatBtn, "Seat " + focus.name, [this, pos, id]() {
                seatChangePending = true;
                seatChangePos = pos;
                seatChangeHolder = id;
            });
            const sf::Color cmp = !holder ? theme::Good : (would > bonus[i] ? theme::Good : (would < bonus[i] ? theme::Bad : theme::TextMuted));
            c.text("would give " + percent(would), seatBtn.left + seatBtn.width + 10.f, by + 13.f, {10, cmp, true});
        }
        if (holder) {
            textButton({r.left + r.width - 14.f - 92.f, by, 92.f, 26.f}, "Dismiss", [this, pos]() {
                seatChangePending = true;
                seatChangePos = pos;
                seatChangeHolder = Character::INVALID_ID;
            });
        }
    }

    // ---- what it adds up to ----------------------------------------------------------
    const float sumTop = gridTop + 2.f * (cardH + gap);
    const sf::FloatRect sum(area.left, sumTop, area.width, area.top + area.height - sumTop);
    const float top = section(c, sum, "WHAT THE COUNCIL GIVES THE CLAN");
    const float colW = (sum.width - 28.f) / 4.f;
    for (int i = 0; i < SEAT_COUNT; ++i) {
        const float x = sum.left + 14.f + colW * static_cast<float>(i);
        if (i > 0) c.divider(x - 8.f, top + 2.f, sum.height - 50.f);
        c.text(upper(SEATS[i].bonus), x, top + 10.f, {9, theme::TextMuted, true});
        c.text(bonus[i] > 0.f ? percent(bonus[i]) : "+0%", x, top + 34.f, {18, bonus[i] > 0.f ? theme::Good : theme::TextMuted, true});
        c.text(std::string("from the ") + SEATS[i].title, x, top + 56.f, {9, theme::TextMuted, false, Align::Left, false, true});
    }
}

}
