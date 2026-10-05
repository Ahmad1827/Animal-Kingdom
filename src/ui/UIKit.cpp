#include "ui/UIKit.h"
#include <algorithm>
#include <cmath>

namespace ui {

// ---------------------------------------------------------------------------
// Icon art. 11x11 each, one character per pixel. Edit the maps to redraw an
// icon; '.' is transparent and every other character is looked up in PALETTE.
// ---------------------------------------------------------------------------
namespace {

const int ICON_PX = 11;

struct PaletteEntry { char key; sf::Color color; };
const PaletteEntry PALETTE[] = {
    {'o', { 20,  12,   6}},   // outline
    {'W', {255, 255, 255}},   // white highlight
    {'Y', {255, 222,  96}},   // light gold
    {'A', {245, 150,  36}},   // amber / orange
    {'D', {176,  84,  16}},   // dark amber
    {'G', {214, 164,  52}},   // gold
    {'g', {150, 104,  30}},   // dark gold
    {'R', {214,  62,  52}},   // red jewel
    {'B', { 88, 170, 240}},   // blue jewel
    {'P', {136,  92, 214}},   // purple
    {'L', {200, 168, 255}},   // light purple
    {'S', {196, 178, 142}},   // stone
    {'s', {140, 122,  94}},   // stone shade
    {'I', {220, 224, 232}},   // steel
    {'i', {150, 158, 172}},   // steel shade
    {'M', {232, 236, 255}},   // moonlight
    {'m', {168, 178, 220}},   // moon shade
};

const char* const ICONS[static_cast<int>(Icon::Count)][ICON_PX] = {
    { // Amber
        ".....o.....",
        "....oYo....",
        "...oYYAo...",
        "..oYWYAAo..",
        ".oYWYAAADo.",
        "oYYYAAAADDo",
        ".oAAAAADDo.",
        "..oAAADDo..",
        "...oADDo...",
        "....oDo....",
        ".....o.....",
    },
    { // Crown
        "...........",
        ".o...o...o.",
        "oYo.oYo.oYo",
        "oYo.oYo.oYo",
        "oYYoYYYoYYo",
        "oYGYYGYYGYo",
        "oGGGGGGGGGo",
        "oGRGGBGGRGo",
        "ogggggggggo",
        ".ooooooooo.",
        "...........",
    },
    { // Piety (spirit flame)
        ".....o.....",
        "....oLo....",
        "....oLPo...",
        "...oLLPo...",
        "..oPLLPPo..",
        "..oPLWLPo..",
        ".oPLWWLPPo.",
        ".oPLWWLLPo.",
        ".oPPLLLPPo.",
        "..oPPPPPo..",
        "...ooooo...",
    },
    { // Tower (domain)
        "ooo.ooo.ooo",
        "oSo.oSo.oSo",
        "oSoooSoooSo",
        "oSSSSSSSSSo",
        ".osSSSSSso.",
        ".oSSSoSSso.",
        ".oSSoooSso.",
        ".oSSoooSso.",
        ".oSSoooSso.",
        "oSSSoooSsso",
        "ooooooooooo",
    },
    { // Crossed swords (levies)
        "II.......II",
        "IiI.....IiI",
        ".IiI...IiI.",
        "..IiI.IiI..",
        "...IiIiI...",
        "....IiI....",
        "...IiIiI...",
        "G.IiI.IiI.G",
        ".GiI...IiG.",
        ".gGg...gGg.",
        "g..G...G..g",
    },
    { // Sun
        ".....Y.....",
        ".Y...Y...Y.",
        "..Y.....Y..",
        "....YYY....",
        "...YWYYG...",
        "YY.YYYYG.YY",
        "...YYYGG...",
        "....GGG....",
        "..Y.....Y..",
        ".Y...Y...Y.",
        ".....Y.....",
    },
    { // Moon
        "....MMM....",
        "..MMMm.....",
        ".MMMm......",
        ".MMM.......",
        "MMMm.......",
        "MMMm.......",
        "MMMm......m",
        ".MMMm....mM",
        ".MMMMmmmMM.",
        "..MMMMMMM..",
        "....mmm....",
    },
};

const sf::Texture& iconAtlas() {
    // Heap-allocated on purpose and never freed: a static sf::Texture would be
    // destroyed after the OpenGL context is gone at exit.
    static sf::Texture* atlas = nullptr;
    if (atlas) return *atlas;

    const int count = static_cast<int>(Icon::Count);
    sf::Image img;
    img.create(ICON_PX * count, ICON_PX, sf::Color::Transparent);
    for (int i = 0; i < count; ++i) {
        for (int y = 0; y < ICON_PX; ++y) {
            for (int x = 0; x < ICON_PX; ++x) {
                const char ch = ICONS[i][y][x];
                if (ch == '.') continue;
                for (const auto& p : PALETTE) {
                    if (p.key == ch) { img.setPixel(i * ICON_PX + x, y, p.color); break; }
                }
            }
        }
    }
    atlas = new sf::Texture();
    atlas->loadFromImage(img);
    atlas->setSmooth(false);
    return *atlas;
}

// Near-white speckle that surfaces are multiplied by. Tiles seamlessly.
const int GRAIN_PX = 64;
const sf::Texture& grainTexture() {
    static sf::Texture* grain = nullptr;    // never freed, same reason as the icon atlas
    if (grain) return *grain;

    sf::Image img;
    img.create(GRAIN_PX, GRAIN_PX, sf::Color::White);
    unsigned seed = 0x9E3779B9u;
    auto next = [&seed]() { seed ^= seed << 13; seed ^= seed >> 17; seed ^= seed << 5; return seed; };
    for (int y = 0; y < GRAIN_PX; ++y) {
        for (int x = 0; x < GRAIN_PX; ++x) {
            const unsigned r = next();
            int v = 232 + static_cast<int>(r % 24);             // fine grain
            if ((r >> 8) % 23 == 0) v -= 26;                    // the odd darker fleck
            const sf::Uint8 g = static_cast<sf::Uint8>(std::clamp(v, 0, 255));
            img.setPixel(x, y, sf::Color(g, g, g));
        }
    }
    grain = new sf::Texture();
    grain->loadFromImage(img);
    grain->setSmooth(false);
    grain->setRepeated(true);
    return *grain;
}

sf::Color scaled(sf::Color c, float k) {
    auto ch = [k](sf::Uint8 v) { return static_cast<sf::Uint8>(std::clamp(static_cast<float>(v) * k, 0.f, 255.f)); };
    return sf::Color(ch(c.r), ch(c.g), ch(c.b), c.a);
}

} // namespace

// ---------------------------------------------------------------------------

Canvas::Canvas(sf::RenderTarget& t, const sf::Font& f) : target(t), font(f) {
    const sf::View& v = t.getView();
    const sf::IntRect vp = t.getViewport(v);
    size = v.getSize();
    origin = v.getCenter() - size / 2.f;
    if (size.x > 0.f && vp.width > 0) s = static_cast<float>(vp.width) / size.x;
    unitPx = std::max(1, static_cast<int>(std::lround(s)));
}

sf::Vector2f Canvas::mouse(const sf::RenderWindow& window) {
    return window.mapPixelToCoords(sf::Mouse::getPosition(window));
}

float Canvas::snapX(float x) const { return std::round((x - origin.x) * s) / s + origin.x; }
float Canvas::snapY(float y) const { return std::round((y - origin.y) * s) / s + origin.y; }

sf::FloatRect Canvas::snap(sf::FloatRect r) const {
    const float l = snapX(r.left), t = snapY(r.top);
    const float rr = snapX(r.left + r.width), b = snapY(r.top + r.height);
    return sf::FloatRect(l, t, rr - l, b - t);
}

unsigned Canvas::pixelSize(unsigned designSize) const {
    return static_cast<unsigned>(std::max(6L, std::lround(designSize * s)));
}

void Canvas::fill(sf::FloatRect r, sf::Color color) {
    r = snap(r);
    if (r.width <= 0.f || r.height <= 0.f) return;
    sf::RectangleShape shape(sf::Vector2f(r.width, r.height));
    shape.setPosition(r.left, r.top);
    shape.setFillColor(color);
    target.draw(shape);
}

void Canvas::frame(sf::FloatRect r, sf::Color color, int steps) {
    r = snap(r);
    const float t = px(steps);
    fill({r.left, r.top, r.width, t}, color);
    fill({r.left, r.top + r.height - t, r.width, t}, color);
    fill({r.left, r.top + t, t, r.height - 2.f * t}, color);
    fill({r.left + r.width - t, r.top + t, t, r.height - 2.f * t}, color);
}

void Canvas::gradient(sf::FloatRect r, sf::Color from, sf::Color to, bool horizontal) {
    r = snap(r);
    if (r.width <= 0.f || r.height <= 0.f) return;
    const sf::Color tr = horizontal ? to : from, bl = horizontal ? from : to;
    const sf::Vertex quad[4] = {
        sf::Vertex({r.left, r.top}, from),
        sf::Vertex({r.left + r.width, r.top}, tr),
        sf::Vertex({r.left + r.width, r.top + r.height}, to),
        sf::Vertex({r.left, r.top + r.height}, bl),
    };
    target.draw(quad, 4, sf::Quads);
}

void Canvas::surface(sf::FloatRect r, sf::Color top, sf::Color bottom) {
    r = snap(r);
    if (r.width <= 0.f || r.height <= 0.f) return;
    // One grain texel per border step, anchored to the view so neighbouring boxes share the pattern.
    const float k = s / static_cast<float>(unitPx);
    const float u0 = (r.left - origin.x) * k, v0 = (r.top - origin.y) * k;
    const float u1 = u0 + r.width * k, v1 = v0 + r.height * k;
    const sf::Vertex quad[4] = {
        sf::Vertex({r.left, r.top}, top, {u0, v0}),
        sf::Vertex({r.left + r.width, r.top}, top, {u1, v0}),
        sf::Vertex({r.left + r.width, r.top + r.height}, bottom, {u1, v1}),
        sf::Vertex({r.left, r.top + r.height}, bottom, {u0, v1}),
    };
    target.draw(quad, 4, sf::Quads, sf::RenderStates(&grainTexture()));
}

void Canvas::panel(sf::FloatRect r, bool shadow) {
    r = snap(r);
    const float u = px(1);
    // Everything here is drawn as edge strips: a big panel is covered once, by its surface.
    if (shadow) {
        const float right = r.left + r.width, bottom = r.top + r.height;
        fill({right, r.top + 2.f * u, u, r.height}, sf::Color(0, 0, 0, 150));
        fill({r.left + u, bottom, r.width - u, 2.f * u}, sf::Color(0, 0, 0, 150));
        fill({right + u, r.top + 4.f * u, 2.f * u, r.height}, sf::Color(0, 0, 0, 55));
        fill({r.left + 3.f * u, bottom + 2.f * u, r.width - 2.f * u, 2.f * u}, sf::Color(0, 0, 0, 55));
    }

    // dark rim, then a bronze band lit from above
    frame(r, theme::EdgeDark, 1);
    const sf::FloatRect band(r.left + u, r.top + u, r.width - 2.f * u, r.height - 2.f * u);
    const sf::Color bandLow = scaled(theme::Bronze, 0.62f);
    fill({band.left, band.top, band.width, u}, theme::BronzeLight);
    fill({band.left, band.top + band.height - u, band.width, u}, bandLow);
    gradient({band.left, band.top + u, u, band.height - 2.f * u}, theme::BronzeLight, bandLow);
    gradient({band.left + band.width - u, band.top + u, u, band.height - 2.f * u}, theme::BronzeLight, bandLow);

    const sf::FloatRect in(band.left + u, band.top + u, band.width - 2.f * u, band.height - 2.f * u);
    surface(in, theme::PanelTop, theme::PanelBottom);
    // the surface sits a step below the band: shadow under the top edge, a glint above the bottom one
    fill({in.left, in.top, in.width, u}, sf::Color(0, 0, 0, 110));
    fill({in.left, in.top + u, in.width, u}, sf::Color(255, 240, 200, 20));
    fill({in.left, in.top + in.height - u, in.width, u}, sf::Color(0, 0, 0, 90));

    // gold corner caps; big panels also get bracket arms along the band
    const float arm = (std::min(r.width, r.height) >= 120.f) ? 7.f * u : 0.f;
    const float cap = 3.f * u;
    for (int i = 0; i < 4; ++i) {
        const bool right = (i % 2 == 1), low = (i >= 2);
        const float cx = right ? r.left + r.width - cap : r.left;
        const float cy = low ? r.top + r.height - cap : r.top;
        if (arm > 0.f) {
            fill({right ? cx - arm : cx + cap, low ? band.top + band.height - u : band.top, arm, u}, theme::GoldBright);
            fill({right ? band.left + band.width - u : band.left, low ? cy - arm : cy + cap, u, arm}, theme::GoldBright);
        }
        fill({cx, cy, cap, cap}, theme::EdgeDark);
        fill({cx + u, cy + u, u, u}, theme::GoldBright);
    }
}

void Canvas::inset(sf::FloatRect r) {
    r = snap(r);
    const float u = px(1);
    // lit lip below, as if cut into the panel
    fill({r.left + u, r.top + r.height, r.width - u, u}, sf::Color(255, 240, 200, 16));
    surface(r, theme::InsetTop, theme::InsetBottom);
    frame(r, theme::BronzeDim, 1);
    fill({r.left + u, r.top + u, r.width - 2.f * u, u}, sf::Color(0, 0, 0, 130));
    fill({r.left + u, r.top + 2.f * u, r.width - 2.f * u, u}, sf::Color(0, 0, 0, 55));
    fill({r.left + u, r.top + 2.f * u, u, r.height - 3.f * u}, sf::Color(0, 0, 0, 70));
}

void Canvas::button(sf::FloatRect r, bool active, bool hovered) {
    r = snap(r);
    const float u = px(1);
    const sf::FloatRect in(r.left + u, r.top + u, r.width - 2.f * u, r.height - 2.f * u);
    if (active)       surface(in, theme::ActiveTop, theme::ActiveBottom);
    else if (hovered) surface(in, theme::HoverTop, theme::HoverBottom);
    else              surface(in, theme::ButtonTop, theme::ButtonBottom);
    frame(r, active ? theme::Gold : (hovered ? theme::Bronze : theme::BronzeDim), 1);
    // bevel: light along the top, shade along the bottom
    fill({in.left, in.top, in.width, u}, sf::Color(255, 244, 210, active ? 95 : (hovered ? 50 : 28)));
    fill({in.left, in.top + in.height - u, in.width, u}, sf::Color(0, 0, 0, active ? 70 : 95));
    if (active) {
        fill({r.left, r.top, u, u}, theme::GoldBright);
        fill({r.left + r.width - u, r.top, u, u}, theme::GoldBright);
    }
}

float Canvas::heading(float x, float y, float w, const std::string& title, const std::string& aside) {
    const float u = px(1);
    const float d = 2.f * u;
    fill({x, y + 6.f - d, d * 2.f, d * 2.f}, theme::EdgeDark);
    fill({x + u, y + 6.f - d + u, d, d}, theme::Gold);
    text(title, x + d * 2.f + 6.f, y + 6.f, {10, theme::Gold, true});
    if (!aside.empty()) text(aside, x + w, y + 6.f, {10, theme::TextMuted, false, Align::Right});
    gradient({x, y + 15.f, w, u}, theme::Bronze, sf::Color(theme::Bronze.r, theme::Bronze.g, theme::Bronze.b, 0), true);
    return y + 24.f;
}

void Canvas::divider(float x, float y, float h) {
    const float u = px(1);
    fill({x, y, u, h}, theme::EdgeDark);
    fill({x + u, y, u, h}, sf::Color(255, 226, 160, 30));
}

void Canvas::tab(sf::FloatRect r, const std::string& key, const std::string& label, bool active, bool hovered, bool alert) {
    button(r, active, hovered);
    const float u = px(1);
    const float mid = r.top + r.height * 0.5f;
    float labelLeft = r.left;
    if (!key.empty()) {
        const float keyW = 24.f;
        // the hotkey sits in a darker well so it reads as a key cap, not part of the label
        fill({r.left + u, r.top + u, keyW - u, r.height - 2.f * u}, sf::Color(0, 0, 0, active ? 60 : 85));
        text(key, r.left + keyW * 0.5f + 1.f, mid, {11, active ? theme::GoldBright : theme::BronzeLight, true, Align::Center});
        divider(r.left + keyW, r.top + u, r.height - 2.f * u);
        labelLeft += keyW;
    }
    const sf::Color labelColor = active ? sf::Color(255, 248, 224) : (alert ? theme::Bad : (hovered ? theme::GoldBright : theme::Text));
    text(label, labelLeft + (r.left + r.width - labelLeft) * 0.5f, mid, {11, labelColor, active, Align::Center});
    if (active) fill({r.left + u, r.top + r.height - 3.f * u, r.width - 2.f * u, 2.f * u}, theme::GoldBright);
    if (alert) {
        fill({r.left + r.width - 10.f, r.top + 3.f, 7.f, 7.f}, theme::EdgeDark);
        fill({r.left + r.width - 9.f, r.top + 4.f, 5.f, 5.f}, theme::Bad);
    }
}

void Canvas::meter(sf::FloatRect r, float frac, sf::Color color) {
    r = snap(r);
    const float u = px(1);
    fill({r.left, r.top + r.height, r.width, u}, sf::Color(255, 240, 200, 18));
    fill(r, theme::EdgeDark);
    const sf::FloatRect in(r.left + u, r.top + u, r.width - 2.f * u, r.height - 2.f * u);
    gradient(in, theme::InsetTop, theme::InsetBottom);
    const float w = std::round(in.width * std::clamp(frac, 0.f, 1.f) * s) / s;
    if (w > 0.f) {
        gradient({in.left, in.top, w, in.height}, scaled(color, 1.18f), scaled(color, 0.62f));
        fill({in.left, in.top, w, u}, sf::Color(255, 255, 255, 80));
        if (w > 2.f * u) fill({in.left + w - u, in.top, u, in.height}, scaled(color, 1.35f));
    }
}

void Canvas::balance(sf::FloatRect r, float frac) {
    r = snap(r);
    const float u = px(1);
    fill({r.left, r.top + r.height, r.width, u}, sf::Color(255, 240, 200, 18));
    fill(r, theme::EdgeDark);
    const sf::FloatRect in(r.left + u, r.top + u, r.width - 2.f * u, r.height - 2.f * u);
    gradient(in, theme::InsetTop, theme::InsetBottom);
    const float mid = snapX(r.left + r.width * 0.5f);
    const float w = std::round(in.width * 0.5f * std::min(std::abs(frac), 1.f) * s) / s;
    if (w > 0.f) {
        const sf::Color col = frac >= 0.f ? theme::Good : theme::Bad;
        gradient({frac >= 0.f ? mid : mid - w, in.top, w, in.height}, scaled(col, 1.18f), scaled(col, 0.62f));
        fill({frac >= 0.f ? mid : mid - w, in.top, w, u}, sf::Color(255, 255, 255, 80));
    }
    fill({mid - u, r.top - u, 2.f * u, r.height + 2.f * u}, theme::EdgeDark);
    fill({mid - u, r.top - u, u, r.height + 2.f * u}, theme::BronzeLight);
}

std::string Canvas::fit(const std::string& str, float maxW, unsigned sizeUnits, bool bold) const {
    if (textWidth(str, sizeUnits, bold) <= maxW) return str;
    std::string cut = str;
    while (!cut.empty() && textWidth(cut + "...", sizeUnits, bold) > maxW) cut.pop_back();
    return cut + "...";
}

float Canvas::paragraph(const std::string& str, float x, float y, float maxW, const TextStyle& st, float lineH) {
    if (lineH <= 0.f) lineH = static_cast<float>(st.size) + 4.f;
    std::string line, word;
    auto flush = [&]() {
        if (line.empty()) return;
        text(line, x, y, st);
        y += lineH;
        line.clear();
    };
    for (size_t i = 0; i <= str.size(); ++i) {
        const bool end = (i == str.size());
        if (!end && str[i] != ' ' && str[i] != '\n') { word += str[i]; continue; }
        if (!word.empty()) {
            const std::string trial = line.empty() ? word : line + " " + word;
            if (!line.empty() && textWidth(trial, st.size, st.bold) > maxW) { flush(); line = word; }
            else line = trial;
            word.clear();
        }
        if (!end && str[i] == '\n') flush();
    }
    flush();
    return y;
}

float Canvas::textWidth(const std::string& str, unsigned sizeUnits, bool bold) const {
    sf::Text t(str, font, pixelSize(sizeUnits));
    if (bold) t.setStyle(sf::Text::Bold);
    const sf::FloatRect b = t.getLocalBounds();
    return (b.left + b.width) / s;
}

float Canvas::text(const std::string& str, float x, float y, const TextStyle& st) {
    const unsigned cs = pixelSize(st.size);
    sf::Text t(str, font, cs);
    sf::Uint32 style = sf::Text::Regular;
    if (st.bold)   style |= sf::Text::Bold;
    if (st.italic) style |= sf::Text::Italic;
    t.setStyle(style);
    t.setFillColor(st.color);
    if (st.outline) {
        t.setOutlineColor(theme::EdgeDark);
        t.setOutlineThickness(static_cast<float>(unitPx));
    }

    // Centre on the capital-letter height so the row does not jump when the
    // string changes (digits, descenders) and different sizes share a midline.
    const sf::Glyph& cap = font.getGlyph(L'H', cs, st.bold);
    const float capMid = static_cast<float>(cs) + cap.bounds.top + cap.bounds.height * 0.5f;

    const sf::FloatRect b = t.getLocalBounds();
    const float w = b.left + b.width;
    float ox = 0.f;
    if (st.align == Align::Center) ox = w * 0.5f;
    else if (st.align == Align::Right) ox = w;

    // Whole-pixel origin + whole-pixel position + 1/s scale = glyphs land 1:1 on screen.
    t.setOrigin(std::round(ox), std::round(capMid));
    t.setScale(1.f / s, 1.f / s);
    t.setPosition(snapX(x), snapY(y));
    target.draw(t);
    return w / s;
}

float Canvas::icon(Icon id, float cx, float cy, sf::Color tint) {
    // Each art pixel becomes a whole number of device pixels.
    const int k = std::max(1, static_cast<int>(std::lround(s * 1.2f)));
    const float drawn = static_cast<float>(ICON_PX * k) / s;

    sf::Sprite sp(iconAtlas(), sf::IntRect(static_cast<int>(id) * ICON_PX, 0, ICON_PX, ICON_PX));
    sp.setScale(static_cast<float>(k) / s, static_cast<float>(k) / s);
    sp.setPosition(snapX(cx - drawn * 0.5f), snapY(cy - drawn * 0.5f));
    sp.setColor(tint);
    target.draw(sp);
    return drawn;
}

void Canvas::tooltip(float anchorX, float topY, const Tooltip& tip) {
    const unsigned titleSize = 12, rowSize = 11, noteSize = 10;
    const float padX = 10.f, padY = 8.f, rowH = 16.f, gap = 18.f;

    float w = textWidth(tip.title, titleSize, true);
    for (const auto& r : tip.rows) {
        w = std::max(w, textWidth(r.label, rowSize) + gap + textWidth(r.value, rowSize, true));
    }
    if (!tip.note.empty()) w = std::max(w, textWidth(tip.note, noteSize));
    w = std::max(w + padX * 2.f, 190.f);

    float h = padY + 16.f;
    if (!tip.rows.empty()) h += 6.f + rowH * static_cast<float>(tip.rows.size());
    for (const auto& r : tip.rows) if (r.total) h += 5.f;
    if (!tip.note.empty()) h += 6.f + 13.f;
    h += padY - 2.f;

    const float x = std::clamp(anchorX - w * 0.5f, left() + 6.f, std::max(left() + 6.f, right() - 6.f - w));
    const float y = topY;

    panel({x, y, w, h});
    fill({x + px(2), y + px(2), w - px(4), px(2)}, tip.accent);

    float cy = y + padY + 8.f;
    TextStyle ts; ts.size = titleSize; ts.bold = true; ts.color = tip.accent;
    text(tip.title, x + padX, cy, ts);
    cy += 8.f;

    if (!tip.rows.empty()) {
        cy += 6.f;
        fill({x + padX, cy - 3.f, w - padX * 2.f, px(1)}, theme::BronzeDim);
        for (const auto& r : tip.rows) {
            if (r.total) {
                fill({x + padX, cy + 2.f, w - padX * 2.f, px(1)}, theme::BronzeDim);
                cy += 5.f;
            }
            cy += rowH * 0.5f;
            TextStyle ls; ls.size = rowSize; ls.color = r.total ? theme::Text : theme::TextMuted;
            text(r.label, x + padX, cy, ls);
            TextStyle vs; vs.size = rowSize; vs.bold = true; vs.color = r.valueColor; vs.align = Align::Right;
            text(r.value, x + w - padX, cy, vs);
            cy += rowH * 0.5f;
        }
    }
    if (!tip.note.empty()) {
        cy += 6.f + 6.f;
        TextStyle ns; ns.size = noteSize; ns.italic = true; ns.color = theme::TextMuted;
        text(tip.note, x + padX, cy, ns);
    }
}

} // namespace ui