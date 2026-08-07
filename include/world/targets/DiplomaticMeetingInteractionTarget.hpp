#pragma once
#include "interaction/InteractionTarget.h"
#include "simulation/SimulationRegistry.h"
#include <string>
#include <vector>

class DiplomaticMeetingInteractionTarget : public InteractionTarget {
private:
    sim::KingdomID kingdomA;
    sim::KingdomID kingdomB;
    float worldX;
    float worldY;
    sim::SimulationRegistry& registry;
    sim::EntityID playerApeId;

public:
    DiplomaticMeetingInteractionTarget(sim::KingdomID kA, sim::KingdomID kB, float x, float y, sim::SimulationRegistry& reg, sim::EntityID pId)
        : kingdomA(kA), kingdomB(kB), worldX(x), worldY(y), registry(reg), playerApeId(pId) {}

    std::string getInteractionType() const override { return "Diplomacy"; }
    
    sf::Vector2f getInteractionPosition() const override { 
        return sf::Vector2f(worldX, worldY - 40.f); 
    }
    
    bool canInteract() const override { 
        // Available as long as the player is part of one of the participating kingdoms
        sim::ApeData* player = registry.getApe(playerApeId);
        if (!player) return false;
        return (player->currentKingdom == kingdomA || player->currentKingdom == kingdomB);
    }

    std::string getInteractionTitle() const override {
        return "Diplomatic Meeting Ground";
    }

    int getPriority() const override { return 30; }

    void onInteract() override {}
    void onClose() override {}

    std::vector<InteractionMenuEntry> buildInteractionMenu() override {
        std::vector<InteractionMenuEntry> entries;
        sim::ApeData* player = registry.getApe(playerApeId);
        if (!player) return entries;

        sim::KingdomID targetKingdomId = (player->currentKingdom == kingdomA) ? kingdomB : kingdomA;
        sim::KingdomData* targetKingdom = registry.getKingdom(targetKingdomId);
        
        std::string targetName = targetKingdom ? targetKingdom->name : "Unknown Kingdom";

        entries.push_back({"REQUEST AUDIENCE", nullptr});
        entries.push_back({"Kingdom: " + targetName, nullptr});
        
        return entries;
    }
};