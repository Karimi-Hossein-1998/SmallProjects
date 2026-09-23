#include "src/Conway.hpp"
#include "src/canvas.hpp"
#include <random>

int main(int argc, char* argv[])
{
	int width = 800;
	int height = 600;

	double probLife = 0.5;
	if (argc>1) probLife = std::stod(argv[1]);
	std::vector<std::pair<int,int>> initials;
	std::random_device rd{};
	std::mt19937_64 rng(rd());
	std::bernoulli_distribution bernoulli(probLife);

	for (int y{0}; y<height; ++y)
	{
		for (int x{0}; x<width; ++x)
		{
			if (bernoulli(rng)) initials.push_back({x,y});
		}
	}

	Conway conway(width,height);
	conway.InitState(initials);

	Canvas canvas(width,height,"Conway");

	if (!canvas.CanvasCreateWindow()) return -1;

	bool running = true;
	SDL_Event event;
	while(running)
	{
		SDL_SetRenderDrawColor(canvas.GetRenderer(),0,0,0,255);
		SDL_RenderClear(canvas.GetRenderer());
		while (SDL_PollEvent(&event))
		{
			if (event.type==SDL_EVENT_QUIT)
			{
				running = false;
			}
		}
		conway.TakeStep();
		conway.Draw(canvas.GetRenderer());

		if (!conway.IsAlive()) running = false;

		SDL_RenderPresent(canvas.GetRenderer());
		SDL_Delay(33);
	}
}

