#include "ui/ProfileViewManager.h"
#include <algorithm>
#include <cmath>

namespace {

using namespace ui;

const float PANEL_W = 430.f;
const float PANEL_H = 540.f;
const float HEADER_H = 48.f;

const char* const APE_TABS[3]     = { "Ape", "Kin", "Standing" };
const char* const COUNTY_TABS[3]  = { "Holding", "Apes", "Stores" };
const char* const REALM_TABS[3]   = { "Realm", "Counties", "Diplomacy" };

struct TraitInfo { const char* name; bool flaw; };
TraitInfo traitInfo(sim::Trait t) {
    switch (t) {
        case sim::Trait::Brave:       return { "Brave", false };
        case sim::Trait::Coward:      return { "Coward", true };
        case sim::Trait::Greedy:      return { "Greedy", true };
        case sim::Trait::Honorable:   return { "Honorable", false };
        case sim::Trait::Cruel:       return { "Cruel", true };
        case sim::Trait::Charismatic: return { "Charismatic", false };
        case sim::Trait::Lazy:        return { "Lazy", true };
        case sim::Trait::Strategic:   return { "Strategic", false };
        case sim::Trait::Impulsive:   return { "Impulsive", true };
        case sim::Trait::Curious:     return { "Curious", false };
        case sim::Trait::Energetic:   return { "Energetic", false };
        case sim::Trait::Clever:      return { "Clever", false };
        case sim::Trait::Hardworking: return { "Hardworking", false };
        case sim::Trait::Patient:     return { "Patient", false };
        case sim::Trait::Aggressive:  return { "Aggressive", true };
        case sim::Trait::Perceptive:  return { "Perceptive", false };
    }
    return { "Trait", false };
}

const char* jobName(sim::Job j) {
    switch (j) {
        case sim::Job::Idle:          return "Resting";
        case sim::Job::Woodcutter:    return "Cutting wood";
        case sim::Job::March:         return "Marching";
        case sim::Job::Builder:       return "Building";
        case sim::Job::CarryResource: return "Hauling";
        case sim::Job::Forage:        return "Foraging";
        case sim::Job::StoneGatherer: return "Gathering stone";
        case sim::Job::Scout:         return "Scouting";
        case sim::Job::Patrol:        return "On patrol";
        case sim::Job::Guard:         return "Standing guard";
        case sim::Job::Wander:        return "Wandering";
        case sim::Job::ReturnHome:    return "Heading home";
        case sim::Job::Sleep:         return "Sleeping";
        case sim::Job::Socialize:     return "Grooming";
        case sim::Job::Eat:           return "Eating";
        case sim::Job::Combat:        return "Fighting";
        case sim::Job::Intimidate:    return "Posturing";
        case sim::Job::Observe:       return "Watching";
        case sim::Job::Muster:        return "Mustering";
        case sim::Job::Flee:          return "Fleeing";
        case sim::Job::Gathering:     return "At the gathering";
    }
    return "Resting";
}

const char* toolName(sim::ToolType t) {
    switch (t) {
        case sim::ToolType::Basket:      return "Basket";
        case sim::ToolType::StoneAxe:    return "Stone axe";
        case sim::ToolType::StonePick:   return "Stone pick";
        case sim::ToolType::WoodenSpear: return "Wooden spear";
        case sim::ToolType::Torch:       return "Torch";
        default:                         return "";
    }
}

const char* diseaseName(sim::DiseaseType d) {
    switch (d) {
        case sim::DiseaseType::Flu:       return "Flu";
        case sim::DiseaseType::Fever:     return "Fever";
        case sim::DiseaseType::Plague:    return "Plague";
        case sim::DiseaseType::Parasite:  return "Parasites";
        case sim::DiseaseType::Infection: return "Infected wound";
        default:                          return "";
    }
}

const char* tierName(sim::SettlementTier t) {
    switch (t) {
        case sim::SettlementTier::FirePit:    return "Fire pit";
        case sim::SettlementTier::Camp:       return "Camp";
        case sim::SettlementTier::Village:    return "Village";
        case sim::SettlementTier::Stronghold: return "Stronghold";
    }
    return "Fire pit";
}

const char* identityName(sim::VillageIdentity i) {
    switch (i) {
        case sim::VillageIdentity::Balanced:     return "A bit of everything";
        case sim::VillageIdentity::WoodFocused:  return "Woodcutters";
        case sim::VillageIdentity::StoneFocused: return "Stone gatherers";
        case sim::VillageIdentity::FoodRich:     return "Rich foragers";
        case sim::VillageIdentity::Aggressive:   return "Raiders";
        case sim::VillageIdentity::Peaceful:     return "Peaceful";
        case sim::VillageIdentity::Expansionist: return "Border pushers";
    }
    return "A bit of everything";
}

struct SeatInfo { sim::CouncilRole role; const char* title; const char* shortName; const char* duty; };
const SeatInfo SEATS[4] = {
    { sim::CouncilRole::WarChief,     "War Chief",     "War Chief", "Leads the warriors and watches the borders." },
    { sim::CouncilRole::ChiefBuilder, "Chief Builder", "Builder",   "Directs the building work." },
    { sim::CouncilRole::LeadForager,  "Lead Forager",  "Forager",   "Leads the foraging parties." },
    { sim::CouncilRole::Shaman,       "Shaman",        "Shaman",    "Speaks to the spirits. Their missions earn piety." },
};
const SeatInfo* seatFor(sim::CouncilRole role) {
    for (const auto& s : SEATS) if (s.role == role) return &s;
    return nullptr;
}

struct StatusInfo { const char* name; sf::Color color; };
StatusInfo statusInfo(sim::DiplomacyStatus s) {
    switch (s) {
        case sim::DiplomacyStatus::War:        return { "At war", theme::Bad };
        case sim::DiplomacyStatus::Rival:      return { "Rival", theme::Amber };
        case sim::DiplomacyStatus::Suspicious: return { "Wary", theme::Amber };
        case sim::DiplomacyStatus::Alliance:   return { "Allied", theme::Prestige };
        case sim::DiplomacyStatus::Trade:      return { "Trading", theme::Good };
        case sim::DiplomacyStatus::Friendly:   return { "Friendly", theme::Good };
        default:                               return { "Neutral", theme::TextMuted };
    }
}

const char* gradeOf(int v) { return v >= 18 ? "Master" : (v >= 14 ? "Adept" : (v >= 10 ? "Able" : "Poor")); }

std::string upper(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char ch) { return static_cast<char>(std::toupper(ch)); });
    return s;
}

std::string ageOf(const sim::ApeData& a) { return "Age " + std::to_string(static_cast<int>(a.age)); }

void addUnique(std::vector<sim::EntityID>& list, sim::EntityID id) {
    if (id != 0 && std::find(list.begin(), list.end(), id) == list.end()) list.push_back(id);
}

// Where the village keeps the id of whoever holds a seat.
sim::EntityID* seatSlot(sim::VillageData& v, sim::CouncilRole role) {
    switch (role) {
        case sim::CouncilRole::WarChief:     return &v.warChiefId;
        case sim::CouncilRole::ChiefBuilder: return &v.chiefBuilderId;
        case sim::CouncilRole::LeadForager:  return &v.leadForagerId;
        case sim::CouncilRole::Shaman:       return &v.shamanId;
        default:                             return nullptr;
    }
}

sim::ApeData* seatHolder(sim::SimulationRegistry& reg, const sim::VillageData& v, sim::CouncilRole role) {
    for (sim::EntityID id : v.members) {
        sim::ApeData* a = reg.getApe(id);
        if (a && a->alive && a->councilRole == role) return a;
    }
    return nullptr;
}

// What an ape is to their home: used as the header line and as list tags.
std::string rankOf(sim::SimulationRegistry& reg, const sim::ApeData& ape) {
    if (!ape.alive) return "Dead";
    if (sim::KingdomData* k = reg.getKingdom(ape.currentKingdom)) {
        if (k->currentKingId == ape.id) return "King";
    }
    if (sim::VillageData* v = reg.getVillage(ape.villageId)) {
        if (v->leaderId == ape.id) return "Alpha";
    }
    if (const SeatInfo* seat = seatFor(ape.councilRole)) return seat->title;
    return "";
}

} // namespace

ProfileViewManager::ProfileViewManager()
    : font(nullptr), currentView(ViewType::None), subjectId(0), tabByView{0, 0, 0, 0}, listScroll(0),
      isDragging(false), dragOffset(0.f, 0.f), profilePanelPos(70.f, 80.f),
      panelWidth(PANEL_W), panelHeight(PANEL_H) {}

void ProfileViewManager::init(const sf::Font& f) {
    font = &f;
}

void ProfileViewManager::open(ViewType type, uint64_t id, bool recordHistory) {
    if (currentView == type && subjectId == id) return;
    if (recordHistory && currentView != ViewType::None) {
        navHistory.push_back({ currentView, subjectId, activeTab() });
    }
    currentView = type;
    subjectId = id;
    listScroll = 0;
}

void ProfileViewManager::inspectCharacter(sim::EntityID id, bool recordHistory) { open(ViewType::Character, id, recordHistory); }
void ProfileViewManager::inspectVillage(sim::VillageID id, bool recordHistory)  { open(ViewType::Village, id, recordHistory); }
void ProfileViewManager::inspectKingdom(sim::KingdomID id, bool recordHistory)  { open(ViewType::Kingdom, id, recordHistory); }

void ProfileViewManager::goBack() {
    if (navHistory.empty()) return;
    const HistoryEntry prev = navHistory.back();
    navHistory.pop_back();
    open(prev.type, prev.id, false);
    activeTab() = prev.tab;
}

void ProfileViewManager::setTab(int tab) {
    if (currentView == ViewType::None) return;
    activeTab() = std::clamp(tab, 0, TAB_COUNT - 1);
    listScroll = 0;
}

void ProfileViewManager::close() {
    currentView = ViewType::None;
    subjectId = 0;
    isDragging = false;
    navHistory.clear();
    interactiveButtons.clear();
}

void ProfileViewManager::registerButton(sf::FloatRect bounds, const std::function<void()>& action) {
    if (action) interactiveButtons.push_back({ bounds, action });
}

void ProfileViewManager::showTip(const ui::Tooltip& t, sf::Vector2f anchor) {
    hasTip = true;
    tip = t;
    tipAnchor = anchor;
}

ProfileViewManager::FamilyInfo ProfileViewManager::resolveFamily(sim::EntityID apeId, sim::SimulationRegistry& reg) {
    FamilyInfo fam;
    sim::ApeData* ape = reg.getApe(apeId);
    if (!ape) return fam;

    if (sim::KingdomData* kd = reg.getKingdom(ape->currentKingdom)) {
        if (kd->currentKingId != 0 && kd->currentKingId != ape->id) {
            fam.liegeId = kd->currentKingId;
            fam.liegeTitle = "King of " + kd->name;
        }
    }
    if (fam.liegeId == 0) {
        if (sim::VillageData* vd = reg.getVillage(ape->villageId)) {
            if (vd->leaderId != 0 && vd->leaderId != ape->id) {
                fam.liegeId = vd->leaderId;
                fam.liegeTitle = "Alpha of " + vd->name;
            }
        }
    }

    fam.fatherId = ape->fatherId;
    fam.motherId = ape->motherId;
    addUnique(fam.spouseIds, ape->spouseId);
    for (sim::EntityID id : ape->children) addUnique(fam.childrenIds, id);
    for (sim::EntityID id : ape->siblings) addUnique(fam.siblingIds, id);

    // The dynasty records know kin the ape record may not.
    const auto& characters = reg.getAllCharacters();
    const auto it = characters.find(apeId);
    if (it != characters.end()) {
        const sim::Character& ch = it->second;
        if (fam.fatherId == 0) fam.fatherId = ch.fatherId;
        if (fam.motherId == 0) fam.motherId = ch.motherId;
        for (auto id : ch.spouseIds)   if (reg.getApe(id)) addUnique(fam.spouseIds, id);
        for (auto id : ch.childrenIds) if (reg.getApe(id)) addUnique(fam.childrenIds, id);
    }

    for (auto& pair : reg.getAllApes()) {
        const sim::ApeData& other = pair.second;
        if (other.id == apeId) continue;
        if (other.fatherId == apeId || other.motherId == apeId) addUnique(fam.childrenIds, other.id);
        const bool sameFather = fam.fatherId != 0 && other.fatherId == fam.fatherId;
        const bool sameMother = fam.motherId != 0 && other.motherId == fam.motherId;
        if (sameFather || sameMother) addUnique(fam.siblingIds, other.id);
    }
    return fam;
}

void ProfileViewManager::appointToCouncil(sim::SimulationRegistry& reg, sim::EntityID apeId, sim::CouncilRole role) {
    sim::ApeData* ape = reg.getApe(apeId);
    if (!ape) return;
    sim::VillageData* village = reg.getVillage(ape->villageId);

    if (village) {
        // One ape per seat, one seat per ape.
        if (role != sim::CouncilRole::None) {
            for (sim::EntityID id : village->members) {
                sim::ApeData* other = reg.getApe(id);
                if (other && other->councilRole == role) other->councilRole = sim::CouncilRole::None;
            }
        }
        for (const auto& seat : SEATS) {
            sim::EntityID* slot = seatSlot(*village, seat.role);
            if (slot && (*slot == apeId || seat.role == role)) *slot = 0;
        }
        if (sim::EntityID* slot = seatSlot(*village, role)) *slot = apeId;
    }
    ape->councilRole = role;
}

bool ProfileViewManager::handleEvent(const sf::Event& event, const sf::RenderWindow& window, const sf::View& letterboxView, sim::SimulationRegistry& reg, sim::EntityID controlledApeId) {
    if (!isInspecting()) return false;

    const sf::FloatRect panel(profilePanelPos.x, profilePanelPos.y, panelWidth, panelHeight);
    const sf::FloatRect header(profilePanelPos.x, profilePanelPos.y, panelWidth, HEADER_H);

    if (event.type == sf::Event::KeyPressed) {
        switch (event.key.code) {
            case sf::Keyboard::Escape:    close(); return true;
            case sf::Keyboard::BackSpace: if (!navHistory.empty()) { goBack(); return true; } return false;
            case sf::Keyboard::Num1: case sf::Keyboard::Numpad1: setTab(0); return true;
            case sf::Keyboard::Num2: case sf::Keyboard::Numpad2: setTab(1); return true;
            case sf::Keyboard::Num3: case sf::Keyboard::Numpad3: setTab(2); return true;
            default: return false;
        }
    }

    if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
        isDragging = false;
    }

    if (event.type == sf::Event::MouseMoved) {
        const sf::Vector2f vMouse = window.mapPixelToCoords(sf::Vector2i(event.mouseMove.x, event.mouseMove.y), letterboxView);
        if (isDragging) {
            profilePanelPos = vMouse - dragOffset;
            profilePanelPos.x = std::clamp(profilePanelPos.x, 10.f, 1270.f - panelWidth);
            profilePanelPos.y = std::clamp(profilePanelPos.y, 44.f, 710.f - panelHeight);
            return true;
        }
        if (panel.contains(vMouse)) return true;
    }

    if (event.type == sf::Event::MouseWheelScrolled) {
        const sf::Vector2f vMouse = window.mapPixelToCoords(sf::Vector2i(event.mouseWheelScroll.x, event.mouseWheelScroll.y), letterboxView);
        if (panel.contains(vMouse)) {
            listScroll -= (event.mouseWheelScroll.delta > 0.f) ? 1 : -1;
            return true;
        }
    }

    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        const sf::Vector2f vMouse = window.mapPixelToCoords(sf::Vector2i(event.mouseButton.x, event.mouseButton.y), letterboxView);

        // Later buttons are drawn on top, so they win. The action may close the
        // panel and clear the list, so it is copied out before it runs.
        for (auto it = interactiveButtons.rbegin(); it != interactiveButtons.rend(); ++it) {
            if (it->bounds.contains(vMouse)) {
                const std::function<void()> action = it->onClick;
                action();
                return true;
            }
        }

        if (header.contains(vMouse)) {
            isDragging = true;
            dragOffset = vMouse - profilePanelPos;
            return true;
        }
        if (panel.contains(vMouse)) return true;

        close();
        return false;
    }

    return false;
}

void ProfileViewManager::draw(sf::RenderWindow& window, sim::SimulationRegistry& reg, sim::EntityID controlledApeId) {
    if (!font || currentView == ViewType::None) return;

    interactiveButtons.clear();
    hasTip = false;

    Canvas c(window, *font);
    mouse = Canvas::mouse(window);

    switch (currentView) {
        case ViewType::Character: drawCharacterProfile(c, subjectId, reg, controlledApeId); break;
        case ViewType::Village:   drawVillageProfile(c, subjectId, reg, controlledApeId); break;
        case ViewType::Kingdom:   drawKingdomProfile(c, subjectId, reg, controlledApeId); break;
        default: break;
    }

    if (hasTip) c.tooltip(tipAnchor.x, tipAnchor.y, tip);
}

// ---------------------------------------------------------------------------
// Shared pieces
// ---------------------------------------------------------------------------
sf::FloatRect ProfileViewManager::drawChrome(ui::Canvas& c, const std::string& title, const std::string& subtitle,
                                             const char* const tabNames[TAB_COUNT], sf::Color banner) {
    const float x = profilePanelPos.x, y = profilePanelPos.y;
    c.panel({x, y, panelWidth, panelHeight});

    // header band: warm light across the top, fading into the panel
    c.gradient({x + c.px(2), y + c.px(2), panelWidth - c.px(4), HEADER_H - c.px(2)}, sf::Color(255, 214, 140, 40), sf::Color(255, 214, 140, 0));

    // drag grip
    for (int g = 0; g < 3; ++g) {
        c.fill({x + (panelWidth - 22.f) * 0.5f, y + 5.f + static_cast<float>(g) * 3.f, 22.f, c.px(1)}, theme::BronzeDim);
    }

    float left = x + 14.f;
    if (!navHistory.empty()) {
        const sf::FloatRect r(x + 10.f, y + 12.f, 56.f, 24.f);
        const bool hov = r.contains(mouse);
        c.button(r, false, hov);
        c.text("< Back", r.left + r.width * 0.5f, r.top + r.height * 0.5f, {10, hov ? theme::GoldBright : theme::Text, true, Align::Center});
        registerButton(r, [this]() { goBack(); });
        left = r.left + r.width + 10.f;
    }

    // small banner in the subject's colour, as on the top bar
    c.fill({left, y + 13.f, 8.f, 22.f}, theme::EdgeDark);
    c.fill({left + c.px(1), y + 13.f + c.px(1), 8.f - c.px(2), 22.f - c.px(2)}, banner);
    left += 16.f;

    const float textW = x + panelWidth - 44.f - left;
    c.text(c.fit(title, textW, 13, true), left, y + 19.f, {13, theme::Gold, true});
    c.text(c.fit(subtitle, textW, 10), left, y + 35.f, {10, theme::TextMuted});

    {
        const sf::FloatRect r(x + panelWidth - 34.f, y + 12.f, 24.f, 24.f);
        const bool hov = r.contains(mouse);
        c.button(r, false, hov);
        c.text("x", r.left + r.width * 0.5f, r.top + r.height * 0.5f - 1.f, {12, hov ? theme::Bad : theme::TextMuted, true, Align::Center});
        registerButton(r, [this]() { close(); });
    }

    c.fill({x + c.px(2), y + HEADER_H, panelWidth - c.px(4), c.px(1)}, theme::Bronze);
    c.fill({x + c.px(2), y + HEADER_H + c.px(1), panelWidth - c.px(4), c.px(1)}, sf::Color(0, 0, 0, 90));

    const float tabY = y + HEADER_H + 6.f, tabH = 26.f, gap = 4.f;
    const float tabW = (panelWidth - 20.f - gap * static_cast<float>(TAB_COUNT - 1)) / static_cast<float>(TAB_COUNT);
    for (int i = 0; i < TAB_COUNT; ++i) {
        const sf::FloatRect r(x + 10.f + static_cast<float>(i) * (tabW + gap), tabY, tabW, tabH);
        const bool active = (activeTab() == i);
        c.tab(r, std::to_string(i + 1), tabNames[i], active, r.contains(mouse));
        if (!active) registerButton(r, [this, i]() { setTab(i); });
    }

    const float top = tabY + tabH + 10.f;
    return sf::FloatRect(x + 14.f, top, panelWidth - 28.f, y + panelHeight - 14.f - top);
}

float ProfileViewManager::heading(ui::Canvas& c, sf::FloatRect area, float y, const std::string& title, const std::string& aside) {
    return c.heading(area.left, y, area.width, title, aside);
}

float ProfileViewManager::fact(ui::Canvas& c, sf::FloatRect area, float y, const std::string& label, const std::string& value, sf::Color valueColor) {
    const float lw = c.text(label, area.left, y + 8.f, {11, theme::TextMuted});
    c.text(c.fit(value, area.width - lw - 12.f, 11, true), area.left + area.width, y + 8.f, {11, valueColor, true, Align::Right});
    return y + 20.f;
}

float ProfileViewManager::linkCard(ui::Canvas& c, sf::FloatRect area, float y, const std::string& label, const std::string& title,
                                   const std::string& detail, const std::function<void()>& action) {
    const sf::FloatRect r(area.left, y, area.width, 46.f);
    const bool hov = action && r.contains(mouse);
    c.button(r, false, hov);
    c.text(label, r.left + 10.f, r.top + 13.f, {9, theme::Bronze, true});
    const float arrowW = action ? 16.f : 0.f;
    const float dw = detail.empty() ? 0.f : c.textWidth(detail, 10) + 12.f;
    c.text(c.fit(title, r.width - 20.f - arrowW - dw, 13, true), r.left + 10.f, r.top + 31.f,
           {13, hov ? theme::GoldBright : (action ? theme::Text : theme::TextMuted), true});
    if (!detail.empty()) c.text(detail, r.left + r.width - 10.f - arrowW, r.top + 31.f, {10, theme::TextMuted, false, Align::Right});
    if (action) c.text(">", r.left + r.width - 10.f, r.top + 31.f, {11, hov ? theme::GoldBright : theme::Bronze, true, Align::Right});
    registerButton(r, action);
    return y + 52.f;
}

void ProfileViewManager::drawList(ui::Canvas& c, sf::FloatRect area, float y, const std::vector<ListRow>& rows, const std::string& emptyText) {
    if (rows.empty()) {
        c.text(emptyText, area.left, y + 10.f, {11, theme::TextMuted, false, Align::Left, false, true});
        return;
    }

    const float rowH = 40.f, step = 44.f;
    const float bottom = area.top + area.height;
    const int visible = std::max(1, static_cast<int>((bottom - y + (step - rowH)) / step));
    const int count = static_cast<int>(rows.size());
    const int maxScroll = std::max(0, count - visible);
    listScroll = std::clamp(listScroll, 0, maxScroll);

    const float barW = maxScroll > 0 ? 10.f : 0.f;
    float ry = y;
    for (int i = listScroll; i < std::min(count, listScroll + visible); ++i) {
        const ListRow& row = rows[static_cast<size_t>(i)];
        const sf::FloatRect r(area.left, ry, area.width - barW, rowH);
        const bool link = static_cast<bool>(row.action);
        const bool hov = link && r.contains(mouse);
        c.button(r, false, hov);

        const float arrowW = link ? 16.f : 0.f;
        const float tagW = row.tag.empty() ? 0.f : c.textWidth(row.tag, 9, true) + 12.f;
        const sf::Color titleColor = hov ? theme::GoldBright : (row.dim ? theme::TextMuted : theme::Text);
        c.text(c.fit(row.title, r.width - 20.f - tagW, 12, true), r.left + 10.f, r.top + 13.f, {12, titleColor, !row.dim, Align::Left, false, row.dim});
        c.text(c.fit(row.detail, r.width - 20.f - arrowW, 10), r.left + 10.f, r.top + 28.f, {10, theme::TextMuted});
        if (!row.tag.empty()) c.text(row.tag, r.left + r.width - 10.f, r.top + 13.f, {9, row.tagColor, true, Align::Right});
        if (link) c.text(">", r.left + r.width - 10.f, r.top + 28.f, {11, hov ? theme::GoldBright : theme::Bronze, true, Align::Right});
        registerButton(r, row.action);
        ry += step;
    }

    if (maxScroll > 0) {
        const float trackH = static_cast<float>(visible) * step - (step - rowH);
        const sf::FloatRect track(area.left + area.width - 5.f, y, 5.f, trackH);
        c.fill(track, theme::PanelInset);
        const float thumbH = std::max(16.f, trackH * static_cast<float>(visible) / static_cast<float>(count));
        const float thumbY = y + (trackH - thumbH) * static_cast<float>(listScroll) / static_cast<float>(maxScroll);
        c.fill({track.left, thumbY, track.width, thumbH}, theme::Bronze);
    }
}

// ---------------------------------------------------------------------------
// Ape
// ---------------------------------------------------------------------------
void ProfileViewManager::drawCharacterProfile(ui::Canvas& c, sim::EntityID apeId, sim::SimulationRegistry& reg, sim::EntityID controlledApeId) {
    sim::ApeData* ape = reg.getApe(apeId);
    if (!ape) { close(); return; }

    sim::VillageData* village = reg.getVillage(ape->villageId);
    sim::KingdomData* kingdom = reg.getKingdom(ape->currentKingdom);
    const bool isPlayer = (ape->id == controlledApeId);

    const std::string rank = rankOf(reg, *ape);
    std::string where = kingdom ? "Kingdom of " + kingdom->name : (village ? village->name : "Wanderer");
    if (rank == "Alpha" && village) where = "Alpha of " + village->name;
    else if (rank == "King" && kingdom) where = "King of " + kingdom->name;
    else if (!rank.empty()) where = rank + ", " + where;
    const std::string subtitle = where + "  |  " + ageOf(*ape) + (isPlayer ? "  |  You" : "");

    const sf::FloatRect area = drawChrome(c, ape->name, subtitle, APE_TABS, kingdom ? kingdom->color : theme::Bronze);
    const float right = area.left + area.width;
    float y = area.top;

    if (activeTab() == 0) {
        // ---- skills -------------------------------------------------------------
        struct Skill { const char* name; float raw; sf::Color color; const char* note; };
        const Skill skills[6] = {
            { "MIGHT",  ape->skills.combat,      theme::Bad,      "Fighting. Warriors and guards live by it." },
            { "VOICE",  ape->skills.leadership,  theme::Gold,     "Leading others. What makes a chief worth following." },
            { "CRAFT",  ape->skills.building,    theme::Good,     "Building and mending." },
            { "FORAGE", ape->skills.gathering,   theme::Amber,    "Finding food, and bringing it home." },
            { "TIMBER", ape->skills.woodcutting, theme::Bronze,   "Felling trees and working wood." },
            { "SCOUT",  ape->skills.scouting,    theme::Prestige, "Ranging far and coming back with news." },
        };
        const float gap = 6.f, boxW = (area.width - gap * 2.f) / 3.f, boxH = 48.f;
        for (int i = 0; i < 6; ++i) {
            const Skill& s = skills[i];
            const int value = static_cast<int>(std::round(std::clamp(s.raw, 1.f, 25.f)));
            const sf::FloatRect r(area.left + static_cast<float>(i % 3) * (boxW + gap), y + static_cast<float>(i / 3) * (boxH + gap), boxW, boxH);
            c.button(r, false, r.contains(mouse));
            c.fill({r.left + c.px(1), r.top + c.px(1), r.width - c.px(2), c.px(2)}, s.color);
            c.text(s.name, r.left + 9.f, r.top + 15.f, {9, theme::TextMuted, true});
            c.text(gradeOf(value), r.left + r.width - 9.f, r.top + 15.f, {9, s.color, false, Align::Right});
            const float vw = c.text(std::to_string(value), r.left + 9.f, r.top + 33.f, {15, theme::Text, true});
            c.meter({r.left + 9.f + std::max(vw, 20.f) + 8.f, r.top + 29.f, r.width - 26.f - std::max(vw, 20.f), 8.f}, static_cast<float>(value) / 25.f, s.color);
            if (r.contains(mouse)) {
                Tooltip t;
                t.title = s.name;
                t.accent = s.color;
                t.rows.push_back({ "Skill", std::to_string(value) + " of 25", theme::Text });
                t.note = s.note;
                showTip(t, {r.left + r.width * 0.5f, r.top + r.height + 2.f});
            }
        }
        y += 2.f * (boxH + gap) + 6.f;

        // ---- nature ---------------------------------------------------------------
        y = heading(c, area, y, "NATURE");
        if (ape->traits.empty()) {
            c.text("Nothing sets this ape apart.", area.left, y + 8.f, {11, theme::TextMuted, false, Align::Left, false, true});
            y += 24.f;
        } else {
            float bx = area.left;
            for (sim::Trait tr : ape->traits) {
                const TraitInfo info = traitInfo(tr);
                const float w = c.textWidth(info.name, 10) + 20.f;
                if (bx + w > right) { bx = area.left; y += 24.f; }
                const sf::FloatRect r(bx, y, w, 20.f);
                c.button(r, false, false);
                c.fill({r.left, r.top, 4.f, r.height}, info.flaw ? theme::Bad : theme::Good);
                c.text(info.name, r.left + 12.f, r.top + 10.f, {10, theme::Text});
                bx += w + 6.f;
            }
            y += 30.f;
        }

        // ---- condition --------------------------------------------------------------
        y = heading(c, area, y, "CONDITION", ape->alive ? "" : "Dead");
        auto gauge = [&](const std::string& label, float value, sf::Color good) {
            const float v = std::clamp(value, 0.f, 100.f);
            const sf::Color col = v >= 60.f ? good : (v >= 30.f ? theme::Amber : theme::Bad);
            c.text(label, area.left, y + 8.f, {11, theme::TextMuted});
            c.meter({area.left + 70.f, y + 4.f, area.width - 70.f - 40.f, 8.f}, v / 100.f, col);
            c.text(std::to_string(static_cast<int>(v)), right, y + 8.f, {11, col, true, Align::Right});
            y += 20.f;
        };
        gauge("Health", ape->health, theme::Good);
        gauge("Fed", ape->hunger, theme::Good);
        y = fact(c, area, y, "Doing", jobName(ape->currentJob));
        if (ape->equippedTool != sim::ToolType::None) y = fact(c, area, y, "Carrying", toolName(ape->equippedTool));
        if (ape->currentDisease != sim::DiseaseType::None) y = fact(c, area, y, "Sick with", diseaseName(ape->currentDisease), theme::Bad);
        y += 8.f;

        // ---- opinion ----------------------------------------------------------------
        y = heading(c, area, y, "OPINION OF YOU");
        sim::ApeData* player = reg.getApe(controlledApeId);
        if (isPlayer) {
            c.text("This is you.", area.left, y + 8.f, {11, theme::TextMuted, false, Align::Left, false, true});
        } else if (player) {
            int op = 0;
            const auto it = ape->opinions.find(player->id);
            if (it != ape->opinions.end()) op = it->second;
            else if (ape->villageId != 0 && ape->villageId == player->villageId) op = 15;      // shared fire
            const sf::Color col = op >= 25 ? theme::Good : (op >= 0 ? theme::Gold : theme::Bad);
            c.balance({area.left, y + 4.f, area.width - 44.f, 10.f}, static_cast<float>(op) / 100.f);
            c.text((op >= 0 ? "+" : "") + std::to_string(op), right, y + 9.f, {13, col, true, Align::Right});
        }
        return;
    }

    const FamilyInfo fam = resolveFamily(apeId, reg);

    if (activeTab() == 1) {
        // ---- kin: every row is a way to that ape ----------------------------------------
        std::vector<ListRow> rows;
        auto kinRow = [&](const std::string& relation, sim::EntityID id, const std::string& extra, bool showUnknown) {
            sim::ApeData* rel = reg.getApe(id);
            if (!rel && !showUnknown) return;
            ListRow row;
            row.tag = upper(relation);
            row.tagColor = theme::Bronze;
            if (rel) {
                const std::string relRank = rankOf(reg, *rel);
                row.title = rel->name;
                row.detail = rel->alive ? ageOf(*rel) : "Dead";
                if (rel->alive && !relRank.empty()) row.detail += ", " + relRank;
                if (!extra.empty()) row.detail += ", " + extra;
                row.dim = !rel->alive;
                row.action = [this, id]() { inspectCharacter(id); };
            } else {
                row.title = id == 0 ? "Unknown" : "Lost to memory";
                row.detail = "No ape remembers who";
                row.dim = true;
            }
            rows.push_back(row);
        };

        kinRow("Liege", fam.liegeId, fam.liegeTitle, false);
        kinRow("Father", fam.fatherId, "", true);
        kinRow("Mother", fam.motherId, "", true);
        for (sim::EntityID id : fam.spouseIds)   kinRow("Mate", id, "", false);
        for (sim::EntityID id : fam.childrenIds) kinRow("Child", id, "", false);
        for (sim::EntityID id : fam.siblingIds)  kinRow("Sibling", id, "", false);

        drawList(c, area, y, rows, "This ape has no known kin.");
        return;
    }

    // ---- standing: seat, wealth, who they answer to --------------------------------------
    sim::ApeData* player = reg.getApe(controlledApeId);
    const bool canAppoint = player && village && village->leaderId == controlledApeId && !isPlayer && ape->alive;
    const SeatInfo* held = seatFor(ape->councilRole);

    y = heading(c, area, y, "COUNCIL SEAT", canAppoint ? "Click a seat to appoint, again to dismiss" : "");
    if (canAppoint) {
        const float gap = 4.f, bw = (area.width - gap * 3.f) / 4.f;
        sim::SimulationRegistry* registry = &reg;
        for (int i = 0; i < 4; ++i) {
            const SeatInfo& seat = SEATS[i];
            const sf::FloatRect r(area.left + static_cast<float>(i) * (bw + gap), y, bw, 26.f);
            const bool active = (ape->councilRole == seat.role);
            const bool hov = r.contains(mouse);
            c.tab(r, "", seat.shortName, active, hov);
            const sim::CouncilRole next = active ? sim::CouncilRole::None : seat.role;
            registerButton(r, [this, registry, apeId, next]() { appointToCouncil(*registry, apeId, next); });
            if (hov) {
                Tooltip t;
                t.title = upper(seat.title);
                sim::ApeData* current = seatHolder(reg, *village, seat.role);
                t.rows.push_back({ "Held by", current ? current->name : "No one", current ? theme::Text : theme::TextMuted });
                t.note = seat.duty;
                showTip(t, {r.left + r.width * 0.5f, r.top + r.height + 2.f});
            }
        }
        y += 36.f;
    } else {
        c.text(held ? held->title : (isPlayer ? "You lead the council" : "Holds no seat"), area.left, y + 9.f,
               {13, held || isPlayer ? theme::Text : theme::TextMuted, held != nullptr || isPlayer, Align::Left, false, !held && !isPlayer});
        if (held) c.text(held->duty, area.left, y + 27.f, {10, theme::TextMuted, false, Align::Left, false, true});
        y += held ? 44.f : 28.f;
    }

    y = heading(c, area, y, "WEALTH");
    {
        struct Purse { Icon icon; int value; const char* name; sf::Color color; };
        const Purse purses[3] = {
            { Icon::Amber, ape->amberCount, "Amber",    theme::Amber },
            { Icon::Crown, ape->prestige,   "Prestige", theme::Prestige },
            { Icon::Piety, ape->piety,      "Piety",    theme::Piety },
        };
        const float gap = 6.f, bw = (area.width - gap * 2.f) / 3.f;
        for (int i = 0; i < 3; ++i) {
            const sf::FloatRect r(area.left + static_cast<float>(i) * (bw + gap), y, bw, 32.f);
            c.inset(r);
            c.icon(purses[i].icon, r.left + 16.f, r.top + 16.f);
            c.text(std::to_string(purses[i].value), r.left + 30.f, r.top + 16.f, {13, theme::Text, true});
            c.text(purses[i].name, r.left + r.width - 8.f, r.top + 16.f, {9, purses[i].color, false, Align::Right});
        }
        y += 42.f;
    }

    y = heading(c, area, y, "ALLEGIANCE");
    if (sim::ApeData* liege = reg.getApe(fam.liegeId)) {
        const sim::EntityID liegeId = liege->id;
        y = linkCard(c, area, y, "SWORN TO", liege->name, fam.liegeTitle, [this, liegeId]() { inspectCharacter(liegeId); });
    }
    if (village) {
        const sim::VillageID vId = village->id;
        y = linkCard(c, area, y, "HOME FIRE", village->name,
                     std::string(tierName(village->tier)) + ", " + std::to_string(village->members.size()) + " apes",
                     [this, vId]() { inspectVillage(vId); });
    }
    if (kingdom) {
        const sim::KingdomID kId = kingdom->id;
        y = linkCard(c, area, y, "REALM", "Kingdom of " + kingdom->name,
                     std::to_string(kingdom->controlledVillages.size()) + (kingdom->controlledVillages.size() == 1 ? " county" : " counties"),
                     [this, kId]() { inspectKingdom(kId); });
    }
    if (!village && !kingdom) {
        c.text("A wanderer with no home fire.", area.left, y + 8.f, {11, theme::TextMuted, false, Align::Left, false, true});
    }
}

// ---------------------------------------------------------------------------
// County
// ---------------------------------------------------------------------------
void ProfileViewManager::drawVillageProfile(ui::Canvas& c, sim::VillageID vId, sim::SimulationRegistry& reg, sim::EntityID controlledApeId) {
    sim::VillageData* v = reg.getVillage(vId);
    if (!v) { close(); return; }

    sim::KingdomData* kd = reg.getKingdom(v->kingdomId);
    const bool isCapital = kd && kd->capitalVillageId == v->id;
    const std::string subtitle = kd ? (std::string(isCapital ? "Capital of the Kingdom of " : "County of the Kingdom of ") + kd->name) : "Free county";

    const sf::FloatRect area = drawChrome(c, v->name, subtitle, COUNTY_TABS, kd ? kd->color : theme::Bronze);
    float y = area.top;

    if (activeTab() == 0) {
        sim::ApeData* ruler = reg.getApe(v->leaderId);
        if (ruler) {
            const sim::EntityID id = ruler->id;
            y = linkCard(c, area, y, "ALPHA", ruler->name, ageOf(*ruler), [this, id]() { inspectCharacter(id); });
        } else {
            y = linkCard(c, area, y, "ALPHA", "No one leads here", "", nullptr);
        }
        if (kd) {
            const sim::KingdomID kId = kd->id;
            y = linkCard(c, area, y, "REALM", "Kingdom of " + kd->name,
                         std::to_string(kd->controlledVillages.size()) + (kd->controlledVillages.size() == 1 ? " county" : " counties"),
                         [this, kId]() { inspectKingdom(kId); });
        }

        y = heading(c, area, y + 2.f, "THE SETTLEMENT");
        y = fact(c, area, y, "Size", tierName(v->tier), theme::Gold);
        y = fact(c, area, y, "Way of life", identityName(v->identity));
        y = fact(c, area, y, "Apes", std::to_string(v->members.size()));
        std::string built = std::to_string(v->finishedStructures.size()) + " built";
        if (!v->constructionQueue.empty()) built += ", " + std::to_string(v->constructionQueue.size()) + " planned";
        y = fact(c, area, y, "Buildings", built);
        if (v->isExpandingBorder) y = fact(c, area, y, "Borders", "Being pushed outward", theme::Amber);
        if (v->isMigrating)       y = fact(c, area, y, "On the move", "The clan is migrating", theme::Amber);

        y = heading(c, area, y + 8.f, "COUNCIL");
        for (const auto& seat : SEATS) {
            sim::ApeData* holder = seatHolder(reg, *v, seat.role);
            const sf::FloatRect r(area.left, y, area.width, 22.f);
            const bool hov = holder && r.contains(mouse);
            if (hov) c.fill(r, theme::HoverFill);
            c.text(seat.title, r.left + 4.f, r.top + 11.f, {11, theme::TextMuted});
            c.text(holder ? holder->name : "empty", r.left + r.width - 4.f, r.top + 11.f,
                   {11, hov ? theme::GoldBright : (holder ? theme::Text : theme::BronzeDim), holder != nullptr, Align::Right, false, !holder});
            if (holder) {
                const sim::EntityID id = holder->id;
                registerButton(r, [this, id]() { inspectCharacter(id); });
            }
            y += 22.f;
        }
        return;
    }

    if (activeTab() == 1) {
        // ---- everyone who sleeps here, most important first ---------------------------
        struct Entry { sim::ApeData* ape; int weight; };
        std::vector<Entry> entries;
        for (sim::EntityID id : v->members) {
            sim::ApeData* a = reg.getApe(id);
            if (!a) continue;
            int weight = 0;
            if (a->id == v->leaderId) weight = 3;
            else if (a->councilRole != sim::CouncilRole::None) weight = 2;
            else if (a->alive) weight = 1;
            entries.push_back({ a, weight });
        }
        std::stable_sort(entries.begin(), entries.end(), [](const Entry& a, const Entry& b) {
            if (a.weight != b.weight) return a.weight > b.weight;
            return a.ape->age > b.ape->age;
        });

        std::vector<ListRow> rows;
        for (const Entry& e : entries) {
            const sim::ApeData& a = *e.ape;
            ListRow row;
            row.title = a.name;
            row.detail = a.alive ? ageOf(a) + ", " + jobName(a.currentJob) : "Dead";
            row.dim = !a.alive;
            const std::string rank = rankOf(reg, a);
            if (a.id == controlledApeId) { row.tag = "YOU"; row.tagColor = theme::Prestige; }
            else if (a.alive && !rank.empty()) { row.tag = upper(rank); row.tagColor = a.id == v->leaderId ? theme::Gold : theme::Text; }
            const sim::EntityID id = a.id;
            row.action = [this, id]() { inspectCharacter(id); };
            rows.push_back(row);
        }
        y = heading(c, area, y, "APES OF " + upper(v->name), std::to_string(rows.size()));
        drawList(c, area, y, rows, "No apes live here.");
        return;
    }

    // ---- stores ---------------------------------------------------------------------
    y = heading(c, area, y, "STOCKPILES");
    struct Stock { const char* name; int have; int cap; sf::Color color; };
    const Stock stocks[4] = {
        { "Food",  v->food,  v->maxFood,  theme::Good },
        { "Wood",  v->wood,  v->maxWood,  sf::Color(196, 138, 72) },
        { "Stone", v->stone, v->maxStone, sf::Color(168, 170, 178) },
        { "Amber", v->amber, v->maxAmber, theme::Amber },
    };
    for (const auto& s : stocks) {
        const float frac = s.cap > 0 ? static_cast<float>(s.have) / static_cast<float>(s.cap) : 0.f;
        c.text(s.name, area.left, y + 9.f, {11, theme::TextMuted});
        c.meter({area.left + 60.f, y + 4.f, area.width - 60.f - 86.f, 10.f}, frac, s.color);
        c.text(std::to_string(s.have) + " / " + std::to_string(s.cap), area.left + area.width, y + 9.f,
               {11, s.have >= s.cap && s.cap > 0 ? theme::Amber : theme::Text, true, Align::Right});
        y += 24.f;
    }

    y = heading(c, area, y + 8.f, "TOOLS");
    struct Tool { const char* name; int count; };
    const Tool tools[6] = {
        { "Axes", v->toolsAxe }, { "Picks", v->toolsPick }, { "Spears", v->toolsSpear },
        { "Torches", v->toolsTorch }, { "Baskets", v->toolsBasket }, { "Rope", v->toolsRope },
    };
    const float gap = 6.f, bw = (area.width - gap * 2.f) / 3.f;
    for (int i = 0; i < 6; ++i) {
        const sf::FloatRect r(area.left + static_cast<float>(i % 3) * (bw + gap), y + static_cast<float>(i / 3) * 34.f, bw, 28.f);
        c.inset(r);
        c.text(tools[i].name, r.left + 9.f, r.top + 14.f, {10, theme::TextMuted});
        c.text(std::to_string(tools[i].count), r.left + r.width - 9.f, r.top + 14.f,
               {13, tools[i].count > 0 ? theme::Text : theme::BronzeDim, true, Align::Right});
    }
    y += 2.f * 34.f + 8.f;

    if (!v->villageMemory.empty()) {
        y = heading(c, area, y, "REMEMBERED HERE");
        const float bottom = area.top + area.height;
        for (auto it = v->villageMemory.rbegin(); it != v->villageMemory.rend() && y < bottom - 28.f; ++it) {
            y = c.paragraph(*it, area.left, y + 8.f, area.width, {10, theme::Text}) + 2.f;
        }
    }
}

// ---------------------------------------------------------------------------
// Realm
// ---------------------------------------------------------------------------
void ProfileViewManager::drawKingdomProfile(ui::Canvas& c, sim::KingdomID kId, sim::SimulationRegistry& reg, sim::EntityID controlledApeId) {
    sim::KingdomData* k = reg.getKingdom(kId);
    if (!k) { close(); return; }

    sim::ApeData* player = reg.getApe(controlledApeId);
    sim::KingdomData* playerRealm = player ? reg.getKingdom(player->currentKingdom) : nullptr;

    // How this realm stands with the player's.
    StatusInfo standing = { "Unknown to you", theme::TextMuted };
    if (playerRealm && playerRealm->id == kId) standing = { "Your realm", theme::Gold };
    else if (playerRealm) {
        const auto it = playerRealm->relations.find(kId);
        standing = statusInfo(it != playerRealm->relations.end() ? it->second : sim::DiplomacyStatus::Neutral);
    }

    const sf::FloatRect area = drawChrome(c, "Kingdom of " + k->name, standing.name, REALM_TABS, k->color);
    float y = area.top;

    if (activeTab() == 0) {
        sim::ApeData* ruler = reg.getApe(k->currentKingId);
        if (ruler) {
            const sim::EntityID id = ruler->id;
            y = linkCard(c, area, y, "KING", ruler->name, ageOf(*ruler), [this, id]() { inspectCharacter(id); });
        } else {
            y = linkCard(c, area, y, "KING", "The throne is empty", "", nullptr);
        }
        if (sim::VillageData* cap = reg.getVillage(k->capitalVillageId)) {
            const sim::VillageID vId = cap->id;
            y = linkCard(c, area, y, "CAPITAL", cap->name, tierName(cap->tier), [this, vId]() { inspectVillage(vId); });
        }

        y = heading(c, area, y + 2.f, "STRENGTH");
        y = fact(c, area, y, "Subjects", std::to_string(k->population));
        y = fact(c, area, y, "Counties", std::to_string(k->controlledVillages.size()));
        y = fact(c, area, y, "Warriors", std::to_string(k->militaryStrength), theme::Amber);
        y = fact(c, area, y, "Standing with you", standing.name, standing.color);

        y = heading(c, area, y + 8.f, "TREASURY");
        struct Coffer { const char* name; int value; sf::Color color; };
        const Coffer coffers[4] = {
            { "Amber", k->treasuryAmber, theme::Amber }, { "Food", k->treasuryFood, theme::Good },
            { "Wood", k->treasuryWood, sf::Color(196, 138, 72) }, { "Stone", k->treasuryStone, sf::Color(168, 170, 178) },
        };
        const float gap = 6.f, bw = (area.width - gap * 3.f) / 4.f;
        for (int i = 0; i < 4; ++i) {
            const sf::FloatRect r(area.left + static_cast<float>(i) * (bw + gap), y, bw, 40.f);
            c.button(r, false, false);
            c.fill({r.left + c.px(1), r.top + c.px(1), r.width - c.px(2), c.px(2)}, coffers[i].color);
            c.text(coffers[i].name, r.left + 8.f, r.top + 14.f, {9, theme::TextMuted, true});
            c.text(std::to_string(coffers[i].value), r.left + 8.f, r.top + 29.f, {13, theme::Text, true});
        }
        y += 50.f;

        if (sim::ActiveWar* war = reg.getActiveWarForKingdom(kId)) {
            const bool attacking = (war->attackerKingdom == kId);
            sim::KingdomData* foe = reg.getKingdom(attacking ? war->defenderKingdom : war->attackerKingdom);
            y = heading(c, area, y, "AT WAR");
            y = fact(c, area, y, attacking ? "Attacking" : "Defending against", foe ? "Kingdom of " + foe->name : "an unknown realm", theme::Bad);
            y = fact(c, area, y, "Cause", war->casusBelli);
            // warScore is from the attacker's side.
            const float score = attacking ? war->warScore : -war->warScore;
            y = fact(c, area, y, "War score", (score >= 0.f ? "+" : "") + std::to_string(static_cast<int>(score)), score >= 0.f ? theme::Good : theme::Bad);
        }
        return;
    }

    if (activeTab() == 1) {
        std::vector<ListRow> rows;
        for (sim::VillageID vId : k->controlledVillages) {
            sim::VillageData* v = reg.getVillage(vId);
            if (!v) continue;
            ListRow row;
            row.title = v->name;
            row.detail = std::string(tierName(v->tier)) + ", " + std::to_string(v->members.size()) + " apes";
            if (sim::ApeData* leader = reg.getApe(v->leaderId)) row.detail += ", led by " + leader->name;
            if (vId == k->capitalVillageId) { row.tag = "CAPITAL"; row.tagColor = theme::Gold; }
            row.action = [this, vId]() { inspectVillage(vId); };
            if (vId == k->capitalVillageId) rows.insert(rows.begin(), row);
            else rows.push_back(row);
        }
        y = heading(c, area, y, "COUNTIES OF " + upper(k->name), std::to_string(rows.size()));
        drawList(c, area, y, rows, "This realm holds no land.");
        return;
    }

    // ---- diplomacy: how this realm stands with every other -------------------------------
    std::vector<sim::KingdomData*> others;
    for (auto& pair : reg.getAllKingdoms()) {
        if (pair.first != kId) others.push_back(&pair.second);
    }
    std::sort(others.begin(), others.end(), [](const sim::KingdomData* a, const sim::KingdomData* b) { return a->name < b->name; });

    std::vector<ListRow> rows;
    for (sim::KingdomData* other : others) {
        const auto rel = k->relations.find(other->id);
        const StatusInfo status = statusInfo(rel != k->relations.end() ? rel->second : sim::DiplomacyStatus::Neutral);
        ListRow row;
        row.title = "Kingdom of " + other->name;
        row.tag = upper(status.name);
        row.tagColor = status.color;

        const auto truce = k->truceYears.find(other->id);
        const auto tension = k->borderTension.find(other->id);
        if (truce != k->truceYears.end() && truce->second > reg.getYear()) {
            row.detail = "Truce until " + std::to_string(truce->second);
        } else if (tension != k->borderTension.end() && tension->second >= 1.f) {
            row.detail = "Border tension " + std::to_string(static_cast<int>(tension->second));
        } else {
            row.detail = std::to_string(other->controlledVillages.size()) + (other->controlledVillages.size() == 1 ? " county, " : " counties, ") +
                         std::to_string(other->militaryStrength) + " warriors";
        }
        if (playerRealm && other->id == playerRealm->id) row.detail += "  |  Your realm";

        const sim::KingdomID otherId = other->id;
        row.action = [this, otherId]() { inspectKingdom(otherId); };
        rows.push_back(row);
    }
    y = heading(c, area, y, "OTHER REALMS", std::to_string(rows.size()));
    drawList(c, area, y, rows, "No other realms are known.");
}
