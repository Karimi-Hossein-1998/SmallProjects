#pragma once
#include <cstddef>
#include <cstdint>
#include <print>
#include <vector>
#include <SDL3/SDL.h>

class Conway
{
	private:
		std::vector<uint8_t> buffer;
		std::vector<uint8_t> backBuffer;
		int width;
		int height;
	public:
		// Constructors
		Conway();
		Conway(const Conway&) = default;
		Conway(Conway&&) noexcept = default;
		explicit Conway(size_t w=800, size_t h=600) : width(w), height(h), buffer(w*h,0), backBuffer(w*h,0) {}
		// Assignments
		Conway& operator=(const Conway&) = default;
		Conway& operator=(Conway&&) noexcept = default;
		// Destructor
		~Conway() = default;

		void InitState(const std::vector<std::pair<int, int>>& alives)
		{
			for (const auto& [w,h] : alives )
			{
				// std::println("x = {:^04d}, y = {:^04d}",w,h);
				if (w>width || h>height) continue;
				size_t index      = width*h+w;
				backBuffer[index] = 1;
				buffer[index]     = 1;
			}
		}
		int CountNeighbors(int x, int y)
		{
			int count = 0;
			for (int dy{-1}; dy<2; ++dy)
			{
				for (int dx{-1}; dx<2; ++dx)
				{
					if (dx==0 && dy==0) continue;
					int indexX = (x+dx+width)%width;
					int indexY = (y+dy+height)%height;
					count += backBuffer[indexY*width+indexX];
				}
			}
			return count;
		}

		bool IsAlive()
		{
			for (int i{0}; i<backBuffer.size(); ++i)
			{
				if (backBuffer[i]) return true;
			}
			return false;
		}

		void TakeStep()
		{
			for (int y{0}; y<height; ++y)
			{
				for (int x{0}; x<width; ++x)
				{
					int index = y*width+x;
					int neighborCount = CountNeighbors(x,y);
					uint8_t currentCell = backBuffer[index];
					buffer[index] = (neighborCount==3) || (currentCell && neighborCount==2);
				}
			}
			backBuffer.swap(buffer);
		}

		void Draw(SDL_Renderer* renderer)
		{
			SDL_SetRenderDrawColor(renderer,255,255,255,255);
			for (int y{0}; y<height; ++y)
			{
				for (int x{0}; x<width; ++x)
				{
					if (!backBuffer[y*width+x]) continue;
					SDL_RenderPoint(renderer, x, y);
				}
			}
		}
};
