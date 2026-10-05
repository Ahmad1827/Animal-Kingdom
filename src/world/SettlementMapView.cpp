// ---------------------------------------------------------------------------
// Strategic map view of SettlementSystem: geometry, camera, input and drawing.
//
// The county rings in world_map.json are coarse. buildMapGeometry() turns them
// into organic outlines (every shared border is roughened identically on both
// sides), triangulates them and records who sits on either side of each border.
// Each frame the land is painted into its own layer so that shading and hatching
// are clipped to the coast, then composed over the sea at a multiple of the
// screen resolution. The chrome around the canvas is drawn with ui::Canvas.
// ---------------------------------------------------------------------------
#include "world/SettlementSystem.h"
#include "ui/UIKit.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <map>
#include <numeric>
#include <tuple>

namespace {

using Vec = sf::Vector2f;

const float PI = 3.14159265f;

// Screen layout in the 1280x720 UI space. The HUD top bar owns y < 40.
namespace layout {
    const sf::FloatRect Frame(40.f, 44.f, 1200.f, 664.f);
    const float HeaderH = 44.f;
    const sf::FloatRect Canvas(50.f, 94.f, 880.f, 604.f);
    const sf::FloatRect Panel(938.f, 94.f, 292.f, 604.f);
    const sf::FloatRect Mini(1062.f, 44.f, 210.f, 252.f);
    const sf::FloatRect MiniSea(1067.f, 67.f, 200.f, 224.f);
}

// mapZoom is world units per UI pixel: smaller is closer.
const float ZOOM_MIN = 0.26f;
const float ZOOM_MAX = 1.12f;
const float ZOOM_REALM = 0.60f;     // above this the map reads (and clicks) as whole realms

// The camera never shows anything outside this world rectangle.
const sf::FloatRect VIEW_LIMIT(-150.f, -50.f, 1050.f, 790.f);
const float SEA_PX_PER_UNIT = 1.5f;     // resolution of the baked sea
const float TEX_PER_UNIT = 4.f;         // paper and hatch texels per world unit
// Where an army's banner hangs relative to the spot it stands on, in UI pixels.
const sf::Vector2f ARMY_FLAG_OFFSET(17.f, -22.f);

namespace pal {
    const sf::Color SeaTop      { 58,  92, 108};
    const sf::Color SeaBottom   { 40,  68,  88};
    const sf::Color Shallows    {150, 204, 204};
    const sf::Color Backdrop    {112, 106,  92};
    const sf::Color BackdropInk { 62,  58,  50};
    const sf::Color Ink         { 26,  19,  14};
    const sf::Color Coast       { 38,  28,  20};
    const sf::Color Parchment   {250, 240, 214};
    const sf::Color Gold        {255, 222, 110};
    const sf::Color Neutral     {128, 122, 112};
}

// --- small maths ------------------------------------------------------------

float length(Vec v) { return std::sqrt(v.x * v.x + v.y * v.y); }
Vec normalize(Vec v) { const float l = length(v); return l > 1e-6f ? v / l : Vec(0.f, 0.f); }
float dot(Vec a, Vec b) { return a.x * b.x + a.y * b.y; }
float cross(Vec a, Vec b, Vec c) { return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x); }
// Right-hand side of a direction on screen (y grows downwards).
Vec rightOf(Vec d) { return Vec(-d.y, d.x); }
float smoothstep(float a, float b, float x) {
    const float t = std::clamp((x - a) / (b - a), 0.f, 1.f);
    return t * t * (3.f - 2.f * t);
}
sf::FloatRect lerpRect(const sf::FloatRect& a, const sf::FloatRect& b, float t) {
    return sf::FloatRect(a.left + (b.left - a.left) * t, a.top + (b.top - a.top) * t,
                         a.width + (b.width - a.width) * t, a.height + (b.height - a.height) * t);
}

sf::Color mix(sf::Color a, sf::Color b, float t) {
    auto ch = [t](sf::Uint8 x, sf::Uint8 y) {
        return static_cast<sf::Uint8>(std::clamp(static_cast<float>(x) + (static_cast<float>(y) - static_cast<float>(x)) * t, 0.f, 255.f));
    };
    return sf::Color(ch(a.r, b.r), ch(a.g, b.g), ch(a.b, b.b), ch(a.a, b.a));
}
sf::Color withAlpha(sf::Color c, float a) {
    c.a = static_cast<sf::Uint8>(std::clamp(a, 0.f, 255.f));
    return c;
}
sf::Color fade(sf::Color c, float k) { return withAlpha(c, static_cast<float>(c.a) * k); }

std::string upper(std::string s) {
    for (char& ch : s) ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
    return s;
}

// --- organic borders --------------------------------------------------------

std::uint32_t hashU(std::uint32_t x) {
    x ^= x >> 16; x *= 0x7feb352dU;
    x ^= x >> 15; x *= 0x846ca68bU;
    x ^= x >> 16;
    return x;
}
float signedNoise(std::uint32_t seed) {
    return static_cast<float>(hashU(seed) & 0xFFFFFFu) / static_cast<float>(0x7FFFFF) - 1.f;
}

void displace(Vec a, Vec b, float amp, int depth, std::uint32_t seed, std::vector<Vec>& out) {
    if (depth <= 0) { out.push_back(b); return; }
    const Vec d = b - a;
    const Vec mid = (a + b) * 0.5f + rightOf(d) * (amp * signedNoise(seed));
    displace(a, mid, amp * 0.88f, depth - 1, hashU(seed * 2u + 1u), out);
    displace(mid, b, amp * 0.88f, depth - 1, hashU(seed * 2u + 2u), out);
}

// Roughened polyline from a to b, both included. The shape depends only on the
// two end points, so neighbouring counties get exactly the same border.
std::vector<Vec> organicEdge(Vec a, Vec b, float amp) {
    const bool flip = std::make_pair(b.x, b.y) < std::make_pair(a.x, a.y);
    if (flip) std::swap(a, b);
    const float len = length(b - a);
    const int depth = std::clamp(static_cast<int>(std::lround(std::log2(std::max(len, 1.f) / 3.2f))), 1, 5);
    const auto u = [](float v) { return static_cast<std::uint32_t>(static_cast<std::int32_t>(std::lround(v))); };
    const std::uint32_t seed = hashU(u(a.x) * 73856093u ^ u(a.y) * 19349663u ^ u(b.x) * 83492791u ^ u(b.y) * 2654435761u);
    std::vector<Vec> pts{a};
    displace(a, b, amp, depth, seed, pts);
    if (flip) std::reverse(pts.begin(), pts.end());
    return pts;
}

float signedArea(const std::vector<Vec>& p) {
    float a = 0.f;
    for (size_t i = 0; i < p.size(); ++i) {
        const Vec& q = p[(i + 1) % p.size()];
        a += p[i].x * q.y - q.x * p[i].y;
    }
    return a * 0.5f;
}

bool insidePolygon(const std::vector<Vec>& poly, Vec pt) {
    bool inside = false;
    for (size_t i = 0, j = poly.size() - 1; i < poly.size(); j = i++) {
        if (((poly[i].y > pt.y) != (poly[j].y > pt.y)) &&
            (pt.x < (poly[j].x - poly[i].x) * (pt.y - poly[i].y) / (poly[j].y - poly[i].y) + poly[i].x)) {
            inside = !inside;
        }
    }
    return inside;
}

// Ear clipping. Takes a simple ring in either winding, returns a triangle list.
std::vector<Vec> triangulate(std::vector<Vec> poly) {
    std::vector<Vec> tris;
    if (poly.size() < 3) return tris;
    if (signedArea(poly) < 0.f) std::reverse(poly.begin(), poly.end());

    std::vector<int> idx(poly.size());
    std::iota(idx.begin(), idx.end(), 0);
    tris.reserve(poly.size() * 3);

    size_t i = 0;
    size_t sinceClip = 0;
    while (idx.size() > 3) {
        const size_t n = idx.size();
        i %= n;
        const int ia = idx[(i + n - 1) % n], ib = idx[i], ic = idx[(i + 1) % n];
        const Vec a = poly[ia], b = poly[ib], c = poly[ic];

        bool ear = cross(a, b, c) > 0.f;
        // After a full fruitless lap the ring is degenerate somewhere: cut anyway.
        const bool forced = sinceClip > n;
        if (ear && !forced) {
            for (int j : idx) {
                if (j == ia || j == ib || j == ic) continue;
                const Vec p = poly[j];
                if (cross(a, b, p) >= 0.f && cross(b, c, p) >= 0.f && cross(c, a, p) >= 0.f) { ear = false; break; }
            }
        }
        if (ear || forced) {
            if (ear) { tris.push_back(a); tris.push_back(b); tris.push_back(c); }
            idx.erase(idx.begin() + static_cast<std::ptrdiff_t>(i));
            sinceClip = 0;
        } else {
            ++i;
            ++sinceClip;
        }
    }
    for (int j : idx) tris.push_back(poly[j]);
    return tris;
}

float distToSegment(Vec p, Vec a, Vec b) {
    const Vec ab = b - a;
    const float l2 = dot(ab, ab);
    const float t = l2 > 0.f ? std::clamp(dot(p - a, ab) / l2, 0.f, 1.f) : 0.f;
    return length(p - (a + ab * t));
}

// The roomiest spot inside a ring: where a label is least likely to cross a border.
Vec visualCentre(const std::vector<Vec>& poly, const sf::FloatRect& box) {
    Vec centroid(0.f, 0.f);
    for (const Vec& p : poly) centroid += p;
    centroid /= static_cast<float>(poly.size());

    auto clearance = [&](Vec p) {
        float d = 1e9f;
        for (size_t i = 0; i < poly.size(); ++i) d = std::min(d, distToSegment(p, poly[i], poly[(i + 1) % poly.size()]));
        return d;
    };
    Vec best = centroid;
    float bestScore = -1e9f;
    sf::FloatRect area = box;
    for (int pass = 0; pass < 2; ++pass) {
        const int steps = 18;
        for (int gy = 0; gy <= steps; ++gy) {
            for (int gx = 0; gx <= steps; ++gx) {
                const Vec p(area.left + area.width * static_cast<float>(gx) / steps,
                            area.top + area.height * static_cast<float>(gy) / steps);
                if (!insidePolygon(poly, p)) continue;
                // A mild pull to the middle keeps the pick from sliding into a far lobe.
                const float score = clearance(p) - 0.12f * length(p - centroid);
                if (score > bestScore) { bestScore = score; best = p; }
            }
        }
        const float w = box.width / steps * 1.5f, h = box.height / steps * 1.5f;
        area = sf::FloatRect(best.x - w, best.y - h, w * 2.f, h * 2.f);
    }
    return best;
}

// --- vertex builders --------------------------------------------------------

void addQuad(std::vector<sf::Vertex>& out, Vec a, Vec b, Vec c, Vec d, sf::Color ca, sf::Color cb, sf::Color cc, sf::Color cd) {
    out.emplace_back(a, ca); out.emplace_back(b, cb); out.emplace_back(c, cc);
    out.emplace_back(a, ca); out.emplace_back(c, cc); out.emplace_back(d, cd);
}

void addDisc(std::vector<sf::Vertex>& out, Vec centre, float r, sf::Color col, int segs = 10) {
    for (int i = 0; i < segs; ++i) {
        const float a0 = 2.f * PI * static_cast<float>(i) / segs, a1 = 2.f * PI * static_cast<float>(i + 1) / segs;
        out.emplace_back(centre, col);
        out.emplace_back(centre + Vec(std::cos(a0), std::sin(a0)) * r, col);
        out.emplace_back(centre + Vec(std::cos(a1), std::sin(a1)) * r, col);
    }
}

// Per-vertex offsets to the right-hand side of a polyline, mitred at the bends.
std::vector<Vec> sideOffsets(const std::vector<Vec>& pts, float dist) {
    std::vector<Vec> off(pts.size());
    for (size_t i = 0; i < pts.size(); ++i) {
        const Vec in = i > 0 ? normalize(pts[i] - pts[i - 1]) : Vec(0.f, 0.f);
        const Vec outDir = i + 1 < pts.size() ? normalize(pts[i + 1] - pts[i]) : Vec(0.f, 0.f);
        const Vec n0 = rightOf(i > 0 ? in : outDir);
        const Vec n1 = rightOf(i + 1 < pts.size() ? outDir : in);
        const Vec m = normalize(n0 + n1);
        const float k = 1.f / std::max(dot(m, n0), 0.5f);
        off[i] = m * (dist * k);
    }
    return off;
}

// Solid line of constant width, with round ends so separate pieces join cleanly.
void addStroke(std::vector<sf::Vertex>& out, const std::vector<Vec>& pts, float width, sf::Color col, bool roundEnds = true) {
    if (pts.size() < 2) return;
    const std::vector<Vec> off = sideOffsets(pts, width * 0.5f);
    for (size_t i = 0; i + 1 < pts.size(); ++i) {
        addQuad(out, pts[i] - off[i], pts[i] + off[i], pts[i + 1] + off[i + 1], pts[i + 1] - off[i + 1], col, col, col, col);
    }
    if (roundEnds) {
        addDisc(out, pts.front(), width * 0.5f, col, 8);
        addDisc(out, pts.back(), width * 0.5f, col, 8);
    }
}

// Soft band that starts on the line and fades out towards one side of it.
void addBand(std::vector<sf::Vertex>& out, const std::vector<Vec>& pts, float width, bool rightSide, sf::Color col) {
    if (pts.size() < 2) return;
    const std::vector<Vec> off = sideOffsets(pts, rightSide ? width : -width);
    const sf::Color clear = withAlpha(col, 0.f);
    for (size_t i = 0; i + 1 < pts.size(); ++i) {
        addQuad(out, pts[i], pts[i + 1], pts[i + 1] + off[i + 1], pts[i] + off[i], col, col, clear, clear);
    }
}

// Ring with soft inner and outer edges.
void addSoftRing(std::vector<sf::Vertex>& out, Vec centre, float radius, float thickness, sf::Color col, int segs = 48) {
    const sf::Color clear = withAlpha(col, 0.f);
    const float r[4] = { radius - thickness * 0.5f - 1.f, radius - thickness * 0.5f, radius + thickness * 0.5f, radius + thickness * 0.5f + 1.f };
    const sf::Color cols[4] = { clear, col, col, clear };
    for (int i = 0; i < segs; ++i) {
        const float a0 = 2.f * PI * static_cast<float>(i) / segs, a1 = 2.f * PI * static_cast<float>(i + 1) / segs;
        const Vec d0(std::cos(a0), std::sin(a0)), d1(std::cos(a1), std::sin(a1));
        for (int k = 0; k < 3; ++k) {
            addQuad(out, centre + d0 * r[k], centre + d1 * r[k], centre + d1 * r[k + 1], centre + d0 * r[k + 1],
                    cols[k], cols[k], cols[k + 1], cols[k + 1]);
        }
    }
}

// Straight segment with a one pixel soft fringe on both sides.
void addSoftSegment(std::vector<sf::Vertex>& out, Vec a, Vec b, float width, sf::Color col) {
    const Vec n = rightOf(normalize(b - a));
    const Vec core = n * (width * 0.5f), fringe = n * (width * 0.5f + 1.f);
    const sf::Color clear = withAlpha(col, 0.f);
    addQuad(out, a - core, a + core, b + core, b - core, col, col, col, col);
    addQuad(out, a + core, a + fringe, b + fringe, b + core, col, clear, clear, col);
    addQuad(out, a - fringe, a - core, b - core, b - fringe, clear, col, col, clear);
}

void drawVerts(sf::RenderTarget& rt, const std::vector<sf::Vertex>& v, const sf::RenderStates& states = sf::RenderStates::Default) {
    if (!v.empty()) rt.draw(v.data(), v.size(), sf::Triangles, states);
}

sf::ConvexShape roundedRect(Vec size, float r) {
    r = std::min(r, std::min(size.x, size.y) * 0.5f);
    const int arc = 5;
    sf::ConvexShape s(static_cast<size_t>(arc + 1) * 4);
    const Vec corners[4] = { {size.x - r, r}, {size.x - r, size.y - r}, {r, size.y - r}, {r, r} };
    size_t k = 0;
    for (int c = 0; c < 4; ++c) {
        for (int i = 0; i <= arc; ++i) {
            const float a = -PI * 0.5f + PI * 0.5f * (static_cast<float>(c) + static_cast<float>(i) / arc);
            s.setPoint(k++, corners[c] + Vec(std::cos(a), std::sin(a)) * r);
        }
    }
    return s;
}

// --- text on the map canvas (pixel space) -----------------------------------

struct Ink {
    sf::Color fill = pal::Ink;
    sf::Color halo = sf::Color::Transparent;
    float     haloPx = 0.f;
    sf::Uint32 style = sf::Text::Regular;
};

// Draws text centred on pos. A positive spanPx spreads the letters out to roughly that width.
sf::FloatRect drawLabel(sf::RenderTarget& rt, const sf::Font& font, const std::string& str, Vec pos, float sizePx,
                        const Ink& ink, float rotation = 0.f, float spanPx = 0.f) {
    const unsigned cs = static_cast<unsigned>(std::max(6.f, std::round(sizePx)));
    sf::Text t(str, font, cs);
    t.setStyle(ink.style);
    if (spanPx > 0.f && str.size() > 1) {
        const float natural = t.getLocalBounds().width;
        const float gap = font.getGlyph(L' ', cs, (ink.style & sf::Text::Bold) != 0).advance / 3.f;
        if (natural < spanPx && gap > 0.f) {
            const float extra = std::min((spanPx - natural) / static_cast<float>(str.size() - 1), static_cast<float>(cs) * 1.25f);
            t.setLetterSpacing(1.f + extra / gap);
        }
    }
    t.setFillColor(ink.fill);
    if (ink.haloPx > 0.f) {
        t.setOutlineColor(ink.halo);
        t.setOutlineThickness(ink.haloPx);
    }
    const sf::FloatRect b = t.getLocalBounds();
    t.setOrigin(std::round(b.left + b.width * 0.5f), std::round(b.top + b.height * 0.5f));
    t.setPosition(rotation == 0.f ? Vec(std::round(pos.x), std::round(pos.y)) : pos);
    t.setRotation(rotation);
    rt.draw(t);
    return sf::FloatRect(pos.x - b.width * 0.5f, pos.y - b.height * 0.5f, b.width, b.height);
}

// Small dark pill with a coloured rim. Returns its height so badges can be stacked.
float drawBadge(sf::RenderTarget& rt, const sf::Font& font, const std::string& str, Vec centre, float S, sf::Color accent, float alpha = 1.f) {
    sf::Text probe(str, font, static_cast<unsigned>(std::round(8.5f * S)));
    probe.setStyle(sf::Text::Bold);
    const Vec size(probe.getLocalBounds().width + 12.f * S, 14.f * S);
    sf::ConvexShape pill = roundedRect(size, 4.f * S);
    pill.setOrigin(size * 0.5f);
    pill.setPosition(std::round(centre.x), std::round(centre.y));
    pill.setFillColor(fade(sf::Color(20, 15, 11, 236), alpha));
    pill.setOutlineColor(fade(accent, alpha));
    pill.setOutlineThickness(std::max(1.f, S * 0.75f));
    rt.draw(pill);
    Ink ink;
    ink.fill = fade(mix(accent, sf::Color::White, 0.72f), alpha);
    ink.style = sf::Text::Bold;
    drawLabel(rt, font, str, Vec(centre.x, centre.y - 0.5f * S), 8.5f * S, ink);
    return size.y;
}

void drawStar(sf::RenderTarget& rt, Vec centre, float r, sf::Color fill, sf::Color rim) {
    for (int pass = 0; pass < 2; ++pass) {
        const float rr = pass == 0 ? r + std::max(1.5f, r * 0.22f) : r;
        sf::VertexArray fan(sf::TriangleFan);
        fan.append(sf::Vertex(centre, pass == 0 ? rim : fill));
        for (int i = 0; i <= 10; ++i) {
            const float a = -PI * 0.5f + PI * static_cast<float>(i) / 5.f;
            const float rad = (i % 2 == 0) ? rr : rr * 0.46f;
            fan.append(sf::Vertex(centre + Vec(std::cos(a), std::sin(a)) * rad, pass == 0 ? rim : fill));
        }
        rt.draw(fan);
    }
}

// Settlement marker: a disc for an open town, a crenellated keep once it is fortified.
void drawSettlementMarker(sf::RenderTarget& rt, Vec p, float S, int fortTier, bool capital, float alpha) {
    const sf::Color rim = fade(pal::Ink, alpha);
    const sf::Color fill = fade(fortTier >= 2 ? sf::Color(214, 226, 236) : pal::Parchment, alpha);
    p = Vec(std::round(p.x), std::round(p.y));
    if (capital) {
        drawStar(rt, p, 6.2f * S, fade(pal::Gold, alpha), rim);
        return;
    }
    if (fortTier <= 0) {
        sf::CircleShape dot(2.8f * S);
        dot.setOrigin(dot.getRadius(), dot.getRadius());
        dot.setPosition(p);
        dot.setFillColor(fill);
        dot.setOutlineColor(rim);
        dot.setOutlineThickness(1.2f * S);
        rt.draw(dot);
        return;
    }
    const float w = (fortTier >= 2 ? 9.f : 7.5f) * S, h = (fortTier >= 2 ? 7.f : 6.f) * S, t = 1.1f * S;
    sf::RectangleShape body(Vec(w, h));
    body.setOrigin(w * 0.5f, h * 0.5f);
    body.setPosition(p.x, p.y + 1.f * S);
    body.setFillColor(fill);
    body.setOutlineColor(rim);
    body.setOutlineThickness(t);
    rt.draw(body);
    const float mw = w / 5.f;
    for (int i = 0; i < 3; ++i) {
        sf::RectangleShape merlon(Vec(mw, 2.4f * S));
        merlon.setPosition(p.x - w * 0.5f + mw * 2.f * static_cast<float>(i), p.y + 1.f * S - h * 0.5f - 2.4f * S);
        merlon.setFillColor(fill);
        merlon.setOutlineColor(rim);
        merlon.setOutlineThickness(t);
        rt.draw(merlon);
    }
}

void drawCompass(sf::RenderTarget& rt, const sf::Font& font, Vec c, float r) {
    const sf::Color light(236, 222, 186, 215), dark(60, 46, 32, 215), rim(24, 18, 13, 200);
    sf::CircleShape ring(r * 0.62f, 40);
    ring.setOrigin(ring.getRadius(), ring.getRadius());
    ring.setPosition(c);
    ring.setFillColor(sf::Color(16, 24, 30, 70));
    ring.setOutlineColor(withAlpha(light, 120.f));
    ring.setOutlineThickness(std::max(1.f, r * 0.035f));
    rt.draw(ring);

    std::vector<sf::Vertex> v;
    auto point = [&](float angle, float len, float halfWidth) {
        const Vec d(std::cos(angle), std::sin(angle));
        const Vec tip = c + d * len, l = c + rightOf(d) * -halfWidth, rgt = c + rightOf(d) * halfWidth;
        v.emplace_back(c, light); v.emplace_back(l, light); v.emplace_back(tip, light);
        v.emplace_back(c, dark);  v.emplace_back(tip, dark); v.emplace_back(rgt, dark);
    };
    for (int i = 0; i < 4; ++i) point(PI * 0.25f + PI * 0.5f * static_cast<float>(i), r * 0.52f, r * 0.11f);
    for (int i = 0; i < 4; ++i) point(-PI * 0.5f + PI * 0.5f * static_cast<float>(i), r, r * 0.15f);
    drawVerts(rt, v);

    Ink ink;
    ink.fill = light;
    ink.halo = rim;
    ink.haloPx = std::max(1.f, r * 0.05f);
    ink.style = sf::Text::Bold;
    drawLabel(rt, font, "N", Vec(c.x, c.y - r * 1.28f), r * 0.42f, ink);
}

// --- textures ---------------------------------------------------------------

// Tileable cloudy grain, multiplied over land and sea so flat colours read as painted paper.
sf::Image makePaperImage() {
    const unsigned N = 256;
    auto lattice = [](int x, int y, int period, std::uint32_t salt) {
        x = ((x % period) + period) % period;
        y = ((y % period) + period) % period;
        return static_cast<float>(hashU(static_cast<std::uint32_t>(x) * 374761393u + static_cast<std::uint32_t>(y) * 668265263u + salt) & 0xFFFFu) / 65535.f;
    };
    sf::Image img;
    img.create(N, N);
    for (unsigned y = 0; y < N; ++y) {
        for (unsigned x = 0; x < N; ++x) {
            float v = 0.f, weight = 0.f, amp = 1.f;
            for (int period = 4; period <= 64; period *= 2) {
                const float fx = static_cast<float>(x) * period / N, fy = static_cast<float>(y) * period / N;
                const int x0 = static_cast<int>(fx), y0 = static_cast<int>(fy);
                const float tx = smoothstep(0.f, 1.f, fx - x0), ty = smoothstep(0.f, 1.f, fy - y0);
                const std::uint32_t salt = static_cast<std::uint32_t>(period) * 977u;
                const float top = lattice(x0, y0, period, salt) + (lattice(x0 + 1, y0, period, salt) - lattice(x0, y0, period, salt)) * tx;
                const float bot = lattice(x0, y0 + 1, period, salt) + (lattice(x0 + 1, y0 + 1, period, salt) - lattice(x0, y0 + 1, period, salt)) * tx;
                v += (top + (bot - top) * ty) * amp;
                weight += amp;
                amp *= 0.6f;
            }
            v /= weight;
            const float grain = static_cast<float>(hashU(x * 7919u + y * 104729u) & 0xFFu) / 255.f;
            const sf::Uint8 g = static_cast<sf::Uint8>(std::clamp(204.f + 46.f * v + 6.f * grain, 0.f, 255.f));
            img.setPixel(x, y, sf::Color(g, g, g));
        }
    }
    return img;
}

// Diagonal stripes, used to hatch counties held by someone other than their fill colour says.
sf::Image makeHatchImage() {
    const unsigned N = 32;
    sf::Image img;
    img.create(N, N, sf::Color(255, 255, 255, 0));
    for (unsigned y = 0; y < N; ++y) {
        for (unsigned x = 0; x < N; ++x) {
            const unsigned stripe = 11;
            const unsigned d = (x + y) % N;
            const float edge = std::min(static_cast<float>(d), static_cast<float>(stripe) - static_cast<float>(d));
            if (d < stripe) img.setPixel(x, y, sf::Color(255, 255, 255, static_cast<sf::Uint8>(std::clamp(edge * 160.f, 0.f, 255.f))));
        }
    }
    return img;
}

const char* const BLUR_FRAG =
    "uniform sampler2D texture;\n"
    "uniform vec2 step;\n"
    "void main() {\n"
    "    vec2 uv = gl_TexCoord[0].xy;\n"
    "    float a = 0.0;\n"
    "    float total = 0.0;\n"
    "    for (int i = -14; i <= 14; ++i) {\n"
    "        float w = exp(-float(i * i) / 60.0);\n"
    "        a += texture2D(texture, uv + step * float(i)).a * w;\n"
    "        total += w;\n"
    "    }\n"
    "    gl_FragColor = vec4(1.0, 1.0, 1.0, a / total);\n"
    "}\n";

// Mainland Europe, drawn as unplayable context. Ring runs along the coast first,
// then closes far outside the camera limits.
const std::vector<Vec>& continentRing() {
    static const std::vector<Vec> ring = {
        {355.f, 712.f}, {362.f, 693.f}, {398.f, 687.f}, {420.f, 681.f}, {456.f, 695.f}, {472.f, 693.f},
        {470.f, 659.f}, {458.f, 627.f}, {482.f, 628.f}, {488.f, 648.f}, {521.f, 653.f}, {532.f, 640.f},
        {567.f, 614.f}, {584.f, 597.f}, {591.f, 568.f}, {602.f, 554.f}, {632.f, 533.f}, {654.f, 520.f},
        {676.f, 485.f}, {699.f, 427.f}, {726.f, 399.f}, {780.f, 380.f}, {823.f, 386.f}, {838.f, 343.f},
        {823.f, 268.f}, {820.f, 169.f}, {906.f, 132.f},
        {1300.f, 132.f}, {1300.f, 1000.f}, {150.f, 1000.f}, {330.f, 770.f}
    };
    return ring;
}

struct SeaName { const char* text; Vec pos; float size; float rotation; float span; bool land; };
const SeaName SEA_NAMES[] = {
    {"NORTH SEA",       {650.f, 250.f}, 15.f,   0.f, 150.f, false},
    {"IRISH SEA",       {366.f, 372.f},  7.f, -62.f,  52.f, false},
    {"ATLANTIC OCEAN",  {178.f, 150.f}, 15.f,   0.f, 210.f, false},
    {"CELTIC SEA",      {262.f, 566.f}, 11.f, -14.f, 110.f, false},
    {"ENGLISH CHANNEL", {468.f, 602.f},  7.5f, -9.f, 118.f, false},
    {"FRANCIA",         {676.f, 590.f}, 14.f, -27.f, 120.f, true},
    {"FRISIA",          {768.f, 446.f}, 10.f, -30.f,  80.f, true},
};

const char* const LENS_NAMES[5] = { "De Facto", "De Jure", "Vassals", "Diplomacy", "Economy" };
const char* const LENS_KEYS[5] = { "Q", "W", "E", "R", "T" };
const char* const LENS_BLURBS[5] = {
    "Who holds each county today",
    "Rightful borders by ancient title",
    "How loyal your chieftains are",
    "Friends, foes and truces",
    "Wealth and the trade roads"
};

} // namespace

// ===========================================================================
// Geometry
// ===========================================================================

void SettlementSystem::buildMapGeometry() {
    counties.clear();
    mapEdges.clear();

    const auto& defs = WorldMapRepository::getInstance().getCounties();

    // Rings wound clockwise on screen, so a county's interior is always to the right of its outline.
    std::vector<std::vector<Vec>> rings;
    for (const auto& def : defs) {
        std::vector<Vec> ring = def.points;
        if (signedArea(ring) < 0.f) std::reverse(ring.begin(), ring.end());
        rings.push_back(ring);
    }

    struct EdgeUse { int a = -1; int b = -1; };
    using EdgeKey = std::tuple<int, int, int, int>;
    using VertKey = std::pair<int, int>;
    auto vertKey = [](Vec p) { return VertKey(static_cast<int>(std::lround(p.x)), static_cast<int>(std::lround(p.y))); };
    auto edgeKey = [&](Vec p, Vec q) {
        VertKey a = vertKey(p), b = vertKey(q);
        if (b < a) std::swap(a, b);
        return EdgeKey(a.first, a.second, b.first, b.second);
    };

    std::map<EdgeKey, EdgeUse> uses;
    std::map<VertKey, std::vector<float>> spokes;      // directions of every border leaving a vertex
    for (size_t ci = 0; ci < rings.size(); ++ci) {
        const auto& ring = rings[ci];
        for (size_t i = 0; i < ring.size(); ++i) {
            const Vec p = ring[i], q = ring[(i + 1) % ring.size()];
            EdgeUse& use = uses[edgeKey(p, q)];
            if (use.a < 0) {
                use.a = static_cast<int>(ci);
                spokes[vertKey(p)].push_back(std::atan2(q.y - p.y, q.x - p.x));
                spokes[vertKey(q)].push_back(std::atan2(p.y - q.y, p.x - q.x));
            } else {
                use.b = static_cast<int>(ci);
            }
        }
    }
    // Angle to the nearest other border at a vertex. Borders that leave a corner
    // close together are kept straighter so their wiggles cannot cross.
    auto gapAt = [&](Vec v, Vec towards) {
        const float ang = std::atan2(towards.y - v.y, towards.x - v.x);
        float best = PI;
        for (float s : spokes[vertKey(v)]) {
            const float d = std::abs(std::remainder(s - ang, 2.f * PI));
            if (d > 1e-3f) best = std::min(best, d);
        }
        return best;
    };

    bool haveBounds = false;
    for (size_t ci = 0; ci < rings.size(); ++ci) {
        const auto& def = defs[ci];
        const auto& ring = rings[ci];

        CountyDef c;
        c.countyId = def.countyId;
        c.countyName = def.countyName;
        c.settlementName = def.settlementName;
        c.modernName = def.modernName;
        c.kingdomName = def.kingdomId;
        c.deJureKingdom = def.deJureKingdomId;
        c.vassalOpinion = def.initialOpinion;
        c.supplyLimit = def.supplyLimit;
        c.fortTier = def.fortTier;

        for (size_t i = 0; i < ring.size(); ++i) {
            const Vec p = ring[i], q = ring[(i + 1) % ring.size()];
            const EdgeUse& use = uses[edgeKey(p, q)];
            const bool coast = use.b < 0;
            const float calm = std::clamp(std::min(gapAt(p, q), gapAt(q, p)) / (PI * 70.f / 180.f), 0.15f, 1.f);
            const std::vector<Vec> pts = organicEdge(p, q, (coast ? 0.17f : 0.10f) * calm);
            c.points.insert(c.points.end(), pts.begin(), pts.end() - 1);
            if (use.a == static_cast<int>(ci)) {
                MapBorderEdge e;
                e.a = use.a;
                e.b = use.b;
                e.pts = pts;
                mapEdges.push_back(e);
            }
        }

        float minX = 1e9f, minY = 1e9f, maxX = -1e9f, maxY = -1e9f;
        for (const Vec& p : c.points) {
            minX = std::min(minX, p.x); maxX = std::max(maxX, p.x);
            minY = std::min(minY, p.y); maxY = std::max(maxY, p.y);
        }
        c.bounds = sf::FloatRect(minX, minY, maxX - minX, maxY - minY);
        c.center = visualCentre(c.points, c.bounds);

        for (const Vec& p : triangulate(c.points)) {
            c.mesh.emplace_back(p, sf::Color::White, p * TEX_PER_UNIT);
        }

        if (!haveBounds) { mapWorldBounds = c.bounds; haveBounds = true; }
        else {
            const float l = std::min(mapWorldBounds.left, c.bounds.left), t = std::min(mapWorldBounds.top, c.bounds.top);
            const float r = std::max(mapWorldBounds.left + mapWorldBounds.width, c.bounds.left + c.bounds.width);
            const float b = std::max(mapWorldBounds.top + mapWorldBounds.height, c.bounds.top + c.bounds.height);
            mapWorldBounds = sf::FloatRect(l, t, r - l, b - t);
        }
        counties.push_back(std::move(c));
    }

    // Mainland backdrop.
    backdropMesh.clear();
    backdropCoast.clear();
    const auto& ring = continentRing();
    std::vector<Vec> outline;
    for (size_t i = 0; i < ring.size(); ++i) {
        const Vec p = ring[i], q = ring[(i + 1) % ring.size()];
        const bool visible = VIEW_LIMIT.contains(p) || VIEW_LIMIT.contains(q);
        const std::vector<Vec> pts = visible ? organicEdge(p, q, 0.11f) : std::vector<Vec>{p, q};
        outline.insert(outline.end(), pts.begin(), pts.end() - 1);
        if (visible) backdropCoast.push_back(pts);
    }
    for (const Vec& p : triangulate(outline)) backdropMesh.emplace_back(p, pal::Backdrop, p * TEX_PER_UNIT);

    mapCenter = Vec(mapWorldBounds.left + mapWorldBounds.width * 0.5f, mapWorldBounds.top + mapWorldBounds.height * 0.5f);
    mapZoom = mapZoomTarget = 1.0f;
    zoomAnchorWorld = mapCenter;
    zoomAnchorOffset = Vec(0.f, 0.f);
    realmLabelSignature.clear();

    bakeMapTextures();
}

void SettlementSystem::bakeMapTextures() {
    paperTexture.loadFromImage(makePaperImage());
    paperTexture.setRepeated(true);
    paperTexture.setSmooth(true);

    hatchTexture.loadFromImage(makeHatchImage());
    hatchTexture.setRepeated(true);
    hatchTexture.setSmooth(true);

    // The sea never changes, so it is painted once: paper grain, the shadow the
    // land casts on the water, and a pale band of shallows hugging every coast.
    seaLayerReady = false;
    const unsigned w = static_cast<unsigned>(VIEW_LIMIT.width * SEA_PX_PER_UNIT);
    const unsigned h = static_cast<unsigned>(VIEW_LIMIT.height * SEA_PX_PER_UNIT);
    sf::RenderTexture mask;
    if (!mask.create(w, h) || !seaLayer.create(w, h)) return;

    const sf::View world(VIEW_LIMIT);
    mask.setView(world);
    mask.clear(sf::Color(255, 255, 255, 0));
    for (auto& c : counties) {
        for (auto& v : c.mesh) v.color = sf::Color::White;
        drawVerts(mask, c.mesh);
    }
    std::vector<sf::Vertex> land = backdropMesh;
    for (auto& v : land) v.color = sf::Color::White;
    drawVerts(mask, land);
    mask.display();
    mask.setSmooth(true);

    seaLayer.setView(world);
    seaLayer.clear(pal::SeaBottom);
    {
        const Vec tl(VIEW_LIMIT.left, VIEW_LIMIT.top), br(VIEW_LIMIT.left + VIEW_LIMIT.width, VIEW_LIMIT.top + VIEW_LIMIT.height);
        const float texPerUnit = 1.1f;
        const sf::Vertex water[4] = {
            sf::Vertex(tl, pal::SeaTop, tl * texPerUnit), sf::Vertex(Vec(br.x, tl.y), pal::SeaTop, Vec(br.x, tl.y) * texPerUnit),
            sf::Vertex(br, pal::SeaBottom, br * texPerUnit), sf::Vertex(Vec(tl.x, br.y), pal::SeaBottom, Vec(tl.x, br.y) * texPerUnit)
        };
        sf::RenderStates st;
        st.texture = &paperTexture;
        seaLayer.draw(water, 4, sf::Quads, st);
    }

    sf::Sprite silhouette(mask.getTexture());
    silhouette.setPosition(VIEW_LIMIT.left, VIEW_LIMIT.top);
    silhouette.setScale(1.f / SEA_PX_PER_UNIT, 1.f / SEA_PX_PER_UNIT);

    sf::Shader blur;
    sf::RenderTexture pass, glow;
    if (sf::Shader::isAvailable() && blur.loadFromMemory(BLUR_FRAG, sf::Shader::Fragment) && pass.create(w, h) && glow.create(w, h)) {
        sf::RenderStates st;
        st.blendMode = sf::BlendNone;
        st.shader = &blur;
        blur.setUniform("texture", sf::Shader::CurrentTexture);

        pass.setSmooth(true);
        blur.setUniform("step", sf::Glsl::Vec2(1.5f / static_cast<float>(w), 0.f));
        pass.clear(sf::Color(255, 255, 255, 0));
        pass.draw(sf::Sprite(mask.getTexture()), st);
        pass.display();

        glow.setSmooth(true);
        blur.setUniform("step", sf::Glsl::Vec2(0.f, 1.5f / static_cast<float>(h)));
        glow.clear(sf::Color(255, 255, 255, 0));
        glow.draw(sf::Sprite(pass.getTexture()), st);
        glow.display();

        sf::Sprite shallows(glow.getTexture());
        shallows.setPosition(VIEW_LIMIT.left, VIEW_LIMIT.top);
        shallows.setScale(1.f / SEA_PX_PER_UNIT, 1.f / SEA_PX_PER_UNIT);
        shallows.setColor(withAlpha(pal::Shallows, 215.f));
        seaLayer.draw(shallows);
    }

    silhouette.setColor(sf::Color(0, 0, 0, 78));
    silhouette.move(1.6f, 2.6f);
    seaLayer.draw(silhouette);

    seaLayer.display();
    seaLayer.setSmooth(true);
    seaLayerReady = true;
}

// ===========================================================================
// Camera and picking
// ===========================================================================

sf::Vector2f SettlementSystem::mapPointAt(sf::Vector2f uiPos) const {
    const Vec canvasCentre(layout::Canvas.left + layout::Canvas.width * 0.5f, layout::Canvas.top + layout::Canvas.height * 0.5f);
    return mapCenter + (uiPos - canvasCentre) * mapZoom;
}

void SettlementSystem::clampMapCenter() {
    // Keep some land under the camera, then keep the camera inside the painted world.
    mapCenter.x = std::clamp(mapCenter.x, mapWorldBounds.left, mapWorldBounds.left + mapWorldBounds.width);
    mapCenter.y = std::clamp(mapCenter.y, mapWorldBounds.top, mapWorldBounds.top + mapWorldBounds.height);

    const float hw = layout::Canvas.width * mapZoom * 0.5f, hh = layout::Canvas.height * mapZoom * 0.5f;
    const float l = VIEW_LIMIT.left + hw, r = VIEW_LIMIT.left + VIEW_LIMIT.width - hw;
    const float t = VIEW_LIMIT.top + hh, b = VIEW_LIMIT.top + VIEW_LIMIT.height - hh;
    mapCenter.x = (l <= r) ? std::clamp(mapCenter.x, l, r) : VIEW_LIMIT.left + VIEW_LIMIT.width * 0.5f;
    mapCenter.y = (t <= b) ? std::clamp(mapCenter.y, t, b) : VIEW_LIMIT.top + VIEW_LIMIT.height * 0.5f;
}

void SettlementSystem::refreshMapHover() {
    hoveredCountyIdx = -1;
    hoveredKingdomName.clear();
    if (!mouseOverMap) return;
    const Vec world = mapPointAt(lastMouseUi);
    for (size_t i = 0; i < counties.size(); ++i) {
        if (counties[i].bounds.contains(world) && pointInPolygon(counties[i].points, world)) {
            hoveredCountyIdx = static_cast<int>(i);
            hoveredKingdomName = counties[i].kingdomName;
            return;
        }
    }
}

void SettlementSystem::updateMapCamera(float dt) {
    if (targetMapMode != 2) return;
    if (mapZoom != mapZoomTarget) {
        mapZoom += (mapZoomTarget - mapZoom) * std::min(1.0f, dt * 14.f);
        if (std::abs(mapZoomTarget - mapZoom) < 0.0008f) mapZoom = mapZoomTarget;
        // Zoom about the point that was under the cursor when the wheel turned.
        mapCenter = zoomAnchorWorld - zoomAnchorOffset * mapZoom;
        clampMapCenter();
        refreshMapHover();
    }
}

const std::string& SettlementSystem::mapRealmKey(const CountyDef& c) const {
    return currentLens == MapLens::DeJure ? c.deJureKingdom : c.kingdomName;
}

sf::Color SettlementSystem::mapFillColor(const CountyDef& c, MapLens lens) const {
    const std::string player = getPlayerKingdomId();
    switch (lens) {
        case MapLens::DeJure:
            return getKingdomColor(c.deJureKingdom);
        case MapLens::Vassals:
            if (c.kingdomName != player) return pal::Neutral;
            if (c.vassalOpinion >= 25) return sf::Color(84, 160, 84);
            if (c.vassalOpinion >= 0) return sf::Color(214, 176, 66);
            return sf::Color(198, 62, 52);
        case MapLens::Diplomacy:
            if (c.kingdomName == player) return sf::Color(66, 126, 204);
            if (activeWar.active && (c.kingdomName == activeWar.enemyKingdom || c.countyName == activeWar.targetCounty)) return sf::Color(200, 54, 48);
            if (isAllyInWar(c.kingdomName)) return sf::Color(70, 176, 204);
            if (hasTruceWith(c.kingdomName)) return sf::Color(206, 178, 110);
            return pal::Neutral;
        case MapLens::Economy:
            if (c.supplyLimit >= 36) return sf::Color(226, 186, 62);
            if (c.supplyLimit >= 26) return sf::Color(160, 168, 88);
            return sf::Color(142, 110, 84);
        case MapLens::DeFacto:
        default:
            return getKingdomColor(c.kingdomName);
    }
}

// ===========================================================================
// Realm names
// ===========================================================================

void SettlementSystem::updateRealmLabels(float dt) {
    if (!fontLoaded) return;

    std::string signature = std::to_string(static_cast<int>(currentLens));
    for (const auto& c : counties) { signature += '|'; signature += mapRealmKey(c); }

    if (signature != realmLabelSignature) {
        realmLabelSignature = signature;

        // Counties of one realm that touch form one named block; an overseas holding gets its own name.
        std::vector<int> group(counties.size());
        std::iota(group.begin(), group.end(), 0);
        auto find = [&](int i) { while (group[i] != i) i = group[i] = group[group[i]]; return i; };
        for (const auto& e : mapEdges) {
            if (e.b >= 0 && mapRealmKey(counties[e.a]) == mapRealmKey(counties[e.b])) group[find(e.a)] = find(e.b);
        }

        for (auto& pair : realmLabels) pair.second.targetAlpha = 0.f;

        for (size_t root = 0; root < counties.size(); ++root) {
            if (find(static_cast<int>(root)) != static_cast<int>(root)) continue;
            const std::string& realm = mapRealmKey(counties[root]);
            if (realm.empty() || realm == "Wilderness") continue;

            auto inBlock = [&](int i) { return i >= 0 && find(i) == static_cast<int>(root); };

            // Area-weighted centre and spread of the block.
            float area = 0.f;
            Vec centroid(0.f, 0.f);
            size_t biggest = root;
            float biggestArea = 0.f;
            for (size_t i = 0; i < counties.size(); ++i) {
                if (!inBlock(static_cast<int>(i))) continue;
                float own = 0.f;
                const auto& m = counties[i].mesh;
                for (size_t k = 0; k + 2 < m.size(); k += 3) {
                    const float a = std::abs(cross(m[k].position, m[k + 1].position, m[k + 2].position)) * 0.5f;
                    centroid += (m[k].position + m[k + 1].position + m[k + 2].position) * (a / 3.f);
                    own += a;
                }
                area += own;
                if (own > biggestArea) { biggestArea = own; biggest = i; }
            }
            if (area <= 0.f) continue;
            centroid /= area;

            float sxx = 0.f, sxy = 0.f, syy = 0.f;
            for (size_t i = 0; i < counties.size(); ++i) {
                if (!inBlock(static_cast<int>(i))) continue;
                const auto& m = counties[i].mesh;
                for (size_t k = 0; k + 2 < m.size(); k += 3) {
                    const float a = std::abs(cross(m[k].position, m[k + 1].position, m[k + 2].position)) * 0.5f;
                    const Vec d = (m[k].position + m[k + 1].position + m[k + 2].position) / 3.f - centroid;
                    sxx += d.x * d.x * a; sxy += d.x * d.y * a; syy += d.y * d.y * a;
                }
            }
            float axis = 0.5f * std::atan2(2.f * sxy, sxx - syy);          // long axis, radians in (-90, 90]
            const float limit = PI * 68.f / 180.f;
            axis = std::clamp(axis, -limit, limit);
            const float spread = std::sqrt(std::max(0.f, std::min(sxx, syy) / area));

            // Outline of the block: every border piece with the block on exactly one side.
            std::vector<std::pair<Vec, Vec>> walls;
            for (const auto& e : mapEdges) {
                if (inBlock(e.a) == inBlock(e.b)) continue;
                for (size_t k = 0; k + 1 < e.pts.size(); ++k) walls.emplace_back(e.pts[k], e.pts[k + 1]);
            }
            // Stretch of the line p + t*d that lies inside the block around t = 0.
            auto chord = [&](Vec p, Vec d, float& t0, float& t1) {
                t0 = -1e9f; t1 = 1e9f;
                int before = 0;
                for (const auto& w : walls) {
                    const Vec s = w.second - w.first;
                    const float den = d.x * s.y - d.y * s.x;
                    if (std::abs(den) < 1e-6f) continue;
                    const Vec ap = w.first - p;
                    const float u = (ap.x * d.y - ap.y * d.x) / den;
                    if (u < 0.f || u >= 1.f) continue;
                    const float t = (ap.x * s.y - ap.y * s.x) / den;
                    if (t < 0.f) { ++before; t0 = std::max(t0, t); }
                    else t1 = std::min(t1, t);
                }
                return (before % 2) == 1 && t0 > -1e8f && t1 < 1e8f;
            };

            const std::string text = upper(realm);
            const float letters = static_cast<float>(text.size());

            float bestScore = -1.f;
            Vec bestPos = counties[biggest].center;
            float bestAngle = 0.f, bestSize = 6.f, bestSpan = 0.f;
            // Try the name along the block's long axis and level, through its middle
            // and through each county, and keep whichever lets it be largest.
            const float angles[3] = { axis, axis * 0.5f, 0.f };
            const float shifts[5] = { 0.f, -0.6f, 0.6f, -1.2f, 1.2f };
            for (float ang : angles) {
                const Vec d(std::cos(ang), std::sin(ang));
                const Vec n = rightOf(d);
                std::vector<Vec> anchors;
                for (float shift : shifts) anchors.push_back(centroid + n * (shift * spread));
                for (size_t i = 0; i < counties.size(); ++i) {
                    if (inBlock(static_cast<int>(i))) anchors.push_back(counties[i].center);
                }
                for (const Vec& p : anchors) {
                    float t0, t1;
                    if (!chord(p, d, t0, t1)) continue;
                    const float run = t1 - t0;
                    const Vec mid = p + d * ((t0 + t1) * 0.5f);
                    // Room above and below the baseline, probed along the middle of the run.
                    float room = 1e9f;
                    for (int s = -3; s <= 3; ++s) {
                        float u0, u1;
                        if (!chord(mid + d * (run * 0.115f * static_cast<float>(s)), n, u0, u1)) { room = 0.f; break; }
                        room = std::min(room, std::min(-u0, u1));
                    }
                    if (room <= 0.f) continue;
                    const float size = std::min(room * 0.95f, run * 0.72f / (letters * 0.70f));
                    // Bigger is better, a little air between letters is better, level text is easier to read.
                    const float score = size * (1.f + 0.12f * std::min(run / (letters * std::max(size, 1.f)), 2.5f))
                                             * (1.f - 0.50f * std::abs(ang) / limit);
                    if (score > bestScore) {
                        bestScore = score;
                        bestPos = mid;
                        bestAngle = ang;
                        bestSize = size;
                        bestSpan = run * 0.70f;
                    }
                }
            }

            auto& lbl = realmLabels[realm + "#" + std::to_string(counties[biggest].countyId)];
            lbl.kingdomId = realm;
            lbl.text = text;
            lbl.targetAlpha = 255.f;
            lbl.targetPos = bestPos;
            lbl.targetRot = bestAngle * 180.f / PI;
            lbl.targetSize = std::clamp(bestSize, 5.f, 30.f);
            lbl.targetSpan = bestSpan;
        }
    }

    const float k = std::min(1.0f, dt * 2.2f);
    for (auto it = realmLabels.begin(); it != realmLabels.end();) {
        auto& lbl = it->second;
        if (!lbl.initialized) {
            lbl.currentPos = lbl.targetPos;
            lbl.currentSize = lbl.targetSize;
            lbl.currentRot = lbl.targetRot;
            lbl.currentSpan = lbl.targetSpan;
            lbl.currentAlpha = 0.f;
            lbl.initialized = true;
        } else {
            lbl.currentPos += (lbl.targetPos - lbl.currentPos) * k;
            lbl.currentSize += (lbl.targetSize - lbl.currentSize) * k;
            lbl.currentRot += (lbl.targetRot - lbl.currentRot) * k;
            lbl.currentSpan += (lbl.targetSpan - lbl.currentSpan) * k;
        }
        lbl.currentAlpha += (lbl.targetAlpha - lbl.currentAlpha) * std::min(1.0f, dt * 6.f);
        if (lbl.targetAlpha <= 0.f && lbl.currentAlpha < 1.f) it = realmLabels.erase(it);
        else ++it;
    }
}

// ===========================================================================
// Input
// ===========================================================================

bool SettlementSystem::handleMapLensInput(const sf::Event& event, const sf::RenderWindow& window, const sf::View& letterboxView) {
    if (expandAnimT < 0.70f) return false;

    if (event.type == sf::Event::KeyPressed) {
        if (event.key.code == sf::Keyboard::Q) { currentLens = MapLens::DeFacto; return true; }
        if (event.key.code == sf::Keyboard::W) { currentLens = MapLens::DeJure; return true; }
        if (event.key.code == sf::Keyboard::E) { currentLens = MapLens::Vassals; return true; }
        if (event.key.code == sf::Keyboard::R) { currentLens = MapLens::Diplomacy; return true; }
        if (event.key.code == sf::Keyboard::T) { currentLens = MapLens::Economy; return true; }
        if (event.key.code == sf::Keyboard::F) { factionModalOpen = !factionModalOpen; return true; }
    }

    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2i clickPixel(event.mouseButton.x, event.mouseButton.y);
        sf::Vector2f uiCoords = window.mapPixelToCoords(clickPixel, letterboxView);
        for (int i = 0; i < 5; ++i) {
            if (lensTabBounds[i].contains(uiCoords)) {
                currentLens = static_cast<MapLens>(i);
                return true;
            }
        }
    }
    return false;
}

bool SettlementSystem::handleWorldMapInput(const sf::Event& event, const sf::RenderWindow& window, const sf::View& letterboxView,
                                           const std::function<void(const RealSettlement&, bool isKingdomLevel)>& onSettlementClicked,
                                           const std::function<void(const RealSettlement&, sf::Vector2f, bool isKingdomLevel)>& onSettlementRightClicked) {
    if (expandAnimT < 0.80f) return false;

    const sf::FloatRect canvasRect = layout::Canvas;
    bool isKingdomLevel = (mapZoom > ZOOM_REALM);

    if (event.type == sf::Event::MouseWheelScrolled) {
        sf::Vector2f mPos = window.mapPixelToCoords(sf::Vector2i(event.mouseWheelScroll.x, event.mouseWheelScroll.y), letterboxView);
        if (canvasRect.contains(mPos)) {
            zoomAnchorOffset = mPos - Vec(canvasRect.left + canvasRect.width * 0.5f, canvasRect.top + canvasRect.height * 0.5f);
            zoomAnchorWorld = mapCenter + zoomAnchorOffset * mapZoom;
            mapZoomTarget = std::clamp(mapZoomTarget * std::pow(0.86f, event.mouseWheelScroll.delta), ZOOM_MIN, ZOOM_MAX);
            return true;
        }
    }

    if (event.type == sf::Event::MouseMoved) {
        lastMouseUi = window.mapPixelToCoords(sf::Vector2i(event.mouseMove.x, event.mouseMove.y), letterboxView);
        mouseOverMap = canvasRect.contains(lastMouseUi);
        refreshMapHover();
    }

    if (event.type == sf::Event::MouseButtonPressed && (event.mouseButton.button == sf::Mouse::Left || event.mouseButton.button == sf::Mouse::Middle || event.mouseButton.button == sf::Mouse::Right)) {
        sf::Vector2f mPos = window.mapPixelToCoords(sf::Vector2i(event.mouseButton.x, event.mouseButton.y), letterboxView);
        dragStartMouse = sf::Vector2i(event.mouseButton.x, event.mouseButton.y);
        lastDragMouse = dragStartMouse;
        if (canvasRect.contains(mPos)) {
            isDraggingMap = (event.mouseButton.button == sf::Mouse::Left || event.mouseButton.button == sf::Mouse::Middle);
            return true;
        }
    }

    if (event.type == sf::Event::MouseMoved && isDraggingMap) {
        sf::Vector2i curMouse(event.mouseMove.x, event.mouseMove.y);
        sf::Vector2f curWorld = window.mapPixelToCoords(curMouse, letterboxView);
        sf::Vector2f lastWorld = window.mapPixelToCoords(lastDragMouse, letterboxView);
        sf::Vector2f delta = (curWorld - lastWorld) * mapZoom;

        mapCenter -= delta;
        clampMapCenter();
        // A drag takes over from any zoom still easing in.
        zoomAnchorOffset = Vec(0.f, 0.f);
        zoomAnchorWorld = mapCenter;

        lastDragMouse = curMouse;
        return true;
    }

    if (event.type == sf::Event::MouseButtonReleased) {
        if (isDraggingMap) isDraggingMap = false;

        int dx = event.mouseButton.x - dragStartMouse.x;
        int dy = event.mouseButton.y - dragStartMouse.y;
        bool isClick = (dx * dx + dy * dy < 256);

        sf::Vector2f mPos = window.mapPixelToCoords(sf::Vector2i(event.mouseButton.x, event.mouseButton.y), letterboxView);

        if (isClick && factionsTabBounds.contains(mPos)) {
            factionModalOpen = !factionModalOpen;
            return true;
        }

        if (isClick && factionModalOpen) {
            if (closeFactionModalBounds.contains(mPos)) {
                factionModalOpen = false;
                return true;
            }

            for (int t = 0; t < 4; ++t) {
                if (authorityTierBounds[t].contains(mPos)) {
                    selectedAuthorityTier = t + 1;
                    return true;
                }
            }

            if (enactLawBtnBounds.contains(mPos)) {
                if (selectedAuthorityTier == crownAuthority) {
                    lawStatusMsg = "Law is already enacted in the realm.";
                    lawStatusTimer = 2.5f;
                    return true;
                }

                sim::ApeData* playerApe = cachedRegistry ? cachedRegistry->getApe(cachedRegistry->getControlledApe()) : nullptr;
                int curPrestige = playerApe ? playerApe->prestige : 0;

                if (curPrestige < 100) {
                    lawStatusMsg = "Need 100 Prestige to pass new Crown Law! (Current: " + std::to_string(curPrestige) + ")";
                    lawStatusTimer = 3.0f;
                    return true;
                }

                if (lawCooldownTimer > 0.f) {
                    lawStatusMsg = "Crown Laws can only be reformed once per reign cycle.";
                    lawStatusTimer = 3.0f;
                    return true;
                }

                if (playerApe) playerApe->prestige -= 100;
                crownAuthority = selectedAuthorityTier;
                lawCooldownTimer = 45.f;

                static const std::string authNames[] = { "Autonomous Chieftains", "Limited Authority", "High Authority", "Absolute Alpha Rule" };
                lawStatusMsg = "Crown Law Reformed: " + authNames[crownAuthority - 1] + "! Vassals have reacted.";
                lawStatusTimer = 4.0f;
                return true;
            }

            sf::FloatRect fModalRect(180.f, 112.f, 620.f, 500.f);
            if (fModalRect.contains(mPos)) {
                return true;
            }
        }

        if (isClick && successionModalOpen) {
            if (closeSuccessionModalBounds.contains(mPos) || confirmSuccessionBtnBounds.contains(mPos)) {
                successionModalOpen = false;
                return true;
            }
            sf::FloatRect succRect(200.f, 170.f, 580.f, 410.f);
            if (succRect.contains(mPos)) {
                return true;
            }
        }

        if (isClick && activeWar.active && callAllyBtnBounds.contains(mPos)) {
            std::string allyK = WorldMapRepository::getInstance().getDefaultAllyKingdom();
            if (!allyK.empty() && !isAllyInWar(allyK)) {
                callAllyToWar(allyK);
            }
            return true;
        }

        if (isClick && activeWar.active && warBadgeBounds.contains(mPos)) {
            peaceModalOpen = !peaceModalOpen;
            return true;
        }

        if (isClick && peaceModalOpen) {
            if (closePeaceModalBounds.contains(mPos)) {
                peaceModalOpen = false;
                return true;
            }

            if (enforceBtnBounds.contains(mPos) && activeWar.warScore >= 75.f) {
                annexCounty(activeWar.targetCounty, activeWar.attackerKingdom);
                return true;
            }

            if (whitePeaceBtnBounds.contains(mPos)) {
                for (auto& c : counties) {
                    c.isOccupied = false;
                    c.occupierKingdom.clear();
                    c.siegeProgress = 0.f;
                }
                std::string enemyK = activeWar.enemyKingdom;
                mapArmies.erase(
                    std::remove_if(mapArmies.begin(), mapArmies.end(),
                                   [&enemyK](const MapArmy& a) { return a.ownerKingdom == enemyK; }),
                    mapArmies.end()
                );
                for (const auto& al : activeWarAllies) {
                    mapArmies.erase(
                        std::remove_if(mapArmies.begin(), mapArmies.end(),
                                       [&al](const MapArmy& a) { return a.ownerKingdom == al; }),
                        mapArmies.end()
                    );
                }
                activeWarAllies.clear();
                kingdomTruces[enemyK] = 3;
                activeWar.active = false;
                peaceModalOpen = false;
                return true;
            }

            if (surrenderBtnBounds.contains(mPos)) {
                for (auto& c : counties) {
                    c.isOccupied = false;
                    c.occupierKingdom.clear();
                    c.siegeProgress = 0.f;
                }
                std::string enemyK = activeWar.enemyKingdom;
                mapArmies.erase(
                    std::remove_if(mapArmies.begin(), mapArmies.end(),
                                   [&enemyK](const MapArmy& a) { return a.ownerKingdom == enemyK; }),
                    mapArmies.end()
                );
                for (const auto& al : activeWarAllies) {
                    mapArmies.erase(
                        std::remove_if(mapArmies.begin(), mapArmies.end(),
                                       [&al](const MapArmy& a) { return a.ownerKingdom == al; }),
                        mapArmies.end()
                    );
                }
                activeWarAllies.clear();
                kingdomTruces[enemyK] = 5;
                activeWar.active = false;
                peaceModalOpen = false;
                return true;
            }

            sf::FloatRect modalRect(240.f, 210.f, 500.f, 335.f);
            if (modalRect.contains(mPos)) {
                return true;
            }
        }

        if (isClick && canvasRect.contains(mPos)) {
                float relX = mPos.x - canvasRect.left;
                float relY = mPos.y - canvasRect.top;
                sf::Vector2f worldClick = mapCenter + sf::Vector2f(relX - canvasRect.width * 0.5f, relY - canvasRect.height * 0.5f) * mapZoom;

                if (event.mouseButton.button == sf::Mouse::Left) {
                    bool armyClicked = false;
                    for (const auto& a : mapArmies) {
                        const sf::Vector2f flagPos = a.pos + ARMY_FLAG_OFFSET * mapZoom;
                        float dist = std::hypot(flagPos.x - worldClick.x, flagPos.y - worldClick.y);
                        if (dist <= 18.f * mapZoom) {
                            selectedArmyId = static_cast<int>(a.id);
                            armyClicked = true;
                            return true;
                        }
                    }
                    if (!armyClicked) {
                        selectedArmyId = -1;
                    }
                }

            if (event.mouseButton.button == sf::Mouse::Right && selectedArmyId != -1) {
                for (size_t i = 0; i < counties.size(); ++i) {
                    if (pointInPolygon(counties[i].points, worldClick)) {
                        for (auto& a : mapArmies) {
                            if (static_cast<int>(a.id) == selectedArmyId) {
                                a.targetPos = counties[i].center + sf::Vector2f(20.f, -12.f);
                                a.targetCounty = counties[i].countyName;
                                a.isMoving = true;
                                break;
                            }
                        }
                        selectedArmyId = -1;
                        return true;
                    }
                }
            }

            for (size_t i = 0; i < counties.size(); ++i) {
                if (pointInPolygon(counties[i].points, worldClick)) {
                    RealSettlement dummyRs;
                    dummyRs.villageId = counties[i].villageId;
                    dummyRs.kingdomId = counties[i].kingdomId;
                    dummyRs.historicalName = counties[i].settlementName;
                    dummyRs.modernName = counties[i].modernName;
                    dummyRs.kingdomName = counties[i].kingdomName;
                    dummyRs.deJureKingdom = counties[i].deJureKingdom;
                    dummyRs.countyName = counties[i].countyName;
                    dummyRs.mapCoord = counties[i].center;
                    dummyRs.hasPlayerClaim = (getPlayerClaims().count(counties[i].countyName) > 0);

                    for (const auto& s : realSettlements) {
                        if (counties[i].villageId != 0 && s.villageId == counties[i].villageId) {
                            dummyRs.villageId = s.villageId;
                            dummyRs.centerX = s.centerX;
                            break;
                        } else if (!counties[i].settlementName.empty() && s.historicalName == counties[i].settlementName) {
                            dummyRs.villageId = s.villageId;
                            dummyRs.centerX = s.centerX;
                            break;
                        }
                    }
                    dummyRs.kingdomName = counties[i].kingdomName;
                    dummyRs.deJureKingdom = counties[i].deJureKingdom;
                    dummyRs.countyName = counties[i].countyName;

                    if (event.mouseButton.button == sf::Mouse::Left && onSettlementClicked) {
                        onSettlementClicked(dummyRs, isKingdomLevel);
                        return true;
                    } else if (event.mouseButton.button == sf::Mouse::Right && onSettlementRightClicked) {
                        onSettlementRightClicked(dummyRs, mPos, isKingdomLevel);
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

// ===========================================================================
// Drawing
// ===========================================================================

// Fingerprint of everything renderMapBase() looks at. While it stays the same the painted map is reused.
std::uint64_t SettlementSystem::mapBaseSignature(float S) const {
    std::uint64_t h = 1469598103934665603ull;
    auto bytes = [&h](const void* data, size_t n) {
        const unsigned char* p = static_cast<const unsigned char*>(data);
        for (size_t i = 0; i < n; ++i) { h ^= p[i]; h *= 1099511628211ull; }
    };
    auto num = [&](long long v) { bytes(&v, sizeof v); };
    auto real = [&](float v, float step) { num(std::llround(v / step)); };
    auto str = [&](const std::string& s) { bytes(s.data(), s.size()); num(static_cast<long long>(s.size())); };

    real(S, 0.01f);
    num(static_cast<int>(currentLens));
    num(hoveredCountyIdx);
    real(mapCenter.x, 0.01f); real(mapCenter.y, 0.01f); real(mapZoom, 0.0005f);
    str(getPlayerKingdomId());
    str(getPlayerCapitalCounty());
    num(activeWar.active);
    if (activeWar.active) str(activeWar.targetCounty);

    for (const auto& c : counties) {
        str(mapRealmKey(c));
        num(mapFillColor(c, currentLens).toInteger());
        num(c.isOccupied);
        if (c.isOccupied) str(c.occupierKingdom);
        num(static_cast<int>(c.siegeProgress));
        num(c.fortTier);
        num(c.inFaction);
        num(c.vassalOpinion);
        num(c.kingdomName != c.deJureKingdom);
    }
    for (const auto& pair : realmLabels) {
        const auto& l = pair.second;
        str(l.text);
        real(l.currentPos.x, 0.05f); real(l.currentPos.y, 0.05f);
        real(l.currentSize, 0.02f); real(l.currentSpan, 0.1f); real(l.currentRot, 0.1f); real(l.currentAlpha, 1.f);
    }
    for (const auto& ca : councilAssignments) {
        num(static_cast<int>(ca.role)); num(static_cast<int>(ca.mission));
        str(ca.targetCounty);
        real(ca.maxProgress > 0.f ? ca.progress / ca.maxProgress : 0.f, 0.02f);
    }
    for (const auto& tr : tradeRoutes) num(tr.isRaided);
    return h;
}

// Everything on the map that only changes when the camera, the lens, the hover or
// the state of the realms changes. Painted into mapCanvas and reused until then.
void SettlementSystem::renderMapBase(int Si, float S) {
    const unsigned w = static_cast<unsigned>(layout::Canvas.width) * static_cast<unsigned>(Si);
    const unsigned h = static_cast<unsigned>(layout::Canvas.height) * static_cast<unsigned>(Si);
    if (!mapCanvasReady || mapCanvas.getSize().x != w || mapCanvas.getSize().y != h) {
        mapCanvas.create(w, h);
        mapCanvas.setSmooth(true);
        landLayer.create(w, h);
        landLayer.setSmooth(true);
        mapCanvasReady = true;
    }

    // S is the resolution actually painted, in canvas pixels per UI pixel. While the
    // camera is moving it is lower than the texture allows, and only a corner is used.
    const Vec sizePx(std::floor(layout::Canvas.width * S), std::floor(layout::Canvas.height * S));
    mapCanvasUsed = sizePx;
    const sf::FloatRect corner(0.f, 0.f, sizePx.x / static_cast<float>(w), sizePx.y / static_cast<float>(h));
    const float ppu = S / mapZoom;                 // canvas pixels per world unit
    const float u = mapZoom;                       // world units per UI pixel
    sf::View world(mapCenter, Vec(layout::Canvas.width, layout::Canvas.height) * mapZoom);
    world.setViewport(corner);
    sf::View pixels(sf::FloatRect(0.f, 0.f, sizePx.x, sizePx.y));
    pixels.setViewport(corner);
    auto toPx = [&](Vec p) { return (p - mapCenter) * ppu + sizePx * 0.5f; };

    const bool realmLevel = mapZoom > ZOOM_REALM;
    const float detail = 1.f - smoothstep(ZOOM_REALM - 0.05f, ZOOM_REALM + 0.05f, mapZoom);   // 1 = county names
    const std::string player = getPlayerKingdomId();

    const std::string hoverKey = hoveredCountyIdx >= 0 ? mapRealmKey(counties[hoveredCountyIdx]) : std::string();
    auto lit = [&](int i) {
        if (i < 0 || hoveredCountyIdx < 0) return false;
        return realmLevel ? mapRealmKey(counties[i]) == hoverKey : i == hoveredCountyIdx;
    };

    // Paints over land without ever touching its silhouette.
    const sf::BlendMode overLand(sf::BlendMode::SrcAlpha, sf::BlendMode::OneMinusSrcAlpha, sf::BlendMode::Add,
                                 sf::BlendMode::Zero, sf::BlendMode::One, sf::BlendMode::Add);

    // ---- land layer ------------------------------------------------------
    sf::RenderStates paper;
    paper.texture = &paperTexture;
    // Wipe only the corner in use; a full clear costs as much as painting it.
    landLayer.setView(pixels);
    {
        const sf::Color none(0, 0, 0, 0);
        const sf::Vertex wipe[4] = { sf::Vertex(Vec(0.f, 0.f), none), sf::Vertex(Vec(sizePx.x, 0.f), none),
                                     sf::Vertex(sizePx, none), sf::Vertex(Vec(0.f, sizePx.y), none) };
        landLayer.draw(wipe, 4, sf::Quads, sf::BlendNone);
    }
    landLayer.setView(world);
    drawVerts(landLayer, backdropMesh, paper);

    std::vector<sf::Color> fills(counties.size());
    for (size_t i = 0; i < counties.size(); ++i) {
        auto& c = counties[i];
        sf::Color fill = mapFillColor(c, currentLens);
        // A step of shade per county keeps neighbours of one realm apart without shouting.
        const float shade = static_cast<float>((c.countyId * 7) % 5 - 2) * (0.012f + 0.018f * detail);
        fill = shade >= 0.f ? mix(fill, sf::Color::White, shade) : mix(fill, sf::Color::Black, -shade);
        if (lit(static_cast<int>(i))) fill = mix(fill, sf::Color(255, 250, 225), 0.15f);
        fills[i] = fill;
        for (auto& v : c.mesh) v.color = fill;
        drawVerts(landLayer, c.mesh, paper);
    }

    // Hatching: stripes in the colour of whoever really holds the land.
    for (const auto& c : counties) {
        sf::Color stripe = sf::Color::Transparent;
        if (c.isOccupied) stripe = withAlpha(getKingdomColor(c.occupierKingdom), 235.f);
        else if (currentLens == MapLens::DeJure && c.kingdomName != c.deJureKingdom) stripe = withAlpha(getKingdomColor(c.kingdomName), 225.f);
        if (stripe.a == 0) continue;
        std::vector<sf::Vertex> hatch = c.mesh;
        for (auto& v : hatch) v.color = stripe;
        sf::RenderStates st;
        st.blendMode = overLand;
        st.texture = &hatchTexture;
        drawVerts(landLayer, hatch, st);
    }

    // Darker rim just inside every realm border and coast, so each realm reads as one raised shape.
    {
        std::vector<sf::Vertex> bands;
        for (const auto& e : mapEdges) {
            const bool coast = e.b < 0;
            if (!coast && mapRealmKey(counties[e.a]) == mapRealmKey(counties[e.b])) continue;
            const float width = (coast ? 5.5f : 8.f) * u;
            const float alpha = coast ? 105.f : 150.f;
            addBand(bands, e.pts, width, true, withAlpha(mix(fills[e.a], sf::Color::Black, 0.62f), alpha));
            if (!coast) addBand(bands, e.pts, width, false, withAlpha(mix(fills[e.b], sf::Color::Black, 0.62f), alpha));
        }
        for (const auto& line : backdropCoast) addBand(bands, line, 5.f * u, true, sf::Color(40, 36, 30, 90));
        sf::RenderStates st;
        st.blendMode = overLand;
        drawVerts(landLayer, bands, st);
    }
    landLayer.display();

    // ---- sea, then the land on top ------------------------------------------------
    if (!seaLayerReady) mapCanvas.clear(pal::SeaBottom);
    mapCanvas.setView(world);
    if (seaLayerReady) {
        sf::Sprite sea(seaLayer.getTexture());
        sea.setPosition(VIEW_LIMIT.left, VIEW_LIMIT.top);
        sea.setScale(1.f / SEA_PX_PER_UNIT, 1.f / SEA_PX_PER_UNIT);
        mapCanvas.draw(sea, sf::BlendNone);
    }
    mapCanvas.setView(pixels);
    mapCanvas.draw(sf::Sprite(landLayer.getTexture(), sf::IntRect(0, 0, static_cast<int>(sizePx.x), static_cast<int>(sizePx.y))));

    // ---- borders ---------------------------------------------------------------
    mapCanvas.setView(world);
    {
        std::vector<sf::Vertex> lines;
        const sf::Color countyLine = withAlpha(pal::Ink, 50.f + 95.f * detail);
        for (const auto& e : mapEdges) {
            if (e.b >= 0 && mapRealmKey(counties[e.a]) == mapRealmKey(counties[e.b])) addStroke(lines, e.pts, 1.0f * u, countyLine, false);
        }
        for (const auto& line : backdropCoast) addStroke(lines, line, 1.2f * u, pal::BackdropInk);
        for (const auto& e : mapEdges) {
            if (e.b < 0) addStroke(lines, e.pts, 1.5f * u, pal::Coast);
            else if (mapRealmKey(counties[e.a]) != mapRealmKey(counties[e.b])) addStroke(lines, e.pts, 2.3f * u, pal::Ink);
        }

        // War goal: a red line around the county being fought over.
        if (activeWar.active) {
            for (const auto& e : mapEdges) {
                const bool a = counties[e.a].countyName == activeWar.targetCounty;
                const bool b = e.b >= 0 && counties[e.b].countyName == activeWar.targetCounty;
                if (a != b) addStroke(lines, e.pts, 2.8f * u, sf::Color(244, 78, 60));
            }
        }
        // Hover outline.
        if (hoveredCountyIdx >= 0) {
            for (const auto& e : mapEdges) {
                if (lit(e.a) != lit(e.b)) addStroke(lines, e.pts, 2.6f * u, pal::Gold);
            }
        }
        drawVerts(mapCanvas, lines);
    }

    // ---- trade roads -------------------------------------------------------------
    auto countyByName = [&](const std::string& name) -> const CountyDef* {
        for (const auto& c : counties) if (c.countyName == name) return &c;
        return nullptr;
    };
    struct RaidTag { Vec at; };
    std::vector<RaidTag> raidTags;
    if (currentLens == MapLens::Economy) {
        std::vector<sf::Vertex> roads;
        for (const auto& tr : tradeRoutes) {
            std::vector<Vec> path;
            for (const auto& name : tr.counties) if (const CountyDef* c = countyByName(name)) path.push_back(c->center);
            if (path.size() < 2) continue;
            const sf::Color road = tr.isRaided ? sf::Color(226, 70, 54) : sf::Color(255, 226, 120);
            addStroke(roads, path, 4.6f * u, pal::Ink);
            addStroke(roads, path, 2.4f * u, road);
            if (tr.isRaided) raidTags.push_back({ (path[path.size() / 2 - 1] + path[path.size() / 2]) * 0.5f });
        }
        drawVerts(mapCanvas, roads);
    }

    // ---- everything below is laid out in canvas pixels, so it stays one size at any zoom ----
    mapCanvas.setView(pixels);

    // Names of the seas and of the lands beyond the map.
    for (const SeaName& sn : SEA_NAMES) {
        const float sizePxl = std::clamp(sn.size * ppu, 8.f * S, 26.f * S);
        Ink ink;
        ink.fill = sn.land ? sf::Color(56, 50, 42, 150) : sf::Color(176, 212, 216, 120);
        ink.style = sn.land ? sf::Text::Bold : sf::Text::Italic;
        drawLabel(mapCanvas, font, sn.text, toPx(sn.pos), sizePxl, ink, sn.rotation, sn.span * ppu);
    }

    // Realm names, sized to the land they sit on.
    if (detail < 0.98f) {
        for (const auto& pair : realmLabels) {
            const auto& lbl = pair.second;
            const float alpha = lbl.currentAlpha * (1.f - detail);
            if (alpha < 2.f || lbl.text.empty()) continue;
            const float sizePxl = std::clamp(lbl.currentSize * ppu, 9.5f * S, 30.f * S);
            Ink ink;
            ink.fill = withAlpha(sf::Color(22, 16, 12), alpha * 0.90f);
            ink.halo = withAlpha(pal::Parchment, alpha * 0.36f);
            ink.haloPx = 1.3f * S;
            ink.style = sf::Text::Bold;
            drawLabel(mapCanvas, font, lbl.text, toPx(lbl.currentPos), sizePxl, ink, lbl.currentRot, lbl.currentSpan * ppu);
        }
    }

    // Settlements, county names and status badges.
    for (const auto& c : counties) {
        const Vec p = toPx(c.center);
        if (p.x < -80.f * S || p.y < -60.f * S || p.x > sizePx.x + 80.f * S || p.y > sizePx.y + 60.f * S) continue;

        // Siege dial under the marker.
        if (c.siegeProgress > 0.f && !c.isOccupied) {
            const float r = 11.f * S;
            sf::CircleShape back(r, 32);
            back.setOrigin(r, r);
            back.setPosition(p);
            back.setFillColor(sf::Color(20, 14, 10, 225));
            back.setOutlineColor(sf::Color(200, 150, 70));
            back.setOutlineThickness(1.2f * S);
            mapCanvas.draw(back);
            sf::VertexArray wedge(sf::TriangleFan);
            wedge.append(sf::Vertex(p, sf::Color(226, 70, 52, 235)));
            const int steps = std::max(1, static_cast<int>(c.siegeProgress / 100.f * 40.f));
            for (int s = 0; s <= steps; ++s) {
                const float a = -PI * 0.5f + 2.f * PI * (c.siegeProgress / 100.f) * static_cast<float>(s) / static_cast<float>(steps);
                wedge.append(sf::Vertex(p + Vec(std::cos(a), std::sin(a)) * (r - 1.5f * S), sf::Color(226, 70, 52, 235)));
            }
            mapCanvas.draw(wedge);
        }

        // At realm level only your own seat is marked; the rest would fight the realm names.
        const bool capital = isPlayerCapital(c.countyName);
        if (capital || detail > 0.02f) drawSettlementMarker(mapCanvas, p, S, c.fortTier, capital, capital ? 1.f : detail);

        // Badges stack upwards from the marker.
        float badgeY = p.y - 15.f * S;
        auto badge = [&](const std::string& text, sf::Color accent) {
            badgeY -= drawBadge(mapCanvas, font, text, Vec(p.x, badgeY), S, accent) + 3.f * S;
        };
        if (c.isOccupied) badge("OCCUPIED", getKingdomColor(c.occupierKingdom));
        else if (c.siegeProgress > 0.f) badge("SIEGE " + std::to_string(static_cast<int>(c.siegeProgress)) + "%", sf::Color(232, 92, 70));
        if (currentLens == MapLens::DeJure && c.kingdomName != c.deJureKingdom) badge("USURPED", sf::Color(236, 110, 84));
        if (currentLens == MapLens::Vassals && c.kingdomName == player && c.inFaction) badge("FACTION", sf::Color(236, 110, 84));

        if (detail > 0.02f) {
            Ink name;
            name.fill = withAlpha(pal::Ink, 255.f * detail);
            name.halo = withAlpha(pal::Parchment, 215.f * detail);
            name.haloPx = 1.6f * S;
            name.style = sf::Text::Bold;
            drawLabel(mapCanvas, font, c.countyName, Vec(p.x, p.y + 13.f * S), 12.f * S, name);

            Ink seat;
            seat.fill = withAlpha(sf::Color(52, 38, 26), 240.f * detail);
            seat.halo = withAlpha(pal::Parchment, 170.f * detail);
            seat.haloPx = 1.2f * S;
            seat.style = sf::Text::Italic;
            drawLabel(mapCanvas, font, c.settlementName, Vec(p.x, p.y + 26.f * S), 9.5f * S, seat);

            if (currentLens == MapLens::Vassals && c.kingdomName == player) {
                const std::string op = (c.vassalOpinion >= 0 ? "+" : "") + std::to_string(c.vassalOpinion);
                drawBadge(mapCanvas, font, op, Vec(p.x, p.y + 41.f * S), S,
                          c.vassalOpinion >= 25 ? sf::Color(120, 214, 110) : (c.vassalOpinion >= 0 ? sf::Color(242, 208, 122) : sf::Color(240, 96, 82)), detail);
            }
        }
    }
    for (const auto& tag : raidTags) drawBadge(mapCanvas, font, "RAIDED", toPx(tag.at) - Vec(0.f, 12.f * S), S, sf::Color(236, 92, 70));

    // Council errands.
    for (const auto& ca : councilAssignments) {
        if (ca.mission == CouncilMissionType::None) continue;
        const CountyDef* target = countyByName(ca.targetCounty);
        if (!target) continue;
        const Vec p = toPx(target->center) + Vec(-19.f, -4.f) * S;

        sf::Color pinCol(160, 110, 220);
        std::string letter = "S";
        if (ca.role == sim::CouncilRole::WarChief) { pinCol = sf::Color(220, 65, 55); letter = "W"; }
        else if (ca.role == sim::CouncilRole::ChiefBuilder) { pinCol = sf::Color(70, 175, 95); letter = "B"; }

        sf::CircleShape pin(7.5f * S, 24);
        pin.setOrigin(pin.getRadius(), pin.getRadius());
        pin.setPosition(p);
        pin.setFillColor(pinCol);
        pin.setOutlineColor(pal::Parchment);
        pin.setOutlineThickness(1.2f * S);
        mapCanvas.draw(pin);
        Ink ink;
        ink.fill = sf::Color::White;
        ink.style = sf::Text::Bold;
        drawLabel(mapCanvas, font, letter, p, 9.5f * S, ink);

        if (ca.mission == CouncilMissionType::FabricateClaim && ca.maxProgress > 0.f) {
            const Vec bar(24.f * S, 3.5f * S);
            sf::RectangleShape bg(bar);
            bg.setPosition(p.x - bar.x * 0.5f, p.y + 10.f * S);
            bg.setFillColor(sf::Color(16, 12, 10, 230));
            bg.setOutlineColor(pal::Ink);
            bg.setOutlineThickness(S);
            mapCanvas.draw(bg);
            sf::RectangleShape fillBar(Vec(bar.x * std::clamp(ca.progress / ca.maxProgress, 0.f, 1.f), bar.y));
            fillBar.setPosition(bg.getPosition());
            fillBar.setFillColor(pinCol);
            mapCanvas.draw(fillBar);
        }
    }

    drawCompass(mapCanvas, font, Vec(sizePx.x - 46.f * S, sizePx.y - 44.f * S), 24.f * S);

    mapCanvas.display();
}

// The few things that move every frame: armies, marching orders, carts and your own marker.
// Drawn straight onto the window, clipped to the canvas, so the painted map underneath can be reused.
void SettlementSystem::drawMapOverlay(sf::RenderWindow& window, const sf::View& letterboxView, float playerX) {
    const sf::Vector2i topLeft = window.mapCoordsToPixel(Vec(layout::Canvas.left, layout::Canvas.top), letterboxView);
    const sf::Vector2i bottomRight = window.mapCoordsToPixel(Vec(layout::Canvas.left + layout::Canvas.width, layout::Canvas.top + layout::Canvas.height), letterboxView);
    const Vec sizePx(static_cast<float>(bottomRight.x - topLeft.x), static_cast<float>(bottomRight.y - topLeft.y));
    const Vec win(static_cast<float>(window.getSize().x), static_cast<float>(window.getSize().y));
    if (sizePx.x < 1.f || sizePx.y < 1.f || win.x < 1.f || win.y < 1.f) return;

    // One unit of this view is one device pixel, and its viewport is exactly the canvas.
    sf::View clip(sf::FloatRect(0.f, 0.f, sizePx.x, sizePx.y));
    clip.setViewport(sf::FloatRect(static_cast<float>(topLeft.x) / win.x, static_cast<float>(topLeft.y) / win.y, sizePx.x / win.x, sizePx.y / win.y));
    window.setView(clip);

    sf::RenderTarget& rt = window;
    const float S = sizePx.x / layout::Canvas.width;       // device pixels per UI pixel
    const float ppu = S / mapZoom;
    auto toPx = [&](Vec p) { return (p - mapCenter) * ppu + sizePx * 0.5f; };
    const std::string player = getPlayerKingdomId();

    // Carts rolling along the trade roads.
    if (currentLens == MapLens::Economy) {
        for (const auto& tr : tradeRoutes) {
            std::vector<Vec> path;
            for (const auto& name : tr.counties) {
                for (const auto& c : counties) if (c.countyName == name) { path.push_back(c.center); break; }
            }
            for (size_t i = 0; i + 1 < path.size(); ++i) {
                for (int k = 0; k < 2; ++k) {
                    const float t = std::fmod(pulseTime * 0.22f + 0.5f * static_cast<float>(k) + 0.17f * static_cast<float>(i), 1.f);
                    sf::CircleShape cart(2.2f * S, 16);
                    cart.setOrigin(cart.getRadius(), cart.getRadius());
                    cart.setPosition(toPx(path[i] + (path[i + 1] - path[i]) * t));
                    cart.setFillColor(tr.isRaided ? sf::Color(255, 170, 140) : sf::Color(255, 250, 215));
                    cart.setOutlineColor(pal::Ink);
                    cart.setOutlineThickness(1.1f * S);
                    rt.draw(cart);
                }
            }
        }
    }

    // Marching orders: a dashed line from each moving army to where it is going.
    {
        std::vector<sf::Vertex> orders;
        const sf::Color col(255, 230, 130, 225);
        for (const auto& a : mapArmies) {
            if (!a.isMoving) continue;
            const Vec from = toPx(a.pos), to = toPx(a.targetPos);
            const Vec d = normalize(to - from);
            const float len = length(to - from);
            for (float s = 0.f; s < len; s += 9.f * S) addSoftSegment(orders, from + d * s, from + d * std::min(s + 5.f * S, len), 2.f * S, col);
            addSoftRing(orders, to, 3.2f * S, 1.6f * S, col, 20);
        }
        drawVerts(rt, orders);
    }

    // You.
    {
        const Vec p = toPx(getPlayerMapCoord(playerX)) + Vec(0.f, -1.f * S);
        const float beat = 0.5f + 0.5f * std::sin(pulseTime * 4.5f);
        std::vector<sf::Vertex> ring;
        addSoftRing(ring, p, (8.f + 5.f * beat) * S, 1.6f * S, sf::Color(255, 232, 140, static_cast<sf::Uint8>(235.f * (1.f - beat))));
        drawVerts(rt, ring);

        sf::CircleShape pin(4.6f * S, 4);     // diamond
        pin.setOrigin(pin.getRadius(), pin.getRadius());
        pin.setPosition(p + Vec(15.f, -13.f) * S);
        pin.setFillColor(sf::Color(255, 250, 235));
        pin.setOutlineColor(pal::Ink);
        pin.setOutlineThickness(1.4f * S);
        std::vector<sf::Vertex> line;
        addSoftSegment(line, p, pin.getPosition(), 1.4f * S, pal::Ink);
        drawVerts(rt, line);
        rt.draw(pin);

        Ink ink;
        ink.fill = sf::Color(255, 250, 235);
        ink.halo = pal::Ink;
        ink.haloPx = 1.5f * S;
        ink.style = sf::Text::Bold;
        drawLabel(rt, font, "YOU", pin.getPosition() + Vec(18.f, -1.f) * S, 9.5f * S, ink);
    }

    // Armies.
    for (const auto& a : mapArmies) {
        const Vec foot = toPx(a.pos);
        const Vec p = foot + Vec(ARMY_FLAG_OFFSET.x, ARMY_FLAG_OFFSET.y) * S;
        const bool selected = static_cast<int>(a.id) == selectedArmyId;
        const Vec flag(30.f * S, 18.f * S);

        if (selected) {
            sf::ConvexShape glow = roundedRect(flag + Vec(8.f, 8.f) * S, 6.f * S);
            glow.setOrigin((flag + Vec(8.f, 8.f) * S) * 0.5f);
            glow.setPosition(p);
            glow.setFillColor(sf::Color(255, 226, 110, 70));
            glow.setOutlineColor(pal::Gold);
            glow.setOutlineThickness(1.6f * S);
            rt.draw(glow);
        }

        sf::RectangleShape pole(Vec(2.4f * S, foot.y - (p.y - flag.y * 0.5f - 2.f * S)));
        pole.setPosition(p.x - flag.x * 0.5f - 2.4f * S, p.y - flag.y * 0.5f - 2.f * S);
        pole.setFillColor(sf::Color(232, 214, 168));
        pole.setOutlineColor(pal::Ink);
        pole.setOutlineThickness(S);
        rt.draw(pole);

        sf::ConvexShape banner = roundedRect(flag, 3.f * S);
        banner.setOrigin(flag * 0.5f);
        banner.setPosition(p);
        banner.setFillColor(getKingdomColor(a.ownerKingdom));
        banner.setOutlineColor(pal::Ink);
        banner.setOutlineThickness(1.5f * S);
        rt.draw(banner);
        sf::RectangleShape sheen(Vec(flag.x - 3.f * S, flag.y * 0.42f));
        sheen.setPosition(p.x - flag.x * 0.5f + 1.5f * S, p.y - flag.y * 0.5f + 1.5f * S);
        sheen.setFillColor(sf::Color(255, 255, 255, 34));
        rt.draw(sheen);

        Ink num;
        num.fill = sf::Color::White;
        num.halo = sf::Color(0, 0, 0, 170);
        num.haloPx = 1.2f * S;
        num.style = sf::Text::Bold;
        drawLabel(rt, font, std::to_string(a.strength), p, 11.f * S, num);

        // Supply bar under the banner.
        const Vec bar(flag.x - 4.f * S, 3.f * S);
        sf::RectangleShape supBg(bar);
        supBg.setPosition(p.x - bar.x * 0.5f, p.y + flag.y * 0.5f + 3.f * S);
        supBg.setFillColor(sf::Color(18, 12, 10, 235));
        supBg.setOutlineColor(pal::Ink);
        supBg.setOutlineThickness(S);
        rt.draw(supBg);
        sf::RectangleShape supFill(Vec(bar.x * std::clamp(a.supply / 100.f, 0.f, 1.f), bar.y));
        supFill.setPosition(supBg.getPosition());
        supFill.setFillColor(a.supply > 55.f ? sf::Color(96, 206, 96) : (a.supply > 25.f ? sf::Color(228, 188, 56) : sf::Color(236, 62, 48)));
        rt.draw(supFill);

        float tagY = p.y - flag.y * 0.5f - 11.f * S;
        if (a.inCombat) {
            const float beat = 0.5f + 0.5f * std::sin(pulseTime * 12.f);
            tagY -= drawBadge(rt, font, "BATTLE", Vec(p.x, tagY), S, sf::Color(255, static_cast<sf::Uint8>(110.f + 110.f * beat), 60)) + 2.f * S;
        }
        if (a.sufferingAttrition) tagY -= drawBadge(rt, font, "ATTRITION", Vec(p.x, tagY), S, sf::Color(236, 92, 70)) + 2.f * S;
        if (selected && a.ownerKingdom == player) drawBadge(rt, font, "RIGHT-CLICK TO MARCH", Vec(p.x, tagY), S, pal::Gold);
    }

    window.setView(letterboxView);
}

void SettlementSystem::drawMiniMap(sf::RenderWindow& window, float playerX) {
    using namespace ui;
    Canvas c(window, font);

    const float slide = (1.0f - miniAnimT) * -(layout::Mini.height + 60.f);
    sf::FloatRect frame = layout::Mini;
    sf::FloatRect sea = layout::MiniSea;
    frame.top += slide;
    sea.top += slide;

    c.panel(frame);
    c.text("REALMS", frame.left + 10.f, frame.top + 12.f, {11, theme::Gold, true});
    c.text("TAB to open", frame.left + frame.width - 10.f, frame.top + 12.f, {9, theme::TextMuted, false, Align::Right});
    c.fill(sea, mix(pal::SeaTop, pal::SeaBottom, 0.5f));

    const float scale = std::min((sea.width - 12.f) / mapWorldBounds.width, (sea.height - 12.f) / mapWorldBounds.height);
    const Vec worldCentre(mapWorldBounds.left + mapWorldBounds.width * 0.5f, mapWorldBounds.top + mapWorldBounds.height * 0.5f);
    sf::Transform toMini;
    toMini.translate(sea.left + sea.width * 0.5f, sea.top + sea.height * 0.5f).scale(scale, scale).translate(-worldCentre);
    sf::RenderStates st;
    st.transform = toMini;

    for (auto& county : counties) {
        const sf::Color fill = mapFillColor(county, MapLens::DeFacto);
        for (auto& v : county.mesh) v.color = fill;
        drawVerts(window, county.mesh, st);
    }
    std::vector<sf::Vertex> lines;
    const float px = 1.f / (scale * c.scale());      // one device pixel, in world units
    for (const auto& e : mapEdges) {
        if (e.b < 0) addStroke(lines, e.pts, 1.4f * px, pal::Coast, false);
        else if (counties[e.a].kingdomName != counties[e.b].kingdomName) addStroke(lines, e.pts, 1.4f * px, withAlpha(pal::Ink, 210.f), false);
    }
    drawVerts(window, lines, st);
    c.frame(sea, theme::BronzeDim);

    const Vec you = toMini.transformPoint(getPlayerMapCoord(playerX));
    const float beat = 0.5f + 0.5f * std::sin(pulseTime * 4.5f);
    sf::CircleShape ring(3.f + 4.f * beat, 24);
    ring.setOrigin(ring.getRadius(), ring.getRadius());
    ring.setPosition(you);
    ring.setFillColor(sf::Color::Transparent);
    ring.setOutlineColor(sf::Color(255, 232, 140, static_cast<sf::Uint8>(235.f * (1.f - beat))));
    ring.setOutlineThickness(1.2f);
    window.draw(ring);
    sf::CircleShape pin(2.6f, 16);
    pin.setOrigin(2.6f, 2.6f);
    pin.setPosition(you);
    pin.setFillColor(sf::Color(255, 250, 235));
    pin.setOutlineColor(pal::Ink);
    pin.setOutlineThickness(1.f);
    window.draw(pin);
}

void SettlementSystem::drawMap(sf::RenderWindow& window, const sf::View& letterboxView, float playerX, const sim::SimulationRegistry& registry) {
    (void)registry;
    if (!fontLoaded) return;
    if (miniAnimT < 0.005f && expandAnimT < 0.005f) return;

    window.setView(letterboxView);

    if (expandAnimT <= 0.005f) {
        drawMiniMap(window, playerX);
        return;
    }

    // Paint a little above screen resolution so lines and labels come out smooth once
    // scaled down, and only repaint when something on the map has actually changed.
    const sf::IntRect viewport = window.getViewport(letterboxView);
    const float deviceScale = letterboxView.getSize().x > 0.f ? static_cast<float>(viewport.width) / letterboxView.getSize().x : 1.f;
    const int Si = std::clamp(static_cast<int>(std::ceil(deviceScale * 1.25f - 0.01f)), 2, 4);
    // While the map is being dragged or zoomed it is repainted every frame, so it is
    // painted coarser; the full-quality pass follows as soon as the camera rests.
    const bool moving = isDraggingMap || mapZoom != mapZoomTarget;
    const float S = moving ? std::clamp(deviceScale * 0.67f, 1.f, static_cast<float>(Si)) : static_cast<float>(Si);
    const std::uint64_t signature = mapBaseSignature(S);
    if (!mapCanvasReady || signature != mapBaseSig) {
        renderMapBase(Si, S);
        mapBaseSig = signature;
    }

    // The minimap grows into the full map.
    const float easeT = expandAnimT * expandAnimT * (3.0f - 2.0f * expandAnimT);
    const sf::FloatRect frame = lerpRect(layout::Mini, layout::Frame, easeT);
    const sf::FloatRect canvas = lerpRect(layout::MiniSea, layout::Canvas, easeT);

    sf::RectangleShape backdrop(letterboxView.getSize());
    backdrop.setPosition(letterboxView.getCenter() - letterboxView.getSize() * 0.5f);
    backdrop.setFillColor(sf::Color(8, 7, 6, static_cast<sf::Uint8>(easeT * 190.f)));
    window.draw(backdrop);

    ui::Canvas c(window, font);
    c.panel(frame);

    // Crop the painting to the current shape instead of squashing it.
    const Vec full = mapCanvasUsed;
    Vec crop = full;
    const float aspect = canvas.width / canvas.height;
    if (full.x / full.y > aspect) crop.x = full.y * aspect; else crop.y = full.x / aspect;
    sf::Sprite canvasSprite(mapCanvas.getTexture());
    canvasSprite.setTextureRect(sf::IntRect(static_cast<int>((full.x - crop.x) * 0.5f), static_cast<int>((full.y - crop.y) * 0.5f),
                                            static_cast<int>(crop.x), static_cast<int>(crop.y)));
    canvasSprite.setPosition(canvas.left, canvas.top);
    canvasSprite.setScale(canvas.width / crop.x, canvas.height / crop.y);
    window.draw(canvasSprite);

    // Darkened edges pull the eye to the middle of the map.
    {
        std::vector<sf::Vertex> v;
        const float d = std::min(46.f, canvas.height * 0.2f);
        const sf::Color edge(8, 12, 16, 105), none(8, 12, 16, 0);
        const Vec a(canvas.left, canvas.top), b(canvas.left + canvas.width, canvas.top);
        const Vec e(canvas.left + canvas.width, canvas.top + canvas.height), f(canvas.left, canvas.top + canvas.height);
        addQuad(v, a, b, b + Vec(-d, d), a + Vec(d, d), edge, edge, none, none);
        addQuad(v, b, e, e + Vec(-d, -d), b + Vec(-d, d), edge, edge, none, none);
        addQuad(v, e, f, f + Vec(d, -d), e + Vec(-d, -d), edge, edge, none, none);
        addQuad(v, f, a, a + Vec(d, d), f + Vec(d, -d), edge, edge, none, none);
        drawVerts(window, v);
    }

    if (expandAnimT > 0.995f) drawMapOverlay(window, letterboxView, playerX);
    c.frame(sf::FloatRect(canvas.left - c.px(1), canvas.top - c.px(1), canvas.width + c.px(2), canvas.height + c.px(2)), ui::theme::EdgeDark);

    if (easeT > 0.86f) {
        drawMapChrome(window);
        drawMapDialogs(window);
    }
}

void SettlementSystem::drawMapChrome(sf::RenderWindow& window) {
    using namespace ui;
    Canvas c(window, font);
    const Vec mouse = Canvas::mouse(window);
    const std::string player = getPlayerKingdomId();
    const int lens = static_cast<int>(currentLens);
    const bool realmLevel = mapZoom > ZOOM_REALM;

    // ---- header: title, lens tabs ---------------------------------------------
    const float headTop = layout::Frame.top;
    const float headMid = headTop + layout::HeaderH * 0.5f + 3.f;
    c.text("REALMS OF BRITANNIA", layout::Frame.left + 18.f, headTop + 19.f, {15, theme::Gold, true});
    c.text(LENS_BLURBS[lens], layout::Frame.left + 18.f, headTop + 36.f, {10, theme::TextMuted, false, Align::Left, false, true});

    float tx = layout::Frame.left + 270.f;
    const float tabW = 104.f, tabH = 28.f, tabY = headMid - tabH * 0.5f;
    for (int i = 0; i < 5; ++i) {
        const sf::FloatRect r(tx, tabY, tabW, tabH);
        lensTabBounds[i] = r;
        const bool active = (i == lens);
        c.tab(r, LENS_KEYS[i], LENS_NAMES[i], active, r.contains(mouse));
        tx += tabW + 4.f;
    }

    const bool hasFactions = !independenceFaction.memberCounties.empty();
    const sf::FloatRect lawsTab(tx + 10.f, tabY, 142.f, tabH);
    factionsTabBounds = lawsTab;
    c.tab(lawsTab, "F", "Laws & Factions", factionModalOpen, lawsTab.contains(mouse), hasFactions);

    c.text("TAB / ESC to close", layout::Frame.left + layout::Frame.width - 18.f, headMid, {10, theme::TextMuted, false, Align::Right});

    // ---- side panel -----------------------------------------------------------------
    const sf::FloatRect panel = layout::Panel;
    c.inset(panel);
    const float left = panel.left + 14.f, right = panel.left + panel.width - 14.f;
    float y = panel.top + 18.f;

    auto swatch = [&](float x, float cy, sf::Color col, float size = 11.f) {
        const sf::FloatRect r(x, cy - size * 0.5f, size, size);
        c.fill(r, col);
        c.frame(r, theme::EdgeDark);
    };
    auto rule = [&]() {
        c.fill({left, y, right - left, c.px(1)}, theme::BronzeDim);
        y += 12.f;
    };
    auto heading = [&](const std::string& s) {
        c.text(s, left, y, {10, theme::Bronze, true});
        y += 17.f;
    };
    auto row = [&](const std::string& label, const std::string& value, sf::Color col = theme::Text) {
        c.text(label, left, y, {11, theme::TextMuted});
        c.text(value, right, y, {11, col, true, Align::Right});
        y += 17.f;
    };
    // Comma list wrapped to the panel width.
    auto wrapped = [&](const std::vector<std::string>& items, sf::Color col) {
        std::string line;
        for (size_t i = 0; i < items.size(); ++i) {
            const std::string piece = items[i] + (i + 1 < items.size() ? ", " : "");
            if (!line.empty() && c.textWidth(line + piece, 11) > right - left) {
                c.text(line, left, y, {11, col});
                y += 15.f;
                line.clear();
            }
            line += piece;
        }
        if (!line.empty()) { c.text(line, left, y, {11, col}); y += 15.f; }
        y += 3.f;
    };
    auto relation = [&](const std::string& realm, sf::Color& col) -> std::string {
        if (realm == player) { col = theme::Gold; return "Your realm"; }
        if (activeWar.active && realm == activeWar.enemyKingdom) { col = theme::Bad; return "At war with you"; }
        if (isAllyInWar(realm)) { col = theme::Prestige; return "Fighting beside you"; }
        if (hasTruceWith(realm)) { col = theme::Amber; return "Truce"; }
        if (realm == WorldMapRepository::getInstance().getDefaultAllyKingdom()) { col = theme::Good; return "Ally"; }
        col = theme::TextMuted;
        return "Neutral";
    };
    auto realmCard = [&](const std::string& realm) {
        swatch(left, y, getKingdomColor(realm), 13.f);
        c.text(upper(getKingdomDisplayName(realm)), left + 20.f, y, {13, theme::Gold, true});
        y += 19.f;
        sf::Color relCol;
        const std::string rel = relation(realm, relCol);
        c.text(rel, left, y, {11, relCol, false, Align::Left, false, true});
        y += 20.f;

        std::vector<std::string> held, lost;
        for (const auto& county : counties) {
            if (county.kingdomName == realm) held.push_back(county.countyName);
            else if (county.deJureKingdom == realm) lost.push_back(county.countyName);
        }
        int armies = 0, soldiers = 0;
        for (const auto& a : mapArmies) if (a.ownerKingdom == realm) { ++armies; soldiers += a.strength; }

        row("Counties", std::to_string(held.size()));
        row("Armies in the field", armies > 0 ? std::to_string(armies) + "  (" + std::to_string(soldiers) + " apes)" : "None",
            armies > 0 ? theme::Text : theme::TextMuted);
        if (realm == player) {
            static const char* const tiers[4] = { "Autonomous", "Limited", "High", "Absolute" };
            row("Crown authority", tiers[std::clamp(crownAuthority, 1, 4) - 1]);
        }
        y += 4.f;
        heading("HOLDS");
        wrapped(held, theme::Text);
        if (!lost.empty()) {
            heading("RIGHTFUL LANDS HELD BY OTHERS");
            wrapped(lost, theme::Bad);
        }
    };
    auto countyCard = [&](const CountyDef& county) {
        swatch(left, y, getKingdomColor(county.kingdomName), 13.f);
        c.text(upper(county.countyName), left + 20.f, y, {13, theme::Gold, true});
        y += 19.f;
        std::string seat = "Seat: " + county.settlementName;
        if (!county.modernName.empty() && county.modernName != county.settlementName) seat += "  (" + county.modernName + ")";
        c.text(seat, left, y, {11, theme::TextMuted, false, Align::Left, false, true});
        y += 20.f;

        sf::Color relCol;
        relation(county.kingdomName, relCol);
        row("Held by", getKingdomDisplayName(county.kingdomName), relCol == theme::TextMuted ? theme::Text : relCol);
        const bool usurped = county.kingdomName != county.deJureKingdom;
        row("Rightful realm", getKingdomDisplayName(county.deJureKingdom), usurped ? theme::Amber : theme::Text);
        if (county.kingdomName == player) {
            const std::string op = (county.vassalOpinion >= 0 ? "+" : "") + std::to_string(county.vassalOpinion);
            row("Chieftain's loyalty", op, county.vassalOpinion >= 25 ? theme::Good : (county.vassalOpinion >= 0 ? theme::Gold : theme::Bad));
            row("Levies", county.leviesRaised ? "Raised" : "At home", county.leviesRaised ? theme::Amber : theme::Text);
        }
        static const char* const forts[3] = { "None", "Palisade (I)", "Stone keep (II)" };
        row("Fortification", forts[std::clamp(county.fortTier, 0, 2)], county.fortTier > 0 ? theme::Text : theme::TextMuted);
        row("Supply limit", std::to_string(county.supplyLimit));
        const int troops = getCountyTroops(county.countyName);
        row("Troops present", std::to_string(troops), troops > county.supplyLimit ? theme::Bad : theme::Text);
        if (isCountyOnTradeRoute(county.countyName)) row("Trade road", "Yes", theme::Gold);

        y += 4.f;
        auto note = [&](const std::string& s, sf::Color col) {
            c.text(s, left, y, {11, col, true});
            y += 16.f;
        };
        if (county.isOccupied) note("Occupied by " + county.occupierKingdom, theme::Bad);
        else if (county.siegeProgress > 0.f) note("Under siege: " + std::to_string(static_cast<int>(county.siegeProgress)) + "%", theme::Bad);
        if (county.kingdomName == player && county.inFaction) note("Backing a faction against you", theme::Bad);
        if (getPlayerClaims().count(county.countyName) > 0 && county.kingdomName != player) note("You hold a claim here", theme::Good);
        if (activeWar.active && county.countyName == activeWar.targetCounty) note("This county is the war goal", theme::Amber);
    };

    const bool hovering = hoveredCountyIdx >= 0 && hoveredCountyIdx < static_cast<int>(counties.size());
    if (hovering && !realmLevel) countyCard(counties[hoveredCountyIdx]);
    else if (hovering) realmCard(mapRealmKey(counties[hoveredCountyIdx]));
    else realmCard(player);

    // ---- map key, pinned to the lower part of the panel ---------------------------------
    struct KeyRow { sf::Color col; std::string label; bool hatch; };
    std::vector<KeyRow> key;
    if (currentLens == MapLens::DeFacto || currentLens == MapLens::DeJure) {
        std::vector<std::string> seenRealms;
        for (const auto& county : counties) {
            const std::string& realm = mapRealmKey(county);
            if (std::find(seenRealms.begin(), seenRealms.end(), realm) != seenRealms.end()) continue;
            seenRealms.push_back(realm);
            key.push_back({getKingdomColor(realm), getKingdomDisplayName(realm) + (realm == player ? "  (you)" : ""), false});
        }
        if (currentLens == MapLens::DeJure) key.push_back({theme::Text, "Striped: held by another realm", true});
    } else if (currentLens == MapLens::Vassals) {
        key.push_back({sf::Color(84, 160, 84), "Loyal  (+25 and above)", false});
        key.push_back({sf::Color(214, 176, 66), "Wavering  (0 to +24)", false});
        key.push_back({sf::Color(198, 62, 52), "Disloyal  (below 0)", false});
        key.push_back({pal::Neutral, "Not your land", false});
    } else if (currentLens == MapLens::Diplomacy) {
        key.push_back({sf::Color(66, 126, 204), "Your realm", false});
        key.push_back({sf::Color(70, 176, 204), "Ally in your war", false});
        key.push_back({sf::Color(200, 54, 48), "Enemy and war goal", false});
        key.push_back({sf::Color(206, 178, 110), "Truce", false});
        key.push_back({pal::Neutral, "Neutral", false});
    } else {
        key.push_back({sf::Color(226, 186, 62), "Rich county", false});
        key.push_back({sf::Color(160, 168, 88), "Prosperous county", false});
        key.push_back({sf::Color(142, 110, 84), "Poor county", false});
        key.push_back({sf::Color(255, 226, 120), "Trade road  (pays tolls)", false});
        key.push_back({sf::Color(226, 70, 54), "Raided trade road", false});
    }

    const float controlsH = 84.f;
    const float keyH = 26.f + 16.f * static_cast<float>(key.size());
    y = std::max(y + 8.f, panel.top + panel.height - controlsH - keyH - 12.f);
    rule();
    heading(std::string("MAP KEY: ") + upper(LENS_NAMES[lens]));
    for (const auto& k : key) {
        if (k.hatch) {
            for (int s = 0; s < 3; ++s) c.fill({left + 1.f + 4.f * static_cast<float>(s), y - 5.f, 2.f, 10.f}, k.col);
        } else {
            swatch(left, y, k.col);
        }
        c.text(k.label, left + 19.f, y, {11, theme::Text});
        y += 16.f;
    }

    y = panel.top + panel.height - controlsH;
    rule();
    const char* const controls[4][2] = {
        {"Scroll", "Zoom between realms and counties"},
        {"Drag", "Move the map"},
        {"Left click", "Inspect, or select an army"},
        {"Right click", "Actions, or march the army"}
    };
    for (const auto& line : controls) {
        c.text(line[0], left, y, {10, theme::Gold, true});
        c.text(line[1], left + 70.f, y, {10, theme::TextMuted});
        y += 15.f;
    }

    // ---- name tag at the cursor ------------------------------------------------------------
    const bool dialogOpen = factionModalOpen || successionModalOpen || peaceModalOpen;
    if (hovering && mouseOverMap && !isDraggingMap && !dialogOpen) {
        const CountyDef& county = counties[hoveredCountyIdx];
        Tooltip tip;
        if (realmLevel) {
            const std::string& realm = mapRealmKey(county);
            tip.title = upper(getKingdomDisplayName(realm));
            tip.accent = mix(getKingdomColor(realm), sf::Color::White, 0.45f);
            tip.rows.push_back({"County", county.countyName, theme::Text});
            tip.note = "Zoom in to pick single counties";
        } else {
            tip.title = upper(county.countyName);
            tip.accent = mix(getKingdomColor(county.kingdomName), sf::Color::White, 0.45f);
            tip.rows.push_back({"Realm", getKingdomDisplayName(county.kingdomName), theme::Text});
            if (currentLens == MapLens::DeJure && county.kingdomName != county.deJureKingdom)
                tip.rows.push_back({"Rightfully", getKingdomDisplayName(county.deJureKingdom), theme::Amber});
            if (currentLens == MapLens::Vassals && county.kingdomName == player)
                tip.rows.push_back({"Loyalty", (county.vassalOpinion >= 0 ? "+" : "") + std::to_string(county.vassalOpinion),
                                    county.vassalOpinion >= 0 ? theme::Good : theme::Bad});
            if (currentLens == MapLens::Economy) tip.rows.push_back({"Supply limit", std::to_string(county.supplyLimit), theme::Text});
        }
        c.tooltip(mouse.x, mouse.y + 20.f, tip);
    }
}

// Dialogs that open on top of the map: realm laws and factions, succession, war status and peace terms.
void SettlementSystem::drawMapDialogs(sf::RenderWindow& window) {
    if (factionModalOpen) {
        sf::FloatRect fR(180.f, 112.f, 620.f, 500.f);

        sf::RectangleShape shadow(sf::Vector2f(fR.width + 8.f, fR.height + 8.f));
        shadow.setPosition(fR.left + 4.f, fR.top + 4.f);
        shadow.setFillColor(sf::Color(0, 0, 0, 225));
        window.draw(shadow);

        sf::RectangleShape modal(sf::Vector2f(fR.width, fR.height));
        modal.setPosition(fR.left, fR.top);
        modal.setFillColor(sf::Color(20, 14, 10, 252));
        modal.setOutlineColor(sf::Color(215, 165, 75));
        modal.setOutlineThickness(1.8f);
        window.draw(modal);

        sf::RectangleShape header(sf::Vector2f(fR.width - 6.f, 34.f));
        header.setPosition(fR.left + 3.f, fR.top + 3.f);
        header.setFillColor(sf::Color(44, 26, 18));
        window.draw(header);

        sf::Text mTitle("REALM LAWS & CROWN AUTHORITY", font, 12);
        mTitle.setStyle(sf::Text::Bold);
        mTitle.setFillColor(sf::Color(255, 230, 140));
        mTitle.setPosition(fR.left + 16.f, fR.top + 8.f);
        window.draw(mTitle);

        float curY = fR.top + 44.f;

        sf::Text sec1("CROWN AUTHORITY TIERS", font, 10);
        sec1.setStyle(sf::Text::Bold);
        sec1.setFillColor(sf::Color(235, 195, 120));
        sec1.setPosition(fR.left + 18.f, curY);
        window.draw(sec1);
        curY += 16.f;

        static const std::string tierTitles[4] = { "I. Autonomous", "II. Limited", "III. High", "IV. Absolute" };
        float tBtnW = (fR.width - 36.f - 18.f) / 4.f;
        float tBtnH = 26.f;

        for (int t = 0; t < 4; ++t) {
            float tX = fR.left + 18.f + t * (tBtnW + 6.f);
            authorityTierBounds[t] = sf::FloatRect(tX, curY, tBtnW, tBtnH);

            bool isSelected = (selectedAuthorityTier == t + 1);
            bool isEnacted = (crownAuthority == t + 1);

            sf::RectangleShape btn(sf::Vector2f(tBtnW, tBtnH));
            btn.setPosition(tX, curY);
            if (isSelected) {
                btn.setFillColor(sf::Color(95, 58, 24));
                btn.setOutlineColor(sf::Color(255, 220, 85));
                btn.setOutlineThickness(1.5f);
            } else if (isEnacted) {
                btn.setFillColor(sf::Color(55, 36, 18));
                btn.setOutlineColor(sf::Color(190, 150, 75));
                btn.setOutlineThickness(1.2f);
            } else {
                btn.setFillColor(sf::Color(26, 18, 12));
                btn.setOutlineColor(sf::Color(75, 52, 30));
                btn.setOutlineThickness(1.f);
            }
            window.draw(btn);

            sf::Text bTxt(tierTitles[t], font, 9);
            bTxt.setStyle(isSelected ? sf::Text::Bold : sf::Text::Regular);
            bTxt.setFillColor(isSelected ? sf::Color(255, 240, 190) : (isEnacted ? sf::Color(240, 210, 140) : sf::Color(170, 145, 115)));
            sf::FloatRect btb = bTxt.getLocalBounds();
            bTxt.setOrigin(btb.left + btb.width * 0.5f, btb.top + btb.height * 0.5f);
            bTxt.setPosition(tX + tBtnW * 0.5f, curY + tBtnH * 0.5f);
            window.draw(bTxt);

            if (isEnacted) {
                sf::CircleShape pip(3.f);
                pip.setOrigin(3.f, 3.f);
                pip.setPosition(tX + tBtnW - 7.f, curY + 7.f);
                pip.setFillColor(sf::Color(245, 205, 55));
                window.draw(pip);
            }
        }
        curY += 32.f;

        sf::RectangleShape descBox(sf::Vector2f(fR.width - 36.f, 70.f));
        descBox.setPosition(fR.left + 18.f, curY);
        descBox.setFillColor(sf::Color(28, 18, 14, 230));
        descBox.setOutlineColor(sf::Color(85, 55, 30));
        descBox.setOutlineThickness(1.f);
        window.draw(descBox);

        std::string d1, d2;
        if (selectedAuthorityTier == 1) {
            d1 = "Autonomous: Clans govern locally. Vassal loyalty flourishes, but taxes and levies are low.";
            d2 = "Effects: Vassal Opinion +10 | Levies: 15/county | Taxes: Base | Faction Growth: -45%";
        } else if (selectedAuthorityTier == 2) {
            d1 = "Limited: King arbitrates clan disputes and sets minimum warrior contributions.";
            d2 = "Effects: Vassal Opinion 0 | Levies: 22/county | Taxes: +15% | Faction Growth: Normal";
        } else if (selectedAuthorityTier == 3) {
            d1 = "High: Royal decrees override chieftain councils. High levy quotas cause resentment.";
            d2 = "Effects: Vassal Opinion -12 | Levies: 30/county | Taxes: +30% | Faction Growth: +45%";
        } else {
            d1 = "Absolute Alpha: Total autocracy. Chieftains are subordinates; dissent brews rapidly.";
            d2 = "Effects: Vassal Opinion -25 | Levies: 40/county | Taxes: +50% | Faction Growth: +120%";
        }

        sf::Text tLine1(d1, font, 9);
        tLine1.setFillColor(sf::Color(235, 220, 195));
        tLine1.setPosition(fR.left + 26.f, curY + 6.f);
        window.draw(tLine1);

        sf::Text tLine2(d2, font, 9);
        tLine2.setStyle(sf::Text::Bold);
        tLine2.setFillColor(sf::Color(245, 205, 115));
        tLine2.setPosition(fR.left + 26.f, curY + 22.f);
        window.draw(tLine2);

        bool isCurrentLaw = (selectedAuthorityTier == crownAuthority);
        enactLawBtnBounds = sf::FloatRect(fR.left + fR.width - 190.f, curY + 44.f, 165.f, 20.f);

        sf::RectangleShape enactBtn(sf::Vector2f(enactLawBtnBounds.width, enactLawBtnBounds.height));
        enactBtn.setPosition(enactLawBtnBounds.left, enactLawBtnBounds.top);
        enactBtn.setFillColor(isCurrentLaw ? sf::Color(35, 26, 18) : sf::Color(115, 60, 22));
        enactBtn.setOutlineColor(isCurrentLaw ? sf::Color(90, 65, 40) : sf::Color(245, 195, 60));
        enactBtn.setOutlineThickness(1.f);
        window.draw(enactBtn);

        sf::Text enactTxt(isCurrentLaw ? "ENACTED (Current)" : "ENACT LAW (-100 Prestige)", font, 8);
        enactTxt.setStyle(sf::Text::Bold);
        enactTxt.setFillColor(isCurrentLaw ? sf::Color(160, 135, 105) : sf::Color(255, 235, 175));
        sf::FloatRect etb = enactTxt.getLocalBounds();
        enactTxt.setOrigin(etb.left + etb.width * 0.5f, etb.top + etb.height * 0.5f);
        enactTxt.setPosition(enactLawBtnBounds.left + enactLawBtnBounds.width * 0.5f, enactLawBtnBounds.top + enactLawBtnBounds.height * 0.5f);
        window.draw(enactTxt);

        curY += 76.f;

        if (lawStatusTimer > 0.f && !lawStatusMsg.empty()) {
            sf::Text sMsg(lawStatusMsg, font, 9);
            sMsg.setStyle(sf::Text::Bold);
            sMsg.setFillColor(sf::Color(255, 215, 120));
            sMsg.setPosition(fR.left + 26.f, curY);
            window.draw(sMsg);
            curY += 16.f;
        }

        sf::Vertex sep[] = {
            sf::Vertex(sf::Vector2f(fR.left + 18.f, curY), sf::Color(85, 55, 30)),
            sf::Vertex(sf::Vector2f(fR.left + fR.width - 18.f, curY), sf::Color(85, 55, 30))
        };
        window.draw(sep, 2, sf::Lines);
        curY += 10.f;

        closeFactionModalBounds = sf::FloatRect(fR.left + fR.width - 28.f, fR.top + 7.f, 20.f, 20.f);
        sf::RectangleShape closeBtn(sf::Vector2f(20.f, 20.f));
        closeBtn.setPosition(closeFactionModalBounds.left, closeFactionModalBounds.top);
        closeBtn.setFillColor(sf::Color(140, 25, 20));
        closeBtn.setOutlineColor(sf::Color(240, 200, 75));
        closeBtn.setOutlineThickness(1.f);
        window.draw(closeBtn);

        sf::Text xT("x", font, 12);
        xT.setStyle(sf::Text::Bold);
        xT.setFillColor(sf::Color::White);
        xT.setPosition(closeFactionModalBounds.left + 6.f, closeFactionModalBounds.top + 1.f);
        window.draw(xT);

        if (independenceFaction.memberCounties.empty()) {
            sf::Text calmTxt("There are no active factions threatening your realm. The realm is at peace.", font, 11);
            calmTxt.setStyle(sf::Text::Italic);
            calmTxt.setFillColor(sf::Color(145, 215, 140));
            calmTxt.setPosition(fR.left + 24.f, curY + 40.f);
            window.draw(calmTxt);
        } else {
            sf::Text fName("Faction: " + independenceFaction.name, font, 11);
            fName.setStyle(sf::Text::Bold);
            fName.setFillColor(sf::Color(245, 185, 115));
            fName.setPosition(fR.left + 18.f, curY);
            window.draw(fName);
            curY += 22.f;

            sf::Text discLabel("Discontent Progress: " + std::to_string(static_cast<int>(independenceFaction.discontent)) + "%", font, 10);
            discLabel.setFillColor(sf::Color(230, 210, 180));
            discLabel.setPosition(fR.left + 18.f, curY);
            window.draw(discLabel);

            std::string powStr = "Military Strength: " + std::to_string(static_cast<int>(independenceFaction.powerRatio)) + "% of Liege (Threshold: 60%)";
            sf::Text powTxt(powStr, font, 10);
            powTxt.setStyle(sf::Text::Bold);
            powTxt.setFillColor(independenceFaction.powerRatio >= 60.f ? sf::Color(245, 65, 55) : sf::Color(240, 200, 80));
            sf::FloatRect ptb = powTxt.getLocalBounds();
            powTxt.setPosition(fR.left + fR.width - ptb.width - 24.f, curY);
            window.draw(powTxt);
            curY += 20.f;

            float bW = fR.width - 36.f;
            sf::RectangleShape barBg(sf::Vector2f(bW, 8.f));
            barBg.setPosition(fR.left + 18.f, curY);
            barBg.setFillColor(sf::Color(14, 10, 8, 240));
            barBg.setOutlineColor(sf::Color(65, 45, 30));
            barBg.setOutlineThickness(1.f);
            window.draw(barBg);

            float fillW = bW * (independenceFaction.discontent / 100.f);
            if (fillW > 0.f) {
                sf::RectangleShape barFill(sf::Vector2f(fillW, 8.f));
                barFill.setPosition(fR.left + 18.f, curY);
                barFill.setFillColor(independenceFaction.discontent >= 75.f ? sf::Color(235, 55, 45) : sf::Color(235, 175, 50));
                window.draw(barFill);
            }
            curY += 22.f;

            sf::RectangleShape tblHead(sf::Vector2f(bW, 22.f));
            tblHead.setPosition(fR.left + 18.f, curY);
            tblHead.setFillColor(sf::Color(32, 22, 16));
            window.draw(tblHead);

            sf::Text th1("COUNTY BACKER", font, 9);
            th1.setStyle(sf::Text::Bold);
            th1.setFillColor(sf::Color(245, 215, 140));
            th1.setPosition(fR.left + 26.f, curY + 4.f);
            window.draw(th1);

            sf::Text th2("LEVY CONTRIBUTION", font, 9);
            th2.setStyle(sf::Text::Bold);
            th2.setFillColor(sf::Color(245, 215, 140));
            th2.setPosition(fR.left + 180.f, curY + 4.f);
            window.draw(th2);

            sf::Text th3("OPINION", font, 9);
            th3.setStyle(sf::Text::Bold);
            th3.setFillColor(sf::Color(245, 215, 140));
            th3.setPosition(fR.left + 350.f, curY + 4.f);
            window.draw(th3);

            sf::Text th4("STATUS", font, 9);
            th4.setStyle(sf::Text::Bold);
            th4.setFillColor(sf::Color(245, 215, 140));
            th4.setPosition(fR.left + 450.f, curY + 4.f);
            window.draw(th4);
            curY += 26.f;

            for (size_t m = 0; m < independenceFaction.memberCounties.size(); ++m) {
                const std::string& cName = independenceFaction.memberCounties[m];
                int op = getVassalOpinion(cName);

                sf::RectangleShape row(sf::Vector2f(bW, 30.f));
                row.setPosition(fR.left + 18.f, curY);
                row.setFillColor((m % 2 == 0) ? sf::Color(26, 18, 14, 210) : sf::Color(18, 12, 10, 210));
                row.setOutlineColor(sf::Color(65, 45, 30));
                row.setOutlineThickness(0.8f);
                window.draw(row);

                sf::Text cTxt(cName, font, 10);
                cTxt.setStyle(sf::Text::Bold);
                cTxt.setFillColor(sf::Color::White);
                cTxt.setPosition(fR.left + 26.f, curY + 7.f);
                window.draw(cTxt);

                sf::Text lTxt("~18 Warriors", font, 9);
                lTxt.setFillColor(sf::Color(210, 195, 165));
                lTxt.setPosition(fR.left + 180.f, curY + 8.f);
                window.draw(lTxt);

                sf::Text opTxt(std::to_string(op), font, 10);
                opTxt.setStyle(sf::Text::Bold);
                opTxt.setFillColor(sf::Color(245, 80, 70));
                opTxt.setPosition(fR.left + 350.f, curY + 7.f);
                window.draw(opTxt);

                sf::Text stTxt("Rebellious", font, 9);
                stTxt.setStyle(sf::Text::Bold);
                stTxt.setFillColor(sf::Color(245, 145, 60));
                stTxt.setPosition(fR.left + 450.f, curY + 8.f);
                window.draw(stTxt);

                curY += 34.f;
            }

            sf::Text adviceTxt("Tip: Right-click rebel counties on the map and select 'Sway Chieftain' to restore loyalty.", font, 9);
            adviceTxt.setStyle(sf::Text::Italic);
            adviceTxt.setFillColor(sf::Color(175, 155, 125));
            adviceTxt.setPosition(fR.left + 20.f, fR.top + fR.height - 30.f);
            window.draw(adviceTxt);
        }
    }

    if (shortReignTimer > 0.f && !successionModalOpen) {
        sf::RectangleShape srBadge(sf::Vector2f(210.f, 22.f));
        srBadge.setPosition(62.f, 106.f);
        srBadge.setFillColor(sf::Color(24, 16, 12, 235));
        srBadge.setOutlineColor(sf::Color(225, 120, 50));
        srBadge.setOutlineThickness(1.f);
        window.draw(srBadge);

        sf::Text srTxt("SHORT REIGN: -15 Vassal Loyalty", font, 9);
        srTxt.setStyle(sf::Text::Bold);
        srTxt.setFillColor(sf::Color(255, 185, 120));
        srTxt.setPosition(70.f, 110.f);
        window.draw(srTxt);
    }

    if (successionModalOpen) {
        sf::FloatRect sR(200.f, 170.f, 580.f, 410.f);

        sf::RectangleShape shadow(sf::Vector2f(sR.width + 8.f, sR.height + 8.f));
        shadow.setPosition(sR.left + 4.f, sR.top + 4.f);
        shadow.setFillColor(sf::Color(0, 0, 0, 225));
        window.draw(shadow);

        sf::RectangleShape modal(sf::Vector2f(sR.width, sR.height));
        modal.setPosition(sR.left, sR.top);
        modal.setFillColor(sf::Color(22, 15, 10, 252));
        modal.setOutlineColor(sf::Color(215, 170, 75));
        modal.setOutlineThickness(2.f);
        window.draw(modal);

        sf::RectangleShape header(sf::Vector2f(sR.width - 6.f, 38.f));
        header.setPosition(sR.left + 3.f, sR.top + 3.f);
        header.setFillColor(sf::Color(48, 28, 16));
        window.draw(header);

        sf::Text mTitle("THE KING IS DEAD - CONFEDERATE PARTITION", font, 12);
        mTitle.setStyle(sf::Text::Bold);
        mTitle.setFillColor(sf::Color(255, 230, 140));
        mTitle.setPosition(sR.left + 16.f, sR.top + 10.f);
        window.draw(mTitle);

        closeSuccessionModalBounds = sf::FloatRect(sR.left + sR.width - 28.f, sR.top + 8.f, 20.f, 20.f);
        sf::RectangleShape closeBtn(sf::Vector2f(20.f, 20.f));
        closeBtn.setPosition(closeSuccessionModalBounds.left, closeSuccessionModalBounds.top);
        closeBtn.setFillColor(sf::Color(140, 25, 20));
        closeBtn.setOutlineColor(sf::Color(240, 200, 75));
        closeBtn.setOutlineThickness(1.f);
        window.draw(closeBtn);

        sf::Text xT("x", font, 12);
        xT.setStyle(sf::Text::Bold);
        xT.setFillColor(sf::Color::White);
        xT.setPosition(closeSuccessionModalBounds.left + 6.f, closeSuccessionModalBounds.top + 1.f);
        window.draw(xT);

        float curY = sR.top + 48.f;

        sf::Text passingTxt("The reign of " + deceasedKingTitle + " has ended. The council acknowledges " + successorTitle + " as Alpha.", font, 10);
        passingTxt.setStyle(sf::Text::Italic);
        passingTxt.setFillColor(sf::Color(210, 195, 165));
        passingTxt.setPosition(sR.left + 18.f, curY);
        window.draw(passingTxt);
        curY += 22.f;

        sf::Text lawTxt("Succession Law: Realm lands partitioned among eligible heirs.", font, 9);
        lawTxt.setFillColor(sf::Color(185, 165, 130));
        lawTxt.setPosition(sR.left + 18.f, curY);
        window.draw(lawTxt);
        curY += 22.f;

        sf::RectangleShape divHeader(sf::Vector2f(sR.width - 36.f, 22.f));
        divHeader.setPosition(sR.left + 18.f, curY);
        divHeader.setFillColor(sf::Color(35, 24, 16));
        window.draw(divHeader);

        sf::Text cHead("COUNTY", font, 9);
        cHead.setStyle(sf::Text::Bold);
        cHead.setFillColor(sf::Color(245, 215, 140));
        cHead.setPosition(sR.left + 26.f, curY + 4.f);
        window.draw(cHead);

        sf::Text hHead("INHERITOR", font, 9);
        hHead.setStyle(sf::Text::Bold);
        hHead.setFillColor(sf::Color(245, 215, 140));
        hHead.setPosition(sR.left + 140.f, curY + 4.f);
        window.draw(hHead);

        sf::Text sHead("DIVISION STATUS", font, 9);
        sHead.setStyle(sf::Text::Bold);
        sHead.setFillColor(sf::Color(245, 215, 140));
        sHead.setPosition(sR.left + 380.f, curY + 4.f);
        window.draw(sHead);
        curY += 26.f;

        for (size_t e = 0; e < successionEntries.size(); ++e) {
            const auto& item = successionEntries[e];
            sf::RectangleShape row(sf::Vector2f(sR.width - 36.f, 32.f));
            row.setPosition(sR.left + 18.f, curY);
            row.setFillColor((e % 2 == 0) ? sf::Color(26, 18, 14, 210) : sf::Color(18, 12, 10, 210));
            row.setOutlineColor(sf::Color(65, 45, 30));
            row.setOutlineThickness(0.8f);
            window.draw(row);

            sf::Text cTxt(item.countyName, font, 10);
            cTxt.setStyle(sf::Text::Bold);
            cTxt.setFillColor(sf::Color(255, 245, 220));
            cTxt.setPosition(sR.left + 26.f, curY + 8.f);
            window.draw(cTxt);

            sf::Text hTxt(item.heirName, font, 9);
            hTxt.setFillColor(sf::Color(215, 200, 175));
            hTxt.setPosition(sR.left + 140.f, curY + 4.f);
            window.draw(hTxt);

            sf::Text tTxt(item.titleType, font, 8);
            tTxt.setStyle(sf::Text::Italic);
            tTxt.setFillColor(sf::Color(165, 145, 120));
            tTxt.setPosition(sR.left + 140.f, curY + 17.f);
            window.draw(tTxt);

            sf::Text stTxt(item.status, font, 9);
            stTxt.setStyle(sf::Text::Bold);
            stTxt.setFillColor(item.statusColor);
            stTxt.setPosition(sR.left + 380.f, curY + 8.f);
            window.draw(stTxt);

            curY += 36.f;
        }

        confirmSuccessionBtnBounds = sf::FloatRect(sR.left + 180.f, sR.top + sR.height - 46.f, 220.f, 32.f);
        sf::RectangleShape confBtn(sf::Vector2f(confirmSuccessionBtnBounds.width, confirmSuccessionBtnBounds.height));
        confBtn.setPosition(confirmSuccessionBtnBounds.left, confirmSuccessionBtnBounds.top);
        confBtn.setFillColor(sf::Color(55, 35, 20));
        confBtn.setOutlineColor(sf::Color(235, 195, 75));
        confBtn.setOutlineThickness(1.5f);
        window.draw(confBtn);

        sf::Text bLabel("LONG LIVE THE ALPHA", font, 10);
        bLabel.setStyle(sf::Text::Bold);
        bLabel.setFillColor(sf::Color(255, 235, 160));
        sf::FloatRect blb = bLabel.getLocalBounds();
        bLabel.setOrigin(blb.left + blb.width * 0.5f, blb.top + blb.height * 0.5f);
        bLabel.setPosition(confirmSuccessionBtnBounds.left + confirmSuccessionBtnBounds.width * 0.5f,
                           confirmSuccessionBtnBounds.top + confirmSuccessionBtnBounds.height * 0.5f);
        window.draw(bLabel);
    }

    if (activeWar.active) {
        using namespace ui;
        Canvas c(window, font);
        const Vec mouse = Canvas::mouse(window);
        const sf::FloatRect b = warBadgeBounds;
        const bool winning = activeWar.warScore >= 75.f;

        c.panel(b);
        const float beat = 0.5f + 0.5f * std::sin(pulseTime * 6.f);
        c.fill({b.left + 10.f, b.top + 9.f, 7.f, 7.f}, sf::Color(232, 62, 48, static_cast<sf::Uint8>(150.f + 105.f * beat)));
        c.text("WAR FOR " + upper(activeWar.targetCounty), b.left + 24.f, b.top + 13.f, {11, theme::Gold, true});
        c.text("against " + activeWar.enemyKingdom, b.left + b.width - 10.f, b.top + 13.f, {10, theme::TextMuted, false, Align::Right, false, true});

        const std::string score = (activeWar.warScore >= 0.f ? "+" : "") + std::to_string(static_cast<int>(std::round(activeWar.warScore))) + "%";
        c.text(score, b.left + 10.f, b.top + 39.f, {15, winning ? theme::Good : theme::Amber, true});
        c.text("war score", b.left + 10.f + c.textWidth(score, 15, true) + 6.f, b.top + 40.f, {9, theme::TextMuted});

        const std::string allyK = WorldMapRepository::getInstance().getDefaultAllyKingdom();
        const bool allyJoined = !allyK.empty() && isAllyInWar(allyK);
        const sf::FloatRect peaceBtn(b.left + b.width - 104.f, b.top + 27.f, 96.f, 24.f);
        callAllyBtnBounds = sf::FloatRect(peaceBtn.left - 122.f, peaceBtn.top, 118.f, 24.f);

        c.button(callAllyBtnBounds, allyJoined, !allyJoined && !allyK.empty() && callAllyBtnBounds.contains(mouse));
        const std::string allyLabel = allyK.empty() ? "No allies" : (allyJoined ? allyK + " fights" : "Call " + allyK);
        c.text(allyLabel, callAllyBtnBounds.left + callAllyBtnBounds.width * 0.5f, callAllyBtnBounds.top + 12.f,
               {10, allyK.empty() ? theme::TextMuted : (allyJoined ? theme::Prestige : theme::Text), true, Align::Center});

        c.button(peaceBtn, peaceModalOpen, peaceBtn.contains(mouse));
        c.text("Sue for peace", peaceBtn.left + peaceBtn.width * 0.5f, peaceBtn.top + 12.f, {10, theme::Text, true, Align::Center});
    }

    if (callAllyStatusTimer > 0.f && !callAllyStatusMsg.empty()) {
        sf::RectangleShape toast(sf::Vector2f(490.f, 26.f));
        toast.setPosition(245.f, 106.f);
        toast.setFillColor(sf::Color(18, 24, 32, 250));
        toast.setOutlineColor(sf::Color(100, 215, 255));
        toast.setOutlineThickness(1.2f);
        window.draw(toast);

        sf::Text tTxt(callAllyStatusMsg, font, 10);
        tTxt.setStyle(sf::Text::Bold);
        tTxt.setFillColor(sf::Color(220, 245, 255));
        sf::FloatRect tb = tTxt.getLocalBounds();
        tTxt.setOrigin(tb.left + tb.width * 0.5f, tb.top + tb.height * 0.5f);
        tTxt.setPosition(490.f, 119.f);
        window.draw(tTxt);
    }

    if (activeWar.active && peaceModalOpen) {
        sf::FloatRect mR(240.f, 210.f, 500.f, 335.f);

        sf::RectangleShape shadow(sf::Vector2f(mR.width + 6.f, mR.height + 6.f));
        shadow.setPosition(mR.left + 3.f, mR.top + 3.f);
        shadow.setFillColor(sf::Color(0, 0, 0, 210));
        window.draw(shadow);

        sf::RectangleShape modal(sf::Vector2f(mR.width, mR.height));
        modal.setPosition(mR.left, mR.top);
        modal.setFillColor(sf::Color(22, 16, 12, 252));
        modal.setOutlineColor(sf::Color(195, 150, 70));
        modal.setOutlineThickness(1.8f);
        window.draw(modal);

        sf::RectangleShape header(sf::Vector2f(mR.width - 6.f, 32.f));
        header.setPosition(mR.left + 3.f, mR.top + 3.f);
        header.setFillColor(sf::Color(46, 30, 20));
        window.draw(header);

        sf::Text title("PEACE TREATY: WAR FOR " + activeWar.targetCounty, font, 12);
        title.setStyle(sf::Text::Bold);
        title.setFillColor(sf::Color(255, 230, 140));
        title.setPosition(mR.left + 14.f, mR.top + 8.f);
        window.draw(title);

        closePeaceModalBounds = sf::FloatRect(mR.left + mR.width - 26.f, mR.top + 6.f, 20.f, 20.f);
        sf::RectangleShape closeBtn(sf::Vector2f(20.f, 20.f));
        closeBtn.setPosition(closePeaceModalBounds.left, closePeaceModalBounds.top);
        closeBtn.setFillColor(sf::Color(140, 25, 20));
        closeBtn.setOutlineColor(sf::Color(240, 200, 75));
        closeBtn.setOutlineThickness(1.f);
        window.draw(closeBtn);

        sf::Text xT("x", font, 12);
        xT.setStyle(sf::Text::Bold);
        xT.setFillColor(sf::Color::White);
        xT.setPosition(closePeaceModalBounds.left + 6.f, closePeaceModalBounds.top + 1.f);
        window.draw(xT);

        float curY = mR.top + 42.f;
        sf::Text sub("Casus Belli: " + activeWar.casusBelli, font, 10);
        sub.setFillColor(sf::Color(195, 180, 150));
        sub.setPosition(mR.left + 16.f, curY);
        window.draw(sub);
        curY += 20.f;

        sf::RectangleShape breakdownBox(sf::Vector2f(mR.width - 32.f, 44.f));
        breakdownBox.setPosition(mR.left + 16.f, curY);
        breakdownBox.setFillColor(sf::Color(14, 10, 8, 235));
        breakdownBox.setOutlineColor(sf::Color(90, 65, 40));
        breakdownBox.setOutlineThickness(1.f);
        window.draw(breakdownBox);

        std::string scStr = "+" + std::to_string(static_cast<int>(std::round(activeWar.warScore))) + "%";
        sf::Text scH("Current War Score: " + scStr, font, 12);
        scH.setStyle(sf::Text::Bold);
        scH.setFillColor(activeWar.warScore >= 75.f ? sf::Color(105, 240, 105) : sf::Color(245, 195, 65));
        scH.setPosition(breakdownBox.getPosition().x + 10.f, breakdownBox.getPosition().y + 6.f);
        window.draw(scH);

        sf::Text dtTxt(activeWar.warScore >= 75.f ? "Superiority established. Demands can be fully enforced!"
                                                  : "War score progressing toward requirement (+75%)...", font, 9);
        dtTxt.setFillColor(sf::Color(185, 170, 140));
        dtTxt.setPosition(breakdownBox.getPosition().x + 10.f, breakdownBox.getPosition().y + 24.f);
        window.draw(dtTxt);
        curY += 56.f;

        auto drawPeaceOption = [&](sf::FloatRect& bounds, const std::string& optTitle, const std::string& desc, bool enabled, sf::Color col) {
            bounds = sf::FloatRect(mR.left + 16.f, curY, mR.width - 32.f, 50.f);

            sf::RectangleShape opt(sf::Vector2f(bounds.width, bounds.height));
            opt.setPosition(bounds.left, bounds.top);
            opt.setFillColor(enabled ? sf::Color(32, 22, 16, 240) : sf::Color(18, 14, 12, 170));
            opt.setOutlineColor(enabled ? col : sf::Color(65, 50, 40));
            opt.setOutlineThickness(1.2f);
            window.draw(opt);

            sf::RectangleShape bar(sf::Vector2f(4.f, bounds.height));
            bar.setPosition(bounds.left, bounds.top);
            bar.setFillColor(enabled ? col : sf::Color(75, 60, 50));
            window.draw(bar);

            sf::Text oT(optTitle, font, 11);
            oT.setStyle(sf::Text::Bold);
            oT.setFillColor(enabled ? sf::Color::White : sf::Color(130, 120, 110));
            oT.setPosition(bounds.left + 12.f, bounds.top + 6.f);
            window.draw(oT);

            sf::Text oD(desc, font, 9);
            oD.setStyle(sf::Text::Italic);
            oD.setFillColor(enabled ? sf::Color(185, 170, 145) : sf::Color(100, 95, 90));
            oD.setPosition(bounds.left + 12.f, bounds.top + 24.f);
            window.draw(oD);

            curY += 56.f;
        };

        bool canEnforce = (activeWar.warScore >= 75.f);
        drawPeaceOption(enforceBtnBounds, "1. Enforce Demands",
                        canEnforce ? ("Annex " + activeWar.targetCounty + " into " + activeWar.attackerKingdom + " and sign a 5-year truce.")
                                   : "Requires at least +75% War Score.",
                        canEnforce, sf::Color(90, 225, 90));

        drawPeaceOption(whitePeaceBtnBounds, "2. White Peace",
                        "Status quo ante bellum. Borders remain unchanged. 3-year truce.",
                        true, sf::Color(225, 195, 75));

        drawPeaceOption(surrenderBtnBounds, "3. Surrender",
                        "Concede defeat, renounce claim, and pay reparations.",
                        true, sf::Color(235, 65, 65));
    }
}
