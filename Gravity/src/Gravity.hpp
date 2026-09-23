#pragma once
#include <SDL3/SDL_oldnames.h>
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_render.h>
#include <algorithm>
#include <cstddef>
#include <random>
#include <span>
#include <stdexcept>
#include <vector>
#include "circle.hpp"

// inline constexpr const double epsSqr=1.0e-10;

class Gravity
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
		size_t numParticles;
		bool periodicBoundaryCondition, bounce;

	public:
		// Constructors
		Gravity() = default;
		Gravity(const Gravity&) = default;
		Gravity(Gravity&&) noexcept = default;
		explicit Gravity(double w=800, double h=600, double ps=4.0, double m=1.0, size_t n=100, bool pbc=false, bool bnc=false, size_t seed=41, Color c=Color(255,255,255,255), double rest=0.05) : width(w), height(h), numParticles(n), periodicBoundaryCondition(pbc), bounce(bnc)
		{
			restitution = std::clamp(rest,0.0,1.0);
			std::mt19937_64 rng(seed);
			std::uniform_real_distribution<double> urd;
			posX.reserve(numParticles); posY.reserve(numParticles); velX.reserve(numParticles); velY.reserve(numParticles); mass.reserve(numParticles); accX.reserve(numParticles); accY.reserve(numParticles); particleRadi.reserve(numParticles); massInv.reserve(numParticles);
			for (size_t i{}; i<numParticles; i++)
			{
				mass.push_back(m);
				posX.push_back(urd(rng)*width); posY.push_back(urd(rng)*height);
				velX.push_back((urd(rng)-0.5)*0.002); velY.push_back((urd(rng)-0.5)*0.002);
				accX.push_back(0.0); accY.push_back(0.0);
				massInv.push_back(1.0/mass[i]);
				particleRadi.push_back(ps);
			}
			CalculateAccelerations();
			red=std::vector<uint8_t>(numParticles,c.GetR());   redF=std::vector<float>(numParticles,static_cast<float>(c.GetR())/255.0);
			green=std::vector<uint8_t>(numParticles,c.GetG()); greenF=std::vector<float>(numParticles,static_cast<float>(c.GetG())/255.0);
			blue=std::vector<uint8_t>(numParticles,c.GetB());  blueF=std::vector<float>(numParticles,static_cast<float>(c.GetB())/255.0); 
			alpha=std::vector<uint8_t>(numParticles,c.GetA()); alphaF=std::vector<float>(numParticles,static_cast<float>(c.GetA())/255.0);
		}
		Gravity(double w, double h, size_t n, std::span<const double> pX, std::span<const double> pY, std::span<const double> vX, std::span<const double> vY, std::span<const double> m, std::span<const double> radi, std::span<const uint8_t> r, std::span<const uint8_t> g, std::span<const uint8_t> b, std::span<const uint8_t> a, bool pbc = false, bool bnc = false, double rest = 0.05)
		: width(w), height(h), numParticles(n), posX(pX.begin(),pX.end()), posY(pY.begin(),pY.end()), velX(vX.begin(), vX.end()), velY(vY.begin(),vY.end()), mass(m.begin(),m.end()), particleRadi(radi.begin(), radi.end()), red(r.begin(), r.end()), green(g.begin(),g.end()), blue(b.begin(),b.end()), alpha(a.begin(), a.end()), periodicBoundaryCondition(pbc), bounce(bnc)
		{
			if (posX.size()!=numParticles || posY.size()!=numParticles || velX.size()!=numParticles || velY.size()!=numParticles ||
			    mass.size()!=numParticles || red.size()!=numParticles || green.size()!=numParticles || blue.size()!=numParticles ||
				alpha.size()!=numParticles || particleRadi.size()!=numParticles)
				throw std::invalid_argument("The size of the attribute lists do not match the size of the system (number of particles)\n");

			restitution = std::clamp(rest,0.0,1.0);

			massInv.reserve(numParticles); redF.reserve(numParticles); greenF.reserve(numParticles); blueF.reserve(numParticles); alphaF.reserve(numParticles); accX.reserve(numParticles); accY.reserve(numParticles);
			for (size_t i=0; i<numParticles; ++i)
			{
				accX.push_back(0.0); accY.push_back(0.0);
				massInv.push_back(1.0/mass[i]);
				redF.push_back(static_cast<float>(red[i])/255.0); greenF.push_back(static_cast<float>(green[i])/255.0); blueF.push_back(static_cast<float>(blue[i])/255.0); alphaF.push_back(static_cast<float>(alpha[i])/255.0); 
 			}
			CalculateAccelerations();
		}
		// Assignments
		Gravity& operator=(const Gravity&) = default;
		Gravity& operator=(Gravity&&) noexcept = default;
		// Destructor
		~Gravity() = default;

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
					// const double distSqr   = deltaPosX*deltaPosX+deltaPosY*deltaPosY+epsSqr;
					// const double distance  = sqrt(distSqr);
					const double distInv  = 1.0 / sqrt(deltaPosX*deltaPosX + deltaPosY*deltaPosY);
					const double distThreeInv = mass[j] * distInv*distInv*distInv;
					ax += deltaPosX * distThreeInv;
					ay += deltaPosY * distThreeInv;
				}
				accX[i] = ax; accY[i] = ay;
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
			ResolveParticleCollisions();
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
