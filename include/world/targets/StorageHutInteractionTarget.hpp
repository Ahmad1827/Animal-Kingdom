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

    std::string getStockpileDescription(int amount, const std::string& itemName) const {
        if (amount == 0) return "No " + itemName + " left.";
        if (amount < 10) return "A small pile of " + std::to_string(amount) + " " + itemName + ".";
        if (amount < 50) return "A healthy stock of " + std::to_string(amount) + " " + itemName + ".";
        return "An overflowing hoard of " + std::to_string(amount) + " " + itemName + "!";
    }

public:
    StorageHutInteractionTarget(sim::EntityID id, sim::SimulationRegistry& reg, float x, float y)
        : villageId(id), registry(reg), posX(x), posY(y) {}

    std::string getInteractionType() const override { return "StorageHut"; }
    sf::Vector2f getInteractionPosition() const override { return sf::Vector2f(posX, posY); }
    bool canInteract() const override { return true; }

    std::string getInteractionTitle() const override {
        sim::VillageData* v = registry.getVillage(villageId);
        if (!v) return "Ruined Storage";
        return v->name + " - Stockpiles";
    }

    int getPriority() const override { return 15; }

    void onInteract() override {}
    void onClose() override {}

    std::vector<InteractionMenuEntry> buildInteractionMenu() override {
        std::vector<InteractionMenuEntry> entries;
        sim::VillageData* v = registry.getVillage(villageId);
        if (!v) return entries;

        entries.push_back({getStockpileDescription(v->food, "Food"), nullptr});
        entries.push_back({getStockpileDescription(v->wood, "Wood"), nullptr});
        entries.push_back({getStockpileDescription(v->stone, "Stone"), nullptr});
        
        entries.push_back({"", nullptr});
        entries.push_back({"--- TOOLS & EQUIPMENT ---", nullptr});
        
        int totalTools = v->toolsAxe + v->toolsPick + v->toolsSpear + v->toolsTorch + v->toolsBasket + v->toolsRope;
        
        if (totalTools == 0) {
            entries.push_back({"The tool racks are completely empty.", nullptr});
        } else {
            if (v->toolsAxe > 0) entries.push_back({std::to_string(v->toolsAxe) + " Stone Axes", nullptr});
            if (v->toolsPick > 0) entries.push_back({std::to_string(v->toolsPick) + " Pickaxes", nullptr});
            if (v->toolsSpear > 0) entries.push_back({std::to_string(v->toolsSpear) + " Wooden Spears", nullptr});
            if (v->toolsBasket > 0) entries.push_back({std::to_string(v->toolsBasket) + " Woven Baskets", nullptr});
        }

        return entries;
    }
};