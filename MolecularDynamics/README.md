# 2D Molecular-Dynamics Simulation

A 2D molecular-dynamics (MD) engine with SDL visualization. The particle state is
stored in *Structure-of-Arrays* layout to keep the inner force loop cache-friendly
and vectorizable (OpenMP is used when enabled).

## Layout

```
src/
  potential.hpp         pair potentials (Lennard-Jones, WCA, Morse) + shifted-force cutoff
  initial-conditions.hpp lattice / random / slab / binary initial conditions
  engine.hpp            the `MolecularDynamics` SoA engine (velocity-Verlet & leapfrog)
  thermostats.hpp       Rescale, Berendsen, Andersen, Langevin, Nose-Hoover + Berendsen barostat
  analysis.hpp          energies, temperature, pressure, psi4/psi6, RDF, MSD, CSV writer
  molecular-dynamics.hpp umbrella header (physics only, no rendering)
  renderer.hpp          SDL drawing helper (demo-only)
  circle.hpp, canvas.hpp SDL drawing primitives (pre-existing)
lennard-jones.cpp       interactive SDL demo (LJ cooling -> crystallisation)
validate.cpp            headless physics checks (no display needed)
```

## Features

- **Potentials** — `LennardJones`, `WCA` (repulsive core), `Morse`. All use a
  *shifted-force* cutoff so forces go continuously to zero at `r_c`, which keeps
  the NVE energy drift tiny (see `validate`).
- **Initial conditions** — `SquareLattice`, `HexagonalLattice`, `Random` (with a
  minimum separation), `TwoPhaseSlab`, `BinaryMixture` (two species).
- **Processes** — thermostats (`Rescale`, `Berendsen`, `Andersen`, `Langevin`,
  `NoseHoover`) and a `Berendsen` barostat (pressure control).
- **Integrators** — symplectic `VelocityVerlet` (default) and `Leapfrog`.
- **Analysis** — kinetic / potential / total energy, temperature, virial pressure,
  bond-orientational order `psi4` / `psi6` (the "order parameter" for a solid),
  radial distribution function `g(r)`, and mean-squared displacement (diffusion).

## Building

```sh
make            # builds `lennard-jones` (SDL demo) and `validate`
make validate   # headless physics checks only
```

Requires SDL3 (`pkg-config --cflags --libs sdl3`) and a C++20 compiler.

## Usage

```sh
./lennard-jones   # interactive; Esc/q quits, 's' toggles colour-by-speed
./validate        # prints energy-conservation / ordering / thermostat checks
```

The demo cools a Lennard-Jones fluid and prints `psi6` / `psi4` as it
crystallises, then writes the collected observables to `md-observables.csv`.
