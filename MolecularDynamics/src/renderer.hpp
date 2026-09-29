#pragma once
// -----------------------------------------------------------------------------
// SDL renderer for the MD engine (demo-only; the EMMA port renders via raylib).
// -----------------------------------------------------------------------------
#include <SDL3/SDL_render.h>
#include <algorithm>
#include <cmath>
#include <vector>

#include "engine.hpp"
#include "circle.hpp"

// Palette for the (up to two) species.
inline Color SpeciesColor(int species)
{
    static const Color palette[] = {
        Color(255, 205,  60, 255),   // species 0: gold
        Color( 60, 200, 255, 255)    // species 1: cyan
    };
    return palette[species & 1];
}

// Lerp a blue -> red gradient by normalized speed (cold = slow, hot = fast).
inline Color SpeedColor(double speedNorm)
{
    const double t = std::clamp(speedNorm, 0.0, 1.0);
    const unsigned short r = static_cast<unsigned short>(60  + t * 195.0);
    const unsigned short g = static_cast<unsigned short>(140 + t * 60.0);
    const unsigned short b = static_cast<unsigned short>(255 - t * 215.0);
    return Color(r, g, b, 255);
}

inline void DrawParticles(SDL_Renderer* renderer, MolecularDynamics& md,
                          bool colorBySpeed = false)
{
    // Normalise the speed scale by the instantaneous temperature.
    md.ComputeKineticEnergy();
    const double vScale = std::sqrt(std::max(1e-12, md.currentTemperature));

    for (size_t i = 0; i < md.numParticles; ++i)
    {
        Color c = SpeciesColor(md.species[i]);
        if (colorBySpeed)
        {
            const double v2 = md.velX[i] * md.velX[i] + md.velY[i] * md.velY[i];
            c = SpeedColor(std::sqrt(v2) / (3.0 * vScale));
        }

        const float size = static_cast<float>(std::max(md.radius[i], 0.6));
        SDL_SetRenderDrawColor(renderer, c.GetR(), c.GetG(), c.GetB(), c.GetA());
        if (size <= 1.0f)
        {
            SDL_RenderPoint(renderer, static_cast<float>(md.posX[i]),
                            static_cast<float>(md.height - md.posY[i]));
        }
        else
        {
            RenderCircle(renderer, static_cast<float>(md.posX[i]),
                         static_cast<float>(md.height - md.posY[i]), size,
                         ColorF(c.GetR(), c.GetG(), c.GetB(), c.GetA()),
                         std::max(size * 0.15f, 1.0f), 24);
        }
    }
}
