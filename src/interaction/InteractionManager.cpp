#include "interaction/InteractionManager.h"
#include <cmath>
#include <algorithm>

InteractionManager::InteractionManager() 
    : currentPromptTarget(nullptr), activeTarget(nullptr), 
      isMenuOpen(false), isClosing(false), selectedMenuIndex(0), fontLoaded(false),
      preInteractionCenter(0.f, 0.f), preInteractionZoom(1.f), interactionTransitionTimer(0.f),
      lastPlayerPos(0.f, 0.f) {
    fontLoaded = menuFont.loadFromFile("font.ttf") ||
                 menuFont.loadFromFile("assets/fonts/font.ttf") ||
                 menuFont.loadFromFile("assets/fonts/PressStart2P-Regular.ttf") ||
                 menuFont.loadFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf");
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
    lastPlayerPos = playerPos;

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
    float closestDistSq = 125.0f * 125.0f;

    for (const auto& target : targets) {
        if (!target->canInteract()) continue;

        sf::Vector2f pos = target->getInteractionPosition();
        float dx = pos.x - playerPos.x;
        float dy = pos.y - playerPos.y;
        float distSq = (dx * dx) + (dy * dy);

        if (distSq < closestDistSq) {
            closestDistSq = distSq;
            bestTarget = target;
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
                
                if (currentMenuEntries.empty()) {
                    activeTarget = nullptr;
                } else {
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
}

void InteractionManager::draw(sf::RenderWindow& window) {
    if (!fontLoaded) return;

    sf::View originalView = window.getView();
    window.setView(window.getDefaultView()); 

    if (!isMenuOpen && !isClosing && currentPromptTarget) {
        sf::Vector2f worldPos = currentPromptTarget->getInteractionPosition();
        
        sf::Vector2f promptWorldPos = worldPos;
        float targetElevatedY = (worldPos.y >= 450.f) ? (worldPos.y - 120.f) : (worldPos.y - 40.f);
        promptWorldPos.y = std::min(targetElevatedY, lastPlayerPos.y - 75.f);

        sf::Vector2i screenPos = window.mapCoordsToPixel(promptWorldPos, originalView);

        std::string rawTitle = currentPromptTarget->getInteractionTitle();

        sf::Text keyText("[E]", menuFont, 9);
        keyText.setFillColor(sf::Color(255, 235, 170));
        keyText.setStyle(sf::Text::Bold);

        sf::Text titleText(rawTitle, menuFont, 9);
        titleText.setFillColor(sf::Color(240, 225, 190));
        titleText.setStyle(sf::Text::Bold);

        sf::FloatRect kb = keyText.getLocalBounds();
        sf::FloatRect tb = titleText.getLocalBounds();

        float pillPaddingX = 7.f;
        float spacing = 5.f;
        float iconSize = 5.f;
        float pillW = pillPaddingX * 2.f + iconSize + spacing + tb.width + spacing + kb.width + 6.f;
        float pillH = 18.f;

        float promptX = static_cast<float>(screenPos.x);
        float promptY = static_cast<float>(screenPos.y);

        sf::RectangleShape pillBg(sf::Vector2f(pillW, pillH));
        pillBg.setOrigin(pillW / 2.f, pillH / 2.f);
        pillBg.setPosition(promptX, promptY);
        pillBg.setFillColor(sf::Color(20, 14, 10, 225));
        pillBg.setOutlineColor(sf::Color(175, 135, 65, 230));
        pillBg.setOutlineThickness(1.f);
        window.draw(pillBg);

        float leftX = promptX - pillW / 2.f + pillPaddingX;

        sf::CircleShape icon(2.5f, 4);
        icon.setOrigin(2.5f, 2.5f);
        icon.setPosition(leftX + 2.5f, promptY);
        icon.setFillColor(sf::Color(225, 175, 55));
        window.draw(icon);

        titleText.setOrigin(tb.left, tb.top + tb.height / 2.f);
        titleText.setPosition(leftX + iconSize + spacing, promptY);
        window.draw(titleText);

        float keyBadgeW = kb.width + 5.f;
        float keyBadgeH = 12.f;
        float keyBadgeX = leftX + iconSize + spacing + tb.width + spacing + keyBadgeW / 2.f;

        sf::RectangleShape keyBadge(sf::Vector2f(keyBadgeW, keyBadgeH));
        keyBadge.setOrigin(keyBadgeW / 2.f, keyBadgeH / 2.f);
        keyBadge.setPosition(keyBadgeX, promptY);
        keyBadge.setFillColor(sf::Color(55, 38, 22, 240));
        keyBadge.setOutlineColor(sf::Color(140, 105, 50));
        keyBadge.setOutlineThickness(1.f);
        window.draw(keyBadge);

        keyText.setOrigin(kb.left + kb.width / 2.f, kb.top + kb.height / 2.f);
        keyText.setPosition(keyBadge.getPosition());
        window.draw(keyText);
    }

    if ((isMenuOpen || isClosing) && activeTarget) {
        float ease = getEase();
        sf::Uint8 alpha = static_cast<sf::Uint8>(255 * ease);
        float yOffset = 20.f * (1.f - ease);

        sf::Vector2f winSize(static_cast<float>(window.getSize().x), static_cast<float>(window.getSize().y));
        float pW = 460.f;
        float pH = 520.f;

        sf::RectangleShape shadow(sf::Vector2f(pW + 16.f, pH + 16.f));
        shadow.setOrigin((pW + 16.f) / 2.f, (pH + 16.f) / 2.f);
        shadow.setPosition(winSize.x / 2.0f + 6.f, winSize.y / 2.0f + 6.f + yOffset);
        shadow.setFillColor(sf::Color(0, 0, 0, static_cast<sf::Uint8>(110 * ease)));
        window.draw(shadow);

        sf::RectangleShape woodBorder(sf::Vector2f(pW + 12.f, pH + 12.f));
        woodBorder.setOrigin((pW + 12.f) / 2.f, (pH + 12.f) / 2.f);
        woodBorder.setPosition(winSize.x / 2.0f, winSize.y / 2.0f + yOffset);
        woodBorder.setFillColor(sf::Color(45, 30, 20, alpha));
        woodBorder.setOutlineColor(sf::Color(15, 10, 5, alpha));
        woodBorder.setOutlineThickness(2.f);
        window.draw(woodBorder);

        sf::RectangleShape panel(sf::Vector2f(pW, pH));
        panel.setOrigin(pW / 2.f, pH / 2.f);
        panel.setPosition(woodBorder.getPosition());
        panel.setFillColor(sf::Color(220, 205, 172, static_cast<sf::Uint8>(250 * ease)));
        panel.setOutlineColor(sf::Color(165, 125, 60, alpha));
        panel.setOutlineThickness(2.f);
        window.draw(panel);

        sf::RectangleShape headerPlate(sf::Vector2f(pW - 24.f, 44.f));
        headerPlate.setOrigin((pW - 24.f) / 2.f, 22.f);
        headerPlate.setPosition(panel.getPosition().x, panel.getPosition().y - pH / 2.f + 32.f);
        headerPlate.setFillColor(sf::Color(65, 45, 28, alpha));
        headerPlate.setOutlineColor(sf::Color(140, 105, 45, alpha));
        headerPlate.setOutlineThickness(1.5f);
        window.draw(headerPlate);

        sf::Text titleText(activeTarget->getInteractionTitle(), menuFont, 18);
        titleText.setFillColor(sf::Color(245, 215, 120, alpha));
        titleText.setStyle(sf::Text::Bold);
        sf::FloatRect titleBounds = titleText.getLocalBounds();
        titleText.setOrigin(titleBounds.left + titleBounds.width / 2.f, titleBounds.top + titleBounds.height / 2.f);
        titleText.setPosition(headerPlate.getPosition());
        window.draw(titleText);

        float startY = panel.getPosition().y - pH / 2.f + 70.f;
        for (size_t i = 0; i < currentMenuEntries.size(); ++i) {
            sf::FloatRect rowRect(panel.getPosition().x - (pW - 32.f) / 2.f, startY + (i * 34.f), pW - 32.f, 28.f);

            if (currentMenuEntries[i].action != nullptr) {
                sf::RectangleShape rowBg(sf::Vector2f(rowRect.width, rowRect.height));
                rowBg.setPosition(rowRect.left, rowRect.top);
                if (static_cast<int>(i) == selectedMenuIndex) {
                    rowBg.setFillColor(sf::Color(190, 165, 125, alpha));
                    rowBg.setOutlineColor(sf::Color(145, 105, 50, alpha));
                    rowBg.setOutlineThickness(1.5f);
                } else {
                    rowBg.setFillColor(sf::Color(208, 192, 158, alpha));
                    rowBg.setOutlineColor(sf::Color(165, 145, 115, alpha));
                    rowBg.setOutlineThickness(1.f);
                }
                window.draw(rowBg);
            }

            sf::Text entryText(currentMenuEntries[i].label, menuFont, 13);
            if (currentMenuEntries[i].action != nullptr) {
                if (static_cast<int>(i) == selectedMenuIndex) {
                    entryText.setFillColor(sf::Color(30, 15, 5, alpha));
                    entryText.setStyle(sf::Text::Bold);
                    sf::CircleShape selDot(3.5f, 4);
                    selDot.setPosition(rowRect.left + 8.f, rowRect.top + 10.f);
                    selDot.setFillColor(sf::Color(160, 45, 25, alpha));
                    window.draw(selDot);
                } else {
                    entryText.setFillColor(sf::Color(55, 38, 22, alpha));
                }
                entryText.setPosition(rowRect.left + 22.f, rowRect.top + 5.f);
            } else {
                entryText.setFillColor(sf::Color(70, 50, 32, alpha));
                entryText.setStyle(sf::Text::Bold);
                entryText.setPosition(rowRect.left + 8.f, rowRect.top + 5.f);
            }
            window.draw(entryText);
        }

        sf::Text footer("[ ESC / E ] Return to Settlement", menuFont, 11);
        footer.setFillColor(sf::Color(110, 85, 60, alpha));
        footer.setStyle(sf::Text::Italic);
        sf::FloatRect footerBounds = footer.getLocalBounds();
        footer.setOrigin(footerBounds.left + footerBounds.width / 2.f, footerBounds.top + footerBounds.height / 2.f);
        footer.setPosition(panel.getPosition().x, panel.getPosition().y + pH / 2.f - 20.f);
        window.draw(footer);
    }

    window.setView(originalView);
}