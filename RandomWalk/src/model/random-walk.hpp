#pragma once
// -----------------------------------------------------------------------------
// Random-walk model (Structure-of-Arrays, backend-independent).
//
// The walker state lives in parallel arrays (`posX`, `posY`, ...) so the hot
// `Step()` loop is cache-friendly. No rendering / SDL dependency here: this is
// the pure model, which mirrors `MathEngine::RandomWalk` in EMMA.
//
// Positions are kept *unwrapped* (raw accumulation of step increments). The
// wrapped/reflected on-canvas coordinate is produced on demand via `WrapX/Y`
// (for the `Periodic` boundary) so that displacement, MSD and the moments in
// `moments.hpp` remain meaningful.
// -----------------------------------------------------------------------------
#include <array>
#include <cstddef>
#include <cstdint>
#include <random>
#include <vector>

namespace rw
{

// Discrete/continuous move styles. The enum values are kept stable so they can
// be reused as combo indices.
enum class MoveStyle : std::uint8_t
{
    Straight                    = 0, // one of the 4 cardinal directions
    Diagonal                    = 1, // one of the 4 diagonal directions
    StraightDiagonal            = 2, // one of the 8 directions
    StraightWCenter             = 3, // cardinal + stay
    DiagonalWCenter             = 4, // diagonal + stay
    StraightDiagonalWCenter     = 5, // 8 directions + stay
    StraightContinuous          = 6, // cardinal axis, continuous step in [-1,1)
    DiagonalContinuous          = 7, // diagonal, continuous step in [-1,1)
    StraightDiagonalContinuous  = 8, // independent continuous steps in x and y
};

// How the walkers interact with the canvas edges.
enum class BoundaryMode : std::uint8_t
{
    Periodic   = 0, // wrap around (torus); displacement kept unwrapped
    Reflective = 1, // bounce off the walls
    Free       = 2, // no boundary; walkers roam freely (view auto-scales)
};

struct RandomWalkConfig
{
    std::size_t  numWalkers = 200;
    double       width      = 900.0;
    double       height     = 600.0;
    double       size       = 5.0;
    double       startX     = 0.0;   // common starting position
    double       startY     = 0.0;
    double       stepSize   = 1.0;   // distance moved per step
    MoveStyle    moveStyle  = MoveStyle::Straight;
    BoundaryMode boundary   = BoundaryMode::Free;
    std::uint64_t seed      = 0;
};

// Discrete direction tables (step increments).
static constexpr std::array<std::array<double, 2>, 4> kStraight{{
    {{1.0, 0.0}}, {{0.0, 1.0}}, {{-1.0, 0.0}}, {{0.0, -1.0}}
}};
static constexpr std::array<std::array<double, 2>, 4> kDiagonal{{
    {{1.0, 1.0}}, {{-1.0, 1.0}}, {{-1.0, -1.0}}, {{1.0, -1.0}}
}};
static constexpr std::array<std::array<double, 2>, 8> kStraightDiagonal{{
    {{1.0, 0.0}}, {{1.0, 1.0}}, {{0.0, 1.0}}, {{-1.0, 1.0}},
    {{-1.0, 0.0}}, {{-1.0, -1.0}}, {{0.0, -1.0}}, {{1.0, -1.0}}
}};
static constexpr std::array<std::array<double, 2>, 5> kStraightWCenter{{
    {{0.0, 0.0}}, {{1.0, 0.0}}, {{0.0, 1.0}}, {{-1.0, 0.0}}, {{0.0, -1.0}}
}};
static constexpr std::array<std::array<double, 2>, 5> kDiagonalWCenter{{
    {{0.0, 0.0}}, {{1.0, 1.0}}, {{-1.0, 1.0}}, {{-1.0, -1.0}}, {{1.0, -1.0}}
}};
static constexpr std::array<std::array<double, 2>, 9> kStraightDiagonalWCenter{{
    {{0.0, 0.0}}, {{1.0, 0.0}}, {{1.0, 1.0}}, {{0.0, 1.0}}, {{-1.0, 1.0}},
    {{-1.0, 0.0}}, {{-1.0, -1.0}}, {{0.0, -1.0}}, {{1.0, -1.0}}
}};

class RandomWalk
{
public:
    // ---- Structure-of-Arrays state -----------------------------------------
    std::vector<double> posX, posY;   // current position (unwrapped)
    std::vector<double> initX, initY; // t = 0 reference (for displacement/MSD)
    std::vector<double> size;         // per-walker draw size

    // ---- geometry / bookkeeping -------------------------------------------
    double width  = 900.0;
    double height = 600.0;
    MoveStyle    moveStyle  = MoveStyle::Straight;
    BoundaryMode boundary   = BoundaryMode::Free;
    double       stepSize   = 1.0;
    std::size_t  numWalkers = 0;
    std::uint64_t stepCount = 0;
    std::uint64_t seed      = 0;

    std::mt19937_64 rng;

    RandomWalk() = default;

    explicit RandomWalk(const RandomWalkConfig& cfg)
        : width(cfg.width), height(cfg.height),
          moveStyle(cfg.moveStyle), boundary(cfg.boundary),
          stepSize(cfg.stepSize),
          numWalkers(cfg.numWalkers), seed(cfg.seed),
          rng(cfg.seed)
    {
        posX.assign(numWalkers, cfg.startX);
        posY.assign(numWalkers, cfg.startY);
        initX.assign(numWalkers, cfg.startX);
        initY.assign(numWalkers, cfg.startY);
        size.assign(numWalkers, cfg.size);
    }

    // Advance every walker by exactly one step.
    void Step()
    {
        std::uniform_int_distribution<int> uid4(0, 3);
        std::uniform_int_distribution<int> uid5(0, 4);
        std::uniform_int_distribution<int> uid8(0, 7);
        std::uniform_int_distribution<int> uid9(0, 8);
        std::uniform_real_distribution<double> urd1(-1.0, 1.0);
        std::bernoulli_distribution bd(0.5);

        for (std::size_t i = 0; i < numWalkers; ++i)
        {
            double dx = 0.0, dy = 0.0;
            switch (moveStyle)
            {
                case MoveStyle::Straight: {
                    const int r = uid4(rng);
                    dx = kStraight[r][0]; dy = kStraight[r][1]; break;
                }
                case MoveStyle::Diagonal: {
                    const int r = uid4(rng);
                    dx = kDiagonal[r][0]; dy = kDiagonal[r][1]; break;
                }
                case MoveStyle::StraightDiagonal: {
                    const int r = uid8(rng);
                    dx = kStraightDiagonal[r][0]; dy = kStraightDiagonal[r][1]; break;
                }
                case MoveStyle::StraightWCenter: {
                    const int r = uid5(rng);
                    dx = kStraightWCenter[r][0]; dy = kStraightWCenter[r][1]; break;
                }
                case MoveStyle::DiagonalWCenter: {
                    const int r = uid5(rng);
                    dx = kDiagonalWCenter[r][0]; dy = kDiagonalWCenter[r][1]; break;
                }
                case MoveStyle::StraightDiagonalWCenter: {
                    const int r = uid9(rng);
                    dx = kStraightDiagonalWCenter[r][0]; dy = kStraightDiagonalWCenter[r][1]; break;
                }
                case MoveStyle::StraightContinuous: {
                    if (bd(rng)) dx = urd1(rng); else dy = urd1(rng); break;
                }
                case MoveStyle::DiagonalContinuous: {
                    const double s = urd1(rng);
                    dx = s; dy = (bd(rng) ? s : -s); break;
                }
                case MoveStyle::StraightDiagonalContinuous: {
                    dx = urd1(rng); dy = urd1(rng); break;
                }
                default: {
                    const int r = uid4(rng);
                    dx = kStraight[r][0]; dy = kStraight[r][1]; break;
                }
            }

            posX[i] += stepSize * dx;
            posY[i] += stepSize * dy;

            if (boundary == BoundaryMode::Reflective)
            {
                posX[i] = Reflect(posX[i], 0.0, width);
                posY[i] = Reflect(posY[i], 0.0, height);
            }
            // Periodic: leave unwrapped (wrap only for display).
            // Free: leave unbounded.
        }
        ++stepCount;
    }

    // On-canvas coordinate for display (periodic wrap). For reflective/free the
    // raw position is already the display coordinate.
    double WrapX(double x) const { return x - width  * std::floor(x / width); }
    double WrapY(double y) const { return y - height * std::floor(y / height); }

    // Display-space coordinate of walker i (respecting the boundary mode).
    void Display(double& x, double& y, std::size_t i) const
    {
        x = posX[i];
        y = posY[i];
        if (boundary == BoundaryMode::Periodic)
        {
            x = WrapX(x);
            y = WrapY(y);
        }
    }

private:
    static double Reflect(double v, double lo, double hi)
    {
        const double span = hi - lo;
        if (span <= 0.0) return v;
        while (v < lo || v > hi)
        {
            if (v < lo) v = 2.0 * lo - v;
            if (v > hi) v = 2.0 * hi - v;
        }
        return v;
    }
};

} // namespace rw
