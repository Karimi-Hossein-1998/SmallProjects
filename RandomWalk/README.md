# Random Walk

A random-walk demo with an SDL3 front-end (`walker`) over a pure, backend-independent
model (`src/model/`) with Structure-of-Arrays state and built-in measurements.

## Building

```sh
cmake -S . -B build
cmake --build build
# or directly:
g++ -std=c++23 -O2 walker.cpp -o walker $(pkg-config --cflags --libs sdl3)
```

## Running

The executable opens a window with `N` random walkers starting from a common origin.

| Flag | Meaning | Default |
|------|---------|---------|
| `-N <n>` | number of walkers | 200 |
| `-W <w>` | canvas width | 900 |
| `-H <h>` | canvas height | 600 |
| `-A <s>` / `--aperture <s>` | walker draw size (px) | 5 |
| `--seed <n>` | RNG seed (reproducible) | 0 |
| `--start-x <x>` | common starting X position | 0 |
| `--start-y <y>` | common starting Y position | 0 |
| `--step-size <s>` | distance moved per step | 1 |
| `-B <m>` / `--boundary <m>` | `p` Periodic, `r` Reflective, `f` Free | `f` |
| `-wms <m>` / `--walker-move-style <m>` | move style (below) | `s` |
| `-NT` / `--no-trail` | disable the fading trail | — |
| `-TL <n>` / `--trail-length <n>` | trail history length (steps) | 500 |
| `--stats-every <n>` | print an observables CSV row every `n` steps | 0 (off) |

Move styles (`-wms`): `s` Straight, `d` Diagonal, `o` StraightDiagonal,
`+` StraightWCenter, `x` DiagonalWCenter, `q` StraightDiagonalWCenter,
`S` StraightContinuous, `D` DiagonalContinuous, `Q` StraightDiagonalContinuous.

Boundary modes:

- **Periodic** (`p`): walkers wrap around the canvas (torus); displacement/MSD are
  tracked *unwrapped* so they stay physically meaningful.
- **Reflective** (`r`): walkers bounce off the walls.
- **Free** (`f`): no boundary — the view **auto-scales to fit the walkers** each
  frame (see `src/render/view.hpp`).

## Measurements

`src/model/moments.hpp` exposes ensemble observables over the `N` walkers:

- first/second moments of position: `⟨x⟩`, `⟨y⟩`, `⟨x²⟩`, `⟨y²⟩`, `⟨xy⟩`
- variances/covariance: `σ_x²`, `σ_y²`, `Cov(x,y)`
- distance/dispersion: `⟨|r|⟩`, `r_rms = √MSD`, `MSD`, `D = MSD/(4t)`, radius of gyration

Use `--stats-every <n>` to stream these as CSV while the walk runs.

## Layout

```
src/
  model/      random-walk.hpp   (SoA model, move styles, boundary modes)
              moments.hpp       (observables)
  render/     view.hpp          (world -> screen transform, auto-fit)
              trail.hpp         (fading trail heatmap)
              canvas.hpp        (SDL window)
              circle.hpp        (SDL geometry helpers)
walker.cpp    CLI + render loop
```

This model is also integrated into the [EMMA](https://github.com/your-org/EMMA)
mathematical-modelling app as `MathEngine::RandomWalk`.
