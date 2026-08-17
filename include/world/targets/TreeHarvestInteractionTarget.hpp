#pragma once
#include "interaction/InteractionTarget.h"
#include "simulation/SimulationRegistry.h"
#include "world/WorldManager.h"
#include "entities/Tree.h"
#include <string>
#include <vector>
#include <iostream>

class TreeHarvestInteractionTarget : public InteractionTarget {
private:
    Tree* targetTree;
    sim::VillageID playerVillageId;
    sim::SimulationRegistry& registry;
    WorldManager* worldManager;

public:
    TreeHarvestInteractionTarget(Tree* tree, sim::VillageID vId, sim::SimulationRegistry& reg, WorldManager* wm)
        : targetTree(tree), playerVillageId(vId), registry(reg), worldManager(wm) {}

    std::string getInteractionType() const override { return "TreeHarvest"; }
    
    sf::Vector2f getInteractionPosition() const override {
        if (!targetTree) return sf::Vector2f(0.f, 0.f);
        return sf::Vector2f(targetTree->getTrunkCenter(), 480.f);
    }

    bool canInteract() const override {
        return targetTree != nullptr && targetTree->getHarvestState() != TreeHarvestState::Harvested;
    }

    std::string getInteractionTitle() const override {
        return "Harvest Tree";
    }

    int getPriority() const override { return 10; }

    void onInteract() override {
        if (!targetTree) return;

        sim::VillageData* v = registry.getVillage(playerVillageId);
        if (!v) {
            for (auto& pair : registry.getAllVillages()) {
                v = &pair.second;
                break;
            }
        }
        if (!v) return;

        sim::EntityID chosenWorkerId = 0;
        for (sim::EntityID mId : v->members) {
            if (mId == v->leaderId) continue;
            sim::ApeData* ape = registry.getApe(mId);
            if (ape && ape->alive) {
                chosenWorkerId = mId;
                break;
            }
        }

        if (chosenWorkerId != 0) {
            sim::ApeData* worker = registry.getApe(chosenWorkerId);
            if (worker) {
                float treeX = targetTree->getTrunkCenter();

                worker->currentJob = sim::Job::Woodcutter;
                worker->hasTravelDestination = true;
                worker->travelDestinationX = treeX;
                worker->currentTargetNode = static_cast<sim::EntityID>(targetTree->getId());

                targetTree->setHarvestState(TreeHarvestState::Targeted);
                targetTree->setAssignedWorkerId(chosenWorkerId);

                std::cout << "[WOODCUT] Worker " << chosenWorkerId << " assigned Tree " << targetTree->getId() << " at X=" << treeX << std::endl << std::flush;
            }
        }
    }

    void onClose() override {}

    std::vector<InteractionMenuEntry> buildInteractionMenu() override {
        std::vector<InteractionMenuEntry> entries;
        if (!targetTree) return entries;

        entries.push_back({"[ Order Clan Worker to Chop ]", [this]() {
            onInteract();
        }});

        return entries;
    }
};