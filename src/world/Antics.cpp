#include "world/Antics.h"
#include "world/WorldManager.h"
#include "entities/Ape.h"
#include <algorithm>
#include <cmath>

namespace {

// One character per art pixel; '.' is transparent.
struct PaletteEntry { char key; sf::Color color; };
const PaletteEntry PALETTE[] = {
    {'o', { 24,  14,  10}}, {'R', {226,  64,  72}}, {'r', {168,  36,  52}}, {'W', {255, 214, 214}},
    {'N', {250, 236, 170}}, {'n', {206, 172,  84}}, {'y', {250, 214,  70}}, {'k', {206, 160,  40}},
};
const std::vector<const char*> ART[3] = {
    { ".oo.oo.", "oRWoRRo", "oRRRRRo", "oRRRRro", ".oRRro.", "..oro..", "...o..." },                 // heart
    { "..ooo.", "..oNNo", "..oNno", "..oNo.", "ooono.", "oNNno.", "oNno..", ".oo..." },                // note
    { "...oo....", "..oyyo.o.", ".oyykoyyo", "oyykoyyko", "oykoyykko", ".oooykko.", "...oooo.." },     // bananas
};

const float EARSHOT = 750.f;
const float ROAR_WAIT = 4.f;
const float TREAT_REACH = 40.f;

} // namespace

void Antics::init() {
    int width = 0, height = 0;
    for (const auto& art : ART) { width += static_cast<int>(std::char_traits<char>::length(art[0])) + 1; height = std::max(height, static_cast<int>(art.size())); }
    sf::Image img;
    img.create(static_cast<unsigned>(width), static_cast<unsigned>(height), sf::Color::Transparent);
    int x0 = 0;
    for (int i = 0; i < SPR_COUNT; ++i) {
        const int w = static_cast<int>(std::char_traits<char>::length(ART[i][0])), h = static_cast<int>(ART[i].size());
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                const char ch = ART[i][static_cast<size_t>(y)][x];
                if (ch == '.') continue;
                for (const auto& p : PALETTE) if (p.key == ch) { img.setPixel(static_cast<unsigned>(x0 + x), static_cast<unsigned>(y), p.color); break; }
            }
        }
        rects[i] = sf::IntRect(x0, 0, w, h);
        x0 += w + 1;
    }
    atlasReady = atlas.loadFromImage(img);
    atlas.setSmooth(false);
}

void Antics::addEmote(float x, float y, int sprite, float delay) {
    Emote e;
    e.x = x; e.y = y; e.sprite = sprite; e.delay = delay;
    emotes.push_back(e);
}

bool Antics::handleKey(sf::Keyboard::Key key, sim::SimulationRegistry& reg, sim::EntityID playerId, WorldManager* world, Ape* playerApe) {
    if (key != sf::Keyboard::R && key != sf::Keyboard::Q) return false;
    sim::ApeData* player = reg.getApe(playerId);
    sim::VillageData* village = player ? reg.getVillage(player->villageId) : nullptr;
    if (!player || !village || !world) return true;
    const float ground = world->getTerrainHeight(player->worldX);

    if (key == sf::Keyboard::R) {
        if (roarCooldown > 0.f) return true;
        roarCooldown = ROAR_WAIT;
        if (playerApe && playerApe->getState() == ApeState::Grounded) {
            playerApe->setVelocity(playerApe->getVelocity().x, -300.f);        // the hop that goes with the drumming
        }
        addEmote(player->worldX, ground - 190.f, SPR_NOTE, 0.f);

        const int day = reg.getYear() * 372 + (reg.getMonth() - 1) * 31 + reg.getDay();
        const bool firstToday = (day != lastCheeredDay);
        lastCheeredDay = day;
        int answered = 0;
        for (sim::EntityID id : village->members) {
            sim::ApeData* a = reg.getApe(id);
            if (!a || !a->alive || a->id == playerId || std::abs(a->worldX - player->worldX) > EARSHOT) continue;
            // further apes answer later, so the cheer rolls outward
            const float delay = 0.25f + std::abs(a->worldX - player->worldX) / 900.f;
            addEmote(a->worldX, world->getTerrainHeight(a->worldX) - 250.f, (answered % 3 == 0) ? SPR_HEART : SPR_NOTE, delay);
            if (firstToday) {
                const auto it = a->opinions.find(playerId);
                const int now = (it != a->opinions.end()) ? it->second : 15;
                a->opinions[playerId] = std::min(100, now + 1);
            }
            ++answered;
        }
        return true;
    }

    // Q: toss a treat
    if (treat.active || village->food <= 0) return true;
    village->food -= 1;

    // aim at the nearest ape of the clan, or straight ahead if no one is about
    float aim = player->worldX + 260.f;
    float best = 900.f;
    for (sim::EntityID id : village->members) {
        const sim::ApeData* a = reg.getApe(id);
        if (!a || !a->alive || a->id == playerId) continue;
        const float d = std::abs(a->worldX - player->worldX);
        if (d < best) { best = d; aim = player->worldX + (a->worldX - player->worldX) * 0.6f; }
    }
    aim = std::clamp(aim, player->worldX - 420.f, player->worldX + 420.f);

    treat = Treat();
    treat.active = true;
    treat.x = player->worldX;
    treat.y = ground - 110.f;
    treat.vy = -420.f;
    const float airTime = 2.f * 420.f / 1100.f + 0.2f;
    treat.vx = (aim - player->worldX) / airTime;
    return true;
}

void Antics::update(float dt, sim::SimulationRegistry& reg, sim::EntityID playerId, WorldManager* world) {
    roarCooldown = std::max(0.f, roarCooldown - dt);

    for (auto& e : emotes) {
        if (e.delay > 0.f) e.delay -= dt;
        else { e.age += dt; e.y -= 46.f * dt; }
    }
    emotes.erase(std::remove_if(emotes.begin(), emotes.end(), [](const Emote& e) { return e.age > 1.6f; }), emotes.end());

    if (!treat.active || !world) return;
    treat.age += dt;
    const float ground = world->getTerrainHeight(treat.x);
    if (!treat.landed) {
        treat.vy += 1100.f * dt;
        treat.x += treat.vx * dt;
        treat.y += treat.vy * dt;
        if (treat.y >= ground - 8.f) { treat.y = ground - 8.f; treat.landed = true; }
        return;
    }
    treat.y = ground - 8.f;

    sim::ApeData* player = reg.getApe(playerId);
    sim::VillageData* village = player ? reg.getVillage(player->villageId) : nullptr;
    if (!village || treat.age > 25.f) { treat.active = false; return; }      // nobody came: it is left for the birds

    sim::ApeData* eater = reg.getApe(treat.eater);
    if (!eater || !eater->alive) {
        eater = nullptr;
        float best = 1200.f;
        for (sim::EntityID id : village->members) {
            sim::ApeData* a = reg.getApe(id);
            if (!a || !a->alive || a->id == playerId) continue;
            const float d = std::abs(a->worldX - treat.x);
            if (d < best) { best = d; eater = a; }
        }
        treat.eater = eater ? eater->id : 0;
    }
    if (!eater) return;

    // Asked again every frame: their daily routine would otherwise call them away.
    eater->hasTravelDestination = true;
    eater->travelDestinationX = treat.x;
    if (std::abs(eater->worldX - treat.x) <= TREAT_REACH) {
        eater->hunger = std::min(100.f, eater->hunger + 25.f);
        const auto it = eater->opinions.find(playerId);
        const int now = (it != eater->opinions.end()) ? it->second : 15;
        eater->opinions[playerId] = std::min(100, now + 2);
        eater->hasTravelDestination = false;
        addEmote(eater->worldX, world->getTerrainHeight(eater->worldX) - 250.f, SPR_HEART, 0.f);
        treat.active = false;
    }
}

void Antics::blit(sf::RenderTarget& target, int sprite, float cx, float cy, float scale, sf::Uint8 alpha) const {
    sf::Sprite sp(atlas, rects[sprite]);
    sp.setOrigin(static_cast<float>(rects[sprite].width) * 0.5f, static_cast<float>(rects[sprite].height) * 0.5f);
    sp.setScale(scale, scale);
    sp.setPosition(std::round(cx), std::round(cy));
    sp.setColor(sf::Color(255, 255, 255, alpha));
    target.draw(sp);
}

void Antics::draw(sf::RenderTarget& target) {
    if (!atlasReady) return;
    if (treat.active) blit(target, SPR_BANANA, treat.x, treat.y - 8.f, 3.f, 255);
    for (const auto& e : emotes) {
        if (e.delay > 0.f) continue;
        const float fade = std::clamp(1.6f - e.age, 0.f, 0.5f) / 0.5f;
        const float pop = std::min(1.f, e.age / 0.12f);                         // snaps in, drifts up, fades out
        blit(target, e.sprite, e.x + std::sin(e.age * 5.f) * 5.f, e.y, 4.f * (0.6f + 0.4f * pop), static_cast<sf::Uint8>(255.f * fade));
    }
}
