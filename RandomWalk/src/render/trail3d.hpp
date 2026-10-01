#pragma once
// -----------------------------------------------------------------------------
// Fading trail as a 3D voxel "cloud". Same idea as the 2D trail heatmap: keyed
// by quantized (display-space) voxel coordinates, aged each step, drawn as
// projected points with fading alpha.
// -----------------------------------------------------------------------------
#include <SDL3/SDL.h>
#include <algorithm>
#include <cstdint>
#include <unordered_map>

#include "../model/random-walk3d.hpp"
#include "camera3d.hpp"

namespace rw
{

struct TrailCell3D
{
    double size{1.0};
    std::uint64_t walkerID{0};
    std::uint64_t walkerAge{0};
    unsigned short R{255}, G{255}, B{255};
    double x{0.0}, y{0.0}, z{0.0};
};

class TrailManager3D
{
private:
    std::unordered_map<std::uint64_t, TrailCell3D> trailMap;
    std::uint64_t W, H, D, maxAge;

    [[nodiscard]] std::uint64_t ToKey(double x, double y, double z) const noexcept
    {
        const std::uint64_t ux = static_cast<std::uint64_t>(std::max(0.0, std::min(x, static_cast<double>(W - 1))));
        const std::uint64_t uy = static_cast<std::uint64_t>(std::max(0.0, std::min(y, static_cast<double>(H - 1))));
        const std::uint64_t uz = static_cast<std::uint64_t>(std::max(0.0, std::min(z, static_cast<double>(D - 1))));
        return ux + uy * W + uz * W * H;
    }

public:
    TrailManager3D(std::uint64_t w, std::uint64_t h, std::uint64_t d, std::uint64_t ma)
        : W(w), H(h), D(d), maxAge(ma ? ma : 1) {}

    void Step()
    {
        for (auto& [key, cell] : trailMap) (void)key, ++cell.walkerAge;
        for (auto it = trailMap.begin(); it != trailMap.end();)
        {
            if (it->second.walkerAge > maxAge) it = trailMap.erase(it);
            else ++it;
        }
    }

    void Record(std::uint64_t walkerId, double x, double y, double z, double size,
                unsigned short r, unsigned short g, unsigned short b)
    {
        const std::uint64_t key = ToKey(x, y, z);
        TrailCell3D cell;
        cell.size = size; cell.walkerID = walkerId; cell.walkerAge = 0;
        cell.R = r; cell.G = g; cell.B = b;
        cell.x = x; cell.y = y; cell.z = z;
        trailMap[key] = cell;
    }

    void Draw(SDL_Renderer* renderer, const Camera3D& cam, double screenW, double screenH)
    {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        for (const auto& [key, cell] : trailMap)
        {
            (void)key;
            const float fade = 1.0f - static_cast<float>(cell.walkerAge) / static_cast<float>(maxAge);
            const Uint8 alpha = static_cast<Uint8>(std::clamp(fade * 255.0f, 0.0f, 255.0f));
            SDL_SetRenderDrawColor(renderer, cell.R, cell.G, cell.B, alpha);
            double sx, sy;
            if (Project(cam, cell.x, cell.y, cell.z, screenW, screenH, sx, sy))
                SDL_RenderPoint(renderer, static_cast<float>(sx), static_cast<float>(sy));
        }
    }
};

} // namespace rw
