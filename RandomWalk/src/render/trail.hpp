#pragma once
// -----------------------------------------------------------------------------
// Fading trail (render concern only). A pixel heatmap keyed by quantized
// (display-space) coordinates, so it is independent of the model and works for
// periodic/reflective boundaries. For the `Free` boundary the trail is simply
// not recorded by the caller (positions are unbounded).
// -----------------------------------------------------------------------------
#include <SDL3/SDL.h>
#include <cstdint>
#include <unordered_map>

#include "../model/random-walk.hpp"
#include "../circle.hpp"
#include "view.hpp"

namespace rw
{

struct TrailCell
{
    double size{1.0};
    std::uint64_t walkerID{0};
    std::uint64_t walkerAge{0};
    unsigned short R{255}, G{255}, B{255};
    double x{0.0}, y{0.0}; // display-space world coords (for the view transform)
};

class TrailManager
{
private:
    std::unordered_map<std::uint64_t, TrailCell> trailMap;
    std::uint64_t Width;
    std::uint64_t Height;
    std::uint64_t maxAge;

    [[nodiscard]] std::uint64_t ToKey(double x, double y) const noexcept
    {
        const std::uint64_t ux = static_cast<std::uint64_t>(std::max(0.0, std::min(x, static_cast<double>(Width - 1))));
        const std::uint64_t uy = static_cast<std::uint64_t>(std::max(0.0, std::min(y, static_cast<double>(Height - 1))));
        return ux + uy * Width;
    }

public:
    TrailManager(std::uint64_t w, std::uint64_t h, std::uint64_t ma)
        : Width(w), Height(h), maxAge(ma ? ma : 1) {}

    void Step()
    {
        for (auto& [key, cell] : trailMap) (void)key, ++cell.walkerAge;
        for (auto it = trailMap.begin(); it != trailMap.end();)
        {
            if (it->second.walkerAge > maxAge) it = trailMap.erase(it);
            else ++it;
        }
    }

    void Record(std::uint64_t walkerId, double x, double y, double size,
                unsigned short r, unsigned short g, unsigned short b)
    {
        const std::uint64_t key = ToKey(x, y);
        TrailCell cell;
        cell.size     = size;
        cell.walkerID = walkerId;
        cell.walkerAge = 0;
        cell.R = r; cell.G = g; cell.B = b;
        cell.x = x; cell.y = y;
        trailMap[key] = cell;
    }

    void Draw(SDL_Renderer* renderer, const ViewTransform& view)
    {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        for (const auto& [key, cell] : trailMap)
        {
            (void)key;
            const float fade = 1.0f - static_cast<float>(cell.walkerAge) / static_cast<float>(maxAge);
            const Uint8 alpha = static_cast<Uint8>(std::clamp(fade * 255.0f, 0.0f, 255.0f));
            SDL_SetRenderDrawColor(renderer, cell.R, cell.G, cell.B, alpha);

            const float sx = static_cast<float>(MapX(view, cell.x));
            const float sy = static_cast<float>(MapY(view, cell.y));
            const float size = static_cast<float>(cell.size); // marker size in px
            if (size <= 1.0f)
                SDL_RenderPoint(renderer, sx, sy);
            else
                RenderFilledCircle(renderer, sx, sy, size * 0.5f, 100, ColorF{1.0f, 1.0f, 1.0f, 1.0f});
        }
    }
};

} // namespace rw
