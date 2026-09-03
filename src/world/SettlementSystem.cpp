#include "world/SettlementSystem.h"
#include <cmath>
#include <algorithm>

SettlementSystem::SettlementSystem() {
    fontLoaded = font.loadFromFile("assets/fonts/Cinzel-Bold.ttf") ||
                 font.loadFromFile("assets/fonts/Cinzel-Regular.ttf") ||
                 font.loadFromFile("font.ttf") ||
                 font.loadFromFile("assets/fonts/font.ttf");

    settlements.push_back({-7000.f, -5000.f, "Rotomagus", "Rouen", "Kingdom of the West Franks", false, "Channel Port - Passage by Ship"});
    settlements.push_back({-5000.f, -3400.f, "Portus Adurni", "Portchester Shore", "Kingdom of Wessex", true, "Allied Royal Haven"});
    settlements.push_back({-3400.f, -1800.f, "Hamwic", "Southampton", "Kingdom of Wessex", true, "Allied Royal Emporium"});
    settlements.push_back({-1800.f, -200.f, "Wintanceaster", "Winchester", "Kingdom of Wessex", true, "Capital of King Alfred - Hearth of Wessex"});
    settlements.push_back({-200.f, 1400.f, "Wiltun", "Wilton", "Kingdom of Wessex", true, "Allied Wessex Heartland"});
    settlements.push_back({1400.f, 3000.f, "Readingas", "Reading", "Royal Wessex Burh", true, "Allied Frontier Garrison"});
    settlements.push_back({3000.f, 4600.f, "Lundenburh", "London", "March of Wessex & Mercia", true, "Allied Fortified Burh"});
    settlements.push_back({4600.f, 6200.f, "Gipeswic", "Ipswich", "Kingdom of East Anglia", false, "Danelaw Sea-Reach"});
    settlements.push_back({6200.f, 7800.f, "Theodford", "Thetford", "Kingdom of East Anglia", false, "Guthrum's Danelaw Seat"});
    settlements.push_back({7800.f, 9400.f, "Northwic", "Norwich", "Kingdom of East Anglia", false, "East Anglian Borough"});
    settlements.push_back({9400.f, 11000.f, "Gleawceaster", "Gloucester", "Kingdom of Mercia", true, "Allied Realm of Aethelred"});
    settlements.push_back({11000.f, 12600.f, "Tamworthig", "Tamworth", "Kingdom of Mercia", true, "Seat of Mercian Power"});
    settlements.push_back({12600.f, 14200.f, "Snotingaham", "Nottingham", "Five Boroughs of the Danelaw", false, "Contested Danish Stronghold"});
    settlements.push_back({14200.f, 15800.f, "Deoraby", "Derby", "Five Boroughs of the Danelaw", false, "Jarl Frontier Fortress"});
    settlements.push_back({15800.f, 17400.f, "Scrobbesbyrig", "Shrewsbury", "March of Wales", false, "Border March - Tribute Land"});
    settlements.push_back({17400.f, 19000.f, "Legaceaster", "Chester", "March of Wales", false, "Roman Walls - Welsh Border"});
    settlements.push_back({19000.f, 20800.f, "Lindcylene", "Lincoln", "Five Boroughs of the Danelaw", false, "Danish Trade Hub"});
    settlements.push_back({20800.f, 22600.f, "Jorvik", "York", "Kingdom of Jorvik", false, "Norse Kingdom of Halfdan"});
    settlements.push_back({22600.f, 24400.f, "Dunholm", "Durham", "Kingdom of Northumbria", false, "Saint Cuthbert's Sanctuary"});
    settlements.push_back({24400.f, 26200.f, "Bebbanburg", "Bamburgh", "Kingdom of Bernicia", false, "High Reeve Stronghold"});
    settlements.push_back({26200.f, 28000.f, "Lindisfarena", "Lindisfarne", "Northumbria Holy Island", false, "Rival Pagan Coast"});
    settlements.push_back({28000.f, 29800.f, "Alt Clut", "Dumbarton", "Kingdom of Strathclyde", false, "Briton Rock Fortress"});
    settlements.push_back({29800.f, 31800.f, "Dun Eideann", "Edinburgh", "Kingdom of Alba", false, "Gaelic Royal Crag"});
    settlements.push_back({31800.f, 34000.f, "Sgain", "Scone", "Royal Realm of Alba", false, "Stone of Destiny"});
    settlements.push_back({34000.f, 37000.f, "Hrossey", "Orkney Sound", "Norse Jarldom", false, "Jarl Sigurd's Haven"});
    settlements.push_back({37000.f, 42000.f, "Kaupang Fjord", "Skagerrak Reach", "Viking Shore to Scandinavia", false, "Longship Voyage North"});
}

void SettlementSystem::update(float dt, float playerX) {
    int activeIdx = -1;
    for (size_t i = 0; i < settlements.size(); ++i) {
        if (playerX >= settlements[i].minX && playerX < settlements[i].maxX) {
            activeIdx = static_cast<int>(i);
            break;
        }
    }

    if (activeIdx != currentSettlementIndex) {
        currentSettlementIndex = activeIdx;
        if (currentSettlementIndex != -1) {
            currentOldName = settlements[currentSettlementIndex].historicalName;
            currentModernName = settlements[currentSettlementIndex].modernName;
            currentKingdom = settlements[currentSettlementIndex].kingdom;
            currentIsAllied = settlements[currentSettlementIndex].isAlliedWithWessex;
            currentStatus = settlements[currentSettlementIndex].passageStatus;
            animTimer = 0.f;
            isAnimating = true;
        }
    }

    if (isAnimating) {
        animTimer += dt;
        if (animTimer >= 5.8f) {
            isAnimating = false;
        }
    }
}

void SettlementSystem::draw(sf::RenderWindow& window, const sf::View& letterboxView) {
    if (!fontLoaded || !isAnimating) return;

    window.setView(letterboxView);

    float alpha = 0.f;
    float morphT = 0.f;

    if (animTimer < 0.7f) {
        alpha = animTimer / 0.7f;
        morphT = 0.f;
    } else if (animTimer < 2.2f) {
        alpha = 1.0f;
        morphT = 0.f;
    } else if (animTimer < 3.4f) {
        alpha = 1.0f;
        morphT = (animTimer - 2.2f) / 1.2f;
    } else if (animTimer < 4.8f) {
        alpha = 1.0f;
        morphT = 1.0f;
    } else {
        alpha = 1.0f - ((animTimer - 4.8f) / 1.0f);
        morphT = 1.0f;
    }

    alpha = std::clamp(alpha, 0.0f, 1.0f);
    morphT = std::clamp(morphT, 0.0f, 1.0f);
    sf::Uint8 byteAlpha = static_cast<sf::Uint8>(alpha * 255);

    float centerY = 95.f;

    sf::Color realmColor = currentIsAllied ? sf::Color(140, 210, 130, byteAlpha) : sf::Color(220, 150, 90, byteAlpha);
    sf::Text kingdomText(currentKingdom + "   -   " + currentStatus, font, 13);
    kingdomText.setStyle(sf::Text::Italic);
    kingdomText.setFillColor(realmColor);
    kingdomText.setOutlineColor(sf::Color(0, 0, 0, byteAlpha));
    kingdomText.setOutlineThickness(1.5f);
    sf::FloatRect kb = kingdomText.getLocalBounds();
    kingdomText.setOrigin(kb.left + kb.width / 2.f, kb.top + kb.height / 2.f);
    kingdomText.setPosition(640.f, centerY - 26.f);
    window.draw(kingdomText);

    if (morphT < 1.0f) {
        sf::Uint8 oldAlpha = static_cast<sf::Uint8>((1.0f - morphT) * 255 * alpha);
        sf::Text oldText(currentOldName, font, 26);
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
        sf::Text modernText(currentModernName, font, 26);
        modernText.setStyle(sf::Text::Bold);
        modernText.setFillColor(sf::Color(245, 245, 250, modAlpha));
        modernText.setOutlineColor(sf::Color(0, 0, 0, modAlpha));
        modernText.setOutlineThickness(2.0f);
        sf::FloatRect mb = modernText.getLocalBounds();
        modernText.setOrigin(mb.left + mb.width / 2.f, mb.top + mb.height / 2.f);
        modernText.setPosition(640.f, centerY + 8.f);
        window.draw(modernText);
    }

    float lineHalfWidth = 160.f * alpha;
    sf::Color lineClr = currentIsAllied ? sf::Color(120, 200, 110, byteAlpha) : sf::Color(210, 140, 80, byteAlpha);
    sf::Color edgeClr = currentIsAllied ? sf::Color(120, 200, 110, 0) : sf::Color(210, 140, 80, 0);

    sf::Vertex line[] = {
        sf::Vertex(sf::Vector2f(640.f - lineHalfWidth, centerY + 30.f), edgeClr),
        sf::Vertex(sf::Vector2f(640.f, centerY + 30.f), lineClr),
        sf::Vertex(sf::Vector2f(640.f + lineHalfWidth, centerY + 30.f), edgeClr)
    };
    window.draw(line, 3, sf::LinesStrip);
}

bool SettlementSystem::canFreelyPass(float playerX) const {
    for (const auto& s : settlements) {
        if (playerX >= s.minX && playerX < s.maxX) {
            return s.isAlliedWithWessex;
        }
    }
    return false;
}

const Settlement* SettlementSystem::getCurrentSettlement() const {
    if (currentSettlementIndex >= 0 && currentSettlementIndex < static_cast<int>(settlements.size())) {
        return &settlements[currentSettlementIndex];
    }
    return nullptr;
}