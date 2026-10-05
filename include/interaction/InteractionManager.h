#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <memory>
#include "interaction/InteractionTarget.h"
#include "world/CameraManager.h"

class InteractionManager {
private:
    std::vector<std::shared_ptr<InteractionTarget>> targets;
    std::shared_ptr<InteractionTarget> currentPromptTarget;
    std::shared_ptr<InteractionTarget> activeTarget;

    bool isMenuOpen;
    bool isClosing;
    std::vector<InteractionMenuEntry> currentMenuEntries;
    int selectedMenuIndex;

    sf::Font menuFont;
    bool fontLoaded;

    sf::Vector2f preInteractionCenter;
    float preInteractionZoom;
    float interactionTransitionTimer;
    const float transitionDuration = 0.5f;

    sf::Vector2f lastPlayerPos;

    // Menu rows as laid out by the last draw, and the cursor in UI space, so
    // clicks land on what is actually on screen at any window size.
    std::vector<sf::FloatRect> rowBounds;
    sf::Vector2f mouseUi;

    float getEase() const;
    void executeEntry(int index);

public:
    InteractionManager();

    void registerTarget(std::shared_ptr<InteractionTarget> target);
    void clearTargets();

    void update(float dt, const sf::Vector2f& playerPos, CameraManager& cameraManager);
    void handleEvent(const sf::Event& event, CameraManager& cameraManager);
    void draw(sf::RenderWindow& window, const sf::View& letterboxView, const sf::View& cameraView);
    void draw(sf::RenderTarget& target);

    bool isInteracting() const { return isMenuOpen || isClosing; }
    // Title of whatever E would use right now, or empty.
    std::string getPromptTitle() const { return (currentPromptTarget && !isInteracting()) ? currentPromptTarget->getInteractionTitle() : std::string(); }
};