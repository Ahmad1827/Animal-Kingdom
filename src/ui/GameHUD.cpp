#include "ui/GameHUD.h"
#include "ui/UIKit.h"
#include "simulation/SimulationManager.h"
#include "world/SettlementSystem.h"
#include <cmath>
#include <algorithm>
#include <string>
#include <iomanip>
#include <sstream>

namespace {

// Top bar geometry, in the UI view's design units (1280x720).
const float BAR_Y = 6.f;
const float BAR_H = 30.f;
const float MARGIN = 8.f;
const float GAP = 6.f;

const float SPEED_MULT[6] = { 0.f, 1.f, 2.f, 5.f, 15.f, 60.f };

std::string signedRate(float v) {
    std::ostringstream s;
    s << std::fixed << std::setprecision(1) << (v >= 0.f ? "+" : "") << v;
    return s.str();
}

sf::Color rateColor(float v) { return v >= 0.f ? ui::theme::Good : ui::theme::Bad; }

std::string speedLabel(int speed) {
    return "x" + std::to_string(static_cast<int>(SPEED_MULT[std::clamp(speed, 1, 5)]));
}

} // namespace

GameHUD::GameHUD()
    : font(nullptr), gameSpeed(1), previousGameSpeed(1), isGamePaused(false),
      amberPulseTimer(0.f), lastObservedAmber(-1) {}

void GameHUD::init(const sf::Font& f) {
    font = &f;
}

float GameHUD::getSpeedMultiplier() const {
    if (isGamePaused) return 0.0f;
    if (gameSpeed >= 1 && gameSpeed <= 5) return SPEED_MULT[gameSpeed];
    return 1.0f;
}

void GameHUD::setGameSpeed(int speed) {
    gameSpeed = std::clamp(speed, 1, 5);
    previousGameSpeed = gameSpeed;
    isGamePaused = false;
}

void GameHUD::toggleGamePause() {
    isGamePaused = !isGamePaused;
    if (!isGamePaused && gameSpeed == 0) {
        gameSpeed = (previousGameSpeed >= 1) ? previousGameSpeed : 1;
    }
}

void GameHUD::update(float dt, sim::ApeData* playerApe, sim::SimulationRegistry& reg) {
    if (amberPulseTimer > 0.f) {
        amberPulseTimer -= dt;
        if (amberPulseTimer < 0.f) amberPulseTimer = 0.f;
    }

    if (playerApe) {
        playerApe->maxAmber = 99999;
    }

    std::string playerK = SettlementSystem::getPlayerKingdomId();
    int demesneCount = std::max(1, SettlementSystem::getKingdomDemesneCount(playerK));
    bool atWar = SettlementSystem::isWarActive();
    cachedAtWar = atWar;

    // Same numbers as before, just stored per source so the tooltips can list them.
    float taxMult = SettlementSystem::getAuthorityTaxMultiplier();
    amberTaxes = (3.5f + static_cast<float>(demesneCount) * 2.2f) * taxMult;
    amberTrade = SettlementSystem::getKingdomTradeIncome(playerK);
    amberUpkeep = SettlementSystem::getKingdomArmyUpkeep(playerK);
    amberWar = atWar ? 2.5f : 0.f;
    cachedAmberRate = amberTaxes + amberTrade - amberUpkeep - amberWar;

    const auto& fState = SettlementSystem::getFactionState();
    bool disloyalVassals = !fState.memberCounties.empty();
    prestigeBase = 1.2f;
    prestigeDomain = static_cast<float>(demesneCount) * 0.4f;
    prestigeVassals = disloyalVassals ? -0.8f : 0.5f;
    cachedPrestigeRate = prestigeBase + prestigeDomain + prestigeVassals;

    const auto* shamanMission = SettlementSystem::getCouncilAssignment(sim::CouncilRole::Shaman);
    bool shamanActive = (shamanMission && shamanMission->mission != CouncilMissionType::None);
    pietyBase = 0.8f;
    pietyShaman = shamanActive ? 1.4f : 0.0f;
    pietyWar = atWar ? -0.3f : 0.2f;
    cachedPietyRate = pietyBase + pietyShaman + pietyWar;

    float simMult = getSpeedMultiplier();
    float dayStepTime = 0.35f;

    if (simMult > 0.f) {
        dayTickTimer += dt * simMult;
        while (dayTickTimer >= dayStepTime) {
            dayTickTimer -= dayStepTime;
            reg.advanceDays(1);
        }

        float deltaDays = (dt * simMult) / dayStepTime;
        float dailyAmber = cachedAmberRate / 30.0f;
        float dailyPrestige = cachedPrestigeRate / 30.0f;
        float dailyPiety = cachedPietyRate / 30.0f;

        amberAccumulator += dailyAmber * deltaDays;
        prestigeAccumulator += dailyPrestige * deltaDays;
        pietyAccumulator += dailyPiety * deltaDays;

        if (playerApe) {
            if (std::abs(amberAccumulator) >= 1.0f) {
                int d = static_cast<int>(amberAccumulator);
                playerApe->amberCount = std::clamp(playerApe->amberCount + d, -500, 99999);
                amberAccumulator -= static_cast<float>(d);
            }
            if (std::abs(prestigeAccumulator) >= 1.0f) {
                int d = static_cast<int>(prestigeAccumulator);
                playerApe->prestige = std::clamp(playerApe->prestige + d, 0, 99999);
                prestigeAccumulator -= static_cast<float>(d);
            }
            if (std::abs(pietyAccumulator) >= 1.0f) {
                int d = static_cast<int>(pietyAccumulator);
                playerApe->piety = std::clamp(playerApe->piety + d, 0, 99999);
                pietyAccumulator -= static_cast<float>(d);
            }
        }
    }

    if (playerApe) {
        if (lastObservedAmber != -1 && playerApe->amberCount != lastObservedAmber) {
            amberPulseTimer = 0.35f;
        }
        lastObservedAmber = playerApe->amberCount;
    }
}

bool GameHUD::handleEvent(const sf::Event& event, const sf::RenderWindow& window, const sf::View& letterboxView, sim::SimulationManager* simManager) {
    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2i clickPixel(event.mouseButton.x, event.mouseButton.y);
        sf::Vector2f uiCoords = window.mapPixelToCoords(clickPixel, letterboxView);
        for (int i = 0; i < 6; ++i) {
            if (timeButtonBounds[i].contains(uiCoords)) {
                if (i == 0) toggleGamePause();
                else setGameSpeed(i);
                return true;
            }
        }
    }

    if (event.type == sf::Event::KeyPressed) {
        if (event.key.code == sf::Keyboard::P) {
            toggleGamePause();
            return true;
        } else if (event.key.code == sf::Keyboard::Num1 || event.key.code == sf::Keyboard::Numpad1) {
            setGameSpeed(1);
            return true;
        } else if (event.key.code == sf::Keyboard::Num2 || event.key.code == sf::Keyboard::Numpad2) {
            setGameSpeed(2);
            return true;
        } else if (event.key.code == sf::Keyboard::Num3 || event.key.code == sf::Keyboard::Numpad3) {
            setGameSpeed(3);
            return true;
        } else if (event.key.code == sf::Keyboard::Num4 || event.key.code == sf::Keyboard::Numpad4) {
            setGameSpeed(4);
            return true;
        } else if (event.key.code == sf::Keyboard::Num5 || event.key.code == sf::Keyboard::Numpad5) {
            setGameSpeed(5);
            return true;
        } else if (event.key.code == sf::Keyboard::Add || event.key.code == sf::Keyboard::Equal) {
            if (isGamePaused) isGamePaused = false;
            else setGameSpeed(std::min(gameSpeed + 1, 5));
            return true;
        } else if (event.key.code == sf::Keyboard::Subtract || event.key.code == sf::Keyboard::Dash) {
            if (gameSpeed > 1) setGameSpeed(gameSpeed - 1);
            return true;
        }
    }
    return false;
}

void GameHUD::draw(sf::RenderWindow& window, const sim::ApeData* playerApe, sim::SimulationManager* simManager) {
    if (!font || !simManager) return;

    using namespace ui;
    Canvas c(window, *font);
    const sf::Vector2f mouse = Canvas::mouse(window);
    const float midY = BAR_Y + BAR_H * 0.5f;

    sim::SimulationRegistry& reg = simManager->getRegistry();

    // Filled in while drawing; the tooltip is drawn last so it sits on top of everything.
    bool    hasTip = false;
    Tooltip tip;
    float   tipX = 0.f;
    auto hover = [&](const sf::FloatRect& r) { return r.contains(mouse); };

    // ---- Realm name ------------------------------------------------------
    std::string realmName = SettlementSystem::getPlayerKingdomId();
    std::transform(realmName.begin(), realmName.end(), realmName.begin(), ::toupper);

    float x = c.left() + MARGIN;
    const float realmW = std::max(104.f, c.textWidth(realmName, 13, true) + 40.f);
    c.panel({x, BAR_Y, realmW, BAR_H});
    // small banner in the realm colour
    c.fill({x + 10.f, BAR_Y + 8.f, 8.f, 14.f}, theme::EdgeDark);
    c.fill({x + 10.f + c.px(1), BAR_Y + 8.f + c.px(1), 8.f - c.px(2), 14.f - c.px(2)}, sf::Color(196, 52, 60));
    c.fill({x + 10.f + c.px(1), BAR_Y + 8.f + c.px(1), 8.f - c.px(2), c.px(1)}, sf::Color(255, 150, 140));
    c.text(realmName, x + 26.f, midY, {13, theme::Gold, true});
    x += realmW + GAP;

    // ---- Resources: amber, prestige, piety --------------------------------
    struct Resource {
        Icon icon; int value; float rate; sf::Color rateTint; sf::Color valueTint;
    };
    const int amber = playerApe ? playerApe->amberCount : 0;
    const int prestige = playerApe ? playerApe->prestige : 100;
    const int piety = playerApe ? playerApe->piety : 50;

    // Amber value flashes bright for a moment when it changes (no size change, so it stays sharp).
    sf::Color amberTint = amber >= 0 ? theme::Text : theme::Bad;
    if (amberPulseTimer > 0.f && amber >= 0) amberTint = sf::Color::White;

    const Resource resources[3] = {
        { Icon::Amber, amber,    cachedAmberRate,    rateColor(cachedAmberRate), amberTint },
        { Icon::Crown, prestige, cachedPrestigeRate, cachedPrestigeRate >= 0.f ? theme::Prestige : theme::Bad, theme::Text },
        { Icon::Piety, piety,    cachedPietyRate,    cachedPietyRate >= 0.f ? theme::Piety : theme::Bad, theme::Text },
    };

    const float chipW = 122.f;
    const float resW = chipW * 3.f;
    c.panel({x, BAR_Y, resW, BAR_H});
    for (int i = 0; i < 3; ++i) {
        const float cx = x + chipW * static_cast<float>(i);
        const sf::FloatRect chip(cx, BAR_Y, chipW, BAR_H);
        if (hover(chip)) c.fill({cx + c.px(2), BAR_Y + c.px(2), chipW - c.px(4), BAR_H - c.px(4)}, sf::Color(255, 226, 160, 18));
        if (i > 0) c.divider(cx, BAR_Y + 6.f, BAR_H - 12.f);

        c.icon(resources[i].icon, cx + 18.f, midY);
        const float vw = c.text(std::to_string(resources[i].value), cx + 32.f, midY, {14, resources[i].valueTint, true});
        c.text(signedRate(resources[i].rate), cx + 32.f + vw + 6.f, midY, {11, resources[i].rateTint, true});

        if (hover(chip)) {
            hasTip = true;
            tipX = cx + chipW * 0.5f;
            tip = Tooltip();
            if (i == 0) {
                tip.title = "AMBER   " + std::to_string(amber);
                tip.accent = theme::Amber;
                tip.rows.push_back({"Taxes", signedRate(amberTaxes), theme::Good});
                if (amberTrade != 0.f)  tip.rows.push_back({"Trade", signedRate(amberTrade), rateColor(amberTrade)});
                if (amberUpkeep != 0.f) tip.rows.push_back({"Army upkeep", signedRate(-amberUpkeep), theme::Bad});
                if (amberWar != 0.f)    tip.rows.push_back({"War costs", signedRate(-amberWar), theme::Bad});
                tip.rows.push_back({"Per month", signedRate(cachedAmberRate), rateColor(cachedAmberRate), true});
                tip.note = "Your treasury. Pays for buildings and armies.";
            } else if (i == 1) {
                tip.title = "PRESTIGE   " + std::to_string(prestige);
                tip.accent = theme::Prestige;
                tip.rows.push_back({"Ruler", signedRate(prestigeBase), theme::Good});
                tip.rows.push_back({"Counties held", signedRate(prestigeDomain), theme::Good});
                tip.rows.push_back({prestigeVassals >= 0.f ? "Loyal vassals" : "Vassal faction", signedRate(prestigeVassals), rateColor(prestigeVassals)});
                tip.rows.push_back({"Per month", signedRate(cachedPrestigeRate), rateColor(cachedPrestigeRate), true});
                tip.note = "Your standing among the clans.";
            } else {
                tip.title = "PIETY   " + std::to_string(piety);
                tip.accent = theme::Piety;
                tip.rows.push_back({"Ruler", signedRate(pietyBase), theme::Good});
                if (pietyShaman != 0.f) tip.rows.push_back({"Shaman on a mission", signedRate(pietyShaman), theme::Good});
                tip.rows.push_back({cachedAtWar ? "At war" : "At peace", signedRate(pietyWar), rateColor(pietyWar)});
                tip.rows.push_back({"Per month", signedRate(cachedPietyRate), rateColor(cachedPietyRate), true});
                tip.note = "Favour of the spirits.";
            }
        }
    }
    x += resW + GAP;

    // ---- Realm status: domain and levies ----------------------------------
    std::string playerKDraw = SettlementSystem::getPlayerKingdomId();
    int demesneCount = SettlementSystem::getKingdomDemesneCount(playerKDraw);
    int maxDemesne = 4;
    int raisedTroops = SettlementSystem::getKingdomRaisedTroops(playerKDraw);
    int levyQuota = SettlementSystem::getAuthorityLevyPerCounty();
    int maxLevies = std::max(25, demesneCount * levyQuota);
    const bool overLimit = demesneCount > maxDemesne;

    const float domW = 112.f;
    const float levW = 130.f;
    c.panel({x, BAR_Y, domW + levW, BAR_H});

    {   // Domain
        const sf::FloatRect chip(x, BAR_Y, domW, BAR_H);
        if (hover(chip)) c.fill({x + c.px(2), BAR_Y + c.px(2), domW - c.px(4), BAR_H - c.px(4)}, sf::Color(255, 226, 160, 18));
        c.icon(Icon::Tower, x + 18.f, midY, overLimit ? sf::Color(255, 130, 120) : sf::Color::White);
        const float lw = c.text("Domain", x + 32.f, midY, {11, theme::TextMuted});
        c.text(std::to_string(demesneCount) + "/" + std::to_string(maxDemesne), x + 32.f + lw + 6.f, midY,
               {13, overLimit ? theme::Bad : theme::Text, true});
        if (hover(chip)) {
            hasTip = true;
            tipX = x + domW * 0.5f;
            tip = Tooltip();
            tip.title = "DOMAIN LIMIT";
            tip.accent = overLimit ? theme::Bad : theme::Gold;
            tip.rows.push_back({"Counties you rule directly", std::to_string(demesneCount), theme::Text});
            tip.rows.push_back({"Limit", std::to_string(maxDemesne), theme::Text});
            tip.note = "Going over the limit lowers vassal loyalty and taxes.";
        }
    }
    {   // Levies
        const float lx = x + domW;
        const sf::FloatRect chip(lx, BAR_Y, levW, BAR_H);
        if (hover(chip)) c.fill({lx + c.px(2), BAR_Y + c.px(2), levW - c.px(4), BAR_H - c.px(4)}, sf::Color(255, 226, 160, 18));
        c.divider(lx, BAR_Y + 6.f, BAR_H - 12.f);
        const bool raised = raisedTroops > 0;
        c.icon(Icon::Swords, lx + 18.f, midY, raised ? sf::Color(255, 190, 130) : sf::Color::White);
        const float lw = c.text("Levies", lx + 32.f, midY, {11, theme::TextMuted});
        const std::string levStr = raised ? (std::to_string(raisedTroops) + "/" + std::to_string(maxLevies))
                                          : std::to_string(maxLevies);
        c.text(levStr, lx + 32.f + lw + 6.f, midY, {13, raised ? theme::Amber : theme::Text, true});
        if (hover(chip)) {
            int auth = SettlementSystem::getCrownAuthority();
            static const std::string authNames[] = { "Autonomous", "Limited", "High", "Absolute" };
            std::string authLabel = (auth >= 1 && auth <= 4) ? authNames[auth - 1] : "Standard";

            hasTip = true;
            tipX = lx + levW * 0.5f;
            tip = Tooltip();
            tip.title = "REALM LEVIES";
            tip.accent = theme::Amber;
            tip.rows.push_back({"Warriors you can muster", std::to_string(maxLevies), theme::Text});
            tip.rows.push_back({"Raised in the field", std::to_string(raisedTroops), raised ? theme::Amber : theme::Text});
            tip.rows.push_back({"Crown authority", authLabel + " (" + std::to_string(levyQuota) + " per county)", theme::Text});
            tip.note = "Raised armies cost amber every month.";
        }
    }

    // ---- Right side: game speed and date ----------------------------------
    const float dateW = 164.f;
    const float dateX = c.right() - MARGIN - dateW;

    const float pauseW = 30.f;
    const float barSlot = 15.f;
    const float labelW = 40.f;
    const float speedW = pauseW + 8.f + barSlot * 5.f + labelW;
    const float speedX = dateX - GAP - speedW;

    c.panel({speedX, BAR_Y, speedW, BAR_H});

    // Pause button
    timeButtonBounds[0] = sf::FloatRect(speedX, BAR_Y, pauseW, BAR_H);
    {
        const bool hov = hover(timeButtonBounds[0]);
        const sf::FloatRect b(speedX + 4.f, BAR_Y + 4.f, pauseW - 8.f, BAR_H - 8.f);
        c.button(b, isGamePaused, hov);
        const sf::Color ic = isGamePaused ? theme::GoldBright : (hov ? theme::Text : theme::TextMuted);
        const float bcx = b.left + b.width * 0.5f;
        c.fill({bcx - 5.f, midY - 5.f, 3.f, 10.f}, ic);
        c.fill({bcx + 2.f, midY - 5.f, 3.f, 10.f}, ic);
        if (hov) {
            hasTip = true;
            tipX = bcx;
            tip = Tooltip();
            tip.title = isGamePaused ? "RESUME" : "PAUSE";
            tip.rows.push_back({"Shortcut", "P", theme::Text});
        }
    }

    // Speed meter: five bars, taller = faster. Click a bar to pick that speed.
    const float barsX = speedX + pauseW + 4.f;
    int hoveredSpeed = 0;
    for (int i = 1; i <= 5; ++i) {
        timeButtonBounds[i] = sf::FloatRect(barsX + barSlot * static_cast<float>(i - 1), BAR_Y, barSlot, BAR_H);
        if (hover(timeButtonBounds[i])) hoveredSpeed = i;
    }
    for (int i = 1; i <= 5; ++i) {
        const float bx = barsX + barSlot * static_cast<float>(i - 1) + 3.f;
        const float bh = 4.f + 3.f * static_cast<float>(i);
        const float baseY = BAR_Y + BAR_H - 7.f;
        sf::Color col = theme::BronzeDim;
        if (i <= gameSpeed) col = isGamePaused ? theme::Bronze : theme::Gold;
        if (hoveredSpeed > 0 && i <= hoveredSpeed) col = theme::GoldBright;
        c.fill({bx - c.px(1), baseY - bh - c.px(1), 9.f + c.px(2), bh + c.px(2)}, theme::EdgeDark);
        c.fill({bx, baseY - bh, 9.f, bh}, col);
    }
    c.text(speedLabel(hoveredSpeed > 0 ? hoveredSpeed : gameSpeed),
           speedX + speedW - 8.f, midY,
           {12, isGamePaused && hoveredSpeed == 0 ? theme::TextMuted : theme::Gold, true, Align::Right});
    if (hoveredSpeed > 0) {
        hasTip = true;
        tipX = barsX + barSlot * (static_cast<float>(hoveredSpeed) - 0.5f);
        tip = Tooltip();
        tip.title = "SPEED " + std::to_string(hoveredSpeed) + "   " + speedLabel(hoveredSpeed);
        tip.rows.push_back({"Shortcut", std::to_string(hoveredSpeed), theme::Text});
        tip.rows.push_back({"Faster / slower", "+  /  -", theme::Text});
    }

    // Date, with sun or moon for the time of day
    float timeOfDay = simManager->getClock().getTimeOfDay();
    float t24 = timeOfDay * 24.0f;
    bool isDay = (t24 >= 5.5f && t24 < 18.5f);

    int year = reg.getYear();
    int month = reg.getMonth();
    int day = reg.getDay();
    std::string mName = reg.getMonthName(month);
    std::string dateStr = std::to_string(day) + " " + mName + ", " + std::to_string(year);

    const sf::FloatRect dateRect(dateX, BAR_Y, dateW, BAR_H);
    c.panel(dateRect);
    c.icon(isDay ? Icon::Sun : Icon::Moon, dateX + 18.f, midY);
    c.divider(dateX + 34.f, BAR_Y + 6.f, BAR_H - 12.f);
    c.text(dateStr, dateX + 34.f + (dateW - 34.f) * 0.5f, midY, {13, theme::Gold, true, Align::Center});
    if (hover(dateRect)) {
        int hh = static_cast<int>(t24) % 24;
        int mm = static_cast<int>((t24 - std::floor(t24)) * 60.f);
        std::ostringstream clock;
        clock << std::setw(2) << std::setfill('0') << hh << ":" << std::setw(2) << std::setfill('0') << mm;
        hasTip = true;
        tipX = dateX + dateW * 0.5f;
        tip = Tooltip();
        tip.title = isDay ? "DAY" : "NIGHT";
        tip.rows.push_back({"Time", clock.str(), theme::Text});
    }

    // Paused banner under the bar so a stopped clock is never missed
    if (isGamePaused) {
        const float bw = 150.f;
        const float bx = (c.left() + c.right()) * 0.5f - bw * 0.5f;
        const float by = BAR_Y + BAR_H + 8.f;
        c.panel({bx, by, bw, 22.f});
        c.text("PAUSED", bx + 12.f, by + 11.f, {12, theme::Bad, true});
        c.text("P to resume", bx + bw - 12.f, by + 11.f, {10, theme::TextMuted, false, Align::Right});
    }

    if (hasTip) c.tooltip(tipX, BAR_Y + BAR_H + 6.f, tip);
}