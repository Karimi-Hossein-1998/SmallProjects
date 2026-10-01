#pragma once
// -----------------------------------------------------------------------------
// SDL renderer for the 3D MD engine (demo-only; the EMMA port renders via raylib).
// Uses a perspective orbit camera and per-species / per-speed colours.
// -----------------------------------------------------------------------------
#include <SDL3/SDL_render.h>
#include <algorithm>
#include <cmath>

#include "engine3d.hpp"
#include "circle.hpp"

// Palette for the (up to two) species.
inline Color SpeciesColor3D(int species)
{
    static const Color palette[] = {
        Color(255, 205,  60, 255),   // species 0: gold
        Color( 60, 200, 255, 255)    // species 1: cyan
    };
    return palette[species & 1];
}

inline Color SpeedColor3D(double speedNorm)
{
    const double t = std::clamp(speedNorm, 0.0, 1.0);
    return Color(static_cast<unsigned short>(60 + t * 195.0),
                 static_cast<unsigned short>(140 + t * 60.0),
                 static_cast<unsigned short>(255 - t * 215.0), 255);
}

struct MDCamera3D
{
    double yaw = 0.6, pitch = 0.35, distance = 60.0;
    double targetX = 0, targetY = 0, targetZ = 0;
    double fov = 1.0471975511965976; // 60 deg
};

inline bool Project3D(const MDCamera3D& cam, double x, double y, double z,
                      double sw, double sh, double& sx, double& sy, double& depth)
{
    const double cp = std::cos(cam.pitch), sp = std::sin(cam.pitch);
    const double cy = std::cos(cam.yaw),   syaw = std::sin(cam.yaw);
    const double ex = cam.targetX + cam.distance * cp * syaw;
    const double ey = cam.targetY + cam.distance * sp;
    const double ez = cam.targetZ + cam.distance * cp * cy;

    double fx = cam.targetX - ex, fy = cam.targetY - ey, fz = cam.targetZ - ez;
    double fl = std::sqrt(fx * fx + fy * fy + fz * fz); if (fl <= 0) fl = 1.0;
    fx /= fl; fy /= fl; fz /= fl;

    double rx = -fz, ry = 0.0, rz = fx;
    double rl = std::sqrt(rx * rx + rz * rz); if (rl <= 0) { rx = 1.0; rz = 0.0; rl = 1.0; }
    rx /= rl; rz /= rl;

    const double ux = ry * fz - rz * fy;
    const double uy = rz * fx - rx * fz;
    const double uz = rx * fy - ry * fx;

    const double dx = x - ex, dy = y - ey, dz = z - ez;
    const double xc = dx * rx + dy * ry + dz * rz;
    const double yc = dx * ux + dy * uy + dz * uz;
    const double zc = dx * fx + dy * fy + dz * fz;
    if (zc < 1e-3) return false;

    const double f = (sh * 0.5) / std::tan(cam.fov * 0.5);
    sx = sw * 0.5 + xc * f / zc;
    sy = sh * 0.5 - yc * f / zc;
    depth = zc;
    return true;
}

inline void Orbit3D(MDCamera3D& cam, double dYaw, double dPitch, double dDist)
{
    cam.yaw += dYaw;
    cam.pitch = std::clamp(cam.pitch + dPitch, -1.5, 1.5);
    cam.distance = std::clamp(cam.distance * (1.0 + dDist), 1.0, 1e7);
}

inline void DrawParticles3D(SDL_Renderer* r, MolecularDynamics3D& md, const MDCamera3D& cam,
                            double sw, double sh, bool colorBySpeed = false)
{
    md.ComputeKineticEnergy();
    const double vScale = std::sqrt(std::max(1e-12, md.currentTemperature));
    const double f = (sh * 0.5) / std::tan(cam.fov * 0.5);

    for (size_t i = 0; i < md.numParticles; ++i)
    {
        double sx, sy, depth;
        if (!Project3D(cam, md.posX[i], md.posY[i], md.posZ[i], sw, sh, sx, sy, depth)) continue;

        Color c = SpeciesColor3D(md.species[i]);
        if (colorBySpeed)
        {
            const double v2 = md.velX[i] * md.velX[i] + md.velY[i] * md.velY[i] + md.velZ[i] * md.velZ[i];
            c = SpeedColor3D(std::sqrt(v2) / (3.0 * vScale));
        }

        // Perspective-correct screen radius (world radius * focal / depth).
        const float size = static_cast<float>(std::max(md.radius[i] * f / depth, 0.7));
        SDL_SetRenderDrawColor(r, c.GetR(), c.GetG(), c.GetB(), c.GetA());
        if (size <= 1.0f) SDL_RenderPoint(r, static_cast<float>(sx), static_cast<float>(sy));
        else RenderCircle(r, static_cast<float>(sx), static_cast<float>(sy), size,
                          ColorF(c.GetR(), c.GetG(), c.GetB(), c.GetA()),
                          std::max(size * 0.15f, 1.0f), 16);
    }
}
