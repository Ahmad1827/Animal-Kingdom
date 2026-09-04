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

    miniFrameOuter.setSize(sf::Vector2f(280.f, 205.f));
    miniFrameOuter.setPosition(980.f, 16.f);
    miniFrameOuter.setFillColor(sf::Color(220, 204, 172));
    miniFrameOuter.setOutlineColor(sf::Color(44, 28, 16));
    miniFrameOuter.setOutlineThickness(2.5f);

    miniFrameInner.setSize(sf::Vector2f(272.f, 197.f));
    miniFrameInner.setPosition(984.f, 20.f);
    miniFrameInner.setFillColor(sf::Color::Transparent);
    miniFrameInner.setOutlineColor(sf::Color(135, 95, 45));
    miniFrameInner.setOutlineThickness(1.f);

    miniSea.setSize(sf::Vector2f(266.f, 150.f));
    miniSea.setPosition(987.f, 42.f);
    miniSea.setFillColor(sf::Color(186, 204, 208, 190));

    sf::Vector2f miniCenter(1120.f, 117.f);
    sf::Vector2f mapCenter(378.f, 342.f);
    float miniScale = 0.23f;

    miniBritain.setPointCount(britPts.size());
    for (size_t i = 0; i < britPts.size(); ++i) {
        sf::Vector2f pt = miniCenter + (britPts[i] - mapCenter) * miniScale;
        miniBritain.setPoint(i, pt);
    }
    miniBritain.setFillColor(sf::Color(215, 198, 160));
    miniBritain.setOutlineColor(sf::Color(65, 45, 25));
    miniBritain.setOutlineThickness(1.f);

    miniIreland.setPointCount(irePts.size());
    for (size_t i = 0; i < irePts.size(); ++i) {
        sf::Vector2f pt = miniCenter + (irePts[i] - mapCenter) * miniScale;
        miniIreland.setPoint(i, pt);
    }
    miniIreland.setFillColor(sf::Color(205, 188, 150));
    miniIreland.setOutlineColor(sf::Color(75, 55, 35));
    miniIreland.setOutlineThickness(1.f);
}

void SettlementSystem::syncDynamicVillages(sim::SimulationRegistry& registry) {
    realSettlements.clear();

    auto allVillages = registry.getAllVillages();
    if (allVillages.empty()) return;

    std::vector<sim::VillageData*> sorted;
    for (auto& pair : allVillages) {
        sorted.push_back(&pair.second);
    }
    std::sort(sorted.begin(), sorted.end(), [](const sim::VillageData* a, const sim::VillageData* b) {
        return a->centerX < b->centerX;
    });

    sim::ApeData* controlled = registry.getApe(registry.getControlledApe());
    sim::VillageID playerVid = (controlled && controlled->villageId != 0) ? controlled->villageId : sorted.front()->id;

    size_t playerIdx = 0;
    for (size_t i = 0; i < sorted.size(); ++i) {
        if (sorted[i]->id == playerVid) {
            playerIdx = i;
            break;
        }
    }

    struct RegionGeo {
        std::string hist;
        std::string modern;
        std::string kingdom;
        sf::Vector2f mapPos;
    };

    std::vector<RegionGeo> eastGeoRoute = {
        {"Readingas", "Reading", "Kingdom of Wessex", {475.f, 485.f}},
        {"Hamwic", "Southampton", "Kingdom of Wessex", {445.f, 525.f}},
        {"Lundenburh", "London", "March of Wessex & Mercia", {520.f, 485.f}},
        {"Theodford", "Thetford", "Kingdom of East Anglia", {565.f, 440.f}},
        {"Tamworthig", "Tamworth", "Kingdom of Mercia", {460.f, 420.f}},
        {"Legaceaster", "Chester", "March of Wales", {390.f, 385.f}},
        {"Lindcylene", "Lincoln", "Five Boroughs of Danelaw", {515.f, 390.f}},
        {"Jorvik", "York", "Kingdom of Jorvik", {490.f, 340.f}},
        {"Dunholm", "Durham", "Kingdom of Northumbria", {470.f, 285.f}},
        {"Bebbanburg", "Bamburgh", "Kingdom of Bernicia", {475.f, 235.f}},
        {"Dun Eideann", "Edinburgh", "Kingdom of Alba", {440.f, 195.f}},
        {"Sgain", "Scone", "Kingdom of Alba", {430.f, 165.f}}
    };

    westCoastX = sorted.front()->borderMinX;

    size_t eastCounter = 0;
    for (size_t i = 0; i < sorted.size(); ++i) {
        sim::VillageData* v = sorted[i];

        RealSettlement rs;
        rs.villageId = v->id;
        rs.kingdomId = v->kingdomId;
        rs.centerX = v->centerX;
        rs.borderLeftX = v->borderMinX;
        rs.borderRightX = v->borderMaxX;

        if (i == playerIdx) {
            rs.historicalName = "Wintanceaster";
            rs.modernName = "Winchester";
            rs.kingdomName = "Kingdom of Wessex";
            rs.mapCoord = sf::Vector2f(430.f, 510.f);
            rs.isAllied = true;
        } else if (i < playerIdx) {
            rs.historicalName = "Kernow (Tintagel)";
            rs.modernName = "Cornwall";
            rs.kingdomName = "Kingdom of Cornwallum";
            rs.mapCoord = sf::Vector2f(300.f, 560.f);
            rs.isAllied = true;
        } else {
            RegionGeo geo = eastGeoRoute[eastCounter % eastGeoRoute.size()];
            eastCounter++;
            rs.historicalName = geo.hist;
            rs.modernName = geo.modern;

            if (v->kingdomId != 0) {
                sim::KingdomData* kd = registry.getKingdom(v->kingdomId);
                rs.kingdomName = kd ? ("Kingdom of " + kd->name) : geo.kingdom;
            } else {
                rs.kingdomName = geo.kingdom;
            }
            rs.mapCoord = geo.mapPos;

            bool allied = false;
            if (rs.kingdomName.find("Wessex") != std::string::npos || rs.kingdomName.find("Cornwall") != std::string::npos) {
                allied = true;
            } else if (controlled) {
                if (v->id == controlled->villageId) {
                    allied = true;
                } else if (controlled->currentKingdom != 0 && v->kingdomId == controlled->currentKingdom) {
                    allied = true;
                } else if (v->personalOpinions.count(controlled->id) && v->personalOpinions[controlled->id] >= 30) {
                    allied = true;
                }
            }
            rs.isAllied = allied;
        }

        realSettlements.push_back(rs);
    }

    std::sort(realSettlements.begin(), realSettlements.end(), [](const RealSettlement& a, const RealSettlement& b) {
        return a.centerX < b.centerX;
    });

    isInitialized = true;
}

bool SettlementSystem::canFreelyPass(float x) const {
    const RealSettlement* s = getSettlementAt(x);
    if (s) {
        if (s->isAllied || s->kingdomName.find("Wessex") != std::string::npos || s->kingdomName.find("Cornwall") != std::string::npos) {
            return true;
        }
    }
    if (!realSettlements.empty() && x <= realSettlements[0].centerX) return true;
    return false;
}

void SettlementSystem::syncWithWorld(sim::SimulationRegistry& registry) {
    syncDynamicVillages(registry);
}

void SettlementSystem::update(float dt, float playerX, sim::SimulationRegistry& registry) {
    if (!isInitialized || realSettlements.size() != registry.getAllVillages().size()) {
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
        if (currentIdx != -1) {
            bannerOldName = realSettlements[currentIdx].historicalName;
            bannerModernName = realSettlements[currentIdx].modernName;
            bannerKingdom = realSettlements[currentIdx].kingdomName;
            bannerAllied = realSettlements[currentIdx].isAllied;
            isExiting = false;
            bannerTimer = 0.f;
            showBanner = true;
        } else if (activeSettlementIdx != -1 && activeSettlementIdx < static_cast<int>(realSettlements.size())) {
            bannerOldName = realSettlements[activeSettlementIdx].historicalName;
            bannerModernName = realSettlements[activeSettlementIdx].modernName;
            bannerKingdom = realSettlements[activeSettlementIdx].kingdomName;
            bannerAllied = realSettlements[activeSettlementIdx].isAllied;
            isExiting = true;
            bannerTimer = 0.f;
            showBanner = true;
        }
        activeSettlementIdx = currentIdx;
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

void SettlementSystem::drawCoast(sf::RenderTarget& rt, const sf::FloatRect& viewBounds, float groundY, float timeOfDay, const sf::Texture* skyTex, const sf::View* cameraView) {
    float cliffX = westCoastX;
    if (viewBounds.left > cliffX + 350.f) return;

    float seaLevelY = groundY + 12.f;
    float seaBottomY = viewBounds.top + viewBounds.height + 400.f;
    float farLeftX = viewBounds.left - 400.f;

    if (skyTex && cameraView) {
        sf::Vector2i sCliff = rt.mapCoordsToPixel(sf::Vector2f(cliffX, 0.f), *cameraView);
        float screenCliffX = static_cast<float>(sCliff.x);
        sf::Vector2i sSea = rt.mapCoordsToPixel(sf::Vector2f(0.f, seaLevelY), *cameraView);
        float screenSeaY = static_cast<float>(sSea.y);

        if (screenCliffX > 0.f) {
            rt.setView(rt.getDefaultView());
            int skyW = static_cast<int>(std::clamp(screenCliffX, 0.f, 1280.f));
            int skyH = static_cast<int>(std::clamp(screenSeaY, 0.f, 720.f));

            if (skyW > 0 && skyH > 0) {
                sf::Sprite cleanSky(*skyTex);
                cleanSky.setTextureRect(sf::IntRect(0, 0, skyW, skyH));
                cleanSky.setPosition(0.f, 0.f);
                rt.draw(cleanSky);
            }
            rt.setView(*cameraView);
        }
    }

    sf::Color waterTop;
    sf::Color waterBottom;
    float t24 = timeOfDay * 24.0f;
    if (t24 >= 6.f && t24 < 18.f) {
        waterTop = sf::Color(32, 68, 92, 255);
        waterBottom = sf::Color(10, 22, 34, 255);
    } else if ((t24 >= 4.5f && t24 < 6.f) || (t24 >= 18.f && t24 < 20.f)) {
        waterTop = sf::Color(48, 38, 58, 255);
        waterBottom = sf::Color(14, 12, 24, 255);
    } else {
        waterTop = sf::Color(14, 22, 32, 255);
        waterBottom = sf::Color(6, 10, 18, 255);
    }

    sf::VertexArray oceanBody(sf::Quads, 4);
    oceanBody[0] = sf::Vertex(sf::Vector2f(farLeftX, seaLevelY), waterTop);
    oceanBody[1] = sf::Vertex(sf::Vector2f(cliffX, seaLevelY), waterTop);
    oceanBody[2] = sf::Vertex(sf::Vector2f(cliffX, seaBottomY), waterBottom);
    oceanBody[3] = sf::Vertex(sf::Vector2f(farLeftX, seaBottomY), waterBottom);
    rt.draw(oceanBody);

    auto drawSeaStack = [&](float sx, float sy, float sw, float sh) {
        sf::ConvexShape stack(5);
        stack.setPoint(0, sf::Vector2f(sx, sy));
        stack.setPoint(1, sf::Vector2f(sx + sw * 0.45f, sy - sh));
        stack.setPoint(2, sf::Vector2f(sx + sw * 0.7f, sy - sh * 0.85f));
        stack.setPoint(3, sf::Vector2f(sx + sw, sy));
        stack.setPoint(4, sf::Vector2f(sx + sw * 0.5f, sy + 15.f));
        stack.setFillColor(sf::Color(35, 42, 48));
        stack.setOutlineColor(sf::Color(20, 24, 28));
        stack.setOutlineThickness(1.f);
        rt.draw(stack);

        sf::Vertex foam[] = {
            sf::Vertex(sf::Vector2f(sx - 10.f, sy + 2.f), sf::Color(180, 220, 235, 180)),
            sf::Vertex(sf::Vector2f(sx + sw + 10.f, sy + 2.f), sf::Color(180, 220, 235, 180))
        };
        rt.draw(foam, 2, sf::Lines);
    };

    drawSeaStack(cliffX - 580.f, seaLevelY + 8.f, 95.f, 65.f);
    drawSeaStack(cliffX - 1100.f, seaLevelY + 5.f, 130.f, 90.f);
    drawSeaStack(cliffX - 1650.f, seaLevelY + 2.f, 180.f, 110.f);

    for (int w = 0; w < 7; ++w) {
        float wy = seaLevelY + 4.f + w * 28.f;
        sf::VertexArray waveStrip(sf::TriangleStrip);
        for (float wx = farLeftX; wx <= cliffX; wx += 24.f) {
            float undulation = std::sin(pulseTime * (1.8f + w * 0.35f) + wx * 0.012f + w * 1.6f) * (3.f + w * 1.6f);
            sf::Uint8 fAlpha = static_cast<sf::Uint8>(std::clamp(210 - w * 15, 30, 240));
            sf::Uint8 wAlpha = static_cast<sf::Uint8>(std::clamp(170 - w * 10, 20, 220));

            waveStrip.append(sf::Vertex(sf::Vector2f(wx, wy + undulation), sf::Color(215, 240, 252, fAlpha)));
            waveStrip.append(sf::Vertex(sf::Vector2f(wx, wy + undulation + 6.f + w * 1.2f), sf::Color(22, 54, 78, wAlpha)));
        }
        rt.draw(waveStrip);
    }

    sf::ConvexShape cliffFace(6);
    cliffFace.setPoint(0, sf::Vector2f(cliffX, groundY));
    cliffFace.setPoint(1, sf::Vector2f(cliffX - 30.f, groundY + 60.f));
    cliffFace.setPoint(2, sf::Vector2f(cliffX - 15.f, groundY + 160.f));
    cliffFace.setPoint(3, sf::Vector2f(cliffX - 45.f, seaBottomY));
    cliffFace.setPoint(4, sf::Vector2f(cliffX, seaBottomY));
    cliffFace.setPoint(5, sf::Vector2f(cliffX, groundY));
    cliffFace.setFillColor(sf::Color(38, 34, 32));
    cliffFace.setOutlineColor(sf::Color(20, 18, 16));
    cliffFace.setOutlineThickness(1.5f);
    rt.draw(cliffFace);

    sf::RectangleShape mossFringe(sf::Vector2f(28.f, 6.f));
    mossFringe.setPosition(cliffX - 6.f, groundY);
    mossFringe.setFillColor(sf::Color(82, 105, 48));
    rt.draw(mossFringe);

    float surfPulse = 1.0f + 0.35f * std::sin(pulseTime * 4.2f);
    for (int b = 0; b < 3; ++b) {
        float by = seaLevelY + 15.f + b * 45.f;
        sf::CircleShape breaker(16.f * surfPulse, 8);
        breaker.setOrigin(16.f * surfPulse, 16.f * surfPulse);
        breaker.setPosition(cliffX - 15.f - b * 8.f, by);
        breaker.setFillColor(sf::Color(230, 248, 255, 175));
        rt.draw(breaker);
    }

    sf::RectangleShape monolith(sf::Vector2f(16.f, 54.f));
    monolith.setOrigin(8.f, 54.f);
    monolith.setPosition(cliffX + 45.f, groundY);
    monolith.setFillColor(sf::Color(58, 54, 50));
    monolith.setOutlineColor(sf::Color(24, 22, 20));
    monolith.setOutlineThickness(1.5f);
    rt.draw(monolith);

    if (fontLoaded) {
        sf::Text markerLbl("OCEANUS ATLANTICUS", font, 11);
        markerLbl.setStyle(sf::Text::Bold);
        markerLbl.setFillColor(sf::Color(240, 215, 140));
        markerLbl.setOutlineColor(sf::Color(0, 0, 0, 220));
        markerLbl.setOutlineThickness(1.5f);
        sf::FloatRect mb = markerLbl.getLocalBounds();
        markerLbl.setOrigin(mb.left + mb.width / 2.f, mb.top + mb.height);
        markerLbl.setPosition(cliffX + 45.f, groundY - 62.f);
        rt.draw(markerLbl);

        sf::Text landsEnd("BELERION - LAND'S END", font, 9);
        landsEnd.setFillColor(sf::Color(190, 180, 160));
        landsEnd.setOutlineColor(sf::Color(0, 0, 0, 220));
        landsEnd.setOutlineThickness(1.2f);
        sf::FloatRect lb = landsEnd.getLocalBounds();
        landsEnd.setOrigin(lb.left + lb.width / 2.f, lb.top + lb.height);
        landsEnd.setPosition(cliffX + 45.f, groundY - 78.f);
        rt.draw(landsEnd);
    }

    auto drawGull = [&](float gx, float gy) {
        sf::Vertex wings[] = {
            sf::Vertex(sf::Vector2f(gx - 8.f, gy + 3.f), sf::Color(235, 240, 245, 200)),
            sf::Vertex(sf::Vector2f(gx, gy), sf::Color(235, 240, 245, 220)),
            sf::Vertex(sf::Vector2f(gx, gy), sf::Color(235, 240, 245, 220)),
            sf::Vertex(sf::Vector2f(gx + 8.f, gy + 3.f), sf::Color(235, 240, 245, 200))
        };
        rt.draw(wings, 4, sf::Lines);
    };

    float gullT = std::fmod(pulseTime * 28.f, 900.f);
    drawGull(cliffX - 220.f - gullT, seaLevelY - 140.f + std::sin(pulseTime * 2.f) * 6.f);
    drawGull(cliffX - 360.f - gullT, seaLevelY - 160.f + std::cos(pulseTime * 2.5f) * 5.f);
    drawGull(cliffX - 160.f - gullT, seaLevelY - 180.f + std::sin(pulseTime * 1.8f) * 4.f);
}

sf::Vector2f SettlementSystem::getPlayerMapCoord(float playerX) const {
    if (realSettlements.empty()) return sf::Vector2f(430.f, 510.f);
    if (playerX <= realSettlements.front().centerX) return realSettlements.front().mapCoord;
    if (playerX >= realSettlements.back().centerX) return realSettlements.back().mapCoord;

    for (size_t i = 0; i + 1 < realSettlements.size(); ++i) {
        if (playerX >= realSettlements[i].centerX && playerX <= realSettlements[i + 1].centerX) {
            float dist = realSettlements[i + 1].centerX - realSettlements[i].centerX;
            float t = (dist > 0.001f) ? (playerX - realSettlements[i].centerX) / dist : 0.f;
            return sf::Vector2f(
                realSettlements[i].mapCoord.x + t * (realSettlements[i + 1].mapCoord.x - realSettlements[i].mapCoord.x),
                realSettlements[i].mapCoord.y + t * (realSettlements[i + 1].mapCoord.y - realSettlements[i].mapCoord.y)
            );
        }
    }
    return realSettlements.front().mapCoord;
}

void SettlementSystem::drawMinimap(sf::RenderWindow& window, const sf::View& letterboxView, float playerX, const sim::SimulationRegistry& registry) {
    (void)registry;
    if (!fontLoaded) return;

    window.setView(letterboxView);

    window.draw(miniFrameOuter);
    window.draw(miniFrameInner);
    window.draw(miniSea);
    window.draw(miniIreland);
    window.draw(miniBritain);

    sf::Text miniTitle("MAP OF REALMS", font, 11);
    miniTitle.setStyle(sf::Text::Bold);
    miniTitle.setFillColor(sf::Color(65, 42, 20));
    miniTitle.setPosition(996.f, 24.f);
    window.draw(miniTitle);

    sf::Text tabPrompt("[TAB] Enlarge", font, 9);
    tabPrompt.setFillColor(sf::Color(115, 85, 50));
    tabPrompt.setPosition(1175.f, 26.f);
    window.draw(tabPrompt);

    sf::Vector2f miniCenter(1120.f, 117.f);
    sf::Vector2f mapCenter(378.f, 342.f);
    float miniScale = 0.23f;

    for (size_t i = 0; i < realSettlements.size(); ++i) {
        const auto& rs = realSettlements[i];
        bool isExplored = (maxExploredX >= rs.borderLeftX && minExploredX <= rs.borderRightX);
        if (!isExplored) continue;

        sf::Vector2f mDot = miniCenter + (rs.mapCoord - mapCenter) * miniScale;
        sf::CircleShape dot(rs.isAllied ? 2.5f : 2.0f);
        dot.setOrigin(dot.getRadius(), dot.getRadius());
        dot.setPosition(mDot);
        dot.setFillColor(rs.isAllied ? sf::Color(60, 140, 60) : sf::Color(180, 75, 40));
        window.draw(dot);
    }

    sf::Vector2f pMap = getPlayerMapCoord(playerX);
    sf::Vector2f pMini = miniCenter + (pMap - mapCenter) * miniScale;

    float pulse = 1.0f + 0.35f * std::sin(pulseTime * 6.f);
    sf::CircleShape aura(4.5f * pulse);
    aura.setOrigin(aura.getRadius(), aura.getRadius());
    aura.setPosition(pMini);
    aura.setFillColor(sf::Color(230, 160, 30, 90));
    window.draw(aura);

    sf::CircleShape pin(3.f);
    pin.setOrigin(3.f, 3.f);
    pin.setPosition(pMini);
    pin.setFillColor(sf::Color(255, 220, 50));
    pin.setOutlineColor(sf::Color(30, 15, 5));
    pin.setOutlineThickness(1.2f);
    window.draw(pin);

    std::string locName = "Wilderness";
    const RealSettlement* curr = getActiveSettlement();
    if (curr) locName = curr->historicalName;

    sf::RectangleShape bar(sf::Vector2f(266.f, 18.f));
    bar.setPosition(987.f, 196.f);
    bar.setFillColor(sf::Color(44, 30, 18, 240));
    window.draw(bar);

    sf::Text locText("Pos: " + locName, font, 9);
    locText.setFillColor(sf::Color(245, 225, 160));
    locText.setPosition(993.f, 198.f);
    window.draw(locText);
}

void SettlementSystem::drawWorldMap(sf::RenderWindow& window, const sf::View& letterboxView, float playerX) {
    sim::SimulationRegistry dummy;
    drawWorldMap(window, letterboxView, playerX, dummy);
}

void SettlementSystem::drawWorldMap(sf::RenderWindow& window, const sf::View& letterboxView, float playerX, const sim::SimulationRegistry& registry) {
    (void)registry;
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

        float r = rs.isAllied ? 4.5f : 3.5f;
        sf::CircleShape pin(r);
        pin.setOrigin(r, r);
        pin.setPosition(rs.mapCoord);
        pin.setFillColor(rs.isAllied ? sf::Color(75, 140, 70) : sf::Color(185, 75, 50));
        pin.setOutlineColor(rs.isAllied ? sf::Color(25, 45, 20) : sf::Color(45, 20, 15));
        pin.setOutlineThickness(1.f);
        window.draw(pin);

        sf::Text nameLbl(rs.historicalName, font, 9);
        nameLbl.setStyle(sf::Text::Regular);
        nameLbl.setFillColor(sf::Color(70, 50, 35));
        nameLbl.setPosition(rs.mapCoord.x + 8.f, rs.mapCoord.y - 7.f);
        window.draw(nameLbl);
    }

    sf::Vector2f playerCoord = getPlayerMapCoord(playerX);

    float pulse = 1.0f + 0.4f * std::sin(pulseTime * 5.0f);
    sf::CircleShape halo(8.f * pulse);
    halo.setOrigin(halo.getRadius(), halo.getRadius());
    halo.setPosition(playerCoord);
    halo.setFillColor(sf::Color(210, 150, 40, 80));
    window.draw(halo);

    sf::CircleShape playerPin(5.5f);
    playerPin.setOrigin(5.5f, 5.5f);
    playerPin.setPosition(playerCoord);
    playerPin.setFillColor(sf::Color(255, 220, 50));
    playerPin.setOutlineColor(sf::Color(40, 20, 5));
    playerPin.setOutlineThickness(2.f);
    window.draw(playerPin);

    sf::RectangleShape badge(sf::Vector2f(44.f, 16.f));
    badge.setOrigin(22.f, 22.f);
    badge.setPosition(playerCoord);
    badge.setFillColor(sf::Color(35, 22, 12, 235));
    badge.setOutlineColor(sf::Color(215, 175, 70));
    badge.setOutlineThickness(1.f);
    window.draw(badge);

    sf::Text youText("YOU", font, 9);
    youText.setStyle(sf::Text::Bold);
    youText.setFillColor(sf::Color(255, 230, 130));
    sf::FloatRect yb = youText.getLocalBounds();
    youText.setOrigin(yb.left + yb.width / 2.f, yb.top + yb.height / 2.f);
    youText.setPosition(playerCoord.x, playerCoord.y - 14.f);
    window.draw(youText);

    sf::Text legend("Green: Allied Wessex & Cornwallum   |   Rust: Danelaw & Norse   |   Gold: You   |   [TAB / ESC] Close", font, 10);
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
