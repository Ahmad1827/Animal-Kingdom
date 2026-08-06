#include "interaction/InteractionManager.h"
#include <cmath>
#include <algorithm>
#include <iostream>

InteractionManager::InteractionManager() 
    : currentPromptTarget(nullptr), activeTarget(nullptr), 
      isMenuOpen(false), selectedMenuIndex(0), fontLoaded(false),
      preInteractionCenter(0.f, 0.f), preInteractionZoom(1.f), interactionTransitionTimer(0.f) {
    fontLoaded = menuFont.loadFromFile("font.ttf");
}

void InteractionManager::registerTarget(std::shared_ptr<InteractionTarget> target) {
    targets.push_back(target);
}

void InteractionManager::clearTargets() {
    targets.clear();
    currentPromptTarget = nullptr;
    if (isMenuOpen) {
        isMenuOpen = false;
        activeTarget = nullptr;
    }
}

void InteractionManager::update(float dt, const sf::Vector2f& playerPos, CameraManager& cameraManager) {
    if (isMenuOpen) {
        if (interactionTransitionTimer < transitionDuration) {
            interactionTransitionTimer += dt;
            float t = std::clamp(interactionTransitionTimer / transitionDuration, 0.f, 1.f);
        }
        return;
    }

    std::shared_ptr<InteractionTarget> bestTarget = nullptr;
    float closestDistSq = 120.0f * 120.0f; 

    int highestPriority = -999;

    for (const auto& target : targets) {
        if (!target->canInteract()) continue;

        sf::Vector2f pos = target->getInteractionPosition();
        float distSq = std::pow(pos.x - playerPos.x, 2) + std::pow(pos.y - playerPos.y, 2);

        if (distSq <= closestDistSq) {
            int prio = target->getPriority();
            if (prio > highestPriority || (prio == highestPriority && distSq < closestDistSq)) {
                highestPriority = prio;
                closestDistSq = distSq;
                bestTarget = target;
            }
        }
    }

    currentPromptTarget = bestTarget;
}

void InteractionManager::handleEvent(const sf::Event& event, CameraManager& cameraManager) {
    if (event.type == sf::Event::KeyPressed) {
        if (isMenuOpen) {
            if (event.key.code == sf::Keyboard::Escape || event.key.code == sf::Keyboard::E) {
                isMenuOpen = false;
                activeTarget = nullptr;
                cameraManager.setZoom(preInteractionZoom);
            }
            else if (event.key.code == sf::Keyboard::W || event.key.code == sf::Keyboard::Up) {
                if (!currentMenuEntries.empty()) {
                    selectedMenuIndex = (selectedMenuIndex - 1 + currentMenuEntries.size()) % currentMenuEntries.size();
                }
            }
            else if (event.key.code == sf::Keyboard::S || event.key.code == sf::Keyboard::Down) {
                if (!currentMenuEntries.empty()) {
                    selectedMenuIndex = (selectedMenuIndex + 1) % currentMenuEntries.size();
                }
            }
            else if (event.key.code == sf::Keyboard::Return || event.key.code == sf::Keyboard::Space) {
                if (!currentMenuEntries.empty() && selectedMenuIndex < static_cast<int>(currentMenuEntries.size())) {
                    if (currentMenuEntries[selectedMenuIndex].action) {
                        currentMenuEntries[selectedMenuIndex].action();
                    }
                }
            }
        } else {
            if (event.key.code == sf::Keyboard::E && currentPromptTarget) {
                activeTarget = currentPromptTarget;
                activeTarget->onInteract();
                currentMenuEntries = activeTarget->buildInteractionMenu();
                selectedMenuIndex = 0;
                
                isMenuOpen = true;
                interactionTransitionTimer = 0.f;
                preInteractionZoom = 1.35f; 
                preInteractionCenter = cameraManager.getView().getCenter();

                cameraManager.setZoom(0.85f);
            }
        }
    }
}

void InteractionManager::draw(sf::RenderWindow& window) {
    if (!fontLoaded) return;

    sf::View originalView = window.getView();
    window.setView(window.getDefaultView()); 

    if (!isMenuOpen && currentPromptTarget) {
        sf::Vector2f worldPos = currentPromptTarget->getInteractionPosition();
        sf::Vector2i screenPos = window.mapCoordsToPixel(worldPos, originalView);

        sf::RectangleShape promptBg(sf::Vector2f(110.f, 30.f));
        promptBg.setFillColor(sf::Color(40, 30, 20, 220));
        promptBg.setOutlineColor(sf::Color(180, 140, 90));
        promptBg.setOutlineThickness(2.f);
        promptBg.setOrigin(55.f, 45.f);
        promptBg.setPosition(static_cast<float>(screenPos.x), static_cast<float>(screenPos.y));

        sf::Text promptText("[E] " + currentPromptTarget->getInteractionTitle(), menuFont, 14);
        promptText.setFillColor(sf::Color::White);
        sf::FloatRect textRect = promptText.getLocalBounds();
        promptText.setOrigin(textRect.left + textRect.width / 2.0f, textRect.top + textRect.height / 2.0f);
        promptText.setPosition(promptBg.getPosition());

        window.draw(promptBg);
        window.draw(promptText);
    }

    if (isMenuOpen && activeTarget) {
        sf::Vector2f winSize(static_cast<float>(window.getSize().x), static_cast<float>(window.getSize().y));

        sf::RectangleShape panel(sf::Vector2f(350.f, 400.f));
        panel.setOrigin(175.f, 200.f);
        panel.setPosition(winSize.x / 2.0f, winSize.y / 2.0f);
        panel.setFillColor(sf::Color(65, 45, 25, 245)); 
        panel.setOutlineColor(sf::Color(139, 90, 43));
        panel.setOutlineThickness(4.f);

        window.draw(panel);

        sf::Text titleText(activeTarget->getInteractionTitle(), menuFont, 22);
        titleText.setFillColor(sf::Color(255, 220, 150));
        titleText.setPosition(panel.getPosition().x - 150.f, panel.getPosition().y - 175.f);
        window.draw(titleText);

        sf::RectangleShape divider(sf::Vector2f(310.f, 2.f));
        divider.setFillColor(sf::Color(139, 90, 43));
        divider.setPosition(panel.getPosition().x - 155.f, panel.getPosition().y - 140.f);
        window.draw(divider);

        float startY = panel.getPosition().y - 110.f;
        for (size_t i = 0; i < currentMenuEntries.size(); ++i) {
            sf::Text entryText(currentMenuEntries[i].label, menuFont, 16);
            
            if (static_cast<int>(i) == selectedMenuIndex) {
                entryText.setFillColor(sf::Color::Yellow);
                sf::Text arrow(">", menuFont, 16);
                arrow.setFillColor(sf::Color::Yellow);
                arrow.setPosition(panel.getPosition().x - 150.f, startY + (i * 35.f));
                window.draw(arrow);
            } else {
                entryText.setFillColor(sf::Color(210, 200, 180));
            }

            entryText.setPosition(panel.getPosition().x - 120.f, startY + (i * 35.f));
            window.draw(entryText);
        }

        sf::Text footer("[W/S] Navigate   [Enter] Select   [ESC] Close", menuFont, 11);
        footer.setFillColor(sf::Color(170, 150, 130));
        footer.setPosition(panel.getPosition().x - 150.f, panel.getPosition().y + 165.f);
        window.draw(footer);
    }

    window.setView(originalView);
}