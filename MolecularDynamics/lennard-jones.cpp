// -----------------------------------------------------------------------------
// Interactive SDL demo for the 2D molecular-dynamics engine.
//
// Scenario (defaults): a Lennard-Jones fluid on a square lattice, cooled toward
// a low temperature with a Berendsen thermostat so it crystallises. The
// bond-orientational order parameter psi6 is printed as it rises from a
// liquid-like value toward ~1, demonstrating the "order parameter" analysis.
//
// Controls:
//   Esc / q          quit
//   s                toggle colour-by-speed
//
// On exit the collected observables are written to md-observables.csv.
// -----------------------------------------------------------------------------
#include <chrono>
#include <cstdio>
#include <string>
#include <vector>

#include "src/molecular-dynamics.hpp"
#include "src/renderer.hpp"
#include "src/canvas.hpp"

int main()
{
    MDConfig cfg;
    cfg.numParticles = 256;
    cfg.width        = 600.0;
    cfg.height       = 600.0;
    cfg.sigma        = 30.0;     // length scale (px); spacing = sigma * 2^(1/6)
    cfg.epsilon      = 1.0;
    cfg.cutoffCoeff  = 2.5;
    cfg.mass         = 1.0;
    cfg.radius       = 4.0;      // visual radius (much smaller than the spacing)
    cfg.temperature  = 1.5;      // start above the 2D melting point (~0.5)
    cfg.restitution  = 1.0;
    cfg.periodicBoundaryCondition = true;
    cfg.bounce       = false;
    cfg.hardSphereCollisions = false;
    cfg.potential    = PotentialType::LennardJones;
    cfg.initialCondition = InitialConditionType::SquareLattice;
    cfg.seed         = 41;

    const double dt = 0.002;
    const int    substepsPerFrame = 50;
    const double T_target = 0.10;   // cool below the melting point
    const double tau      = 2.0;    // Berendsen relaxation time (reduced units)

    MolecularDynamics md(cfg);

    Canvas canvas(static_cast<uint64_t>(cfg.width), static_cast<uint64_t>(cfg.height), "Molecular Dynamics (Lennard-Jones)");
    if (!canvas.CanvasCreateWindow())
    {
        std::printf("Couldn't create canvas... quitting!\n");
        return -1;
    }

    bool running = true;
    bool colorBySpeed = false;
    size_t frame = 0;

    // time-series buffers for the CSV dump
    std::vector<double> tHist, THist, KEHist, PEHist, EHist, psi6Hist, psi4Hist, msdHist;

    while (running)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_QUIT) { running = false; }
            else if (event.type == SDL_EVENT_KEY_DOWN)
            {
                if (event.key.key == SDLK_ESCAPE || event.key.key == SDLK_Q) running = false;
                else if (event.key.key == SDLK_S) colorBySpeed = !colorBySpeed;
            }
        }

        // integrate + thermostat
        for (int k = 0; k < substepsPerFrame; ++k)
        {
            md.Step(dt);
            ApplyBerendsen(md, T_target, dt, tau);
        }

        // render
        SDL_SetRenderDrawColor(canvas.GetRenderer(), 10, 10, 12, 255);
        SDL_RenderClear(canvas.GetRenderer());
        DrawParticles(canvas.GetRenderer(), md, colorBySpeed);
        SDL_RenderPresent(canvas.GetRenderer());

        ++frame;
        if (frame % 50 == 0)
        {
            const double time = frame * substepsPerFrame * dt;
            const Observables o = CollectObservables(md, time, /*orderCutoff*/ 1.4);
            std::printf("[t=%8.2f] T=%.4f  KE=%.4f  PE=%.4f  E=%.4f  psi6=%.4f  psi4=%.4f  MSD=%.4f\n",
                        o.time, o.temperature, o.kineticEnergy, o.potentialEnergy,
                        o.totalEnergy, o.psi6, o.psi4, o.msd);

            tHist.push_back(o.time);     THist.push_back(o.temperature);
            KEHist.push_back(o.kineticEnergy); PEHist.push_back(o.potentialEnergy);
            EHist.push_back(o.totalEnergy);   psi6Hist.push_back(o.psi6);
            psi4Hist.push_back(o.psi4);       msdHist.push_back(o.msd);
        }
    }

    WriteCSV("md-observables.csv",
             { "time", "temperature", "kineticEnergy", "potentialEnergy", "totalEnergy", "psi6", "psi4", "msd" },
             { tHist, THist, KEHist, PEHist, EHist, psi6Hist, psi4Hist, msdHist });

    std::printf("Observables written to md-observables.csv\n");
    return 0;
}
