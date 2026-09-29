// -----------------------------------------------------------------------------
// Random Walk — SDL3 CLI front-end for the `rw::RandomWalk` model.
//
// A thin driver: parse arguments -> build the model -> step + render. All the
// physics/measurement logic lives in `src/model/`, all drawing concerns in
// `src/render/`.
// -----------------------------------------------------------------------------
#include "src/model/random-walk.hpp"
#include "src/model/moments.hpp"
#include "src/render/view.hpp"
#include "src/render/trail.hpp"
#include "src/canvas.hpp"

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
inline constexpr std::string_view SETN   = "-N";
inline constexpr std::string_view APERT  = "-A";
inline constexpr std::string_view apert  = "--aperture";
inline constexpr std::string_view SEED   = "--seed";
inline constexpr std::string_view STARTX = "--start-x";
inline constexpr std::string_view STARTY = "--start-y";
inline constexpr std::string_view STEPSZ = "--step-size";
inline constexpr std::string_view BOUND  = "-B";
inline constexpr std::string_view bound  = "--boundary";
inline constexpr std::string_view NOTR   = "-NT";
inline constexpr std::string_view notr   = "--no-trail";
inline constexpr std::string_view TLEN   = "-TL";
inline constexpr std::string_view tlen   = "--trail-length";
inline constexpr std::string_view STATS  = "--stats-every";
}

struct CLIOptions
{
    rw::RandomWalkConfig cfg;
    bool trail         = true;
    std::uint64_t maxAge    = 500;
    std::uint64_t statsEvery = 0; // 0 = off
};

static void PrintHelp()
{
    std::printf("Usage: walker [flags]\n");
    std::printf("\t-N <count>            Number of random walkers (default 200).\n");
    std::printf("\t-W <width>            Canvas width (default 900).\n");
    std::printf("\t-H <height>           Canvas height (default 600).\n");
    std::printf("\t-A | --aperture <s>   Walker draw size in pixels (default 5).\n");
    std::printf("\t--seed <n>            RNG seed (default 0, reproducible).\n");
    std::printf("\t--start-x <x>         Common starting X position (default 0).\n");
    std::printf("\t--start-y <y>         Common starting Y position (default 0).\n");
    std::printf("\t--step-size <s>       Distance moved per step (default 1).\n");
    std::printf("\t-B | --boundary <m>   Boundary mode: p=Periodic, r=Reflective, f=Free (default f).\n");
    std::printf("\t-wms | --walker-move-style <m>  Move style:\n");
    std::printf("\t    s Straight | d Diagonal | o StraightDiagonal\n");
    std::printf("\t    + StraightWCenter | x DiagonalWCenter | q StraightDiagonalWCenter\n");
    std::printf("\t    S StraightContinuous | D DiagonalContinuous | Q StraightDiagonalContinuous\n");
    std::printf("\t-NT | --no-trail      Disable the fading trail.\n");
    std::printf("\t-TL | --trail-length <n>  Trail history length in steps (default 500).\n");
    std::printf("\t--stats-every <n>     Print an observables CSV row every n steps (0=off).\n");
    std::printf("\t-h | --help           Show this help.\n");
}

static bool ParseMoveStyle(std::string_view s, rw::MoveStyle& out)
{
    if (s == "Straight" || s == "s") out = rw::MoveStyle::Straight;
    else if (s == "Diagonal" || s == "d") out = rw::MoveStyle::Diagonal;
    else if (s == "StraightDiagonal" || s == "o") out = rw::MoveStyle::StraightDiagonal;
    else if (s == "StraightWCenter" || s == "+") out = rw::MoveStyle::StraightWCenter;
    else if (s == "DiagonalWCenter" || s == "x") out = rw::MoveStyle::DiagonalWCenter;
    else if (s == "StraightDiagonalWCenter" || s == "q") out = rw::MoveStyle::StraightDiagonalWCenter;
    else if (s == "StraightContinuous" || s == "S") out = rw::MoveStyle::StraightContinuous;
    else if (s == "DiagonalContinuous" || s == "D") out = rw::MoveStyle::DiagonalContinuous;
    else if (s == "StraightDiagonalContinuous" || s == "Q") out = rw::MoveStyle::StraightDiagonalContinuous;
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

// Compute the world-space rectangle the view should show.
static void ComputeWorldBounds(const rw::RandomWalk& rw, double& minX, double& maxX,
                               double& minY, double& maxY)
{
    if (rw.boundary != rw::BoundaryMode::Free)
    {
        minX = 0.0; maxX = rw.width;
        minY = 0.0; maxY = rw.height;
        return;
    }
    minX = minY = 1e300;
    maxX = maxY = -1e300;
    for (std::size_t i = 0; i < rw.numWalkers; ++i)
    {
        const double x = rw.posX[i];
        const double y = rw.posY[i];
        if (x < minX) minX = x;
        if (x > maxX) maxX = x;
        if (y < minY) minY = y;
        if (y > maxY) maxY = y;
    }
    if (minX >= maxX) { minX -= 1.0; maxX += 1.0; }
    if (minY >= maxY) { minY -= 1.0; maxY += 1.0; }
}

static void Run(const CLIOptions& opt)
{
    Canvas canvas(opt.cfg.width, opt.cfg.height, "Random Walk");
    if (!canvas.CanvasCreateWindow())
    {
        std::printf("Couldn't create canvas... quitting!\n");
        return;
    }

    rw::RandomWalk rw(opt.cfg);

    // Per-walker colours (derived from the seed, so runs are reproducible).
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

    rw::TrailManager trails(static_cast<std::uint64_t>(opt.cfg.width),
                            static_cast<std::uint64_t>(opt.cfg.height), opt.maxAge);
    const bool useTrail = opt.trail && (opt.cfg.boundary != rw::BoundaryMode::Free);

    // Observables CSV header (when stats are requested).
    if (opt.statsEvery > 0)
    {
        std::printf("step,meanX,meanY,varX,varY,covXY,meanR,rmsR,msd,diffusion,radiusOfGyration\n");
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
        }

        rw.Step();
        ++frames;

        double minX, maxX, minY, maxY;
        ComputeWorldBounds(rw, minX, maxX, minY, maxY);
        const rw::ViewTransform view = rw::ComputeView(
            minX, maxX, minY, maxY,
            static_cast<double>(opt.cfg.width), static_cast<double>(opt.cfg.height));

        if (useTrail)
        {
            trails.Step();
            for (std::size_t i = 0; i < rw.numWalkers; ++i)
            {
                double x, y;
                rw.Display(x, y, i);
                trails.Record(i, x, y, rw.size[i], cr[i], cg[i], cb[i]);
            }
            trails.Draw(canvas.GetRenderer(), view);
        }

        // Draw the walkers.
        for (std::size_t i = 0; i < rw.numWalkers; ++i)
        {
            double x, y;
            rw.Display(x, y, i);
            const float sx = static_cast<float>(rw::MapX(view, x));
            const float sy = static_cast<float>(rw::MapY(view, y));
            SDL_SetRenderDrawColor(canvas.GetRenderer(),
                                   static_cast<Uint8>(cr[i]), static_cast<Uint8>(cg[i]),
                                   static_cast<Uint8>(cb[i]), 255);
            const float sz = static_cast<float>(rw.size[i]);
            if (sz <= 1.0f)
                SDL_RenderPoint(canvas.GetRenderer(), sx, sy);
            else
                RenderFilledCircle(canvas.GetRenderer(), sx, sy, sz * 0.5f, 100,
                                   ColorF{1.0f, 1.0f, 1.0f, 1.0f});
        }

        if (opt.statsEvery > 0 && (frames % opt.statsEvery) == 0)
        {
            const rw::WalkerObservables o = rw::CollectObservables(rw, static_cast<double>(frames));
            std::printf("%llu,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f\n",
                        static_cast<unsigned long long>(o.step),
                        o.meanX, o.meanY, o.varX, o.varY, o.covXY,
                        o.meanR, o.rmsR, o.msd, o.diffusion, o.radiusOfGyration);
        }

        SDL_RenderPresent(canvas.GetRenderer());
        SDL_Delay(16);
    }
}

int main(int argc, char* argv[])
{
    CLIOptions opt;

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
            if (value(v) && !ParseMoveStyle(v, opt.cfg.moveStyle))
                std::printf("Unknown move style \"%s\", ignoring.\n", std::string(v).c_str());
        }
        else if (arg == SETW) { if (value(v)) opt.cfg.width = std::stoul(std::string(v)); }
        else if (arg == SETH) { if (value(v)) opt.cfg.height = std::stoul(std::string(v)); }
        else if (arg == SETN) { if (value(v)) opt.cfg.numWalkers = std::stoul(std::string(v)); }
        else if (arg == APERT || arg == apert) { if (value(v)) opt.cfg.size = std::stod(std::string(v)); }
        else if (arg == SEED) { if (value(v)) opt.cfg.seed = std::stoull(std::string(v)); }
        else if (arg == STARTX) { if (value(v)) opt.cfg.startX = std::stod(std::string(v)); }
        else if (arg == STARTY) { if (value(v)) opt.cfg.startY = std::stod(std::string(v)); }
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
