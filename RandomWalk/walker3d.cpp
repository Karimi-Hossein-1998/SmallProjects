// -----------------------------------------------------------------------------
// Random Walk 3D — SDL3 CLI front-end for the `rw::RandomWalk3D` model.
//
// Renders the walker cloud with a perspective orbit camera. Key controls:
//   arrows / WASD  rotate yaw & pitch
//   +/- (or Q/E)   zoom in / out
//   R              reset camera
//   Esc / close    quit
// -----------------------------------------------------------------------------
#include "src/model/random-walk3d.hpp"
#include "src/model/moments3d.hpp"
#include "src/render/camera3d.hpp"
#include "src/render/trail3d.hpp"
#include "src/canvas.hpp"
#include "src/circle.hpp"

#include <cstdint>
#include <cstdio>
#include <random>
#include <string>
#include <string_view>
#include <vector>

namespace
{
inline constexpr std::string_view HELP   = "-h";
inline constexpr std::string_view help   = "--help";
inline constexpr std::string_view SWMS   = "-wms";
inline constexpr std::string_view swms   = "--walker-move-style";
inline constexpr std::string_view SETW   = "-W";
inline constexpr std::string_view SETH   = "-H";
inline constexpr std::string_view SETD   = "-D";
inline constexpr std::string_view SETN   = "-N";
inline constexpr std::string_view APERT  = "-A";
inline constexpr std::string_view apert  = "--aperture";
inline constexpr std::string_view SEED   = "--seed";
inline constexpr std::string_view STARTX = "--start-x";
inline constexpr std::string_view STARTY = "--start-y";
inline constexpr std::string_view STARTZ = "--start-z";
inline constexpr std::string_view STEPSZ = "--step-size";
inline constexpr std::string_view BOUND  = "-B";
inline constexpr std::string_view bound  = "--boundary";
inline constexpr std::string_view NOTR   = "-NT";
inline constexpr std::string_view notr   = "--no-trail";
inline constexpr std::string_view TLEN   = "-TL";
inline constexpr std::string_view tlen   = "--trail-length";
inline constexpr std::string_view STATS  = "--stats-every";
}

struct CLIOptions3D
{
    rw::RandomWalk3DConfig cfg;
    bool trail = true;
    std::uint64_t maxAge = 500;
    std::uint64_t statsEvery = 0;
};

static void PrintHelp()
{
    std::printf("Usage: walker3d [flags]\n");
    std::printf("\t-N <count>            Number of random walkers (default 200).\n");
    std::printf("\t-W <width>            Box width (default 900).\n");
    std::printf("\t-H <height>           Box height (default 600).\n");
    std::printf("\t-D <depth>            Box depth (default 600).\n");
    std::printf("\t-A | --aperture <s>   Walker draw size in pixels (default 5).\n");
    std::printf("\t--seed <n>            RNG seed (default 0, reproducible).\n");
    std::printf("\t--start-x/-y/-z <v>   Common starting position (default 0).\n");
    std::printf("\t--step-size <s>       Distance moved per step (default 1).\n");
    std::printf("\t-B | --boundary <m>   p=Periodic, r=Reflective, f=Free (default f).\n");
    std::printf("\t-wms | --walker-move-style <m>  Move style:\n");
    std::printf("\t    Straight | PlaneDiagonal | Diagonal | FullDiagonal\n");
    std::printf("\t    StraightPlaneDiagonal | StraightDiagonal | StraightFullDiagonal\n");
    std::printf("\t    (each also with a WCenter variant, and a Continuous variant)\n");
    std::printf("\t-NT | --no-trail      Disable the fading trail.\n");
    std::printf("\t-TL | --trail-length <n>  Trail history length in steps (default 500).\n");
    std::printf("\t--stats-every <n>     Print an observables CSV row every n steps (0=off).\n");
    std::printf("\t-h | --help           Show this help.\n");
    std::printf("\nCamera: Arrow keys / WASD rotate, + / - (or Q / E) zoom, R reset, Esc quit.\n");
}

static bool ParseMoveStyle3D(std::string_view s, rw::MoveStyle3D& out)
{
    if (s == "Straight" || s == "s") out = rw::MoveStyle3D::Straight;
    else if (s == "PlaneDiagonal" || s == "pd") out = rw::MoveStyle3D::PlaneDiagonal;
    else if (s == "Diagonal" || s == "d") out = rw::MoveStyle3D::Diagonal;
    else if (s == "FullDiagonal" || s == "fd") out = rw::MoveStyle3D::FullDiagonal;
    else if (s == "StraightPlaneDiagonal" || s == "spd") out = rw::MoveStyle3D::StraightPlaneDiagonal;
    else if (s == "StraightDiagonal" || s == "sd") out = rw::MoveStyle3D::StraightDiagonal;
    else if (s == "StraightFullDiagonal" || s == "sfd") out = rw::MoveStyle3D::StraightFullDiagonal;
    else if (s == "StraightWCenter" || s == "s+") out = rw::MoveStyle3D::StraightWCenter;
    else if (s == "PlaneDiagonalWCenter" || s == "pd+") out = rw::MoveStyle3D::PlaneDiagonalWCenter;
    else if (s == "DiagonalWCenter" || s == "d+") out = rw::MoveStyle3D::DiagonalWCenter;
    else if (s == "FullDiagonalWCenter" || s == "fd+") out = rw::MoveStyle3D::FullDiagonalWCenter;
    else if (s == "StraightPlaneDiagonalWCenter" || s == "spd+") out = rw::MoveStyle3D::StraightPlaneDiagonalWCenter;
    else if (s == "StraightDiagonalWCenter" || s == "sd+") out = rw::MoveStyle3D::StraightDiagonalWCenter;
    else if (s == "StraightFullDiagonalWCenter" || s == "sfd+") out = rw::MoveStyle3D::StraightFullDiagonalWCenter;
    else if (s == "StraightContinuous" || s == "Sc") out = rw::MoveStyle3D::StraightContinuous;
    else if (s == "PlaneDiagonalContinuous" || s == "pdc") out = rw::MoveStyle3D::PlaneDiagonalContinuous;
    else if (s == "DiagonalContinuous" || s == "dc") out = rw::MoveStyle3D::DiagonalContinuous;
    else if (s == "FullDiagonalContinuous" || s == "fdc") out = rw::MoveStyle3D::FullDiagonalContinuous;
    else if (s == "StraightPlaneDiagonalContinuous" || s == "spdc") out = rw::MoveStyle3D::StraightPlaneDiagonalContinuous;
    else if (s == "StraightDiagonalContinuous" || s == "sdc") out = rw::MoveStyle3D::StraightDiagonalContinuous;
    else if (s == "StraightFullDiagonalContinuous" || s == "sfdc") out = rw::MoveStyle3D::StraightFullDiagonalContinuous;
    else return false;
    return true;
}

static bool ParseBoundary(std::string_view s, rw::BoundaryMode& out)
{
    if (s == "Periodic" || s == "p") out = rw::BoundaryMode::Periodic;
    else if (s == "Reflective" || s == "r") out = rw::BoundaryMode::Reflective;
    else if (s == "Free" || s == "f") out = rw::BoundaryMode::Free;
    else return false;
    return true;
}

static void DrawMarker(SDL_Renderer* r, float sx, float sy, float size,
                       unsigned short cr, unsigned short cg, unsigned short cb)
{
    SDL_SetRenderDrawColor(r, static_cast<Uint8>(cr), static_cast<Uint8>(cg), static_cast<Uint8>(cb), 255);
    if (size <= 1.0f)
        SDL_RenderPoint(r, sx, sy);
    else
        RenderFilledCircle(r, sx, sy, size * 0.5f, 100,
                           ColorF{cr / 255.0f, cg / 255.0f, cb / 255.0f, 1.0f});
}

// Draws an RGB = XYZ coordinate frame anchored at the camera target, so the
// world directions are always visible.
static void DrawAxes(SDL_Renderer* r, const rw::Camera3D& cam, double sw, double sh)
{
    // Coordinate frame anchored at the world origin (0,0,0).
    const double tx = 0.0, ty = 0.0, tz = 0.0;
    const double len = cam.distance * 0.3;
    double ox, oy, ex, ey;
    if (!rw::Project(cam, tx, ty, tz, sw, sh, ox, oy)) return;

    SDL_SetRenderDrawColor(r, 255, 70, 70, 255);   // X
    if (rw::Project(cam, tx + len, ty, tz, sw, sh, ex, ey))
        SDL_RenderLine(r, static_cast<float>(ox), static_cast<float>(oy), static_cast<float>(ex), static_cast<float>(ey));
    SDL_SetRenderDrawColor(r, 70, 255, 70, 255);   // Y
    if (rw::Project(cam, tx, ty + len, tz, sw, sh, ex, ey))
        SDL_RenderLine(r, static_cast<float>(ox), static_cast<float>(oy), static_cast<float>(ex), static_cast<float>(ey));
    SDL_SetRenderDrawColor(r, 70, 120, 255, 255);  // Z
    if (rw::Project(cam, tx, ty, tz + len, sw, sh, ex, ey))
        SDL_RenderLine(r, static_cast<float>(ox), static_cast<float>(oy), static_cast<float>(ex), static_cast<float>(ey));
}

static void Run(const CLIOptions3D& opt)
{
    Canvas canvas(opt.cfg.width, opt.cfg.height, "Random Walk 3D");
    if (!canvas.CanvasCreateWindow())
    {
        std::printf("Couldn't create canvas... quitting!\n");
        return;
    }

    rw::RandomWalk3D rw(opt.cfg);

    std::vector<unsigned short> cr(opt.cfg.numWalkers), cg(opt.cfg.numWalkers),
                                cb(opt.cfg.numWalkers), ca(opt.cfg.numWalkers, 255);
    {
        std::mt19937_64 crng(opt.cfg.seed + 0x9E3779B97F4A7C15ull);
        std::uniform_int_distribution<int> cdist(0, 255);
        for (std::size_t i = 0; i < opt.cfg.numWalkers; ++i)
        {
            cr[i] = static_cast<unsigned short>(cdist(crng));
            cg[i] = static_cast<unsigned short>(cdist(crng));
            cb[i] = static_cast<unsigned short>(cdist(crng));
        }
    }

    rw::TrailManager3D trails(static_cast<std::uint64_t>(opt.cfg.width),
                              static_cast<std::uint64_t>(opt.cfg.height),
                              static_cast<std::uint64_t>(opt.cfg.depth), opt.maxAge);
    const bool useTrail = opt.trail;

    rw::Camera3D cam;
    const bool free = (opt.cfg.boundary == rw::BoundaryMode::Free);
    cam.targetX = free ? 0.0 : opt.cfg.width * 0.5;
    cam.targetY = free ? 0.0 : opt.cfg.height * 0.5;
    cam.targetZ = free ? 0.0 : opt.cfg.depth * 0.5;
    cam.distance = std::max(std::max(opt.cfg.width, opt.cfg.height), opt.cfg.depth) * 1.5;
    const rw::Camera3D camDefault = cam;

    bool held[SDL_SCANCODE_COUNT] = {};

    if (opt.statsEvery > 0)
    {
        std::printf("step,meanX,meanY,meanZ,varX,varY,varZ,covXY,covXZ,covYZ,meanR,rmsR,msd,diffusion,radiusOfGyration\n");
    }

    bool running = true;
    SDL_Event event;
    std::uint64_t frames = 0;

    while (running)
    {
        SDL_SetRenderDrawColor(canvas.GetRenderer(), 5, 5, 5, 255);
        SDL_RenderClear(canvas.GetRenderer());

        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_QUIT) running = false;
            else if (event.type == SDL_EVENT_KEY_DOWN)
            {
                if (event.key.scancode == SDL_SCANCODE_ESCAPE) running = false;
                else if (event.key.scancode < SDL_SCANCODE_COUNT) held[event.key.scancode] = true;
            }
            else if (event.type == SDL_EVENT_KEY_UP)
            {
                if (event.key.scancode < SDL_SCANCODE_COUNT) held[event.key.scancode] = false;
            }
        }

        // Camera controls (realtime).
        const double rot = 0.04, zoom = 0.05;
        if (held[SDL_SCANCODE_LEFT]  || held[SDL_SCANCODE_A]) rw::Orbit(cam, -rot, 0.0, 0.0);
        if (held[SDL_SCANCODE_RIGHT] || held[SDL_SCANCODE_D]) rw::Orbit(cam,  rot, 0.0, 0.0);
        if (held[SDL_SCANCODE_UP]    || held[SDL_SCANCODE_W]) rw::Orbit(cam, 0.0,  rot, 0.0);
        if (held[SDL_SCANCODE_DOWN]  || held[SDL_SCANCODE_S]) rw::Orbit(cam, 0.0, -rot, 0.0);
        if (held[SDL_SCANCODE_EQUALS] || held[SDL_SCANCODE_KP_PLUS] || held[SDL_SCANCODE_E]) rw::Orbit(cam, 0.0, 0.0, -zoom);
        if (held[SDL_SCANCODE_MINUS] || held[SDL_SCANCODE_Q]) rw::Orbit(cam, 0.0, 0.0, +zoom);
        if (held[SDL_SCANCODE_R]) cam = camDefault;

        rw.Step();
        ++frames;

        const double sw = static_cast<double>(opt.cfg.width);
        const double sh = static_cast<double>(opt.cfg.height);

        if (useTrail)
        {
            trails.Step();
            for (std::size_t i = 0; i < rw.numWalkers; ++i)
            {
                double x, y, z;
                rw.Display(x, y, z, i);
                trails.Record(i, x, y, z, rw.size[i], cr[i], cg[i], cb[i]);
            }
            trails.Draw(canvas.GetRenderer(), cam, sw, sh);
        }

        for (std::size_t i = 0; i < rw.numWalkers; ++i)
        {
            double x, y, z;
            rw.Display(x, y, z, i);
            double sx, sy;
            if (rw::Project(cam, x, y, z, sw, sh, sx, sy))
                DrawMarker(canvas.GetRenderer(), static_cast<float>(sx), static_cast<float>(sy),
                           static_cast<float>(rw.size[i]), cr[i], cg[i], cb[i]);
        }

        DrawAxes(canvas.GetRenderer(), cam, sw, sh);

        if (opt.statsEvery > 0 && (frames % opt.statsEvery) == 0)
        {
            const rw::WalkerObservables3D o = rw::CollectObservables3D(rw, static_cast<double>(frames));
            std::printf("%llu,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f\n",
                        static_cast<unsigned long long>(o.step),
                        o.meanX, o.meanY, o.meanZ, o.varX, o.varY, o.varZ,
                        o.covXY, o.covXZ, o.covYZ, o.meanR, o.rmsR, o.msd, o.diffusion, o.radiusOfGyration);
        }

        SDL_RenderPresent(canvas.GetRenderer());
        SDL_Delay(16);
    }
}

int main(int argc, char* argv[])
{
    CLIOptions3D opt;

    for (int i = 1; i < argc; ++i)
    {
        const std::string_view arg = argv[i];
        const auto value = [&](std::string_view& out) -> bool
        {
            if (i + 1 >= argc) { std::printf("Missing value for flag \"%s\".\n", std::string(arg).c_str()); return false; }
            out = std::string_view(argv[++i]);
            return true;
        };

        std::string_view v;
        if (arg == help || arg == HELP) { PrintHelp(); return 0; }
        else if (arg == SWMS || arg == swms)
        {
            if (value(v) && !ParseMoveStyle3D(v, opt.cfg.moveStyle))
                std::printf("Unknown move style \"%s\", ignoring.\n", std::string(v).c_str());
        }
        else if (arg == SETW) { if (value(v)) opt.cfg.width = std::stoul(std::string(v)); }
        else if (arg == SETH) { if (value(v)) opt.cfg.height = std::stoul(std::string(v)); }
        else if (arg == SETD) { if (value(v)) opt.cfg.depth = std::stoul(std::string(v)); }
        else if (arg == SETN) { if (value(v)) opt.cfg.numWalkers = std::stoul(std::string(v)); }
        else if (arg == APERT || arg == apert) { if (value(v)) opt.cfg.size = std::stod(std::string(v)); }
        else if (arg == SEED) { if (value(v)) opt.cfg.seed = std::stoull(std::string(v)); }
        else if (arg == STARTX) { if (value(v)) opt.cfg.startX = std::stod(std::string(v)); }
        else if (arg == STARTY) { if (value(v)) opt.cfg.startY = std::stod(std::string(v)); }
        else if (arg == STARTZ) { if (value(v)) opt.cfg.startZ = std::stod(std::string(v)); }
        else if (arg == STEPSZ) { if (value(v)) opt.cfg.stepSize = std::stod(std::string(v)); }
        else if (arg == BOUND || arg == bound)
        {
            if (value(v) && !ParseBoundary(v, opt.cfg.boundary))
                std::printf("Unknown boundary \"%s\" (use p|r|f), ignoring.\n", std::string(v).c_str());
        }
        else if (arg == NOTR || arg == notr) { opt.trail = false; }
        else if (arg == TLEN || arg == tlen) { if (value(v)) opt.maxAge = std::stoul(std::string(v)); }
        else if (arg == STATS) { if (value(v)) opt.statsEvery = std::stoul(std::string(v)); }
        else { std::printf("Unknown flag \"%s\", ignoring.\n", std::string(arg).c_str()); }
    }

    Run(opt);
    return 0;
}
