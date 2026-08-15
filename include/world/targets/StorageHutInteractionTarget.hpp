#pragma once
#include "interaction/InteractionTarget.h"
#include "simulation/SimulationRegistry.h"
#include <string>
#include <vector>

class StorageHutInteractionTarget : public InteractionTarget {
private:
    sim::EntityID villageId;
    sim::SimulationRegistry& registry;
    float posX;
    float posY;

public:
    StorageHutInteractionTarget(sim::EntityID id, sim::SimulationRegistry& reg, float x, float y)
        : villageId(id), registry(reg), posX(x), posY(y) {}

    std::string getInteractionType() const override { return "StorageHut"; }
    sf::Vector2f getInteractionPosition() const override { return sf::Vector2f(posX, posY - 20.f); }
    bool canInteract() const override { return true; }

    std::string getInteractionTitle() const override {
        sim::VillageData* v = registry.getVillage(villageId);
        if (!v) return "Tribal Granary";
        return v->name + " - Granary & Armory";
    }

    int getPriority() const override { return 15; }
    void onInteract() override {}
    void onClose() override {}

    std::vector<InteractionMenuEntry> buildInteractionMenu() override {
        std::vector<InteractionMenuEntry> entries;
        sim::VillageData* v = registry.getVillage(villageId);
        if (!v) return entries;

        entries.push_back({"Food Reserves: " + std::to_string(v->food) + " units", nullptr});
        entries.push_back({"Timber Stores: " + std::to_string(v->wood) + " logs", nullptr});
        entries.push_back({"Worked Stone:  " + std::to_string(v->stone) + " blocks", nullptr});
        entries.push_back({"", nullptr});
        entries.push_back({"--- ARMORY & TOOL RACKS ---", nullptr});

        int totalTools = v->toolsAxe + v->toolsPick + v->toolsSpear + v->toolsTorch + v->toolsBasket + v->toolsRope;
        if (totalTools == 0) {
            entries.push_back({"The tool racks are empty.", nullptr});
        } else {
            if (v->toolsAxe > 0) entries.push_back({std::to_string(v->toolsAxe) + " Stone Axes", nullptr});
            if (v->toolsPick > 0) entries.push_back({std::to_string(v->toolsPick) + " Quarrying Picks", nullptr});
            if (v->toolsSpear > 0) entries.push_back({std::to_string(v->toolsSpear) + " Hunting Spears", nullptr});
            if (v->toolsBasket > 0) entries.push_back({std::to_string(v->toolsBasket) + " Woven Baskets", nullptr});
        }

        return entries;
    }
};