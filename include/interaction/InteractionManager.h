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
    std::vector<InteractionMenuEntry> currentMenuEntries;
    int selectedMenuIndex;

    sf::Font menuFont;
    bool fontLoaded;

    // Camera transition state
    sf::Vector2f preInteractionCenter;
    float preInteractionZoom;
    float interactionTransitionTimer;
    const float transitionDuration = 0.4f;

public:
    InteractionManager();

    void registerTarget(std::shared_ptr<InteractionTarget> target);
    void clearTargets();

    void update(float dt, const sf::Vector2f& playerPos, CameraManager& cameraManager);
    void handleEvent(const sf::Event& event, CameraManager& cameraManager);
    void draw(sf::RenderWindow& window);

    bool isInteracting() const { return isMenuOpen; }
};