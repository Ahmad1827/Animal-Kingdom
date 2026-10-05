#pragma once
// ---------------------------------------------------------------------------
// UIKit - shared drawing helpers for every screen-space UI class.
//
// Why this exists: the UI is laid out in a 1280x720 "design" space and the
// letterbox view stretches it to the real window (1.5x at 1080p). Drawing
// sf::Text at size 11 and letting the view stretch it is what makes the text
// soft. Canvas rasterises glyphs at the real on-screen pixel size, snaps every
// edge to a whole device pixel and draws icons as nearest-neighbour pixel art,
// so the UI stays sharp at any window size.
//
// Usage (once per draw call, after the UI view is set on the target):
//     ui::Canvas c(window, *font);
//     c.panel({8, 6, 120, 30});
//     c.text("WESSEX", 68, 21, {13, ui::theme::Gold, true, ui::Align::Center});
// All coordinates are in the same design units the UI already uses.
// ---------------------------------------------------------------------------
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>

namespace ui {

namespace theme {
    // Jungle night: deep canopy greens for surfaces, bronze and gold for trim.
    // Every UIKit box takes its colours from here, so this is the one place to retune them.
    inline const sf::Color PanelFill   { 26,  40,  35, 248};   // flat stand-in for a panel surface
    inline const sf::Color PanelTop    { 38,  58,  49, 250};   // panel surfaces run top -> bottom
    inline const sf::Color PanelBottom { 17,  28,  25, 250};
    inline const sf::Color PanelInset  { 10,  17,  16, 255};   // flat stand-in for a recessed box
    inline const sf::Color InsetTop    {  7,  12,  12, 255};
    inline const sf::Color InsetBottom { 15,  25,  23, 255};
    inline const sf::Color ButtonTop   { 40,  60,  51, 255};   // raised cards and idle buttons
    inline const sf::Color ButtonBottom{ 21,  33,  29, 255};
    inline const sf::Color HoverTop    { 60,  88,  72, 255};
    inline const sf::Color HoverBottom { 32,  50,  42, 255};
    inline const sf::Color ActiveTop   {206, 142,  44, 255};   // selected: polished amber
    inline const sf::Color ActiveBottom{122,  74,  18, 255};
    inline const sf::Color EdgeDark    {  5,   9,   8, 255};
    inline const sf::Color BronzeLight {214, 172,  98, 255};
    inline const sf::Color Bronze      {156, 116,  60, 255};
    inline const sf::Color BronzeDim   { 88,  74,  46, 255};
    inline const sf::Color Gold        {246, 208, 112, 255};
    inline const sf::Color GoldBright  {255, 238, 176, 255};
    inline const sf::Color Text        {242, 234, 210, 255};
    inline const sf::Color TextMuted   {158, 168, 148, 255};
    inline const sf::Color Good        {120, 214, 110, 255};
    inline const sf::Color Bad         {240,  96,  82, 255};
    inline const sf::Color Prestige    {120, 196, 245, 255};
    inline const sf::Color Piety       {196, 160, 250, 255};
    inline const sf::Color Amber       {248, 160,  44, 255};
    inline const sf::Color ActiveFill  {150,  98,  26, 255};
    inline const sf::Color HoverFill   { 44,  66,  56, 255};
    inline const sf::Color Shadow      {  0,   0,   0, 150};
}

enum class Icon { Amber, Crown, Piety, Tower, Swords, Sun, Moon, Fruit, Scroll, Horn, Hand, Count };
enum class Align { Left, Center, Right };

struct TextStyle {
    unsigned  size    = 12;                 // design units, like the old sf::Text sizes
    sf::Color color   = theme::Text;
    bool      bold    = false;
    Align     align   = Align::Left;
    bool      outline = false;              // dark 1px rim, for text over the world
    bool      italic  = false;
};

struct TooltipRow {
    std::string label;
    std::string value;
    sf::Color   valueColor = theme::Text;
    bool        total = false;      // sum row: ruled off from the rows above
};

struct Tooltip {
    std::string             title;
    sf::Color               accent = theme::Gold;
    std::vector<TooltipRow> rows;           // label left, value right
    std::string             note;           // muted italic footer, optional
};

class Canvas {
public:
    Canvas(sf::RenderTarget& target, const sf::Font& font);

    // Device pixels per design unit (1.5 at 1080p with a 1280x720 UI view).
    float scale() const { return s; }
    // Design-unit length of n border steps. One step is a whole number of device pixels.
    float px(int n = 1) const { return static_cast<float>(n * unitPx) / s; }

    float left()   const { return origin.x; }
    float top()    const { return origin.y; }
    float right()  const { return origin.x + size.x; }
    float bottom() const { return origin.y + size.y; }

    // Mouse position in design units, using the view currently set on the window.
    static sf::Vector2f mouse(const sf::RenderWindow& window);

    void fill(sf::FloatRect r, sf::Color color);
    // Two-colour blend, top to bottom (or left to right). Colours may be translucent.
    void gradient(sf::FloatRect r, sf::Color from, sf::Color to, bool horizontal = false);
    // Top-to-bottom blend with a fine grain, so large areas read as a material rather than flat colour.
    void surface(sf::FloatRect r, sf::Color top, sf::Color bottom);
    void frame(sf::FloatRect r, sf::Color color, int steps = 1);   // border drawn inside r
    void panel(sf::FloatRect r, bool shadow = true);
    void inset(sf::FloatRect r);                                   // recessed box inside a panel
    void button(sf::FloatRect r, bool active, bool hovered);       // raised; also the look for small cards
    // Section title: gold stud, text, and a rule that fades out to the right. Returns the y below it.
    float heading(float x, float y, float w, const std::string& title, const std::string& aside = "");
    void divider(float x, float y, float h);                       // vertical separator

    // Tab in a strip: hotkey cell on the left (skipped when key is empty), label
    // centred in the rest. alert adds the red pip used for "needs attention".
    void tab(sf::FloatRect r, const std::string& key, const std::string& label,
             bool active, bool hovered, bool alert = false);
    // Horizontal meter filled left to right; frac is clamped to 0..1.
    void meter(sf::FloatRect r, float frac, sf::Color color);
    // Meter that grows left or right from its centre; frac is clamped to -1..1.
    void balance(sf::FloatRect r, float frac);

    // y is the vertical centre of the capital letters, so mixed sizes line up on a row.
    // Returns the drawn width in design units.
    float text(const std::string& str, float x, float y, const TextStyle& st);
    float textWidth(const std::string& str, unsigned size, bool bold = false) const;
    // str cut down with a trailing "..." so it fits maxW.
    std::string fit(const std::string& str, float maxW, unsigned size, bool bold = false) const;
    // Word-wrapped text; y is the cap-centre of the first line. Returns the y of the line after the last.
    float paragraph(const std::string& str, float x, float y, float maxW, const TextStyle& st, float lineH = 0.f);

    // Pixel-art icon centred on (cx, cy). Returns its drawn size in design units.
    float icon(Icon id, float cx, float cy, sf::Color tint = sf::Color::White);

    // Tooltip hanging below (anchorX, topY), kept inside the view. With above set
    // it sits on top of that point instead, for things along the bottom edge.
    void tooltip(float anchorX, float topY, const Tooltip& tip, bool above = false);

private:
    sf::RenderTarget& target;
    const sf::Font&   font;
    sf::Vector2f      origin;   // top-left of the view, design units
    sf::Vector2f      size;     // view size, design units
    float             s = 1.f;
    int               unitPx = 1;

    float snapX(float x) const;
    float snapY(float y) const;
    sf::FloatRect snap(sf::FloatRect r) const;
    unsigned pixelSize(unsigned designSize) const;
};

} // namespace ui