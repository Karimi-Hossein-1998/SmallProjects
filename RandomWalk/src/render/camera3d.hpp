#pragma once
// -----------------------------------------------------------------------------
// Perspective orbit camera + projection.
//
// `Project` maps a 3D world point to 2D screen coordinates using a look-at
// basis and a perspective projection. `Orbit` adjusts the yaw/pitch/distance,
// which is what the realtime key controls call each frame.
// -----------------------------------------------------------------------------
#include <algorithm>
#include <cmath>

namespace rw
{

struct Camera3D
{
    double yaw      = 0.6;   // radians (horizontal orbit)
    double pitch    = 0.35;  // radians (elevation, clamped to avoid gimbal lock)
    double distance = 60.0;
    double targetX  = 0.0, targetY = 0.0, targetZ = 0.0;
    double fov      = 1.0471975511965976; // 60 degrees
};

// Projects (x,y,z) -> (sx,sy). Returns false if the point is behind the camera.
inline bool Project(const Camera3D& cam, double x, double y, double z,
                    double screenW, double screenH, double& sx, double& sy)
{
    const double cp = std::cos(cam.pitch), sp = std::sin(cam.pitch);
    const double cy = std::cos(cam.yaw),   syaw = std::sin(cam.yaw);

    const double ex = cam.targetX + cam.distance * cp * syaw;
    const double ey = cam.targetY + cam.distance * sp;
    const double ez = cam.targetZ + cam.distance * cp * cy;

    // forward = normalize(target - eye)
    double fx = cam.targetX - ex, fy = cam.targetY - ey, fz = cam.targetZ - ez;
    double fl = std::sqrt(fx * fx + fy * fy + fz * fz);
    if (fl <= 0.0) fl = 1.0;
    fx /= fl; fy /= fl; fz /= fl;

    // right = normalize(cross(forward, up=(0,1,0)))
    double rx = -fz, ry = 0.0, rz = fx;
    double rl = std::sqrt(rx * rx + ry * ry + rz * rz);
    if (rl <= 0.0) { rx = 1.0; rz = 0.0; rl = 1.0; }
    rx /= rl; rz /= rl;

    // upv = cross(right, forward)
    const double ux = ry * fz - rz * fy;
    const double uy = rz * fx - rx * fz;
    const double uz = rx * fy - ry * fx;

    const double dx = x - ex, dy = y - ey, dz = z - ez;
    const double xc = dx * rx + dy * ry + dz * rz;
    const double yc = dx * ux + dy * uy + dz * uz;
    const double zc = dx * fx + dy * fy + dz * fz; // depth along view axis

    if (zc < 1e-3) return false;

    const double f = (screenH * 0.5) / std::tan(cam.fov * 0.5);
    sx = screenW * 0.5 + xc * f / zc;
    sy = screenH * 0.5 - yc * f / zc;
    return true;
}

// Adjust the orbit camera. dDist is a relative factor (negative = zoom in).
inline void Orbit(Camera3D& cam, double dYaw, double dPitch, double dDist)
{
    cam.yaw   += dYaw;
    cam.pitch  = std::clamp(cam.pitch + dPitch, -1.5, 1.5);
    cam.distance = std::clamp(cam.distance * (1.0 + dDist), 1.0, 1e7);
}

} // namespace rw
