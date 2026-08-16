#pragma once
#include "interaction/InteractionTarget.h"
#include "simulation/SimulationRegistry.h"
#include "world/WorldManager.h"
#include <string>
#include <vector>

class BuildNodeInteractionTarget : public InteractionTarget {
private:
    sim::StructureID structureId;
    sim::VillageID villageId;
    sim::SimulationRegistry& registry;
    WorldManager* worldManager;

public:
    BuildNodeInteractionTarget(sim::StructureID sId, sim::VillageID vId, sim::SimulationRegistry& reg, WorldManager* wm)
        : structureId(sId), villageId(vId), registry(reg), worldManager(wm) {}

    std::string getInteractionType() const override { return "BuildNode"; }

    sf::Vector2f getInteractionPosition() const override {
        sim::StructureData* s = registry.getStructure(structureId);
        if (!s) return sf::Vector2f(0.f, 0.f);
        return sf::Vector2f(s->worldX, 480.f);
    }

    bool canInteract() const override {
        return registry.getStructure(structureId) != nullptr;
    }

    std::string getInteractionTitle() const override {
        sim::StructureData* s = registry.getStructure(structureId);
        if (!s) return "Construction Site";

        if (s->isFinished) {
            return s->name + " (Operational)";
        }
        if (s->isUnderConstruction) {
            int pct = static_cast<int>((s->progress / s->maxProgress) * 100.f);
            return s->name + " (Building: " + std::to_string(pct) + "%)";
        }
        return "Build Site: " + s->name;
    }

    int getPriority() const override { return 15; }

    void onInteract() override {
        sim::StructureData* s = registry.getStructure(structureId);
        sim::VillageData* v = registry.getVillage(villageId);
        if (!s || !v) return;

        // If planned and affordable, commence construction
        if (s->isPlanned && !s->isUnderConstruction && !s->isFinished) {
            if (v->wood >= s->reqWood && v->stone >= s->reqStone) {
                v->wood -= s->reqWood;
                v->stone -= s->reqStone;
                s->curWood = s->reqWood;
                s->curStone = s->reqStone;
                s->isPlanned = false;
                s->isUnderConstruction = true;
                v->constructionQueue.push_back(s->id);

                // Dispatch first available idle clan worker
                for (sim::EntityID mId : v->members) {
                    if (mId == v->leaderId) continue;
                    sim::ApeData* ape = registry.getApe(mId);
                    if (ape && ape->alive && (ape->currentJob == sim::Job::Idle || ape->currentJob == sim::Job::Wander || ape->currentJob == sim::Job::Socialize)) {
                        ape->currentJob = sim::Job::Builder;
                        ape->currentTargetStructure = s->id;
                        ape->hasTravelDestination = true;
                        ape->travelDestinationX = s->worldX;
                        s->currentBuilder = ape->id;
                        break;
                    }
                }
            }
        }
    }

    void onClose() override {}

    std::vector<InteractionMenuEntry> buildInteractionMenu() override {
        std::vector<InteractionMenuEntry> entries;
        sim::StructureData* s = registry.getStructure(structureId);
        sim::VillageData* v = registry.getVillage(villageId);
        if (!s || !v) return entries;

        if (s->isFinished) {
            entries.push_back({"Structure: " + s->name, nullptr});
            entries.push_back({"Benefit: " + s->benefitText, nullptr});
            entries.push_back({"Status: Fully Operational", nullptr});
            return entries;
        }

        if (s->isUnderConstruction) {
            int pct = static_cast<int>((s->progress / s->maxProgress) * 100.f);
            entries.push_back({"Project: " + s->name + " in progress", nullptr});
            entries.push_back({"Progress: " + std::to_string(pct) + "% Completed", nullptr});
            entries.push_back({"", nullptr});
            std::string builderName = "Workers En Route";
            sim::ApeData* b = registry.getApe(s->currentBuilder);
            if (b) builderName = "Builder: " + b->name;
            entries.push_back({builderName, nullptr});
            return entries;
        }

        // Planned Build Node Menu
        entries.push_back({"Project: " + s->name, nullptr});
        entries.push_back({"Benefit: " + s->benefitText, nullptr});
        entries.push_back({"", nullptr});
        entries.push_back({"Required: " + std::to_string(s->reqWood) + " Wood", nullptr});
        entries.push_back({"Clan Stockpile: " + std::to_string(v->wood) + " Wood", nullptr});
        entries.push_back({"", nullptr});

        if (v->wood >= s->reqWood && v->stone >= s->reqStone) {
            entries.push_back({"[ Order Clan Workers to Build ]", [this]() {
                onInteract();
            }});
        } else {
            entries.push_back({"[ Insufficient Wood in Stockpile ]", nullptr});
        }

        return entries;
    }
};