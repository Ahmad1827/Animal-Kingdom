#pragma once
#include "interaction/InteractionTarget.h"
#include "simulation/SimulationRegistry.h"
#include "world/WorldManager.h"
#include <string>
#include <vector>

class BorderTotemInteractionTarget : public InteractionTarget {
private:
    sim::KingdomID kingdomId;
    sim::SimulationRegistry& registry;
    WorldManager* worldManager;
    bool isLeftBorder;

    std::string getTensionDescription(int tension) const {
        if (tension > 75) return "Hostile (War is imminent)";
        if (tension > 50) return "Tense (Borders are heavily guarded)";
        if (tension > 25) return "Uneasy (Travelers are watched closely)";
        return "Peaceful (Trade flows freely)";
    }

public:
    BorderTotemInteractionTarget(sim::KingdomID id, sim::SimulationRegistry& reg, WorldManager* wm, bool isLeft)
        : kingdomId(id), registry(reg), worldManager(wm), isLeftBorder(isLeft) {}

    std::string getInteractionType() const override { return "BorderTotem"; }

    // Dynamically track the moving border every frame
    sf::Vector2f getInteractionPosition() const override {
        sim::KingdomData* kd = registry.getKingdom(kingdomId);
        if (!kd) return sf::Vector2f(0.f, 0.f);
        
        float x = isLeftBorder ? kd->territoryMinX : kd->territoryMaxX;
        float y = worldManager->getTerrainHeight(x) - 40.f; 
        return sf::Vector2f(x, y);
    }
    
    bool canInteract() const override { return registry.getKingdom(kingdomId) != nullptr; }

    std::string getInteractionTitle() const override {
        sim::KingdomData* kd = registry.getKingdom(kingdomId);
        if (!kd) return "Forgotten Border";
        return "Border of " + kd->name;
    }

    int getPriority() const override { return 15; }

    void onInteract() override {}
    void onClose() override {}

    std::vector<InteractionMenuEntry> buildInteractionMenu() override {
        std::vector<InteractionMenuEntry> entries;
        sim::KingdomData* kd = registry.getKingdom(kingdomId);
        if (!kd) return entries;

        sim::ApeData* king = registry.getApe(kd->currentKingId);
        std::string rulerName = king ? king->name : "Unknown";

        entries.push_back({"This land is ruled by King " + rulerName + ".", nullptr});
        entries.push_back({"The population is roughly " + std::to_string(kd->population) + " apes.", nullptr});
        
        entries.push_back({"", nullptr});
        entries.push_back({"--- DIPLOMATIC STANDING ---", nullptr});
        
        if (kd->borderTension.empty()) {
            entries.push_back({"This kingdom is largely isolated.", nullptr});
        } else {
            for (const auto& tensionPair : kd->borderTension) {
                sim::KingdomData* other = registry.getKingdom(tensionPair.first);
                if (other) {
                    entries.push_back({"With " + other->name + ": " + getTensionDescription(tensionPair.second), nullptr});
                }
            }
        }
        return entries;
    }
};