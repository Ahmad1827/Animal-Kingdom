#include "dynasty/DynastyUI.h"
#include "dynasty/Succession.h"
#include <algorithm>
#include <cmath>

namespace sim {

DynastyUI::DynastyUI() = default;

void DynastyUI::init(const sf::Font& loadedFont) {
    font = loadedFont;
}

void DynastyUI::toggle(DynastyUIMode mode) {
    if (currentMode == mode) {
        currentMode = DynastyUIMode::CLOSED;
    } else {
        currentMode = mode;
    }
    activeTooltip.active = false;
}

void DynastyUI::close() {
    currentMode = DynastyUIMode::CLOSED;
    activeTooltip.active = false;
}

void DynastyUI::nextCharacter(const Dynasty& dynasty) {
    if (!dynasty.memberIds.empty()) {
        inspectedMemberIndex = (inspectedMemberIndex + 1) % dynasty.memberIds.size();
    }
}

void DynastyUI::previousCharacter(const Dynasty& dynasty) {
    if (!dynasty.memberIds.empty()) {
        inspectedMemberIndex = (inspectedMemberIndex + dynasty.memberIds.size() - 1) % dynasty.memberIds.size();
    }
}

void DynastyUI::handleMouseMove(sf::Vector2f mousePos) {
    currentMousePos = mousePos;
}

bool DynastyUI::handleMouseClick(sf::Vector2f mousePos, const Dynasty& dynasty) {
    if (currentMode == DynastyUIMode::CLOSED) return false;
    return false;
}

void DynastyUI::render(
    sf::RenderTarget& target,
    const Dynasty& dynasty,
    const Clan& clan,
    const std::unordered_map<Character::ID, Character>& registry,
    const std::vector<Faction>& factions,
    Character::ID currentAlphaId
) {
    if (currentMode == DynastyUIMode::CLOSED) return;

    activeTooltip.active = false;

    sf::Vector2f viewSize = target.getView().getSize();
    sf::Vector2f viewCenter = target.getView().getCenter();

    // 1. World Dimmer Layer
    sf::RectangleShape dimmer(viewSize);
    dimmer.setOrigin(viewSize.x / 2.f, viewSize.y / 2.f);
    dimmer.setPosition(viewCenter);
    dimmer.setFillColor(sf::Color(10, 8, 6, 175));
    target.draw(dimmer);

    // 2. Main Parchment Dimensions
    float panelW = std::min(viewSize.x * 0.88f, 1100.f);
    float panelH = std::min(viewSize.y * 0.88f, 640.f);
    sf::FloatRect panelBounds(viewCenter.x - panelW / 2.f, viewCenter.y - panelH / 2.f, panelW, panelH);

    // 3. Screen Routing
    switch (currentMode) {
        case DynastyUIMode::CHARACTER_VIEW: {
            if (!dynasty.memberIds.empty()) {
                if (inspectedMemberIndex >= dynasty.memberIds.size()) inspectedMemberIndex = 0;
                Character::ID targetCharId = dynasty.memberIds[inspectedMemberIndex];
                if (registry.count(targetCharId)) {
                    drawParchmentFrame(target, panelBounds, "COUNCIL & LINEAGE DOSSIER", "Clan of " + clan.name);
                    drawCharacterView(target, panelBounds, registry.at(targetCharId), dynasty, clan, currentAlphaId);
                }
            }
            break;
        }
        case DynastyUIMode::FAMILY_TREE_VIEW:
            drawParchmentFrame(target, panelBounds, "DYNASTIC GENEALOGY", dynasty.name + " Bloodline Records");
            drawFamilyTreeView(target, panelBounds, dynasty, registry, currentAlphaId);
            break;
        case DynastyUIMode::SUCCESSION_VIEW:
            drawParchmentFrame(target, panelBounds, "SUCCESSION & TRIBAL FACTIONS", "Realmpolitik of the High Canopy");
            drawSuccessionView(target, panelBounds, dynasty, clan, registry, factions);
            break;
        default:
            break;
    }

    drawNavTabs(target, panelBounds);

    if (activeTooltip.active) {
        drawTooltip(target);
    }
}

void DynastyUI::drawParchmentFrame(sf::RenderTarget& target, sf::FloatRect bounds, const std::string& title, const std::string& subtitle) {
    // Outer Dark Wooden Rim
    sf::RectangleShape woodBorder(sf::Vector2f(bounds.width + 16.f, bounds.height + 16.f));
    woodBorder.setPosition(bounds.left - 8.f, bounds.top - 8.f);
    woodBorder.setFillColor(sf::Color(42, 28, 18));
    woodBorder.setOutlineColor(sf::Color(20, 12, 8));
    woodBorder.setOutlineThickness(3.f);
    target.draw(woodBorder);

    // Carved Corner Accents
    float cSize = 24.f;
    auto drawCorner = [&](float x, float y) {
        sf::RectangleShape c(sf::Vector2f(cSize, cSize));
        c.setPosition(x, y);
        c.setFillColor(sf::Color(70, 48, 28));
        c.setOutlineColor(sf::Color(185, 145, 65));
        c.setOutlineThickness(1.5f);
        target.draw(c);
    };
    drawCorner(bounds.left - 8.f, bounds.top - 8.f);
    drawCorner(bounds.left + bounds.width + 8.f - cSize, bounds.top - 8.f);
    drawCorner(bounds.left - 8.f, bounds.top + bounds.height + 8.f - cSize);
    drawCorner(bounds.left + bounds.width + 8.f - cSize, bounds.top + bounds.height + 8.f - cSize);

    // Main Aged Parchment Body
    sf::RectangleShape parchment(sf::Vector2f(bounds.width, bounds.height));
    parchment.setPosition(bounds.left, bounds.top);
    parchment.setFillColor(sf::Color(222, 207, 174));
    target.draw(parchment);

    // Inner Antique Gold Filigree Rim
    sf::RectangleShape goldRim(sf::Vector2f(bounds.width - 12.f, bounds.height - 12.f));
    goldRim.setPosition(bounds.left + 6.f, bounds.top + 6.f);
    goldRim.setFillColor(sf::Color::Transparent);
    goldRim.setOutlineColor(sf::Color(170, 130, 55, 180));
    goldRim.setOutlineThickness(2.f);
    target.draw(goldRim);

    // Header Plaque
    sf::RectangleShape headerPlate(sf::Vector2f(bounds.width - 24.f, 46.f));
    headerPlate.setPosition(bounds.left + 12.f, bounds.top + 12.f);
    headerPlate.setFillColor(sf::Color(55, 38, 24));
    headerPlate.setOutlineColor(sf::Color(140, 105, 45));
    headerPlate.setOutlineThickness(1.5f);
    target.draw(headerPlate);

    sf::Text tTitle(title, font, 20);
    tTitle.setFillColor(sf::Color(245, 215, 120));
    tTitle.setStyle(sf::Text::Bold);
    tTitle.setPosition(bounds.left + 24.f, bounds.top + 16.f);
    target.draw(tTitle);

    sf::Text tSub(subtitle, font, 14);
    tSub.setFillColor(sf::Color(195, 180, 155));
    sf::FloatRect subBounds = tSub.getLocalBounds();
    tSub.setPosition(bounds.left + bounds.width - subBounds.width - 32.f, bounds.top + 26.f);
    target.draw(tSub);
}

void DynastyUI::drawSubPanel(sf::RenderTarget& target, sf::FloatRect bounds, const std::string& header) {
    sf::RectangleShape bg(sf::Vector2f(bounds.width, bounds.height));
    bg.setPosition(bounds.left, bounds.top);
    bg.setFillColor(sf::Color(210, 193, 158));
    bg.setOutlineColor(sf::Color(150, 125, 90));
    bg.setOutlineThickness(1.5f);
    target.draw(bg);

    if (!header.empty()) {
        sf::RectangleShape hPlate(sf::Vector2f(bounds.width, 24.f));
        hPlate.setPosition(bounds.left, bounds.top);
        hPlate.setFillColor(sf::Color(70, 50, 32));
        target.draw(hPlate);

        sf::Text tHead(header, font, 12);
        tHead.setFillColor(sf::Color(235, 215, 160));
        tHead.setStyle(sf::Text::Bold);
        tHead.setPosition(bounds.left + 8.f, bounds.top + 4.f);
        target.draw(tHead);
    }
}

void DynastyUI::drawNavTabs(sf::RenderTarget& target, sf::FloatRect panelBounds) {
    struct TabDef { DynastyUIMode mode; std::string label; };
    std::vector<TabDef> tabs = {
        { DynastyUIMode::CHARACTER_VIEW, "CHARACTER (C)" },
        { DynastyUIMode::FAMILY_TREE_VIEW, "GENEALOGY (F)" },
        { DynastyUIMode::SUCCESSION_VIEW, "SUCCESSION (U)" }
    };

    float tabW = 140.f;
    float tabH = 26.f;
    float startX = panelBounds.left + 20.f;
    float startY = panelBounds.top - tabH - 4.f;

    for (size_t i = 0; i < tabs.size(); ++i) {
        bool isActive = (currentMode == tabs[i].mode);
        sf::FloatRect tRect(startX + i * (tabW + 6.f), startY, tabW, tabH);

        sf::RectangleShape tabShape(sf::Vector2f(tRect.width, tRect.height));
        tabShape.setPosition(tRect.left, tRect.top);
        tabShape.setFillColor(isActive ? sf::Color(222, 207, 174) : sf::Color(50, 35, 22));
        tabShape.setOutlineColor(sf::Color(170, 130, 55));
        tabShape.setOutlineThickness(1.5f);
        target.draw(tabShape);

        sf::Text tText(tabs[i].label, font, 11);
        tText.setFillColor(isActive ? sf::Color(40, 25, 15) : sf::Color(200, 185, 155));
        tText.setStyle(sf::Text::Bold);
        sf::FloatRect tb = tText.getLocalBounds();
        tText.setOrigin(tb.left + tb.width / 2.f, tb.top + tb.height / 2.f);
        tText.setPosition(tRect.left + tRect.width / 2.f, tRect.top + tRect.height / 2.f);
        target.draw(tText);
    }
}

void DynastyUI::drawCharacterView(
    sf::RenderTarget& target,
    sf::FloatRect bounds,
    const Character& character,
    const Dynasty& dynasty,
    const Clan& clan,
    Character::ID alphaId
) {
    float startY = bounds.top + 68.f;
    float availH = bounds.height - 84.f;

    // LEFT COLUMN: Portrait & Core Identity
    float col1W = 260.f;
    sf::FloatRect col1(bounds.left + 16.f, startY, col1W, availH);
    drawSubPanel(target, col1, "PERSONAGE");

    // Portrait Frame Box
    float pBoxSize = 130.f;
    sf::FloatRect pBox(col1.left + (col1W - pBoxSize) / 2.f, col1.top + 34.f, pBoxSize, pBoxSize);
    sf::RectangleShape portBg(sf::Vector2f(pBox.width, pBox.height));
    portBg.setPosition(pBox.left, pBox.top);
    portBg.setFillColor(sf::Color(40, 30, 22));
    portBg.setOutlineColor(sf::Color(160, 120, 50));
    portBg.setOutlineThickness(3.f);
    target.draw(portBg);

    // Decorative Portrait Placeholder / Coat of Arms
    sf::CircleShape crest(42.f, 6);
    crest.setOrigin(42.f, 42.f);
    crest.setPosition(pBox.left + pBox.width / 2.f, pBox.top + pBox.height / 2.f - 6.f);
    crest.setFillColor(character.id == alphaId ? sf::Color(180, 135, 45) : sf::Color(90, 70, 55));
    crest.setOutlineColor(sf::Color(230, 195, 100));
    crest.setOutlineThickness(2.f);
    target.draw(crest);

    sf::Text crestText(character.sex == Sex::MALE ? "M" : "F", font, 24);
    crestText.setFillColor(sf::Color::White);
    crestText.setStyle(sf::Text::Bold);
    sf::FloatRect ctb = crestText.getLocalBounds();
    crestText.setOrigin(ctb.left + ctb.width / 2.f, ctb.top + ctb.height / 2.f);
    crestText.setPosition(crest.getPosition());
    target.draw(crestText);

    // Character Name Banner
    sf::Text nameText(character.name, font, 18);
    nameText.setFillColor(sf::Color(35, 20, 10));
    nameText.setStyle(sf::Text::Bold);
    sf::FloatRect ntb = nameText.getLocalBounds();
    nameText.setOrigin(ntb.left + ntb.width / 2.f, 0.f);
    nameText.setPosition(col1.left + col1W / 2.f, pBox.top + pBox.height + 10.f);
    target.draw(nameText);

    std::string rankStr = (character.id == alphaId) ? "Ruling Alpha" : "Clan Member";
    sf::Text titleText(rankStr + " | Age " + std::to_string(character.age), font, 13);
    titleText.setFillColor(sf::Color(90, 70, 50));
    sf::FloatRect ttb = titleText.getLocalBounds();
    titleText.setOrigin(ttb.left + ttb.width / 2.f, 0.f);
    titleText.setPosition(col1.left + col1W / 2.f, nameText.getPosition().y + 24.f);
    target.draw(titleText);

    // Ambition Ribbon
    sf::RectangleShape ambBox(sf::Vector2f(col1W - 24.f, 44.f));
    ambBox.setPosition(col1.left + 12.f, titleText.getPosition().y + 30.f);
    ambBox.setFillColor(sf::Color(190, 172, 138));
    ambBox.setOutlineColor(sf::Color(135, 110, 75));
    ambBox.setOutlineThickness(1.f);
    target.draw(ambBox);

    sf::Text ambLbl("CURRENT AMBITION", font, 10);
    ambLbl.setFillColor(sf::Color(80, 55, 30));
    ambLbl.setStyle(sf::Text::Bold);
    ambLbl.setPosition(ambBox.getPosition().x + 6.f, ambBox.getPosition().y + 4.f);
    target.draw(ambLbl);

    sf::Text ambVal(character.ambition.getName(), font, 11);
    ambVal.setFillColor(sf::Color(25, 15, 10));
    ambVal.setPosition(ambBox.getPosition().x + 6.f, ambBox.getPosition().y + 20.f);
    target.draw(ambVal);

    // Prestige & Tension Status
    std::string metaStr = "Prestige: " + std::to_string(character.prestige) + "   |   Tension: " + std::to_string(clan.tension) + "%";
    sf::Text metaText(metaStr, font, 12);
    metaText.setFillColor(sf::Color(60, 45, 30));
    metaText.setPosition(col1.left + 14.f, col1.top + availH - 46.f);
    target.draw(metaText);

    sf::Text navHint("[Left / Right Arrow] Cycle Kin", font, 11);
    navHint.setFillColor(sf::Color(120, 95, 70));
    navHint.setStyle(sf::Text::Italic);
    navHint.setPosition(col1.left + 14.f, col1.top + availH - 24.f);
    target.draw(navHint);

    // RIGHT COLUMN 1: Attributes Ledger & Traits Plaque
    float col2X = col1.left + col1W + 14.f;
    float col2W = 340.f;
    sf::FloatRect col2(col2X, startY, col2W, availH);

    // 1. Attributes Box
    sf::FloatRect attrBox(col2X, startY, col2W, 190.f);
    drawSubPanel(target, attrBox, "ATTRIBUTES & PROWESS");

    CharacterStats stats = character.getEffectiveStats();
    struct AttrRow { std::string name; int val; std::string tip; sf::Color col; };
    std::vector<AttrRow> attrRows = {
        { "Prowess", stats.prowess, "Combat strength and dominance challenge modifier.", sf::Color(160, 40, 30) },
        { "Martial", stats.martial, "Warfare command, hunting skill, and morale boost.", sf::Color(170, 80, 20) },
        { "Stewardship", stats.stewardship, "Foraging output, food preservation, and tools.", sf::Color(40, 110, 50) },
        { "Intrigue", stats.intrigue, "Plot scheming, stealth foraging, and betrayal detection.", sf::Color(80, 40, 120) },
        { "Diplomacy", stats.diplomacy, "Clan cohesion, opinion sway, and grooming loyalty.", sf::Color(30, 80, 140) }
    };

    float aY = attrBox.top + 30.f;
    for (const auto& row : attrRows) {
        sf::FloatRect rowBounds(attrBox.left + 8.f, aY, attrBox.width - 16.f, 26.f);

        // Hover Detection for Tooltip
        if (rowBounds.contains(currentMousePos)) {
            activeTooltip = { true, row.name, row.tip, currentMousePos };
            sf::RectangleShape hov(sf::Vector2f(rowBounds.width, rowBounds.height));
            hov.setPosition(rowBounds.left, rowBounds.top);
            hov.setFillColor(sf::Color(255, 255, 255, 45));
            target.draw(hov);
        }

        // Color Pill
        sf::RectangleShape pill(sf::Vector2f(6.f, 18.f));
        pill.setPosition(rowBounds.left + 4.f, rowBounds.top + 4.f);
        pill.setFillColor(row.col);
        target.draw(pill);

        sf::Text lbl(row.name, font, 13);
        lbl.setFillColor(sf::Color(40, 25, 15));
        lbl.setPosition(rowBounds.left + 18.f, rowBounds.top + 3.f);
        target.draw(lbl);

        sf::Text val(std::to_string(row.val), font, 14);
        val.setFillColor(sf::Color(20, 10, 5));
        val.setStyle(sf::Text::Bold);
        sf::FloatRect vb = val.getLocalBounds();
        val.setPosition(rowBounds.left + rowBounds.width - vb.width - 8.f, rowBounds.top + 2.f);
        target.draw(val);

        aY += 30.f;
    }

    // 2. Traits Plaques Box
    sf::FloatRect traitBox(col2X, startY + 200.f, col2W, availH - 200.f);
    drawSubPanel(target, traitBox, "DISTINCTIVE TRAITS");

    float trY = traitBox.top + 32.f;
    if (character.traits.empty()) {
        sf::Text none("No distinct genetic or behavioral traits.", font, 12);
        none.setFillColor(sf::Color(100, 80, 60));
        none.setStyle(sf::Text::Italic);
        none.setPosition(traitBox.left + 12.f, trY);
        target.draw(none);
    } else {
        for (TraitID trait : character.traits) {
            std::string tName = "Unknown";
            std::string tDesc = "";
            switch (trait) {
                case TraitID::SILVERBACK: tName = "Silverback"; tDesc = "+Prowess, +Martial, Alpha Respect"; break;
                case TraitID::FIERCE_ROAR: tName = "Fierce Roar"; tDesc = "+Morale, Dominance Intimidation"; break;
                case TraitID::SNEAKY_FORAGER: tName = "Sneaky Forager"; tDesc = "+Intrigue, Gathering, -Honesty"; break;
                case TraitID::WISE_ELDER: tName = "Wise Elder"; tDesc = "+Diplomacy, Clan Stability Boost"; break;
                case TraitID::AMBITIOUS: tName = "Ambitious"; tDesc = "Wants Power, Faction Organizer"; break;
                case TraitID::LOYAL: tName = "Loyal"; tDesc = "Resists Betrayal, +Opinion Boost"; break;
                case TraitID::COWARD: tName = "Coward"; tDesc = "-Prowess, Likely to Flee Danger"; break;
                case TraitID::NATURAL_LEADER: tName = "Natural Leader"; tDesc = "+Diplomacy, Better Recruitment"; break;
                case TraitID::FICKLE_GROOMER: tName = "Fickle Groomer"; tDesc = "Unstable Loyalty, Opinion Volatility"; break;
            }

            sf::FloatRect tCard(traitBox.left + 8.f, trY, traitBox.width - 16.f, 32.f);
            sf::RectangleShape cardShape(sf::Vector2f(tCard.width, tCard.height));
            cardShape.setPosition(tCard.left, tCard.top);
            cardShape.setFillColor(sf::Color(198, 180, 146));
            cardShape.setOutlineColor(sf::Color(140, 115, 80));
            cardShape.setOutlineThickness(1.f);
            target.draw(cardShape);

            sf::Text tTxt(tName, font, 12);
            tTxt.setFillColor(sf::Color(40, 20, 10));
            tTxt.setStyle(sf::Text::Bold);
            tTxt.setPosition(tCard.left + 8.f, tCard.top + 2.f);
            target.draw(tTxt);

            sf::Text dTxt(tDesc, font, 10);
            dTxt.setFillColor(sf::Color(85, 65, 45));
            dTxt.setPosition(tCard.left + 8.f, tCard.top + 16.f);
            target.draw(dTxt);

            trY += 38.f;
        }
    }

    // RIGHT COLUMN 2: Opinion Breakdown Ledger
    float col3X = col2X + col2W + 14.f;
    float col3W = bounds.left + bounds.width - col3X - 16.f;
    sf::FloatRect col3(col3X, startY, col3W, availH);
    drawSubPanel(target, col3, "OPINION OF ALPHA");

    const OpinionMatrix* opMatrix = character.getOpinionBreakdown(alphaId);
    float oY = col3.top + 34.f;

    if (character.id == alphaId) {
        sf::Text selfTxt("Current Ruler of the Clan\n(Self Opinion: +100)", font, 13);
        selfTxt.setFillColor(sf::Color(60, 40, 20));
        selfTxt.setStyle(sf::Text::Bold);
        selfTxt.setPosition(col3.left + 12.f, oY);
        target.draw(selfTxt);
    } else if (opMatrix && !opMatrix->modifiers.empty()) {
        for (const auto& mod : opMatrix->modifiers) {
            sf::Text rTxt(mod.reason, font, 12);
            rTxt.setFillColor(sf::Color(45, 30, 20));
            rTxt.setPosition(col3.left + 10.f, oY);
            target.draw(rTxt);

            std::string sign = (mod.value >= 0) ? "+" : "";
            sf::Text vTxt(sign + std::to_string(mod.value), font, 12);
            vTxt.setFillColor(mod.value >= 0 ? sf::Color(30, 110, 40) : sf::Color(160, 30, 20));
            vTxt.setStyle(sf::Text::Bold);
            sf::FloatRect vb = vTxt.getLocalBounds();
            vTxt.setPosition(col3.left + col3W - vb.width - 12.f, oY);
            target.draw(vTxt);

            oY += 24.f;
        }

        // Total Score Pill
        sf::RectangleShape totBox(sf::Vector2f(col3W - 20.f, 32.f));
        totBox.setPosition(col3.left + 10.f, col3.top + availH - 42.f);
        totBox.setFillColor(sf::Color(60, 42, 26));
        target.draw(totBox);

        int total = opMatrix->calculateTotal();
        sf::Text totLbl("Total Opinion", font, 12);
        totLbl.setFillColor(sf::Color(220, 200, 160));
        totLbl.setPosition(totBox.getPosition().x + 8.f, totBox.getPosition().y + 7.f);
        target.draw(totLbl);

        sf::Text totVal((total >= 0 ? "+" : "") + std::to_string(total), font, 14);
        totVal.setFillColor(total >= 0 ? sf::Color(120, 230, 120) : sf::Color(240, 110, 100));
        totVal.setStyle(sf::Text::Bold);
        sf::FloatRect tvb = totVal.getLocalBounds();
        totVal.setPosition(totBox.getPosition().x + totBox.getSize().x - tvb.width - 10.f, totBox.getPosition().y + 6.f);
        target.draw(totVal);
    } else {
        sf::Text neutr("Neutral Standpoint (Score: 0)", font, 12);
        neutr.setFillColor(sf::Color(80, 60, 40));
        neutr.setPosition(col3.left + 12.f, oY);
        target.draw(neutr);
    }
}

void DynastyUI::drawFamilyTreeView(
    sf::RenderTarget& target,
    sf::FloatRect bounds,
    const Dynasty& dynasty,
    const std::unordered_map<Character::ID, Character>& registry,
    Character::ID currentAlphaId
) {
    float startY = bounds.top + 68.f;
    float availH = bounds.height - 84.f;

    sf::FloatRect treePanel(bounds.left + 16.f, startY, bounds.width - 32.f, availH);
    drawSubPanel(target, treePanel, "GENEALOGICAL TREE VIEW");

    if (!registry.count(currentAlphaId)) return;
    const Character& alpha = registry.at(currentAlphaId);

    float centerX = treePanel.left + treePanel.width / 2.f;

    // LEVEL 1: Parents / Ancestors
    float yGen1 = treePanel.top + 45.f;
    if (alpha.fatherId != Character::INVALID_ID && registry.count(alpha.fatherId)) {
        drawCharacterNode(target, sf::Vector2f(centerX - 130.f, yGen1), registry.at(alpha.fatherId), false, false);
    }
    if (alpha.motherId != Character::INVALID_ID && registry.count(alpha.motherId)) {
        drawCharacterNode(target, sf::Vector2f(centerX + 130.f, yGen1), registry.at(alpha.motherId), false, false);
    }

    // Connectors from Parents to Alpha
    sf::Vertex lineA[] = {
        sf::Vertex(sf::Vector2f(centerX - 50.f, yGen1 + 50.f), sf::Color(100, 75, 45)),
        sf::Vertex(sf::Vector2f(centerX, yGen1 + 105.f), sf::Color(100, 75, 45))
    };
    target.draw(lineA, 2, sf::Lines);

    sf::Vertex lineB[] = {
        sf::Vertex(sf::Vector2f(centerX + 50.f, yGen1 + 50.f), sf::Color(100, 75, 45)),
        sf::Vertex(sf::Vector2f(centerX, yGen1 + 105.f), sf::Color(100, 75, 45))
    };
    target.draw(lineB, 2, sf::Lines);

    // LEVEL 2: Current Alpha & Siblings
    float yGen2 = treePanel.top + 180.f;
    drawCharacterNode(target, sf::Vector2f(centerX, yGen2), alpha, true, true);

    // Siblings
    float sibOffset = 220.f;
    for (Character::ID mid : dynasty.memberIds) {
        if (mid != alpha.id && registry.count(mid)) {
            const Character& sib = registry.at(mid);
            if (sib.fatherId == alpha.fatherId && sib.fatherId != Character::INVALID_ID) {
                drawCharacterNode(target, sf::Vector2f(centerX + sibOffset, yGen2), sib, false, false);
                sibOffset += 220.f;
            }
        }
    }

    // LEVEL 3: Children / Descendants
    float yGen3 = treePanel.top + 330.f;
    size_t childCount = alpha.childrenIds.size();
    if (childCount > 0) {
        float spacing = 180.f;
        float startChildX = centerX - ((childCount - 1) * spacing) / 2.f;

        for (size_t i = 0; i < childCount; ++i) {
            Character::ID cId = alpha.childrenIds[i];
            if (registry.count(cId)) {
                sf::Vector2f cPos(startChildX + i * spacing, yGen3);

                sf::Vertex branch[] = {
                    sf::Vertex(sf::Vector2f(centerX, yGen2 + 50.f), sf::Color(100, 75, 45)),
                    sf::Vertex(sf::Vector2f(cPos.x, cPos.y), sf::Color(100, 75, 45))
                };
                target.draw(branch, 2, sf::Lines);

                drawCharacterNode(target, cPos, registry.at(cId), false, false);
            }
        }
    }
}

void DynastyUI::drawCharacterNode(
    sf::RenderTarget& target,
    sf::Vector2f pos,
    const Character& character,
    bool isAlpha,
    bool isCurrentInspected
) {
    float nW = 160.f;
    float nH = 54.f;
    sf::FloatRect nodeRect(pos.x - nW / 2.f, pos.y, nW, nH);

    sf::RectangleShape card(sf::Vector2f(nW, nH));
    card.setPosition(nodeRect.left, nodeRect.top);
    card.setFillColor(character.isAlive ? (isAlpha ? sf::Color(210, 180, 125) : sf::Color(195, 180, 150)) : sf::Color(160, 145, 125));
    card.setOutlineColor(isAlpha ? sf::Color(180, 130, 40) : sf::Color(120, 95, 65));
    card.setOutlineThickness(isAlpha ? 2.5f : 1.5f);
    target.draw(card);

    // Crest / Alive indicator
    sf::CircleShape dot(5.f);
    dot.setPosition(nodeRect.left + 8.f, nodeRect.top + 8.f);
    dot.setFillColor(character.isAlive ? sf::Color(40, 160, 40) : sf::Color(160, 40, 40));
    target.draw(dot);

    sf::Text nameTxt(character.name, font, 12);
    nameTxt.setFillColor(sf::Color(30, 15, 5));
    nameTxt.setStyle(sf::Text::Bold);
    nameTxt.setPosition(nodeRect.left + 24.f, nodeRect.top + 4.f);
    target.draw(nameTxt);

    std::string role = isAlpha ? "Alpha Ruler" : (character.isAlive ? "Age " + std::to_string(character.age) : "Deceased");
    sf::Text roleTxt(role, font, 10);
    roleTxt.setFillColor(sf::Color(70, 50, 30));
    roleTxt.setPosition(nodeRect.left + 8.f, nodeRect.top + 24.f);
    target.draw(roleTxt);

    std::string prStr = "Prowess: " + std::to_string(character.getEffectiveStats().prowess);
    sf::Text prTxt(prStr, font, 10);
    prTxt.setFillColor(sf::Color(90, 40, 30));
    prTxt.setPosition(nodeRect.left + 8.f, nodeRect.top + 38.f);
    target.draw(prTxt);
}

void DynastyUI::drawSuccessionView(
    sf::RenderTarget& target,
    sf::FloatRect bounds,
    const Dynasty& dynasty,
    const Clan& clan,
    const std::unordered_map<Character::ID, Character>& registry,
    const std::vector<Faction>& factions
) {
    float startY = bounds.top + 68.f;
    float availH = bounds.height - 84.f;

    // LEFT SECTION: Ranked Candidates
    float leftW = (bounds.width - 46.f) * 0.58f;
    sf::FloatRect leftBox(bounds.left + 16.f, startY, leftW, availH);

    std::string lawName;
    switch (clan.successionLaw) {
        case SuccessionLaw::BLOODLINE_PRIMOGENITURE: lawName = "BLOODLINE PRIMOGENITURE"; break;
        case SuccessionLaw::ELDER_SENIORITY: lawName = "ELDER SENIORITY"; break;
        case SuccessionLaw::RIGHT_OF_THE_STRONGEST: lawName = "RIGHT OF THE STRONGEST"; break;
    }
    drawSubPanel(target, leftBox, "SUCCESSION ORDER - " + lawName);

    auto candidates = SuccessionSystem::evaluateSuccession(dynasty, registry, factions, clan.successionLaw);

    float cY = leftBox.top + 34.f;
    int rank = 1;
    for (const auto& c : candidates) {
        if (!registry.count(c.characterId)) continue;
        const Character& ape = registry.at(c.characterId);

        sf::FloatRect cCard(leftBox.left + 8.f, cY, leftBox.width - 16.f, 56.f);
        sf::RectangleShape cShape(sf::Vector2f(cCard.width, cCard.height));
        cShape.setPosition(cCard.left, cCard.top);
        cShape.setFillColor(rank == 1 ? sf::Color(215, 195, 150) : sf::Color(195, 178, 142));
        cShape.setOutlineColor(rank == 1 ? sf::Color(190, 140, 45) : sf::Color(135, 110, 75));
        cShape.setOutlineThickness(rank == 1 ? 2.f : 1.f);
        target.draw(cShape);

        // Rank Badge
        sf::CircleShape badge(12.f);
        badge.setPosition(cCard.left + 8.f, cCard.top + (cCard.height - 24.f) / 2.f);
        badge.setFillColor(rank == 1 ? sf::Color(180, 130, 40) : sf::Color(80, 60, 40));
        target.draw(badge);

        sf::Text rTxt(std::to_string(rank), font, 11);
        rTxt.setFillColor(sf::Color::White);
        rTxt.setStyle(sf::Text::Bold);
        sf::FloatRect rtb = rTxt.getLocalBounds();
        rTxt.setOrigin(rtb.left + rtb.width / 2.f, rtb.top + rtb.height / 2.f);
        rTxt.setPosition(badge.getPosition().x + 12.f, badge.getPosition().y + 12.f);
        target.draw(rTxt);

        // Candidate Info
        sf::Text nTxt(ape.name + " (Age " + std::to_string(ape.age) + ")", font, 13);
        nTxt.setFillColor(sf::Color(30, 15, 5));
        nTxt.setStyle(sf::Text::Bold);
        nTxt.setPosition(cCard.left + 40.f, cCard.top + 6.f);
        target.draw(nTxt);

        sf::Text rnl(c.rationale, font, 10);
        rnl.setFillColor(sf::Color(75, 55, 35));
        rnl.setPosition(cCard.left + 40.f, cCard.top + 24.f);
        target.draw(rnl);

        // Score
        sf::Text scText("Score: " + std::to_string(static_cast<int>(c.score)), font, 13);
        scText.setFillColor(sf::Color(120, 40, 20));
        scText.setStyle(sf::Text::Bold);
        sf::FloatRect scb = scText.getLocalBounds();
        scText.setPosition(cCard.left + cCard.width - scb.width - 12.f, cCard.top + 16.f);
        target.draw(scText);

        cY += 62.f;
        rank++;
    }

    // RIGHT SECTION: Tribal Factions
    float rightX = leftBox.left + leftW + 14.f;
    float rightW = bounds.left + bounds.width - rightX - 16.f;
    sf::FloatRect rightBox(rightX, startY, rightW, availH);
    drawSubPanel(target, rightBox, "DISSIDENT CLAN FACTIONS");

    float fY = rightBox.top + 34.f;
    if (factions.empty()) {
        sf::Text calm("The clan is united. No active dissident factions.", font, 12);
        calm.setFillColor(sf::Color(90, 70, 50));
        calm.setStyle(sf::Text::Italic);
        calm.setPosition(rightBox.left + 12.f, fY);
        target.draw(calm);
    } else {
        for (const auto& f : factions) {
            sf::FloatRect fCard(rightBox.left + 8.f, fY, rightBox.width - 16.f, 85.f);
            sf::RectangleShape fShape(sf::Vector2f(fCard.width, fCard.height));
            fShape.setPosition(fCard.left, fCard.top);
            fShape.setFillColor(sf::Color(190, 170, 135));
            fShape.setOutlineColor(sf::Color(140, 50, 40));
            fShape.setOutlineThickness(1.5f);
            target.draw(fShape);

            sf::Text fTitle(f.name, font, 12);
            fTitle.setFillColor(sf::Color(130, 25, 15));
            fTitle.setStyle(sf::Text::Bold);
            fTitle.setPosition(fCard.left + 8.f, fCard.top + 6.f);
            target.draw(fTitle);

            std::string leadName = registry.count(f.leaderId) ? registry.at(f.leaderId).name : "Unknown";
            sf::Text fLeader("Leader: " + leadName + " | Members: " + std::to_string(f.memberIds.size()), font, 11);
            fLeader.setFillColor(sf::Color(50, 35, 20));
            fLeader.setPosition(fCard.left + 8.f, fCard.top + 24.f);
            target.draw(fLeader);

            sf::Text fPow("Military Threat Power: " + std::to_string(static_cast<int>(f.powerRating)), font, 11);
            fPow.setFillColor(sf::Color(140, 30, 20));
            fPow.setStyle(sf::Text::Bold);
            fPow.setPosition(fCard.left + 8.f, fCard.top + 42.f);
            target.draw(fPow);

            // Power Meter Bar
            sf::RectangleShape barBg(sf::Vector2f(fCard.width - 16.f, 8.f));
            barBg.setPosition(fCard.left + 8.f, fCard.top + 64.f);
            barBg.setFillColor(sf::Color(80, 60, 40));
            target.draw(barBg);

            float fillRatio = std::clamp(f.powerRating / 200.f, 0.05f, 1.0f);
            sf::RectangleShape barFill(sf::Vector2f((fCard.width - 16.f) * fillRatio, 8.f));
            barFill.setPosition(fCard.left + 8.f, fCard.top + 64.f);
            barFill.setFillColor(sf::Color(190, 45, 30));
            target.draw(barFill);

            fY += 95.f;
        }
    }
}

void DynastyUI::drawTooltip(sf::RenderTarget& target) {
    sf::Text tTitle(activeTooltip.title, font, 12);
    tTitle.setFillColor(sf::Color(245, 220, 130));
    tTitle.setStyle(sf::Text::Bold);

    sf::Text tDesc(activeTooltip.description, font, 11);
    tDesc.setFillColor(sf::Color(220, 210, 190));

    sf::FloatRect tb = tTitle.getLocalBounds();
    sf::FloatRect db = tDesc.getLocalBounds();
    float boxW = std::max(tb.width, db.width) + 16.f;
    float boxH = tb.height + db.height + 20.f;

    sf::Vector2f pos = activeTooltip.position + sf::Vector2f(12.f, 12.f);
    sf::RectangleShape bg(sf::Vector2f(boxW, boxH));
    bg.setPosition(pos);
    bg.setFillColor(sf::Color(28, 18, 12, 245));
    bg.setOutlineColor(sf::Color(160, 125, 65));
    bg.setOutlineThickness(1.5f);
    target.draw(bg);

    tTitle.setPosition(pos.x + 8.f, pos.y + 6.f);
    tDesc.setPosition(pos.x + 8.f, pos.y + tb.height + 12.f);
    target.draw(tTitle);
    target.draw(tDesc);
}

}