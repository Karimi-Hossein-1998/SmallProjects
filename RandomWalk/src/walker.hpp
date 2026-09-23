#pragma once
#include <SDL3/SDL.h>
#include <SDL3/SDL_assert.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_oldnames.h>
#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>
#include <cstdint>
#include <stdfloat>
#include <stddef.h>
#include <stdint.h>
#include <array>
#include <random>
#include <utility>
#include <algorithm>
#include "../../ankerl/unordered_dense.h"
#include "circle.hpp"
// #include <print>

namespace RNG{
	inline std::random_device rd;
	inline std::mt19937_64 rngr(rd());
	inline std::uniform_int_distribution<short> uid1(-1,1);
	inline std::uniform_int_distribution<short> uid4(0,3);
	inline std::uniform_int_distribution<short> uid5(0,4);
	inline std::uniform_int_distribution<short> uid8(0,7);
	inline std::uniform_int_distribution<short> uid256(0,255);
	inline std::uniform_real_distribution<double> urd1(-1,1);
	inline std::uniform_real_distribution<double> urd(0.0,1.0);
	inline std::bernoulli_distribution bd(0.5);
}

static constexpr std::array<std::pair<double,double>,4> DirectionStraight{
	{{1.0,0.0},{0.0,1.0},{-1.0,0.0},{0.0,-1.0}}
};
static constexpr std::array<std::pair<double,double>,4> DirectionDiagonal{
	{{1.0,1.0},{-1.0,1.0},{-1.0,-1.0},{1.0,-1.0}}
};
static constexpr std::array<std::pair<double,double>,5> DirectionStraightWCenter{
	{{0.0,0.0},{1.0,0.0},{0.0,1.0},{-1.0,0.0},{0.0,-1.0}}
};
static constexpr std::array<std::pair<double,double>,5> DirectionDiagonalWCenter{
	{{0.0,0.0},{1.0,1.0},{-1.0,1.0},{-1.0,-1.0},{1.0,-1.0}}
};
static constexpr std::array<std::pair<double,double>,8> DirectionStraightDiagonal{
	{{1.0,0.0},{1.0,1.0},{0.0,1.0},{-1.0,1.0},{-1.0,0.0},{-1.0,-1.0},{0.0,-1.0},{1.0,-1.0}}
};

enum class WalkerMoveStyle
{
	Straight=0,
	Diagonal,
	StraightDiagonal,
	StraightWCenter,
	DiagonalWCenter,
	StraightDiagonalWCenter,
	StraightContinuus,
	DiagonalContinuous,
	StraightDiagonalContinuous
};

class Walker
{
	private:
		Color  WalkerColor;
		ColorF WalkerColorF;
		double X;
		double Y;
		double Size;
		WalkerMoveStyle MoveStyle;
	public:
		// Constructor
		Walker() = default;
		Walker(const Walker&) = default;
		Walker(Walker&&) noexcept = default;
		explicit Walker(Color c=Color(0,255,0,255), double x=0.0, double y = 0.0, double s = 0.0, WalkerMoveStyle wms=WalkerMoveStyle::Straight) : WalkerColor(c), X(x), Y(y), Size(s), MoveStyle(wms) {WalkerColorF=WalkerColor;} 
		// Assignment
		Walker& operator=(const Walker&) = default;
		Walker& operator=(Walker&&) noexcept = default;
		// Destructor
		~Walker() = default;

		// Methods
		// Step
		void Step(uint64_t W, uint64_t H)
		{
			switch(MoveStyle)
			{
				case WalkerMoveStyle::Straight: {
					short rand = RNG::uid4(RNG::rngr);
					auto [dX,dY] = DirectionStraight[rand];
					X += dX; Y += dY;
					break;
				}
				case WalkerMoveStyle::Diagonal: {
					short rand = RNG::uid4(RNG::rngr);
					auto [dX,dY] = DirectionDiagonal[rand];
					X += dX; Y += dY;
					break;
				}
				case WalkerMoveStyle::StraightDiagonal: {
					short rand = RNG::uid8(RNG::rngr);
					auto [dX,dY] = DirectionStraightDiagonal[rand];
					X += dX; Y += dY;
					break;
				}
				case WalkerMoveStyle::StraightWCenter: {
					short rand = RNG::uid5(RNG::rngr);
					auto [dX,dY] = DirectionStraightWCenter[rand];
					X += dX; Y += dY;
					break;
				}
				case WalkerMoveStyle::DiagonalWCenter: {
					short rand = RNG::uid5(RNG::rngr);
					auto [dX,dY] = DirectionDiagonalWCenter[rand];
					X += dX; Y += dY;
					break;
				}
				case WalkerMoveStyle::StraightDiagonalWCenter: {
					X += RNG::uid1(RNG::rngr); Y += RNG::uid1(RNG::rngr);
					break;
				}
				case WalkerMoveStyle::StraightContinuus: {
					if (RNG::bd(RNG::rngr)) X += RNG::urd1(RNG::rngr);
					else          Y += RNG::urd1(RNG::rngr);
					break;
				}
				case WalkerMoveStyle::DiagonalContinuous: {
					double MoveSign = RNG::bd(RNG::rngr)?1.0:-1.0;
					double Move = RNG::urd1(RNG::rngr);
					X += Move; Y += Move*MoveSign;
					break;
				}
				case WalkerMoveStyle::StraightDiagonalContinuous: {
					X += RNG::urd1(RNG::rngr); Y += RNG::urd1(RNG::rngr);
					break;
				}
				default: {
					short rand = RNG::uid4(RNG::rngr);
					auto [dX,dY] = DirectionStraight[rand];
					X += dX; Y += dY;
					break;
				}
			}
			if (X>=W) X -= W; else if (X<=0.0) X += W;
			if (Y>=H) Y -= H; else if (Y<=0.0) Y += H;
		}
		void Draw(SDL_Renderer* renderer)
		{
			SDL_SetRenderDrawColor(renderer,static_cast<Uint8>(WalkerColor.GetR()),static_cast<Uint8>(WalkerColor.GetG()),static_cast<Uint8>(WalkerColor.GetB()),static_cast<Uint8>(WalkerColor.GetA()));
			float x = static_cast<float>(X); float y = static_cast<float>(Y); float size = static_cast<float>(Size);
			if (size<=1.0f)
			{
				SDL_RenderPoint(renderer,x,y);
			}
			else
			{
				// SDL_FRect rect{x-size/2.0f,y-size/2.0f,size,size};
				// SDL_RenderFillRect(renderer,&rect);
				// RenderFilledCircle(renderer,x,y,size*0.5,100,WalkerColorF);
				// RenderCircle(renderer,x,y,size*0.5,WalkerColorF,std::max(size*0.01,2.0),5*static_cast<int>(size));
				RenderEllipse(renderer,x,y,size,size*0.5,-std::numbers::pi_v<float>*0.125,WalkerColorF,std::max(size*0.01,2.0),5*static_cast<int>(size));
				// RenderCircleMidpoint(renderer,x,y,size*0.5);
			}
		}
		const double GetX() const noexcept {return X;}
		void SetX(double x) noexcept {X=x;} 
		const double GetY() const noexcept {return Y;}
		void SetY(double y) noexcept {Y=y;}
		const unsigned short GetR() const noexcept {return WalkerColor.GetR();}
		const unsigned short GetG() const noexcept {return WalkerColor.GetG();}
		const unsigned short GetB() const noexcept {return WalkerColor.GetB();}
		const unsigned short GetA() const noexcept {return WalkerColor.GetA();}
		const double GetSize() const noexcept {return Size;}
		void SetSize(double s) noexcept {Size=s;}
};


// Details for Trail effect
struct TrailCell
{
	double Size{1.0};
	uint64_t walkerID;
	uint64_t walkerAge{0};
	unsigned short R,G,B;
};

using TrailMap = ankerl::unordered_dense::map<uint64_t,TrailCell>;

class TrailManager
{
	private:
		TrailMap trailMap;
		uint64_t Width;
		uint64_t Height;
		uint16_t maxAge;

		[[nodiscard]] uint64_t ToKey(double x, double y) const noexcept
		{
			uint64_t ux = static_cast<uint64_t>(std::clamp(x,0.0,static_cast<double>(Width-1)));
			uint64_t uy = static_cast<uint64_t>(std::clamp(y,0.0,static_cast<double>(Height-1)));
			return ux + uy*Width;
		}
	public:
		TrailManager(uint64_t w, uint64_t h, uint16_t ma) : Width(w), Height(h), maxAge(ma) {}

		void Step()
		{
			for (auto& [key,cell] : trailMap) ++cell.walkerAge;
			std::erase_if(trailMap,[this](const auto& item)->bool{return item.second.walkerAge > maxAge;});
		}
		void RecordWalker(uint64_t walkerId, double x, double y, double size, unsigned short r, unsigned short g, unsigned short b)
		{
			uint64_t key = ToKey(x,y);
			trailMap[key] = TrailCell{
				.Size      = size,
				.walkerID = walkerId,
				.walkerAge = 0,
				.R = r,
				.G = g,
				.B = b
			};
		}

		void Draw(SDL_Renderer* renderer, ColorF WalkerColorF=ColorF{1.0f,0.0f,0.0f,1.0f})
		{
			SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
			for (const auto& [key,cell] : trailMap)
			{
				float x = static_cast<float>(key%Width);
				float y = static_cast<float>(key)/static_cast<float>(Width);
				float fade = 1.0f - static_cast<float>(cell.walkerAge)/static_cast<float>(maxAge);
				Uint8 alpha = static_cast<Uint8>(std::clamp(fade*255.0f,0.0f,255.0f));
				SDL_SetRenderDrawColor(renderer, cell.R, cell.G, cell.B, alpha);
				float size = static_cast<float>(cell.Size);
				if (cell.Size<=1.0) SDL_RenderPoint(renderer,x,y);
				else
				{
					// SDL_FRect
					// rect{x-size/2.0f,y-size/2.0f,size,size}; SDL_RenderFillRect(renderer,&rect);
					RenderFilledCircle(renderer,x,y,size*0.5,100,WalkerColorF);
				}
			}
		}
};

