#include "interaction/InteractionManager.h"
#include <cmath>
#include <algorithm>

InteractionManager::InteractionManager() 
    : currentPromptTarget(nullptr), activeTarget(nullptr), 
      isMenuOpen(false), isClosing(false), selectedMenuIndex(0), fontLoaded(false),
      preInteractionCenter(0.f, 0.f), preInteractionZoom(1.f), interactionTransitionTimer(0.f) {
    fontLoaded = menuFont.loadFromFile("font.ttf");
}

void InteractionManager::registerTarget(std::shared_ptr<InteractionTarget> target) {
    targets.push_back(target);
}

void InteractionManager::clearTargets() {
    targets.clear();
    currentPromptTarget = nullptr;
    if (isMenuOpen || isClosing) {
        if (activeTarget) activeTarget->onClose();
        isMenuOpen = false;
        isClosing = false;
        activeTarget = nullptr;
    }
}

float InteractionManager::getEase() const {
    float t = std::clamp(interactionTransitionTimer / transitionDuration, 0.f, 1.f);
    return t * t * (3.0f - 2.0f * t);
}

void InteractionManager::update(float dt, const sf::Vector2f& playerPos, CameraManager& cameraManager) {
    if (isMenuOpen || isClosing) {
        if (isClosing) {
            interactionTransitionTimer -= dt;
            if (interactionTransitionTimer <= 0.f) {
                isClosing = false;
                isMenuOpen = false;
                if (activeTarget) activeTarget->onClose();
                activeTarget = nullptr;
                cameraManager.setZoom(preInteractionZoom);
            }
        } else if (interactionTransitionTimer < transitionDuration) {
            interactionTransitionTimer += dt;
        }

        if (isMenuOpen || isClosing) {
            float ease = getEase();
            float targetZoom = 0.75f;
            cameraManager.setZoom(preInteractionZoom + (targetZoom - preInteractionZoom) * ease);
        }
        return;
    }

    std::shared_ptr<InteractionTarget> bestTarget = nullptr;
    float closestDistSq = 140.0f * 140.0f; 

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
        if (isMenuOpen && !isClosing) {
            if (event.key.code == sf::Keyboard::Escape || event.key.code == sf::Keyboard::E) {
                isClosing = true;
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
                        
                        currentMenuEntries = activeTarget->buildInteractionMenu();
                        if (selectedMenuIndex >= static_cast<int>(currentMenuEntries.size())) {
                            selectedMenuIndex = 0;
                        }
                    }
                }
            }
        } else if (!isMenuOpen && !isClosing) {
            if (event.key.code == sf::Keyboard::E && currentPromptTarget) {
                activeTarget = currentPromptTarget;
                activeTarget->onInteract();
                currentMenuEntries = activeTarget->buildInteractionMenu();
                selectedMenuIndex = 0;
                
                isMenuOpen = true;
                isClosing = false;
                interactionTransitionTimer = 0.f;
                preInteractionZoom = 1.35f; 
                preInteractionCenter = cameraManager.getView().getCenter();
            }
        }
    }
}

void InteractionManager::draw(sf::RenderWindow& window) {
    if (!fontLoaded) return;

    sf::View originalView = window.getView();
    window.setView(window.getDefaultView()); 

    if (!isMenuOpen && !isClosing && currentPromptTarget) {
        sf::Vector2f worldPos = currentPromptTarget->getInteractionPosition();
        sf::Vector2i screenPos = window.mapCoordsToPixel(worldPos, originalView);

        sf::RectangleShape promptBg(sf::Vector2f(110.f, 30.f));
        promptBg.setFillColor(sf::Color(40, 30, 20, 220));
        promptBg.setOutlineColor(sf::Color(180, 140, 90));
        promptBg.setOutlineThickness(2.f);
        promptBg.setOrigin(55.f, 45.f);
        promptBg.setPosition(static_cast<float>(screenPos.x), static_cast<float>(screenPos.y - 40.f));

        sf::Text promptText("[E] " + currentPromptTarget->getInteractionTitle(), menuFont, 14);
        promptText.setFillColor(sf::Color::White);
        sf::FloatRect textRect = promptText.getLocalBounds();
        promptText.setOrigin(textRect.left + textRect.width / 2.0f, textRect.top + textRect.height / 2.0f);
        promptText.setPosition(promptBg.getPosition());

        window.draw(promptBg);
        window.draw(promptText);
    }

    if ((isMenuOpen || isClosing) && activeTarget) {
        float ease = getEase();
        sf::Uint8 alpha = static_cast<sf::Uint8>(255 * ease);
        float yOffset = 20.f * (1.f - ease);

        sf::Vector2f winSize(static_cast<float>(window.getSize().x), static_cast<float>(window.getSize().y));

        sf::RectangleShape shadow(sf::Vector2f(420.f, 480.f));
        shadow.setOrigin(210.f, 240.f);
        shadow.setPosition(winSize.x / 2.0f + 5.f, winSize.y / 2.0f + 5.f + yOffset);
        shadow.setFillColor(sf::Color(0, 0, 0, static_cast<sf::Uint8>(100 * ease)));
        window.draw(shadow);

        sf::RectangleShape panel(sf::Vector2f(420.f, 480.f));
        panel.setOrigin(210.f, 240.f);
        panel.setPosition(winSize.x / 2.0f, winSize.y / 2.0f + yOffset);
        panel.setFillColor(sf::Color(50, 35, 20, static_cast<sf::Uint8>(250 * ease))); 
        panel.setOutlineColor(sf::Color(120, 80, 40, alpha));
        panel.setOutlineThickness(3.f);
        window.draw(panel);

        sf::Text titleText(activeTarget->getInteractionTitle(), menuFont, 24);
        titleText.setFillColor(sf::Color(255, 210, 130, alpha));
        sf::FloatRect titleBounds = titleText.getLocalBounds();
        titleText.setOrigin(titleBounds.left + titleBounds.width / 2.f, titleBounds.top + titleBounds.height / 2.f);
        titleText.setPosition(panel.getPosition().x, panel.getPosition().y - 200.f);
        window.draw(titleText);

        sf::RectangleShape divider(sf::Vector2f(360.f, 2.f));
        divider.setFillColor(sf::Color(120, 80, 40, alpha));
        divider.setOrigin(180.f, 1.f);
        divider.setPosition(panel.getPosition().x, panel.getPosition().y - 170.f);
        window.draw(divider);

        float startY = panel.getPosition().y - 140.f;
        for (size_t i = 0; i < currentMenuEntries.size(); ++i) {
            sf::Text entryText(currentMenuEntries[i].label, menuFont, 16);
            
            if (currentMenuEntries[i].action != nullptr && static_cast<int>(i) == selectedMenuIndex) {
                entryText.setFillColor(sf::Color(255, 255, 150, alpha));
                sf::Text arrow(">", menuFont, 16);
                arrow.setFillColor(sf::Color(255, 255, 150, alpha));
                arrow.setPosition(panel.getPosition().x - 180.f, startY + (i * 32.f));
                window.draw(arrow);
            } else {
                entryText.setFillColor(sf::Color(210, 200, 180, alpha));
            }

            entryText.setPosition(panel.getPosition().x - 160.f, startY + (i * 32.f));
            window.draw(entryText);
        }

        sf::Text footer("[ESC/E] Return to Village", menuFont, 12);
        footer.setFillColor(sf::Color(150, 130, 110, alpha));
        sf::FloatRect footerBounds = footer.getLocalBounds();
        footer.setOrigin(footerBounds.left + footerBounds.width / 2.f, footerBounds.top + footerBounds.height / 2.f);
        footer.setPosition(panel.getPosition().x, panel.getPosition().y + 215.f);
        window.draw(footer);
    }

    window.setView(originalView);
}