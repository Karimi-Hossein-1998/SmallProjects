#pragma once
#include <SDL3/SDL_camera.h>
#include <SDL3/SDL_oldnames.h>
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_render.h>
#include <algorithm>
#include <cstddef>
#include <limits>
#include <print>
#include <random>
#include <span>
#include <stdexcept>
#include <vector>
#include "circle.hpp"

// inline constexpr const double epsSqr=1.0e-10;

class LennardJones
{
	private:
		std::vector<double> mass;
		std::vector<double> massInv;
		std::vector<double> posX;
		std::vector<double> posY;
		std::vector<double> velX;
		std::vector<double> velY;
		std::vector<double> accX;
		std::vector<double> accY;
		std::vector<double> particleRadi;
		std::vector<uint8_t> red;
		std::vector<uint8_t> green;
		std::vector<uint8_t> blue;
		std::vector<uint8_t> alpha;
		std::vector<float> redF;
		std::vector<float> greenF;
		std::vector<float> blueF;
		std::vector<float> alphaF;
		double width, height, restitution;
		double ljFactor, ljSigma, cutOffCoefficient, epsilon;
		double ljSigmaSqr, distCutOff, distCutOffSqr, numParticlesInv;
		size_t numParticles;
		bool periodicBoundaryCondition, bounce;

		double theKineticEnergy, currentTemperature;

	public:
		// Constructors
		LennardJones() = default;
		LennardJones(const LennardJones&) = default;
		LennardJones(LennardJones&&) noexcept = default;
		explicit LennardJones(double w=800, double h=600, double ps=4.0, double m=1.0, size_t n=100, bool pbc=false, bool bnc=false, size_t seed=41, Color c=Color(255,255,255,255), double sig=5.0, double coc=2.5, double eps=1.0, double temperature=1.0, double rest=0.5)
		: width(w), height(h), numParticles(n), periodicBoundaryCondition(pbc), bounce(bnc), ljSigma(sig), epsilon(eps), cutOffCoefficient((coc))
		{
			numParticlesInv = 1.0 / static_cast<double>(numParticles);
			restitution = std::clamp(rest,0.0,1.0);
			ljFactor = 24.0*epsilon; ljSigmaSqr = ljSigma*ljSigma; distCutOff = ljSigma*cutOffCoefficient; distCutOffSqr = distCutOff*distCutOff;
			std::mt19937_64 rng(seed);
			std::normal_distribution<double> velDist(0.0,1.0);
			posX.reserve(numParticles); posY.reserve(numParticles); velX.reserve(numParticles); velY.reserve(numParticles); mass.reserve(numParticles); accX.reserve(numParticles); accY.reserve(numParticles); particleRadi.reserve(numParticles); massInv.reserve(numParticles);
			const double distOpt = ljSigma*pow(2,0.17);
			const size_t cols    = static_cast<size_t>(width/distOpt);
			double totalPX       = 0.0;
			double totalPY       = 0.0;
			double totalMass     = 0.0;
			theKineticEnergy     = 0.0;
			for (size_t i{}; i<numParticles; i++)
			{
				size_t row = i/cols;
				size_t col = i%cols;
				mass.push_back(m);
				posX.push_back(col*distOpt+1.0); posY.push_back(row*distOpt+1.0);
				accX.push_back(0.0); accY.push_back(0.0);
				massInv.push_back(1.0/mass[i]);
				totalMass += mass[i];
				velX.push_back(velDist(rng)*sqrt(temperature*massInv[i])); velY.push_back(velDist(rng)*sqrt(temperature*massInv[i]));
				totalPX += mass[i]*velX[i]; totalPY += mass[i]*velY[i];
				particleRadi.push_back(ps);
			}

			const double totalMassInv = 1.0 / totalMass;
			const double velCoMX = totalPX * totalMassInv;
			const double velCoMY = totalPY * totalMassInv;

			for (size_t i{}; i<numParticles; ++i)
			{
				velX[i] -= velCoMX; velY[i] -= velCoMY;
				theKineticEnergy += 0.5*mass[i]*(velX[i]*velX[i]+velY[i]*velY[i]);
			}
			double currentTemp = theKineticEnergy  * numParticlesInv;
			std::print("Initial temperature before tuning is: {}\n",currentTemp);
			if (currentTemp > std::numeric_limits<double>::epsilon())
			{
				const double scale = sqrt(temperature/currentTemp);
				theKineticEnergy = 0.0;
				for (size_t i{}; i<numParticles; ++i)
				{
					velX[i] *= scale;
					velY[i] *= scale;
					theKineticEnergy += 0.5*mass[i]*(velX[i]*velX[i]+velY[i]*velY[i]);
				}
				currentTemperature = theKineticEnergy * numParticlesInv;
				std::print("Initial temperature after tuning is: {}\n",currentTemperature);
			}
			
			CalculateAccelerations();
			red=std::vector<uint8_t>(numParticles,c.GetR());   redF=std::vector<float>(numParticles,static_cast<float>(c.GetR())/255.0);
			green=std::vector<uint8_t>(numParticles,c.GetG()); greenF=std::vector<float>(numParticles,static_cast<float>(c.GetG())/255.0);
			blue=std::vector<uint8_t>(numParticles,c.GetB());  blueF=std::vector<float>(numParticles,static_cast<float>(c.GetB())/255.0); 
			alpha=std::vector<uint8_t>(numParticles,c.GetA()); alphaF=std::vector<float>(numParticles,static_cast<float>(c.GetA())/255.0);
		}
		// Assignments
		LennardJones& operator=(const LennardJones&) = default;
		LennardJones& operator=(LennardJones&&) noexcept = default;
		// Destructor
		~LennardJones() = default;

		void SetTemperature(double temperature)
		{
			if (currentTemperature > std::numeric_limits<double>::epsilon())
			{
				theKineticEnergy  = 0.0;
				const double scale = sqrt(temperature/currentTemperature);
				for (size_t i{}; i<numParticles; ++i)
				{
					velX[i] *= scale;
					velY[i] *= scale;
					theKineticEnergy += mass[i]*(velX[i]*velX[i] + velY[i]*velY[i]);
				}
				currentTemperature = theKineticEnergy * numParticlesInv;
				std::print("Set temperature to (SetTemperature): {}\n",currentTemperature);
			}
		}
		void ScaleTemperature(double scale)
		{
			const double velScale = sqrt(scale);
			theKineticEnergy      = 0.0;
			for (size_t i{}; i<numParticles; ++i)
			{
				velX[i] *= velScale;
				velY[i] *= velScale;
				theKineticEnergy += mass[i]*(velX[i]*velX[i] + velY[i]*velY[i]);
			}
			currentTemperature = theKineticEnergy * numParticlesInv;
			std::print("Set temperature to (ScaleTemperature): {}\n",currentTemperature);
		}
		void CalculateAccelerations()
		{
			#pragma omp parallel for schedule(static)
			for (size_t i=0; i<numParticles; ++i)
			{
				const double posXNow = posX[i]; const double posYNow = posY[i]; double ax = 0.0; double ay = 0.0;
				#pragma omp simd reduction(+:ax,ay)
				for (size_t j=0; j<numParticles; ++j)
				{
					if (i==j) continue;

					const double deltaPosX = posX[j]-posXNow;
					const double deltaPosY = posY[j]-posYNow;
					const double distSqr = deltaPosX*deltaPosX + deltaPosY*deltaPosY;

					if (distSqr>distCutOffSqr) continue;

					const double distSqrInv  = 1.0 / (distSqr+ epsSqr);
					const double ljSigmaSqrOdSI = ljSigmaSqr*distSqrInv;
					const double ljSigmaSqrOdSIThree = ljSigmaSqrOdSI*ljSigmaSqrOdSI*ljSigmaSqrOdSI;
					const double ljSigmaSqrOdSISix = ljSigmaSqrOdSIThree*ljSigmaSqrOdSIThree;
					const double potential = ljFactor*distSqrInv*(ljSigmaSqrOdSIThree-2*ljSigmaSqrOdSISix);

					ax += deltaPosX * potential;
					ay += deltaPosY * potential;
				}
				accX[i] = ax*massInv[i]; accY[i] = ay*massInv[i];
			}
		}

		void Update(double dt)
		{
			const double dtHalf = dt*0.5;
			#pragma omp parallel for schedule(static)
			for (size_t i=0; i<numParticles; ++i)
			{
				velX[i] += dtHalf*accX[i];
				posX[i] += dt*velX[i];
				velY[i] += dtHalf*accY[i];
				posY[i] += dt*velY[i];
			}
			// ResolveParticleCollisions();
			// Bounds Checking
			if (periodicBoundaryCondition)
			{
				int64_t nX{}, nY{};
				for (size_t i=0; i<numParticles; ++i)
				{
					nX = static_cast<int64_t>(floor(posX[i]/width));
					nY = static_cast<int64_t>(floor(posY[i]/height));
					posX[i] -= nX*width;
					posY[i] -= nY*height;
				}
			}
			if (bounce)
			{
				ResolveWallCollisions();
			}
			CalculateAccelerations();
			#pragma omp parallel for schedule(static)
			for (size_t i=0; i<numParticles; ++i)
			{
				velX[i] += dtHalf*accX[i];
				velY[i] += dtHalf*accY[i];
			}
		}
		void ResolveParticleCollisions()
		{
			for (size_t i{}; i<numParticles; ++i)
			{
				for (size_t j{i+1}; j<numParticles; ++j)
				{
					const double deltaPosX = posX[j]-posX[i];
					const double deltaPosY = posY[j]-posY[i];
					const double distSqr   = deltaPosX*deltaPosX + deltaPosY*deltaPosY;
					const double minDist   = particleRadi[i] + particleRadi[j];
					if (distSqr>minDist*minDist) continue;

					const double dist    = sqrt(distSqr);
					const double normalX = deltaPosX / dist;
					const double normalY = deltaPosY / dist;

					const double overlap    = minDist - dist;
					const double massInvSum = massInv[i] + massInv[j];

					const double corrI = overlap * massInv[i] / massInvSum;
					const double corrJ = overlap * massInv[j] / massInvSum;

					posX[i] -= normalX * corrI;
					posY[i] -= normalY * corrI;
					posX[j] += normalX * corrJ;
					posY[j] += normalY * corrJ;

					const double relVelX   = velX[j] - velX[i];
					const double relVelY   = velY[j] - velY[i];
					const double normalVel = relVelX*normalX + relVelY*normalY;

					if (normalVel>=0.0) continue;

					const double impulse  = -(1.0+restitution)*normalVel / massInvSum;
					const double impulseI = impulse*massInv[i];
					const double impulseJ = impulse*massInv[j];

					velX[i] -= impulseI*normalX;
					velY[i] -= impulseI*normalY;
					velX[j] += impulseJ*normalX;
					velY[j] += impulseJ*normalY;
				}
			}
		}

		void ResolveWallCollisions()
		{
			#pragma omp parallel for schedule(static)
			for (size_t i=0; i<numParticles; ++i)
			{
				const double r = particleRadi[i];
				if (posX[i] - r < 0.0)
				{
					posX[i] = r;
					velX[i] = -velX[i]*restitution;
				}
				else if (posX[i] + r > width)
				{
					posX[i] = width-r;
					velX[i] = -velX[i]*restitution;
				}
				if (posY[i] - r < 0.0)
				{
					posY[i] = r;
					velY[i] = -velY[i]*restitution;
				}
				else if (posY[i] + r > height)
				{
					posY[i] = height-r;
					velY[i] = -velY[i]*restitution;
				}
			}
		}

		void Draw(SDL_Renderer* renderer)
		{
			float size{0.0};
			for (size_t i{}; i<numParticles; ++i)
			{
				SDL_SetRenderDrawColor(renderer,red[i],green[i],blue[i],alpha[i]);
				size = static_cast<float>(particleRadi[i]);
				if (size<=1.0f)
				{
					SDL_RenderPoint(renderer,static_cast<float>(posX[i]),static_cast<float>(height-posY[i]));
				}
				else
				{
					RenderCircle(renderer,static_cast<float>(posX[i]),static_cast<float>(height-posY[i]),size,ColorF(red[i],green[i],blue[i],alpha[i]),std::max(size*0.1f,2.0f),10*static_cast<int>(size));
				}
			}
		}
};
