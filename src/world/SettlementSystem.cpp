#include "world/SettlementSystem.h"
#include <cmath>
#include <algorithm>

SettlementSystem::SettlementSystem() {
    fontLoaded = font.loadFromFile("assets/fonts/Cinzel-Bold.ttf") ||
                 font.loadFromFile("assets/fonts/Cinzel-Regular.ttf") ||
                 font.loadFromFile("font.ttf") ||
                 font.loadFromFile("assets/fonts/font.ttf");

    buildAuthenticMapGeometry();
}

void SettlementSystem::buildAuthenticMapGeometry() {
    mapOuterVellum.setSize(sf::Vector2f(1060.f, 650.f));
    mapOuterVellum.setOrigin(530.f, 325.f);
    mapOuterVellum.setPosition(640.f, 360.f);
    mapOuterVellum.setFillColor(sf::Color(214, 196, 161));
    mapOuterVellum.setOutlineColor(sf::Color(44, 28, 16));
    mapOuterVellum.setOutlineThickness(4.f);

    mapInnerVellum.setSize(sf::Vector2f(1036.f, 626.f));
    mapInnerVellum.setOrigin(518.f, 313.f);
    mapInnerVellum.setPosition(640.f, 360.f);
    mapInnerVellum.setFillColor(sf::Color(226, 210, 178));
    mapInnerVellum.setOutlineColor(sf::Color(135, 95, 45));
    mapInnerVellum.setOutlineThickness(2.f);

    mapInnerBorder.setSize(sf::Vector2f(1016.f, 606.f));
    mapInnerBorder.setOrigin(508.f, 303.f);
    mapInnerBorder.setPosition(640.f, 360.f);
    mapInnerBorder.setFillColor(sf::Color(186, 204, 208, 170));
    mapInnerBorder.setOutlineColor(sf::Color(90, 65, 35));
    mapInnerBorder.setOutlineThickness(1.5f);

    std::vector<sf::Vector2f> britPts = {
        {260.f, 575.f}, {275.f, 585.f}, {310.f, 570.f}, {365.f, 565.f},
        {425.f, 560.f}, {480.f, 555.f}, {535.f, 535.f}, {525.f, 505.f},
        {575.f, 470.f}, {540.f, 435.f}, {525.f, 390.f}, {530.f, 355.f},
        {505.f, 315.f}, {490.f, 260.f}, {465.f, 225.f}, {480.f, 210.f},
        {485.f, 190.f}, {510.f, 155.f}, {455.f, 145.f}, {460.f, 100.f},
        {415.f, 110.f}, {395.f, 150.f}, {375.f, 195.f}, {380.f, 225.f},
        {385.f, 265.f}, {415.f, 265.f}, {390.f, 295.f}, {435.f, 300.f},
        {440.f, 340.f}, {430.f, 380.f}, {410.f, 400.f}, {355.f, 395.f},
        {340.f, 420.f}, {360.f, 445.f}, {330.f, 475.f}, {370.f, 500.f},
        {415.f, 505.f}, {320.f, 530.f}
    };
    britainCoast.setPointCount(britPts.size());
    for (size_t i = 0; i < britPts.size(); ++i) britainCoast.setPoint(i, britPts[i]);
    britainCoast.setFillColor(sf::Color(215, 198, 160));
    britainCoast.setOutlineColor(sf::Color(65, 45, 25));
    britainCoast.setOutlineThickness(2.f);

    std::vector<sf::Vector2f> irePts = {
        {230.f, 335.f}, {270.f, 325.f}, {285.f, 360.f}, {275.f, 415.f},
        {250.f, 475.f}, {205.f, 485.f}, {180.f, 430.f}, {195.f, 360.f}
    };
    irelandCoast.setPointCount(irePts.size());
    for (size_t i = 0; i < irePts.size(); ++i) irelandCoast.setPoint(i, irePts[i]);
    irelandCoast.setFillColor(sf::Color(205, 188, 150));
    irelandCoast.setOutlineColor(sf::Color(75, 55, 35));
    irelandCoast.setOutlineThickness(1.5f);

    std::vector<sf::Vector2f> fraPts = {
        {160.f, 630.f}, {320.f, 615.f}, {540.f, 605.f}, {740.f, 615.f},
        {880.f, 645.f}, {880.f, 660.f}, {160.f, 660.f}
    };
    frankiaCoast.setPointCount(fraPts.size());
    for (size_t i = 0; i < fraPts.size(); ++i) frankiaCoast.setPoint(i, fraPts[i]);
    frankiaCoast.setFillColor(sf::Color(210, 192, 155));
    frankiaCoast.setOutlineColor(sf::Color(70, 50, 30));
    frankiaCoast.setOutlineThickness(1.5f);

    std::vector<sf::Vector2f> scaPts = {
        {720.f, 80.f}, {800.f, 70.f}, {890.f, 80.f}, {940.f, 135.f},
        {920.f, 210.f}, {845.f, 215.f}, {780.f, 175.f}, {710.f, 120.f}
    };
    scandiCoast.setPointCount(scaPts.size());
    for (size_t i = 0; i < scaPts.size(); ++i) scandiCoast.setPoint(i, scaPts[i]);
    scandiCoast.setFillColor(sf::Color(202, 185, 152));
    scandiCoast.setOutlineColor(sf::Color(70, 55, 38));
    scandiCoast.setOutlineThickness(1.5f);

    rhumbLines.clear();
    sf::Vector2f roseA(760.f, 360.f);
    sf::Vector2f roseB(320.f, 240.f);
    for (int i = 0; i < 16; ++i) {
        float a = i * (3.14159265f / 8.f);
        sf::Vector2f d(std::cos(a), std::sin(a));
        rhumbLines.push_back(sf::Vertex(roseA, sf::Color(140, 95, 55, 60)));
        rhumbLines.push_back(sf::Vertex(roseA + d * 380.f, sf::Color(140, 95, 55, 0)));
        rhumbLines.push_back(sf::Vertex(roseB, sf::Color(70, 105, 130, 45)));
        rhumbLines.push_back(sf::Vertex(roseB + d * 280.f, sf::Color(70, 105, 130, 0)));
    }

    seaWaves.clear();
    auto addWave = [&](float x, float y) {
        sf::Color wCol(115, 145, 155, 140);
        seaWaves.push_back(sf::Vertex(sf::Vector2f(x, y), wCol));
        seaWaves.push_back(sf::Vertex(sf::Vector2f(x + 10.f, y - 3.f), wCol));
        seaWaves.push_back(sf::Vertex(sf::Vector2f(x + 10.f, y - 3.f), wCol));
        seaWaves.push_back(sf::Vertex(sf::Vector2f(x + 20.f, y), wCol));
        seaWaves.push_back(sf::Vertex(sf::Vector2f(x + 20.f, y), wCol));
        seaWaves.push_back(sf::Vertex(sf::Vector2f(x + 30.f, y - 3.f), wCol));
    };
    addWave(630.f, 260.f);
    addWave(680.f, 330.f);
    addWave(610.f, 410.f);
    addWave(720.f, 480.f);
    addWave(250.f, 280.f);
    addWave(310.f, 350.f);
}

void SettlementSystem::syncDynamicVillages(sim::SimulationRegistry& registry) {
    struct HistProfile {
        std::string hist;
        std::string modern;
        std::string kingdom;
        bool allied;
        sf::Vector2f mapPos;
    };

    std::vector<HistProfile> profiles = {
        {"Wintanceaster", "Winchester", "Kingdom of Wessex", true, {430.f, 510.f}},
        {"Readingas", "Reading", "Royal Wessex Burh", true, {475.f, 485.f}},
        {"Lundenburh", "London", "March of Wessex & Mercia", true, {520.f, 485.f}},
        {"Theodford", "Thetford", "Kingdom of East Anglia", false, {555.f, 440.f}},
        {"Tamworthig", "Tamworth", "Kingdom of Mercia", true, {455.f, 425.f}},
        {"Legaceaster", "Chester", "March of Wales", false, {395.f, 385.f}},
        {"Lindcylene", "Lincoln", "Five Boroughs of Danelaw", false, {515.f, 390.f}},
        {"Jorvik", "York", "Kingdom of Jorvik", false, {490.f, 340.f}},
        {"Dunholm", "Durham", "Kingdom of Northumbria", false, {470.f, 285.f}},
        {"Bebbanburg", "Bamburgh", "Kingdom of Bernicia", false, {475.f, 235.f}},
        {"Dun Eideann", "Edinburgh", "Kingdom of Alba", false, {440.f, 195.f}},
        {"Sgain", "Scone", "Kingdom of Alba", false, {430.f, 165.f}},
        {"Hrossey Sound", "Orkney Coast", "Norse Jarldom", false, {445.f, 110.f}},
        {"Kaupang Fjord", "Skagerrak Reach", "Viking Shore to Scandinavia", false, {740.f, 130.f}}
    };

    auto allVillages = registry.getAllVillages();
    std::vector<const sim::VillageData*> sorted;
    for (const auto& pair : allVillages) {
        sorted.push_back(&pair.second);
    }
    std::sort(sorted.begin(), sorted.end(), [](const sim::VillageData* a, const sim::VillageData* b) {
        return a->centerX < b->centerX;
    });

    realSettlements.clear();

    for (size_t i = 0; i < sorted.size(); ++i) {
        const sim::VillageData* v = sorted[i];
        HistProfile hp = profiles[i % profiles.size()];

        RealSettlement rs;
        rs.villageId = v->id;
        rs.kingdomId = v->kingdomId;
        rs.borderLeftX = v->borderMinX;
        rs.borderRightX = v->borderMaxX;
        rs.centerX = v->centerX;

        if (v->kingdomId != 0) {
            sim::KingdomData* kd = registry.getKingdom(v->kingdomId);
            if (kd) {
                hp.kingdom = "Kingdom of " + kd->name;
            }
        }

        rs.historicalName = hp.hist;
        rs.modernName = hp.modern;
        rs.kingdomName = hp.kingdom;
        rs.isAllied = hp.allied;
        rs.mapCoord = hp.mapPos;

        realSettlements.push_back(rs);
    }
}

void SettlementSystem::update(float dt, float playerX, sim::SimulationRegistry& registry) {
    if (realSettlements.empty() || registry.getAllVillages().size() != realSettlements.size()) {
        syncDynamicVillages(registry);
    }

    if (!hasExplored) {
        minExploredX = playerX - 500.f;
        maxExploredX = playerX + 500.f;
        hasExplored = true;
    } else {
        minExploredX = std::min(minExploredX, playerX);
        maxExploredX = std::max(maxExploredX, playerX);
    }

    pulseTime += dt;

    int currentIdx = -1;
    for (size_t i = 0; i < realSettlements.size(); ++i) {
        if (playerX >= realSettlements[i].borderLeftX && playerX <= realSettlements[i].borderRightX) {
            currentIdx = static_cast<int>(i);
            break;
        }
    }

    if (currentIdx != activeSettlementIdx) {
        lastSettlementIdx = activeSettlementIdx;
        activeSettlementIdx = currentIdx;

        if (activeSettlementIdx != -1) {
            const auto& curr = realSettlements[activeSettlementIdx];
            bannerOldName = curr.historicalName;
            bannerModernName = curr.modernName;
            bannerKingdom = curr.kingdomName;
            bannerAllied = curr.isAllied;
            isExiting = false;
            bannerTimer = 0.f;
            showBanner = true;
        } else if (lastSettlementIdx != -1 && lastSettlementIdx < static_cast<int>(realSettlements.size())) {
            const auto& prev = realSettlements[lastSettlementIdx];
            bannerOldName = prev.historicalName;
            bannerModernName = prev.modernName;
            bannerKingdom = prev.kingdomName;
            bannerAllied = prev.isAllied;
            isExiting = true;
            bannerTimer = 0.f;
            showBanner = true;
        }
    }

    if (showBanner) {
        bannerTimer += dt;
        if (bannerTimer >= 5.2f) {
            showBanner = false;
        }
    }
}

void SettlementSystem::draw(sf::RenderWindow& window, const sf::View& letterboxView) {
    if (!fontLoaded || !showBanner) return;

    window.setView(letterboxView);

    float alpha = 0.f;
    float morphT = 0.f;

    if (bannerTimer < 0.5f) {
        alpha = bannerTimer / 0.5f;
    } else if (bannerTimer < 1.9f) {
        alpha = 1.0f;
    } else if (bannerTimer < 3.1f) {
        alpha = 1.0f;
        morphT = (bannerTimer - 1.9f) / 1.2f;
    } else if (bannerTimer < 4.2f) {
        alpha = 1.0f;
        morphT = 1.0f;
    } else {
        alpha = 1.0f - ((bannerTimer - 4.2f) / 1.0f);
        morphT = 1.0f;
    }

    alpha = std::clamp(alpha, 0.0f, 1.0f);
    morphT = std::clamp(morphT, 0.0f, 1.0f);
    sf::Uint8 byteAlpha = static_cast<sf::Uint8>(alpha * 255);

    float centerY = 88.f;
    std::string prefix = isExiting ? "Departing Boundary of " : "Entering Realm of ";
    sf::Color realmColor = bannerAllied ? sf::Color(140, 215, 125, byteAlpha) : sf::Color(225, 150, 90, byteAlpha);

    sf::Text kingdomText(prefix + bannerKingdom, font, 13);
    kingdomText.setStyle(sf::Text::Italic);
    kingdomText.setFillColor(realmColor);
    kingdomText.setOutlineColor(sf::Color(0, 0, 0, byteAlpha));
    kingdomText.setOutlineThickness(1.5f);
    sf::FloatRect kb = kingdomText.getLocalBounds();
    kingdomText.setOrigin(kb.left + kb.width / 2.f, kb.top + kb.height / 2.f);
    kingdomText.setPosition(640.f, centerY - 24.f);
    window.draw(kingdomText);

    if (morphT < 1.0f) {
        sf::Uint8 oldAlpha = static_cast<sf::Uint8>((1.0f - morphT) * 255 * alpha);
        sf::Text oldText(bannerOldName, font, 24);
        oldText.setStyle(sf::Text::Bold);
        oldText.setFillColor(sf::Color(255, 225, 140, oldAlpha));
        oldText.setOutlineColor(sf::Color(0, 0, 0, oldAlpha));
        oldText.setOutlineThickness(2.0f);
        sf::FloatRect ob = oldText.getLocalBounds();
        oldText.setOrigin(ob.left + ob.width / 2.f, ob.top + ob.height / 2.f);
        oldText.setPosition(640.f, centerY + 8.f);
        window.draw(oldText);
    }

    if (morphT > 0.0f) {
        sf::Uint8 modAlpha = static_cast<sf::Uint8>(morphT * 255 * alpha);
        sf::Text modernText(bannerModernName, font, 24);
        modernText.setStyle(sf::Text::Bold);
        modernText.setFillColor(sf::Color(245, 245, 250, modAlpha));
        modernText.setOutlineColor(sf::Color(0, 0, 0, modAlpha));
        modernText.setOutlineThickness(2.0f);
        sf::FloatRect mb = modernText.getLocalBounds();
        modernText.setOrigin(mb.left + mb.width / 2.f, mb.top + mb.height / 2.f);
        modernText.setPosition(640.f, centerY + 8.f);
        window.draw(modernText);
    }

    float lineHalfW = 150.f * alpha;
    sf::Color lineClr = bannerAllied ? sf::Color(120, 200, 110, byteAlpha) : sf::Color(210, 140, 80, byteAlpha);
    sf::Color edgeClr = bannerAllied ? sf::Color(120, 200, 110, 0) : sf::Color(210, 140, 80, 0);

    sf::Vertex line[] = {
        sf::Vertex(sf::Vector2f(640.f - lineHalfW, centerY + 30.f), edgeClr),
        sf::Vertex(sf::Vector2f(640.f, centerY + 30.f), lineClr),
        sf::Vertex(sf::Vector2f(640.f + lineHalfW, centerY + 30.f), edgeClr)
    };
    window.draw(line, 3, sf::LinesStrip);
}

void SettlementSystem::drawWorldMap(sf::RenderWindow& window, const sf::View& letterboxView, float playerX, const sim::SimulationRegistry& registry) {
    if (!fontLoaded) return;

    window.setView(letterboxView);

    sf::RectangleShape backdrop(sf::Vector2f(1280.f, 720.f));
    backdrop.setFillColor(sf::Color(12, 10, 8, 225));
    window.draw(backdrop);

    window.draw(mapOuterVellum);
    window.draw(mapInnerVellum);
    window.draw(mapInnerBorder);

    if (!rhumbLines.empty()) window.draw(rhumbLines.data(), rhumbLines.size(), sf::Lines);
    if (!seaWaves.empty()) window.draw(seaWaves.data(), seaWaves.size(), sf::Lines);

    window.draw(frankiaCoast);
    window.draw(scandiCoast);
    window.draw(irelandCoast);
    window.draw(britainCoast);

    sf::Text header("BRITANNIA ET PARTES SEPTENTRIONALES", font, 15);
    header.setStyle(sf::Text::Bold);
    header.setFillColor(sf::Color(65, 42, 20));
    sf::FloatRect hb = header.getLocalBounds();
    header.setOrigin(hb.left + hb.width / 2.f, hb.top + hb.height / 2.f);
    header.setPosition(640.f, 85.f);
    window.draw(header);

    sf::Text sub("AD 878 • REGNA ANGLO-SAXONUM ET DANELAGH", font, 10);
    sub.setStyle(sf::Text::Italic);
    sub.setFillColor(sf::Color(120, 85, 50));
    sf::FloatRect sb = sub.getLocalBounds();
    sub.setOrigin(sb.left + sb.width / 2.f, sb.top + sb.height / 2.f);
    sub.setPosition(640.f, 104.f);
    window.draw(sub);

    sf::Text seaLatin1("OCEANUS GERMANICUS", font, 11);
    seaLatin1.setFillColor(sf::Color(80, 110, 125));
    seaLatin1.setPosition(620.f, 300.f);
    window.draw(seaLatin1);

    sf::Text seaLatin2("MARE HIBERNICUM", font, 10);
    seaLatin2.setFillColor(sf::Color(80, 110, 125));
    seaLatin2.setPosition(250.f, 380.f);
    window.draw(seaLatin2);

    sf::Text fraText("REGNUM FRANCORUM", font, 11);
    fraText.setFillColor(sf::Color(105, 80, 55));
    fraText.setPosition(430.f, 625.f);
    window.draw(fraText);

    sf::Text scaText("NORDRVEGR", font, 11);
    scaText.setFillColor(sf::Color(95, 80, 60));
    scaText.setPosition(770.f, 105.f);
    window.draw(scaText);

    for (size_t i = 0; i < realSettlements.size(); ++i) {
        const auto& rs = realSettlements[i];
        bool isExplored = (maxExploredX >= rs.borderLeftX && minExploredX <= rs.borderRightX);
        bool isCurrent = (playerX >= rs.borderLeftX && playerX <= rs.borderRightX);

        if (!isExplored) {
            sf::CircleShape fog(15.f, 6);
            fog.setOrigin(15.f, 15.f);
            fog.setPosition(rs.mapCoord);
            fog.setFillColor(sf::Color(160, 140, 115, 240));
            window.draw(fog);
            continue;
        }

        if (i + 1 < realSettlements.size()) {
            const auto& nextRs = realSettlements[i + 1];
            bool nextExplored = (maxExploredX >= nextRs.borderLeftX && minExploredX <= nextRs.borderRightX);
            if (nextExplored) {
                sf::Vertex road[] = {
                    sf::Vertex(rs.mapCoord, sf::Color(145, 100, 50, 140)),
                    sf::Vertex(nextRs.mapCoord, sf::Color(145, 100, 50, 140))
                };
                window.draw(road, 2, sf::Lines);
            }
        }

        float r = isCurrent ? 6.f : 3.5f;
        sf::CircleShape pin(r);
        pin.setOrigin(r, r);
        pin.setPosition(rs.mapCoord);

        if (isCurrent) {
            float pulse = 1.0f + 0.35f * std::sin(pulseTime * 5.0f);
            sf::CircleShape halo(r * 2.4f * pulse);
            halo.setOrigin(halo.getRadius(), halo.getRadius());
            halo.setPosition(rs.mapCoord);
            halo.setFillColor(sf::Color(210, 150, 40, 75));
            window.draw(halo);

            pin.setFillColor(sf::Color(245, 195, 50));
            pin.setOutlineColor(sf::Color(35, 20, 10));
            pin.setOutlineThickness(1.5f);
        } else if (rs.isAllied) {
            pin.setFillColor(sf::Color(75, 140, 70));
            pin.setOutlineColor(sf::Color(25, 45, 20));
            pin.setOutlineThickness(1.f);
        } else {
            pin.setFillColor(sf::Color(185, 75, 50));
            pin.setOutlineColor(sf::Color(45, 20, 15));
            pin.setOutlineThickness(1.f);
        }
        window.draw(pin);

        sf::Text nameLbl(rs.historicalName, font, isCurrent ? 11 : 9);
        nameLbl.setStyle(isCurrent ? sf::Text::Bold : sf::Text::Regular);
        nameLbl.setFillColor(isCurrent ? sf::Color(40, 25, 10) : sf::Color(70, 50, 35));
        nameLbl.setPosition(rs.mapCoord.x + 8.f, rs.mapCoord.y - 7.f);
        window.draw(nameLbl);
    }

    sf::Text legend("Green: Allied Realm   |   Rust: Danelaw / Marches   |   Gold: Current Presence   |   [TAB / ESC] Close", font, 10);
    legend.setFillColor(sf::Color(95, 70, 45));
    sf::FloatRect legB = legend.getLocalBounds();
    legend.setOrigin(legB.left + legB.width / 2.f, legB.top + legB.height / 2.f);
    legend.setPosition(640.f, 634.f);
    window.draw(legend);
}

const RealSettlement* SettlementSystem::getActiveSettlement() const {
    if (activeSettlementIdx >= 0 && activeSettlementIdx < static_cast<int>(realSettlements.size())) {
        return &realSettlements[activeSettlementIdx];
    }
    return nullptr;
}

void SettlementSystem::syncWithWorld(sim::SimulationRegistry& registry) {
    syncDynamicVillages(registry);
}

void SettlementSystem::drawWorldMap(sf::RenderWindow& window, const sf::View& letterboxView, float playerX) {
    sim::SimulationRegistry dummy;
    drawWorldMap(window, letterboxView, playerX, dummy);
}

const RealSettlement* SettlementSystem::getSettlementAt(float x) const {
    for (const auto& s : realSettlements) {
        if (x >= s.borderLeftX && x <= s.borderRightX) {
            return &s;
        }
    }
    return nullptr;
}

const RealSettlement* SettlementSystem::getSettlementByVillageId(sim::VillageID id) const {
    for (const auto& s : realSettlements) {
        if (s.villageId == id) {
            return &s;
        }
    }
    return nullptr;
}

bool SettlementSystem::canFreelyPass(float x) const {
    const RealSettlement* s = getSettlementAt(x);
    return s ? s->isAllied : false;
}