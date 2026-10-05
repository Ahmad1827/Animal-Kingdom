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
    inline const sf::Color PanelFill   { 27,  20,  14, 244};
    inline const sf::Color PanelInset  { 17,  12,   9, 255};
    inline const sf::Color EdgeDark    {  8,   6,   4, 255};
    inline const sf::Color Bronze      {122,  90,  48, 255};
    inline const sf::Color BronzeDim   { 70,  51,  30, 255};
    inline const sf::Color Gold        {242, 208, 122, 255};
    inline const sf::Color GoldBright  {255, 236, 170, 255};
    inline const sf::Color Text        {238, 226, 200, 255};
    inline const sf::Color TextMuted   {168, 150, 122, 255};
    inline const sf::Color Good        {120, 214, 110, 255};
    inline const sf::Color Bad         {240,  96,  82, 255};
    inline const sf::Color Prestige    {120, 196, 245, 255};
    inline const sf::Color Piety       {196, 160, 250, 255};
    inline const sf::Color Amber       {248, 160,  44, 255};
    inline const sf::Color ActiveFill  {120,  80,  28, 255};
    inline const sf::Color HoverFill   { 58,  42,  26, 255};
    inline const sf::Color Shadow      {  0,   0,   0, 150};
}

enum class Icon { Amber, Crown, Piety, Tower, Swords, Sun, Moon, Count };
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
    void frame(sf::FloatRect r, sf::Color color, int steps = 1);   // border drawn inside r
    void panel(sf::FloatRect r, bool shadow = true);
    void inset(sf::FloatRect r);                                   // recessed box inside a panel
    void button(sf::FloatRect r, bool active, bool hovered);
    void divider(float x, float y, float h);                       // vertical separator

    // y is the vertical centre of the capital letters, so mixed sizes line up on a row.
    // Returns the drawn width in design units.
    float text(const std::string& str, float x, float y, const TextStyle& st);
    float textWidth(const std::string& str, unsigned size, bool bold = false) const;

    // Pixel-art icon centred on (cx, cy). Returns its drawn size in design units.
    float icon(Icon id, float cx, float cy, sf::Color tint = sf::Color::White);

    // Tooltip hanging below (anchorX, topY), kept inside the view.
    void tooltip(float anchorX, float topY, const Tooltip& tip);

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