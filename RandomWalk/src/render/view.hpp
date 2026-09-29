#pragma once
// -----------------------------------------------------------------------------
// View transform: maps world-space coordinates into screen-space coordinates.
//
// `ComputeView` fits an axis-aligned world bounding box into a screen rectangle
// while preserving the aspect ratio (letterboxed), so the walk is never
// stretched. This is what lets the `Free` boundary mode "zoom to fit" the
// walkers: just feed it the current bounding box of all walkers each frame.
// -----------------------------------------------------------------------------
#include <algorithm>

namespace rw
{

struct ViewTransform
{
    double worldMinX = 0.0, worldMaxX = 1.0;
    double worldMinY = 0.0, worldMaxY = 1.0;
    double screenX = 0.0, screenY = 0.0; // offset of the (letterboxed) draw rect
    double screenW = 1.0, screenH = 1.0; // full target rect size
    double scale   = 1.0;                // pixels per world unit (uniform)
    double offsetX = 0.0, offsetY = 0.0; // world coord at the draw rect origin
};

inline ViewTransform ComputeView(double worldMinX, double worldMaxX,
                                 double worldMinY, double worldMaxY,
                                 double screenW, double screenH,
                                 double pad = 0.05)
{
    const double wx = worldMaxX - worldMinX;
    const double wy = worldMaxY - worldMinY;
    const double px = (wx > 0.0) ? wx * pad : 1.0;
    const double py = (wy > 0.0) ? wy * pad : 1.0;

    const double wMinX = worldMinX - px, wMaxX = worldMaxX + px;
    const double wMinY = worldMinY - py, wMaxY = worldMaxY + py;

    ViewTransform t;
    t.worldMinX = wMinX; t.worldMaxX = wMaxX;
    t.worldMinY = wMinY; t.worldMaxY = wMaxY;
    t.screenW = screenW; t.screenH = screenH;

    const double ww = wMaxX - wMinX;
    const double wh = wMaxY - wMinY;
    const double sx = (ww > 0.0) ? screenW / ww : 0.0;
    const double sy = (wh > 0.0) ? screenH / wh : 0.0;
    t.scale = std::min(sx, sy);
    if (t.scale <= 0.0) t.scale = 1.0;

    const double drawnW = ww * t.scale;
    const double drawnH = wh * t.scale;
    t.screenX = (screenW - drawnW) * 0.5;
    t.screenY = (screenH - drawnH) * 0.5;
    t.offsetX = wMinX;
    t.offsetY = wMinY;
    return t;
}

inline double MapX(const ViewTransform& t, double x) { return t.screenX + (x - t.offsetX) * t.scale; }
inline double MapY(const ViewTransform& t, double y) { return t.screenY + (y - t.offsetY) * t.scale; }

} // namespace rw
