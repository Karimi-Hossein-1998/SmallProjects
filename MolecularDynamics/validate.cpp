// -----------------------------------------------------------------------------
// Headless validation harness for the molecular-dynamics engine.
//
// Runs a few short, self-contained checks and prints the numbers so the physics
// can be sanity-checked without a display:
//   1. NVE energy conservation (velocity-Verlet)
//   2. crystallisation via Berendsen cooling -> psi6 should rise
//   3. thermostat temperature control
//   4. pair-potential force sign / cutoff sanity
// -----------------------------------------------------------------------------
#include <cmath>
#include <cstdio>

#include "src/molecular-dynamics.hpp"

static void printPotentialSanity()
{
    std::printf("== 4. pair-potential sanity ==\n");
    const PotentialType types[] = { PotentialType::LennardJones, PotentialType::WCA, PotentialType::Morse };
    const char* names[] = { "LennardJones", "WCA", "Morse" };

    for (int t = 0; t < 3; ++t)
    {
        PotentialParams p;
        p.sigma = 1.0; p.epsilon = 1.0; p.cutoffCoeff = 2.5; p.morseAlpha = 1.0;
        FinalizePotential(types[t], p);

        // at r = 0.9 sigma we expect repulsion (force pushes particles apart)
        double fFactor, pe;
        PairForceEnergy(0.81, types[t], p, fFactor, pe);
        const bool repulsive = fFactor < 0.0;   // dx>0, fFactor<0 -> force on i points -x (apart)

        // beyond the cutoff the interaction must vanish
        double fFar, peFar;
        PairForceEnergy(9.0, types[t], p, fFar, peFar);

        std::printf("  %-14s r=0.9: fFactor=%+.4f pe=%.4f (%s)   r=3.0: f=%.4f pe=%.4f (%s)\n",
                    names[t], fFactor, pe, repulsive ? "repulsive OK" : "WRONG",
                    fFar, peFar, (fFar == 0.0 && peFar == 0.0) ? "cutoff OK" : "cutoff WRONG");
    }
    std::printf("\n");
}

int main()
{
    printPotentialSanity();

    // ---- 1. NVE energy conservation ----------------------------------------
    {
        std::printf("== 1. NVE energy conservation (velocity-Verlet, dt=0.001) ==\n");
        MDConfig cfg;
        cfg.numParticles = 64;
        cfg.width = cfg.height = 11.22;   // 10x10 lattice of spacing 2^(1/6)
        cfg.sigma = 1.0; cfg.epsilon = 1.0; cfg.cutoffCoeff = 2.5;
        cfg.temperature = 0.5; cfg.seed = 41;
        cfg.periodicBoundaryCondition = true;
        cfg.bounce = false;
        cfg.initialCondition = InitialConditionType::SquareLattice;
        cfg.potential = PotentialType::LennardJones;

        MolecularDynamics md(cfg);
        md.ComputeKineticEnergy(); md.ComputePotentialEnergy();
        const double E0 = md.kineticEnergy + md.potentialEnergy;
        double Emin = E0, Emax = E0;

        const double dt = 0.001;
        const int steps = 5000;
        for (int s = 0; s < steps; ++s)
        {
            md.Step(dt);
            if (s % 10 == 0)
            {
                md.ComputeKineticEnergy(); md.ComputePotentialEnergy();
                const double E = md.kineticEnergy + md.potentialEnergy;
                if (E < Emin) Emin = E;
                if (E > Emax) Emax = E;
            }
        }
        const double drift = (Emax - Emin) / std::abs(E0);
        std::printf("  E0=%.6f  E in [%.6f, %.6f]  relative drift=%.3e  %s\n\n",
                    E0, Emin, Emax, drift, drift < 5e-3 ? "(OK)" : "(large)");
    }

    // ---- 2. crystallisation (psi6 should rise) -----------------------------
    {
        std::printf("== 2. Berendsen cooling -> psi6 ordering ==\n");
        MDConfig cfg;
        cfg.numParticles = 100;
        cfg.width = cfg.height = 11.22;
        cfg.sigma = 1.0; cfg.epsilon = 1.0; cfg.cutoffCoeff = 2.5;
        cfg.temperature = 1.5; cfg.seed = 7;
        cfg.periodicBoundaryCondition = true;
        cfg.bounce = false;
        cfg.initialCondition = InitialConditionType::SquareLattice;
        cfg.potential = PotentialType::LennardJones;

        MolecularDynamics md(cfg);
        const OrderParams start = ComputeBondOrientationalOrder(md, 1.4);

        const double dt = 0.001;
        const int steps = 20000;
        for (int s = 0; s < steps; ++s)
        {
            md.Step(dt);
            ApplyBerendsen(md, 0.10, dt, 2.0);
        }
        const Observables o = CollectObservables(md, steps * dt, 1.4);
        const OrderParams end = ComputeBondOrientationalOrder(md, 1.4);

        std::printf("  psi6: start=%.4f -> end=%.4f   T: 1.5 -> %.4f  %s\n\n",
                    start.psi6, end.psi6, o.temperature,
                    end.psi6 > start.psi6 + 0.3 ? "(ordered, OK)" : "(check)");
    }

    // ---- 3. thermostat temperature control ---------------------------------
    {
        std::printf("== 3. thermostat control (target T = 0.8) ==\n");
        const char* names[] = { "Rescale", "Berendsen", "Andersen", "Langevin" };
        for (int m = 0; m < 4; ++m)
        {
            MDConfig cfg;
            cfg.numParticles = 64;
            cfg.width = cfg.height = 11.22;
            cfg.sigma = 1.0; cfg.epsilon = 1.0; cfg.cutoffCoeff = 2.5;
            cfg.temperature = 2.0; cfg.seed = 41;
            cfg.periodicBoundaryCondition = true;
            cfg.bounce = false;
            cfg.initialCondition = InitialConditionType::SquareLattice;
            cfg.potential = PotentialType::LennardJones;

            MolecularDynamics md(cfg);
            size_t rngSeed = 123;
            const double dt = 0.001;
            double Tsum = 0.0;
            int    Tcount = 0;
            for (int s = 0; s < 8000; ++s)
            {
                md.Step(dt);
                switch (m)
                {
                    case 0: ApplyVelocityRescale(md, 0.8); break;
                    case 1: ApplyBerendsen(md, 0.8, dt, 1.0); break;
                    case 2: ApplyAndersen(md, 0.8, dt, 5.0, rngSeed); break;
                    case 3: ApplyLangevin(md, 0.8, dt, 1.0, rngSeed); break;
                }
                if (s >= 4000)   // time-average after equilibration (small-N T fluctuates ~18%)
                {
                    md.ComputeKineticEnergy();
                    Tsum += md.currentTemperature;
                    ++Tcount;
                }
            }
            std::printf("  %-10s <T> = %.4f (target 0.8)\n", names[m], Tsum / Tcount);
        }
        std::printf("\n");
    }

    std::printf("Done.\n");
    return 0;
}
