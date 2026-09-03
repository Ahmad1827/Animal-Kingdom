#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>

struct Settlement {
    float minX;
    float maxX;
    std::string historicalName;
    std::string modernName;
    std::string kingdom;
    bool isAlliedWithWessex;
    std::string passageStatus;
};

class SettlementSystem {
private:
    std::vector<Settlement> settlements;
    int currentSettlementIndex = -1;
    sf::Font font;
    bool fontLoaded = false;

    float animTimer = 0.f;
    bool isAnimating = false;
    std::string currentOldName;
    std::string currentModernName;
    std::string currentKingdom;
    bool currentIsAllied = true;
    std::string currentStatus;

public:
    SettlementSystem();
    void update(float dt, float playerX);
    void draw(sf::RenderWindow& window, const sf::View& letterboxView);
    bool canFreelyPass(float playerX) const;
    const Settlement* getCurrentSettlement() const;
};