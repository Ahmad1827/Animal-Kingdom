#pragma once
#include "interaction/InteractionTarget.h"
#include "simulation/SimulationRegistry.h"
#include <string>
#include <vector>

class KingInteractionTarget : public InteractionTarget {
private:
    sim::EntityID kingId;
    sim::SimulationRegistry& registry;

public:
    // Notice we removed posX and posY from the constructor
    KingInteractionTarget(sim::EntityID id, sim::SimulationRegistry& reg)
        : kingId(id), registry(reg) {}

    std::string getInteractionType() const override { return "King"; }
    
    // Dynamically fetch the King's exact physical position every single frame
    sf::Vector2f getInteractionPosition() const override { 
        sim::ApeData* king = registry.getApe(kingId);
        if (king) {
            return sf::Vector2f(king->worldX, king->worldY - 30.f); 
        }
        return sf::Vector2f(0.f, 0.f);
    }
    
    bool canInteract() const override { return registry.getApe(kingId) != nullptr; }

    std::string getInteractionTitle() const override {
        sim::ApeData* king = registry.getApe(kingId);
        if (!king) return "Empty Throne";
        return "King " + king->name;
    }

    int getPriority() const override { return 25; } // Higher priority than buildings

    void onInteract() override {}
    void onClose() override {}

    std::vector<InteractionMenuEntry> buildInteractionMenu() override {
        std::vector<InteractionMenuEntry> entries;
        sim::ApeData* king = registry.getApe(kingId);
        if (!king) return entries;

        sim::DynastyData* dynasty = registry.getDynasty(king->dynastyId);
        std::string dynName = dynasty ? dynasty->name : "Unknown Dynasty";
        
        entries.push_back({"House of " + dynName, nullptr});
        entries.push_back({"Age: " + std::to_string(king->age), nullptr});
        entries.push_back({"", nullptr});
        
        if (king->currentKingdom != 0) {
            sim::KingdomData* kd = registry.getKingdom(king->currentKingdom);
            if (kd) {
                entries.push_back({"Ruler of " + kd->name, nullptr});
                entries.push_back({"Military Strength: " + std::to_string(kd->militaryStrength), nullptr});
                entries.push_back({"Influence: " + std::to_string(kd->influence), nullptr});
            }
        }
        
        return entries;
    }
};