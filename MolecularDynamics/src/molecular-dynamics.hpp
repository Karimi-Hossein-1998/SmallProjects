#pragma once
// -----------------------------------------------------------------------------
// Umbrella header for the 2D molecular-dynamics engine.
//
// Include this to pull in the full physics stack (potentials, initial
// conditions, integrator/engine, thermostats & barostat, and analysis).
// Rendering (renderer.hpp, SDL) is deliberately kept separate.
// -----------------------------------------------------------------------------
#include "potential.hpp"
#include "initial-conditions.hpp"
#include "engine.hpp"
#include "thermostats.hpp"
#include "analysis.hpp"
