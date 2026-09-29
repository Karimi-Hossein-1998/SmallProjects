# Build for the 2D molecular-dynamics project.
#   make            -> build the SDL demo and the headless validation harness
#   make validate   -> headless physics checks (no display needed)
#   make clean
CXX      ?= g++
CXXSTD   ?= c++20
OPT      ?= -O2
WARN     ?= -Wall -Wextra
OMP      ?= -fopenmp

SDL_CFLAGS := $(shell pkg-config --cflags sdl3 2>/dev/null)
SDL_LIBS   := $(shell pkg-config --libs sdl3 2>/dev/null)

CXXFLAGS := -std=$(CXXSTD) $(OPT) $(WARN) $(OMP) $(SDL_CFLAGS)

.PHONY: all clean validate

all: lennard-jones validate

lennard-jones: lennard-jones.cpp src/molecular-dynamics.hpp src/renderer.hpp \
               src/potential.hpp src/initial-conditions.hpp src/engine.hpp \
               src/thermostats.hpp src/analysis.hpp src/circle.hpp src/canvas.hpp
	$(CXX) $(CXXFLAGS) lennard-jones.cpp -o $@ $(SDL_LIBS)

validate: validate.cpp src/molecular-dynamics.hpp src/potential.hpp \
          src/initial-conditions.hpp src/engine.hpp src/thermostats.hpp src/analysis.hpp
	$(CXX) $(CXXFLAGS) validate.cpp -o $@

clean:
	rm -f lennard-jones validate md-observables.csv
