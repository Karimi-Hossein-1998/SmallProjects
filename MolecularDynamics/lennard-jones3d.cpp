// -----------------------------------------------------------------------------
// Interactive SDL demo for the 3D molecular-dynamics engine.
//
// Default: a Lennard-Jones FCC crystal in a periodic box, evolved with plain
// NVE (no thermostat) so the total energy conservation can be checked, plus the
// Steinhardt bond-orientational order Q4/Q6 (Q6 ~ 0.575 for a perfect FCC
// crystal, ~0 for a liquid).
//
// Controls:
//   Esc / q             quit
//   s                   toggle colour-by-speed
//   arrows / WASD       rotate camera
//   + / - (or Q / E)    zoom
//   R                   reset camera
//
// On exit the observables are written to md3d-observables.csv.
// -----------------------------------------------------------------------------
#include <cstdio>
#include <string>
#include <vector>

#include "src/molecular-dynamics3d.hpp"
#include "src/renderer3d.hpp"
#include "src/canvas.hpp"

int main()
{
    MDConfig3D cfg;
    cfg.numParticles = 256;             // 4x4x4 FCC unit cells
    cfg.width  = 6.35;                  // reduced units (~1.12 sigma NN distance)
    cfg.height = 6.35;
    cfg.depth  = 6.35;
    cfg.sigma       = 1.0;
    cfg.epsilon     = 1.0;
    cfg.cutoffCoeff = 2.5;
    cfg.mass        = 1.0;
    cfg.radius      = 0.35;             // visual radius
    cfg.temperature = 0.2;              // low T so the crystal is stable
    cfg.restitution = 1.0;
    cfg.periodicBoundaryCondition = true;
    cfg.bounce       = false;
    cfg.hardSphereCollisions = false;
    cfg.potential    = PotentialType3D::LennardJones;
    cfg.initialCondition = InitialConditionType3D::FCC;
    cfg.seed         = 41;

    const double dt = 0.001;
    const int substepsPerFrame = 20;

    MolecularDynamics3D md(cfg);

    Canvas canvas(800, 600, "Molecular Dynamics 3D (Lennard-Jones)");
    if (!canvas.CanvasCreateWindow())
    {
        std::printf("Couldn't create canvas... quitting!\n");
        return -1;
    }

    MDCamera3D cam;
    cam.targetX = cfg.width * 0.5;
    cam.targetY = cfg.height * 0.5;
    cam.targetZ = cfg.depth * 0.5;
    cam.distance = 1.6 * std::max(std::max(cfg.width, cfg.height), cfg.depth);
    const MDCamera3D camDefault = cam;

    bool held[SDL_SCANCODE_COUNT] = {};
    bool running = true;
    bool colorBySpeed = false;
    size_t frame = 0;

    std::vector<double> tHist, THist, KEHist, PEHist, EHist, q6Hist, q4Hist, msdHist;

    while (running)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_QUIT) running = false;
            else if (event.type == SDL_EVENT_KEY_DOWN)
            {
                if (event.key.scancode == SDL_SCANCODE_ESCAPE || event.key.scancode == SDL_SCANCODE_Q) running = false;
                else if (event.key.scancode == SDL_SCANCODE_S) colorBySpeed = !colorBySpeed;
                else if (event.key.scancode < SDL_SCANCODE_COUNT) held[event.key.scancode] = true;
            }
            else if (event.type == SDL_EVENT_KEY_UP)
            {
                if (event.key.scancode < SDL_SCANCODE_COUNT) held[event.key.scancode] = false;
            }
        }

        const double rot = 0.04, zoom = 0.05;
        if (held[SDL_SCANCODE_LEFT]  || held[SDL_SCANCODE_A]) Orbit3D(cam, -rot, 0.0, 0.0);
        if (held[SDL_SCANCODE_RIGHT] || held[SDL_SCANCODE_D]) Orbit3D(cam,  rot, 0.0, 0.0);
        if (held[SDL_SCANCODE_UP]    || held[SDL_SCANCODE_W]) Orbit3D(cam, 0.0,  rot, 0.0);
        if (held[SDL_SCANCODE_DOWN]  || held[SDL_SCANCODE_S]) Orbit3D(cam, 0.0, -rot, 0.0);
        if (held[SDL_SCANCODE_EQUALS] || held[SDL_SCANCODE_KP_PLUS] || held[SDL_SCANCODE_E]) Orbit3D(cam, 0.0, 0.0, -zoom);
        if (held[SDL_SCANCODE_MINUS] || held[SDL_SCANCODE_Q]) Orbit3D(cam, 0.0, 0.0, +zoom);
        if (held[SDL_SCANCODE_R]) cam = camDefault;

        for (int k = 0; k < substepsPerFrame; ++k) md.Step(dt);

        SDL_SetRenderDrawColor(canvas.GetRenderer(), 10, 10, 12, 255);
        SDL_RenderClear(canvas.GetRenderer());
        DrawParticles3D(canvas.GetRenderer(), md, cam, 800.0, 600.0, colorBySpeed);
        SDL_RenderPresent(canvas.GetRenderer());

        ++frame;
        if (frame % 100 == 0)
        {
            const double time = frame * substepsPerFrame * dt;
            const Observables3D o = CollectObservables3D(md, time, /*orderCutoff*/ 1.4);
            std::printf("[t=%8.3f] T=%.4f  KE=%.4f  PE=%.4f  E=%.6f  Q6=%.4f  Q4=%.4f  MSD=%.4f\n",
                        o.time, o.temperature, o.kineticEnergy, o.potentialEnergy,
                        o.totalEnergy, o.q6, o.q4, o.msd);
            tHist.push_back(o.time);     THist.push_back(o.temperature);
            KEHist.push_back(o.kineticEnergy); PEHist.push_back(o.potentialEnergy);
            EHist.push_back(o.totalEnergy);   q6Hist.push_back(o.q6);
            q4Hist.push_back(o.q4);       msdHist.push_back(o.msd);
        }
    }

    WriteCSV3D("md3d-observables.csv",
               { "time", "temperature", "kineticEnergy", "potentialEnergy", "totalEnergy", "q6", "q4", "msd" },
               { tHist, THist, KEHist, PEHist, EHist, q6Hist, q4Hist, msdHist });
    std::printf("Observables written to md3d-observables.csv\n");
    return 0;
}
