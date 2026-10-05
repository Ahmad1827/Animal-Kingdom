#include "interaction/InteractionManager.h"
#include "ui/UIKit.h"
#include <cmath>
#include <algorithm>

InteractionManager::InteractionManager() 
    : currentPromptTarget(nullptr), activeTarget(nullptr), 
      isMenuOpen(false), isClosing(false), selectedMenuIndex(0), fontLoaded(false),
      preInteractionCenter(0.f, 0.f), preInteractionZoom(1.f), interactionTransitionTimer(0.f),
      lastPlayerPos(0.f, 0.f) {
    fontLoaded = menuFont.loadFromFile("assets/fonts/Cinzel-Bold.ttf") ||
                 menuFont.loadFromFile("assets/fonts/Cinzel-Regular.ttf") ||
                 menuFont.loadFromFile("font.ttf") ||
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
    float closestDistSq = 300.0f * 300.0f;

    for (const auto& target : targets) {
        if (!target || !target->canInteract()) continue;

        sf::Vector2f pos = target->getInteractionPosition();
        float dx = std::abs(pos.x - playerPos.x);
        float dy = std::abs(pos.y - playerPos.y);

        if (dx <= 170.0f && dy <= 220.0f) {
            float distSq = (dx * dx) + (dy * dy);
            if (distSq < closestDistSq) {
                closestDistSq = distSq;
                bestTarget = target;
            }
        }
    }

    currentPromptTarget = bestTarget;
}

void InteractionManager::executeEntry(int index) {
    if (index < 0 || index >= static_cast<int>(currentMenuEntries.size())) return;

    if (currentMenuEntries[index].action) {
        currentMenuEntries[index].action();
        if (activeTarget && activeTarget->canInteract()) {
            currentMenuEntries = activeTarget->buildInteractionMenu();
            if (currentMenuEntries.empty()) {
                isClosing = true;
            } else if (selectedMenuIndex >= static_cast<int>(currentMenuEntries.size())) {
                selectedMenuIndex = 0;
            }
        } else {
            isClosing = true;
        }
    }
}

void InteractionManager::handleEvent(const sf::Event& event, CameraManager& cameraManager) {
    if (isMenuOpen && !isClosing) {
        if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
            for (size_t i = 0; i < currentMenuEntries.size() && i < rowBounds.size(); ++i) {
                if (rowBounds[i].contains(mouseUi) && currentMenuEntries[i].action != nullptr) {
                    selectedMenuIndex = static_cast<int>(i);
                    executeEntry(selectedMenuIndex);
                    return;
                }
            }
        }
        else if (event.type == sf::Event::KeyPressed) {
            if (event.key.code == sf::Keyboard::Escape) {
                isClosing = true;
            }
            else if (event.key.code == sf::Keyboard::W || event.key.code == sf::Keyboard::Up) {
                // step to the previous row that does something, skipping plain lines
                const int n = static_cast<int>(currentMenuEntries.size());
                for (int tries = 0; tries < n; ++tries) {
                    selectedMenuIndex = (selectedMenuIndex - 1 + n) % n;
                    if (currentMenuEntries[selectedMenuIndex].action) break;
                }
            }
            else if (event.key.code == sf::Keyboard::S || event.key.code == sf::Keyboard::Down) {
                const int n = static_cast<int>(currentMenuEntries.size());
                for (int tries = 0; tries < n; ++tries) {
                    selectedMenuIndex = (selectedMenuIndex + 1) % n;
                    if (currentMenuEntries[selectedMenuIndex].action) break;
                }
            }
            else if (event.key.code == sf::Keyboard::Return || event.key.code == sf::Keyboard::Space || event.key.code == sf::Keyboard::E) {
                executeEntry(selectedMenuIndex);
            }
            else if (event.key.code >= sf::Keyboard::Num1 && event.key.code <= sf::Keyboard::Num9) {
                int numIdx = event.key.code - sf::Keyboard::Num1;
                executeEntry(numIdx);
            }
            else if (event.key.code >= sf::Keyboard::Numpad1 && event.key.code <= sf::Keyboard::Numpad9) {
                int numIdx = event.key.code - sf::Keyboard::Numpad1;
                executeEntry(numIdx);
            }
        }
    } 
    else if (!isMenuOpen && !isClosing) {
        if (event.type == sf::Event::KeyPressed && (event.key.code == sf::Keyboard::E || event.key.code == sf::Keyboard::Return) && currentPromptTarget) {
            activeTarget = currentPromptTarget;
            activeTarget->onInteract();
            currentMenuEntries = activeTarget->buildInteractionMenu();
            
            if (currentMenuEntries.empty()) {
                activeTarget = nullptr;
            } else {
                selectedMenuIndex = 0;
                for (size_t i = 0; i < currentMenuEntries.size(); ++i) {
                    if (currentMenuEntries[i].action) { selectedMenuIndex = static_cast<int>(i); break; }
                }
                isMenuOpen = true;
                isClosing = false;
                interactionTransitionTimer = 0.f;
                preInteractionZoom = cameraManager.getView().getSize().x / 1280.f; 
                preInteractionCenter = cameraManager.getView().getCenter();
            }
        }
    }
}

namespace {

// Menu labels come from the targets written as "[ Do a thing ]" and "--- HEADER ---".
std::string trimmed(const std::string& label, const char* open, const char* close) {
    std::string s = label;
    const std::string o(open), c(close);
    if (s.rfind(o, 0) == 0) s = s.substr(o.size());
    if (s.size() >= c.size() && s.compare(s.size() - c.size(), c.size(), c) == 0) s = s.substr(0, s.size() - c.size());
    while (!s.empty() && s.front() == ' ') s.erase(s.begin());
    while (!s.empty() && s.back() == ' ') s.pop_back();
    return s;
}

bool isHeader(const std::string& label) { return label.rfind("---", 0) == 0; }

} // namespace

void InteractionManager::draw(sf::RenderWindow& window, const sf::View& letterboxView, const sf::View& cameraView) {
    if (!fontLoaded) return;
    using namespace ui;

    // ---- prompt over the ape's head ---------------------------------------------
    if (!isMenuOpen && !isClosing && currentPromptTarget) {
        sf::View activeWorldView = cameraView;
        activeWorldView.setViewport(letterboxView.getViewport());
        const sf::Vector2i sPixel = window.mapCoordsToPixel(sf::Vector2f(lastPlayerPos.x, lastPlayerPos.y - 105.f), activeWorldView);
        const sf::Vector2f at = window.mapPixelToCoords(sPixel, letterboxView);

        window.setView(letterboxView);
        Canvas c(window, menuFont);

        const std::string title = currentPromptTarget->getInteractionTitle();
        const float w = c.textWidth(title, 12, true) + 52.f, h = 28.f;
        const float x = std::clamp(at.x - w * 0.5f, c.left() + 6.f, c.right() - 6.f - w);
        const float y = std::clamp(at.y - h * 0.5f, 44.f, c.bottom() - 90.f);
        c.panel({x, y, w, h});
        const sf::FloatRect cap(x + 7.f, y + 5.f, 20.f, 18.f);
        c.button(cap, true, false);
        c.text("E", cap.left + 10.f, cap.top + 9.f, {11, sf::Color(255, 248, 224), true, Align::Center});
        c.text(title, x + 35.f, y + h * 0.5f, {12, theme::Text, true});
    }

    // ---- menu -------------------------------------------------------------------
    if ((isMenuOpen || isClosing) && activeTarget) {
        window.setView(letterboxView);
        Canvas c(window, menuFont);
        mouseUi = Canvas::mouse(window);

        const float ease = getEase();
        if (ease < 0.05f) return;

        // Row heights: buttons for things you can do, plain lines for what you are told.
        const float pW = 460.f;
        float bodyH = 0.f;
        std::vector<float> heights;
        for (const auto& e : currentMenuEntries) {
            float h = 20.f;
            if (e.action) h = 34.f;
            else if (e.label.empty()) h = 8.f;
            else if (isHeader(e.label)) h = 28.f;
            heights.push_back(h);
            bodyH += h;
        }
        const float pH = 58.f + bodyH + 36.f;
        const sf::FloatRect P(640.f - pW * 0.5f, std::max(46.f, 360.f - pH * 0.5f) + 16.f * (1.f - ease), pW, pH);

        c.fill({c.left(), 40.f, c.right() - c.left(), c.bottom() - 40.f}, sf::Color(8, 7, 6, static_cast<sf::Uint8>(110.f * ease)));
        c.panel(P);
        c.gradient({P.left + c.px(2), P.top + c.px(2), P.width - c.px(4), 44.f}, sf::Color(255, 214, 140, 40), sf::Color(255, 214, 140, 0));
        c.icon(Icon::Hand, P.left + 24.f, P.top + 24.f);
        c.text(c.fit(activeTarget->getInteractionTitle(), P.width - 60.f, 15, true), P.left + 42.f, P.top + 24.f, {15, theme::Gold, true});
        c.fill({P.left + c.px(2), P.top + 46.f, P.width - c.px(4), c.px(1)}, theme::Bronze);

        const float left = P.left + 16.f, w = P.width - 32.f;
        float y = P.top + 58.f;
        rowBounds.assign(currentMenuEntries.size(), sf::FloatRect());
        for (size_t i = 0; i < currentMenuEntries.size(); ++i) {
            const InteractionMenuEntry& e = currentMenuEntries[i];
            if (e.action) {
                const sf::FloatRect r(left, y, w, 30.f);
                rowBounds[i] = r;
                const bool hov = r.contains(mouseUi);
                if (hov) selectedMenuIndex = static_cast<int>(i);
                const bool selected = (static_cast<int>(i) == selectedMenuIndex);
                c.button(r, selected, hov);
                if (i < 9) {
                    c.fill({r.left + c.px(1), r.top + c.px(1), 24.f - c.px(1), r.height - c.px(2)}, sf::Color(0, 0, 0, selected ? 60 : 85));
                    c.text(std::to_string(i + 1), r.left + 13.f, r.top + 15.f, {11, selected ? theme::GoldBright : theme::BronzeLight, true, Align::Center});
                    c.divider(r.left + 24.f, r.top + c.px(1), r.height - c.px(2));
                }
                c.text(c.fit(trimmed(e.label, "[", "]"), w - 44.f, 12, selected), r.left + 34.f, r.top + 15.f,
                       {12, selected ? sf::Color(255, 248, 224) : theme::Text, selected});
            } else if (isHeader(e.label)) {
                c.heading(left, y + 6.f, w, trimmed(trimmed(e.label, "---", "---"), "", ""));
            } else if (!e.label.empty()) {
                c.text(c.fit(e.label, w, 11), left + 2.f, y + 10.f, {11, theme::Text});
            }
            y += heights[i];
        }

        c.text("ESC leave      W / S choose      E confirm", P.left + P.width * 0.5f, P.top + P.height - 18.f,
               {10, theme::TextMuted, false, Align::Center, false, true});
    }
}

void InteractionManager::draw(sf::RenderTarget& target) {
    (void)target;
}