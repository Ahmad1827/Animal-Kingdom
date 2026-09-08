#pragma once
#include <SFML/Graphics.hpp>
#include "simulation/ApeData.h"

namespace sim {
    class SimulationManager;
}

class GameHUD {
public:
    GameHUD();

    void init(const sf::Font& font);
    void update(float dt, const sim::ApeData* playerApe);
    bool handleEvent(const sf::Event& event, const sf::RenderWindow& window, const sf::View& letterboxView, sim::SimulationManager* simManager);

    void draw(sf::RenderWindow& window, const sim::ApeData* playerApe, sim::SimulationManager* simManager);

    void setGameSpeed(int speed);
    void toggleGamePause();
    int getGameSpeed() const { return gameSpeed; }
    bool isPaused() const { return isGamePaused; }
    float getSpeedMultiplier() const;

private:
    const sf::Font* font;
    int gameSpeed;
    int previousGameSpeed;
    bool isGamePaused;
    float amberPulseTimer;
    int lastObservedAmber;
    sf::FloatRect timeButtonBounds[6];

    void drawOrnatePanel(sf::RenderWindow& window, float x, float y, float w, float h);
};