#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <functional>

struct InteractionMenuEntry {
    std::string label;
    std::function<void()> action;
};

class InteractionTarget {
public:
    virtual ~InteractionTarget() = default;
    
    virtual std::string getInteractionType() const = 0;
    virtual sf::Vector2f getInteractionPosition() const = 0;
    virtual bool canInteract() const = 0;
    virtual std::string getInteractionTitle() const = 0;
    virtual std::vector<InteractionMenuEntry> buildInteractionMenu() = 0;
    virtual int getPriority() const { return 10; } // Higher priority overrides lower
    
    virtual void onInteract() {}
};