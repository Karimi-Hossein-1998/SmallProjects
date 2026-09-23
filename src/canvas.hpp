#pragma once
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>
#include <string>
#include <utility>

class Canvas
{
	private:
		uint64_t Width;
		uint64_t Height;
		std::string Title;

		SDL_Window* Window;
		SDL_Renderer* Renderer;
	public:
		// Constructors
		Canvas() = default;
		Canvas(const Canvas&) = delete;
		Canvas(Canvas&& other) noexcept : Width(other.Width), Height(other.Height), Title(std::move(other.Title)), Window(std::exchange(other.Window, nullptr)), Renderer(std::exchange(other.Renderer,nullptr)) {}
		explicit Canvas(uint64_t w=900, uint64_t h=600, std::string t="SDL App") : Width(w), Height(h), Title(std::move(t)) {}
		// Assignments
		Canvas& operator=(const Canvas&) = delete;
		Canvas& operator=(Canvas&& other) noexcept
		{
			if((*this)==other)
			{
				return *this;
			}
			else
			{
				Destroy();
				Width=other.Width;
				Height=other.Height;
				Title=std::move(other.Title);
				Window=std::exchange(other.Window,nullptr);
				Renderer=std::exchange(other.Renderer,nullptr);
				return *this;
			}
		}
		// Destructor
		~Canvas() {Destroy();}
		// Comparison
		bool operator==(const Canvas& other) const noexcept
		{
			return (Width==other.Width && Height==other.Height && Title==other.Title && Window==other.Window && Renderer==other.Renderer)?true:false;
		}
		// Destroy()
		void Destroy() noexcept
		{
			if (Renderer)
			{
				SDL_DestroyRenderer(Renderer);
				Renderer = nullptr;
			}
			if (Window)
			{
				SDL_DestroyWindow(Window);
				Window = nullptr;
			}
			SDL_Quit();
		}
		// Create Window
		bool CanvasCreateWindow()
		{
			if (!SDL_Init(SDL_INIT_VIDEO))
			{
				std::printf("SDL Init Error: [SDL] %s\n",SDL_GetError());
				return false;
			}
			if (!SDL_CreateWindowAndRenderer(Title.c_str(), static_cast<int>(Width), static_cast<int>(Height), SDL_WINDOW_RESIZABLE, &Window, &Renderer))
			{
				std::printf("Window Creation Error: [SDL] %s\n",SDL_GetError());
				SDL_Quit();
				return false;
			}
			return true;
		}

		// Getters
		[[nodiscard]] SDL_Renderer* GetRenderer() const noexcept {return Renderer;}
		[[nodiscard]] SDL_Window* GetWindow() const noexcept {return Window;}
};

