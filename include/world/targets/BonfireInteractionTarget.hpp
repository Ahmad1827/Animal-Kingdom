#pragma once
#include "interaction/InteractionTarget.h"
#include "simulation/SimulationRegistry.h"
#include <iostream>

class BonfireInteractionTarget : public InteractionTarget {
private:
    sim::EntityID structureId;
    sim::SimulationRegistry& registry;
    float posX;
    float posY;

public:
    BonfireInteractionTarget(sim::EntityID id, sim::SimulationRegistry& reg, float x, float y)
        : structureId(id), registry(reg), posX(x), posY(y) {}

    std::string getInteractionType() const override { return "Bonfire"; }
    
    sf::Vector2f getInteractionPosition() const override { return sf::Vector2f(posX, posY); }
    
    bool canInteract() const override { return true; }
    
    std::string getInteractionTitle() const override { return "Village Bonfire"; }
    
    int getPriority() const override { return 15; } // Higher than regular props

    std::vector<InteractionMenuEntry> buildInteractionMenu() override {
        std::vector<InteractionMenuEntry> entries;

        entries.push_back({
            "View Village News", 
            []() { std::cout << "Menu Action: Viewing recent village news and history...\n"; }
        });

        entries.push_back({
            "Inspect Happiness & Mood", 
            []() { std::cout << "Menu Action: Checking village population morale...\n"; }
        });

        entries.push_back({
            "Upcoming Work Schedule", 
            []() { std::cout << "Menu Action: Inspecting active construction projects...\n"; }
        });

        return entries;
    }
};