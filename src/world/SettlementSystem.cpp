#include "world/SettlementSystem.h"
#include <cmath>
#include <algorithm>

SettlementSystem::SettlementSystem() {
    fontLoaded = font.loadFromFile("assets/fonts/Cinzel-Bold.ttf") ||
                 font.loadFromFile("assets/fonts/Cinzel-Regular.ttf") ||
                 font.loadFromFile("font.ttf") ||
                 font.loadFromFile("assets/fonts/font.ttf");

    initMapGraphics();
}

void SettlementSystem::initMapGraphics() {
    mapOverlay.setSize(sf::Vector2f(1280.f, 720.f));
    mapOverlay.setFillColor(sf::Color(8, 7, 6, 210));

    parchmentBg.setSize(sf::Vector2f(980.f, 630.f));
    parchmentBg.setOrigin(490.f, 315.f);
    parchmentBg.setPosition(640.f, 360.f);
    parchmentBg.setFillColor(sf::Color(24, 19, 14));

    parchmentBorderOuter.setSize(sf::Vector2f(970.f, 620.f));
    parchmentBorderOuter.setOrigin(485.f, 310.f);
    parchmentBorderOuter.setPosition(640.f, 360.f);
    parchmentBorderOuter.setFillColor(sf::Color::Transparent);
    parchmentBorderOuter.setOutlineColor(sf::Color(160, 120, 55));
    parchmentBorderOuter.setOutlineThickness(2.f);

    parchmentBorderInner.setSize(sf::Vector2f(954.f, 604.f));
    parchmentBorderInner.setOrigin(477.f, 302.f);
    parchmentBorderInner.setPosition(640.f, 360.f);
    parchmentBorderInner.setFillColor(sf::Color::Transparent);
    parchmentBorderInner.setOutlineColor(sf::Color(70, 52, 28));
    parchmentBorderInner.setOutlineThickness(1.f);

    compassRays.clear();
    sf::Color rayClr(75, 58, 35, 65);
    sf::Vector2f roseCenter(760.f, 380.f);
    for (int i = 0; i < 16; ++i) {
        float angle = i * (3.14159265f / 8.0f);
        sf::Vector2f dir(std::cos(angle), std::sin(angle));
        compassRays.push_back(sf::Vertex(roseCenter, rayClr));
        compassRays.push_back(sf::Vertex(roseCenter + dir * 340.f, sf::Color(75, 58, 35, 0)));
    }

    coastOutline.clear();
    sf::Color ink(130, 100, 55);
    std::vector<sf::Vector2f> britPts = {
        {260.f, 575.f}, {295.f, 540.f}, {340.f, 525.f}, {410.f, 520.f},
        {480.f, 510.f}, {550.f, 495.f}, {585.f, 455.f}, {565.f, 420.f},
        {535.f, 380.f}, {510.f, 335.f}, {485.f, 255.f}, {460.f, 195.f},
        {435.f, 120.f}, {420.f, 170.f}, {405.f, 230.f}, {375.f, 310.f},
        {340.f, 370.f}, {325.f, 435.f}, {350.f, 480.f}, {295.f, 530.f},
        {260.f, 575.f}
    };
    for (size_t i = 0; i + 1 < britPts.size(); ++i) {
        coastOutline.push_back(sf::Vertex(britPts[i], ink));
        coastOutline.push_back(sf::Vertex(britPts[i + 1], ink));
    }

    channelOutline.clear();
    std::vector<sf::Vector2f> frankPts = {
        {220.f, 620.f}, {320.f, 615.f}, {440.f, 610.f}, {560.f, 612.f}, {640.f, 625.f}
    };
    for (size_t i = 0; i + 1 < frankPts.size(); ++i) {
        channelOutline.push_back(sf::Vertex(frankPts[i], sf::Color(95, 75, 45)));
        channelOutline.push_back(sf::Vertex(frankPts[i + 1], sf::Color(95, 75, 45)));
    }

    scandiOutline.clear();
    std::vector<sf::Vector2f> scandPts = {
        {680.f, 95.f}, {750.f, 85.f}, {840.f, 100.f}, {870.f, 160.f}, {810.f, 195.f}, {740.f, 150.f}, {680.f, 95.f}
    };
    for (size_t i = 0; i + 1 < scandPts.size(); ++i) {
        scandiOutline.push_back(sf::Vertex(scandPts[i], sf::Color(105, 85, 55)));
        scandiOutline.push_back(sf::Vertex(scandPts[i + 1], sf::Color(105, 85, 55)));
    }
}

void SettlementSystem::syncWithWorld(sim::SimulationRegistry& registry) {
    settlements.clear();

    struct SettlementTemplate {
        std::string hist;
        std::string modern;
        std::string kingdom;
        bool allied;
        sf::Vector2f mapCoord;
    };

    std::vector<SettlementTemplate> templates = {
        {"Cornovii Coast", "Tintagel Reach", "Kingdom of Cornwallum", false, {270.f, 565.f}},
        {"Wintanceaster", "Winchester", "Kingdom of Wessex", true, {425.f, 510.f}},
        {"Hamwic", "Southampton", "Kingdom of Wessex", true, {450.f, 530.f}},
        {"Readingas", "Reading", "Royal Wessex Burh", true, {480.f, 485.f}},
        {"Lundenburh", "London", "March of Wessex & Mercia", true, {525.f, 480.f}},
        {"Theodford", "Thetford", "Kingdom of East Anglia", false, {565.f, 435.f}},
        {"Tamworthig", "Tamworth", "Kingdom of Mercia", true, {460.f, 420.f}},
        {"Gleawceaster", "Gloucester", "Kingdom of Mercia", true, {395.f, 460.f}},
        {"Legaceaster", "Chester", "March of Wales", false, {385.f, 380.f}},
        {"Snotingaham", "Nottingham", "Five Boroughs of Danelaw", false, {485.f, 385.f}},
        {"Jorvik", "York", "Kingdom of Jorvik", false, {490.f, 335.f}},
        {"Dunholm", "Durham", "Kingdom of Northumbria", false, {465.f, 280.f}},
        {"Bebbanburg", "Bamburgh", "Kingdom of Bernicia", false, {470.f, 235.f}},
        {"Dun Eideann", "Edinburgh", "Kingdom of Alba", false, {435.f, 195.f}},
        {"Sgain", "Scone", "Kingdom of Alba", false, {425.f, 165.f}},
        {"Hrossey Sound", "Orkney Coast", "Norse Jarldom", false, {440.f, 105.f}},
        {"Kaupang Fjord", "Skagerrak Reach", "Viking Shore to Scandinavia", false, {750.f, 130.f}}
    };

    auto allVillages = registry.getAllVillages();
    std::vector<sim::VillageData*> sortedVillages;
    for (auto& pair : allVillages) {
        sortedVillages.push_back(&pair.second);
    }
    std::sort(sortedVillages.begin(), sortedVillages.end(), [](const sim::VillageData* a, const sim::VillageData* b) {
        return a->centerX < b->centerX;
    });

    const float villageSpan = 540.f;

    if (!sortedVillages.empty()) {
        float firstCenter = sortedVillages.front()->centerX;
        settlements.push_back({
            0,
            firstCenter - 3600.f,
            firstCenter - 1400.f,
            templates[0].hist,
            templates[0].modern,
            templates[0].kingdom,
            templates[0].allied,
            templates[0].mapCoord
        });

        for (size_t i = 0; i < sortedVillages.size(); ++i) {
            sim::VillageData* v = sortedVillages[i];
            size_t tIdx = (i + 1) % templates.size();
            settlements.push_back({
                v->id,
                v->centerX - villageSpan,
                v->centerX + villageSpan,
                templates[tIdx].hist,
                templates[tIdx].modern,
                templates[tIdx].kingdom,
                templates[tIdx].allied,
                templates[tIdx].mapCoord
            });
        }

        float lastCenter = sortedVillages.back()->centerX;
        float currentX = lastCenter + 2200.f;
        for (size_t i = sortedVillages.size() + 1; i < templates.size(); ++i) {
            settlements.push_back({
                0,
                currentX - villageSpan,
                currentX + villageSpan,
                templates[i].hist,
                templates[i].modern,
                templates[i].kingdom,
                templates[i].allied,
                templates[i].mapCoord
            });
            currentX += 2400.f;
        }
    } else {
        float startX = -1200.f;
        for (size_t i = 0; i < templates.size(); ++i) {
            settlements.push_back({
                0,
                startX - villageSpan,
                startX + villageSpan,
                templates[i].hist,
                templates[i].modern,
                templates[i].kingdom,
                templates[i].allied,
                templates[i].mapCoord
            });
            startX += 2400.f;
        }
    }
}

void SettlementSystem::update(float dt, float playerX) {
    minExploredX = std::min(minExploredX, playerX);
    maxExploredX = std::max(maxExploredX, playerX);

    int activeIdx = -1;
    for (size_t i = 0; i < settlements.size(); ++i) {
        if (playerX >= settlements[i].borderLeftX && playerX <= settlements[i].borderRightX) {
            activeIdx = static_cast<int>(i);
            break;
        }
    }

    if (activeIdx != currentSettlementIdx) {
        previousSettlementIdx = currentSettlementIdx;
        currentSettlementIdx = activeIdx;

        if (currentSettlementIdx != -1) {
            displayOldName = settlements[currentSettlementIdx].historicalName;
            displayModernName = settlements[currentSettlementIdx].modernName;
            displayKingdom = settlements[currentSettlementIdx].kingdom;
            displayAllied = settlements[currentSettlementIdx].isAlliedWithWessex;
            isLeaving = false;
        } else if (previousSettlementIdx != -1) {
            displayOldName = settlements[previousSettlementIdx].historicalName;
            displayModernName = settlements[previousSettlementIdx].modernName;
            displayKingdom = settlements[previousSettlementIdx].kingdom;
            displayAllied = settlements[previousSettlementIdx].isAlliedWithWessex;
            isLeaving = true;
        }

        animTimer = 0.f;
        isAnimating = true;
    }

    if (isAnimating) {
        animTimer += dt;
        if (animTimer >= 5.0f) {
            isAnimating = false;
        }
    }
}

void SettlementSystem::draw(sf::RenderWindow& window, const sf::View& letterboxView) {
    if (!fontLoaded || !isAnimating) return;

    window.setView(letterboxView);

    float alpha = 0.f;
    float morphT = 0.f;

    if (animTimer < 0.5f) {
        alpha = animTimer / 0.5f;
    } else if (animTimer < 1.8f) {
        alpha = 1.0f;
    } else if (animTimer < 3.0f) {
        alpha = 1.0f;
        morphT = (animTimer - 1.8f) / 1.2f;
    } else if (animTimer < 4.0f) {
        alpha = 1.0f;
        morphT = 1.0f;
    } else {
        alpha = 1.0f - ((animTimer - 4.0f) / 1.0f);
        morphT = 1.0f;
    }

    alpha = std::clamp(alpha, 0.0f, 1.0f);
    morphT = std::clamp(morphT, 0.0f, 1.0f);
    sf::Uint8 byteAlpha = static_cast<sf::Uint8>(alpha * 255);

    float centerY = 90.f;
    std::string prefix = isLeaving ? "Departing Territory of " : "Entering Realm of ";
    sf::Color realmColor = displayAllied ? sf::Color(145, 215, 130, byteAlpha) : sf::Color(225, 150, 90, byteAlpha);

    sf::Text kingdomText(prefix + displayKingdom, font, 13);
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
        sf::Text oldText(displayOldName, font, 24);
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
        sf::Text modernText(displayModernName, font, 24);
        modernText.setStyle(sf::Text::Bold);
        modernText.setFillColor(sf::Color(245, 245, 250, modAlpha));
        modernText.setOutlineColor(sf::Color(0, 0, 0, modAlpha));
        modernText.setOutlineThickness(2.0f);
        sf::FloatRect mb = modernText.getLocalBounds();
        modernText.setOrigin(mb.left + mb.width / 2.f, mb.top + mb.height / 2.f);
        modernText.setPosition(640.f, centerY + 8.f);
        window.draw(modernText);
    }

    float lineHalfW = 140.f * alpha;
    sf::Color lineClr = displayAllied ? sf::Color(120, 200, 110, byteAlpha) : sf::Color(210, 140, 80, byteAlpha);
    sf::Color edgeClr = displayAllied ? sf::Color(120, 200, 110, 0) : sf::Color(210, 140, 80, 0);

    sf::Vertex line[] = {
        sf::Vertex(sf::Vector2f(640.f - lineHalfW, centerY + 28.f), edgeClr),
        sf::Vertex(sf::Vector2f(640.f, centerY + 28.f), lineClr),
        sf::Vertex(sf::Vector2f(640.f + lineHalfW, centerY + 28.f), edgeClr)
    };
    window.draw(line, 3, sf::LinesStrip);
}

void SettlementSystem::drawWorldMap(sf::RenderWindow& window, const sf::View& letterboxView, float playerX) {
    if (!fontLoaded) return;

    window.setView(letterboxView);

    window.draw(mapOverlay);
    window.draw(parchmentBg);
    window.draw(parchmentBorderOuter);
    window.draw(parchmentBorderInner);

    if (!compassRays.empty()) {
        window.draw(compassRays.data(), compassRays.size(), sf::Lines);
    }
    if (!coastOutline.empty()) {
        window.draw(coastOutline.data(), coastOutline.size(), sf::Lines);
    }
    if (!channelOutline.empty()) {
        window.draw(channelOutline.data(), channelOutline.size(), sf::Lines);
    }
    if (!scandiOutline.empty()) {
        window.draw(scandiOutline.data(), scandiOutline.size(), sf::Lines);
    }

    sf::Text header("BRITANNIA ET MARE SEPTENTRIONALE", font, 14);
    header.setStyle(sf::Text::Bold);
    header.setFillColor(sf::Color(220, 180, 100));
    header.setPosition(200.f, 75.f);
    window.draw(header);

    sf::Text sub("Anno Domini DCCCLXXVIII  •  Alfred Rex Anglo-Saxonum", font, 10);
    sub.setStyle(sf::Text::Italic);
    sub.setFillColor(sf::Color(150, 125, 95));
    sub.setPosition(200.f, 96.f);
    window.draw(sub);

    sf::Text legend("Green: Allied Realm   |   Rust: Danelaw / March   |   Gold: Current Presence", font, 9);
    legend.setFillColor(sf::Color(135, 115, 90));
    legend.setPosition(200.f, 115.f);
    window.draw(legend);

    sf::Text sea1("OCEANUS GERMANICUS", font, 10);
    sea1.setFillColor(sf::Color(80, 65, 45));
    sea1.setPosition(630.f, 320.f);
    window.draw(sea1);

    sf::Text sea2("MARE HIBERNICUM", font, 10);
    sea2.setFillColor(sf::Color(80, 65, 45));
    sea2.setPosition(240.f, 360.f);
    window.draw(sea2);

    sf::Text frankia("REGNUM FRANCORUM", font, 10);
    frankia.setFillColor(sf::Color(90, 72, 50));
    frankia.setPosition(320.f, 625.f);
    window.draw(frankia);

    sf::Text scandi("NORDRVEGR", font, 10);
    scandi.setFillColor(sf::Color(95, 80, 60));
    scandi.setPosition(720.f, 100.f);
    window.draw(scandi);

    for (size_t i = 0; i < settlements.size(); ++i) {
        const auto& st = settlements[i];
        bool isExplored = (maxExploredX >= st.borderLeftX && minExploredX <= st.borderRightX);
        bool isCurrent = (playerX >= st.borderLeftX && playerX <= st.borderRightX);

        if (!isExplored) {
            sf::CircleShape shroud(14.f, 4);
            shroud.setOrigin(14.f, 14.f);
            shroud.setPosition(st.mapCoord);
            shroud.setFillColor(sf::Color(18, 14, 11, 230));
            window.draw(shroud);
            continue;
        }

        if (i + 1 < settlements.size()) {
            const auto& nextSt = settlements[i + 1];
            bool nextExplored = (maxExploredX >= nextSt.borderLeftX && minExploredX <= nextSt.borderRightX);
            if (nextExplored) {
                sf::Vertex trail[] = {
                    sf::Vertex(st.mapCoord, sf::Color(140, 105, 55, 110)),
                    sf::Vertex(nextSt.mapCoord, sf::Color(140, 105, 55, 110))
                };
                window.draw(trail, 2, sf::Lines);
            }
        }

        sf::CircleShape node(isCurrent ? 6.f : 3.5f);
        node.setOrigin(node.getRadius(), node.getRadius());
        node.setPosition(st.mapCoord);

        if (isCurrent) {
            node.setFillColor(sf::Color(255, 220, 75));
            node.setOutlineColor(sf::Color(255, 255, 255));
            node.setOutlineThickness(2.f);
        } else if (st.isAlliedWithWessex) {
            node.setFillColor(sf::Color(115, 195, 105));
            node.setOutlineColor(sf::Color(15, 25, 15));
            node.setOutlineThickness(1.f);
        } else {
            node.setFillColor(sf::Color(210, 120, 70));
            node.setOutlineColor(sf::Color(25, 15, 10));
            node.setOutlineThickness(1.f);
        }
        window.draw(node);

        sf::Text townLabel(st.historicalName, font, isCurrent ? 11 : 9);
        townLabel.setStyle(isCurrent ? sf::Text::Bold : sf::Text::Regular);
        townLabel.setFillColor(isCurrent ? sf::Color(255, 235, 160) : sf::Color(205, 185, 155));
        townLabel.setOutlineColor(sf::Color::Black);
        townLabel.setOutlineThickness(1.f);
        townLabel.setPosition(st.mapCoord.x + 8.f, st.mapCoord.y - 7.f);
        window.draw(townLabel);
    }
}

bool SettlementSystem::canFreelyPass(float playerX) const {
    for (const auto& s : settlements) {
        if (playerX >= s.borderLeftX && playerX <= s.borderRightX) {
            return s.isAlliedWithWessex;
        }
    }
    return false;
}