#include "core/StateMachine.h"

StateMachine::StateMachine() : isAdding(false), isRemoving(false), isReplacing(false) {}

void StateMachine::addState(std::unique_ptr<GameState> state, bool replace) {
    isAdding = true;
    isReplacing = replace;
    newState = std::move(state);
}

void StateMachine::removeState() {
    isRemoving = true;
}

void StateMachine::processStateChanges() {
    if (isRemoving && !states.empty()) {
        states.pop();
        isRemoving = false;
    }
    if (isAdding) {
        if (isReplacing && !states.empty()) {
            states.pop();
        }
        states.push(std::move(newState));
        states.top()->init();
        isAdding = false;
    }
}

void StateMachine::processEvents(const sf::Event& event) {
    if (!states.empty()) {
        states.top()->processEvents(event);
    }
}

void StateMachine::update(float dt) {
    processStateChanges();
    if (!states.empty()) {
        states.top()->update(dt);
    }
}

void StateMachine::draw(sf::RenderWindow& window) {
    if (!states.empty()) {
        states.top()->draw(window);
    }
}