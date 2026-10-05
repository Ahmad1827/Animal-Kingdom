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

void Canvas::panel(sf::FloatRect r, bool shadow) {
    r = snap(r);
    const float u = px(1);
    if (shadow) fill({r.left + u, r.top + 2.f * u, r.width, r.height}, theme::Shadow);
    fill(r, theme::EdgeDark);
    sf::FloatRect in(r.left + u, r.top + u, r.width - 2.f * u, r.height - 2.f * u);
    fill(in, theme::PanelFill);
    frame(in, theme::Bronze, 1);
    // soft top light and bottom shade inside the bronze line
    fill({in.left + u, in.top + u, in.width - 2.f * u, u}, sf::Color(255, 226, 160, 26));
    fill({in.left + u, in.top + in.height - 2.f * u, in.width - 2.f * u, u}, sf::Color(0, 0, 0, 70));
    // gold corner studs
    const sf::Color stud = theme::Gold;
    fill({in.left, in.top, u, u}, stud);
    fill({in.left + in.width - u, in.top, u, u}, stud);
    fill({in.left, in.top + in.height - u, u, u}, stud);
    fill({in.left + in.width - u, in.top + in.height - u, u, u}, stud);
}

void Canvas::inset(sf::FloatRect r) {
    r = snap(r);
    fill(r, theme::PanelInset);
    frame(r, theme::BronzeDim, 1);
}

void Canvas::button(sf::FloatRect r, bool active, bool hovered) {
    r = snap(r);
    const float u = px(1);
    fill(r, active ? theme::ActiveFill : (hovered ? theme::HoverFill : theme::PanelInset));
    frame(r, active ? theme::Gold : (hovered ? theme::Bronze : theme::BronzeDim), 1);
    if (active) fill({r.left + u, r.top + u, r.width - 2.f * u, u}, sf::Color(255, 236, 170, 60));
}

void Canvas::divider(float x, float y, float h) {
    const float u = px(1);
    fill({x, y, u, h}, theme::EdgeDark);
    fill({x + u, y, u, h}, sf::Color(255, 226, 160, 22));
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