#include "src/Gravity.hpp"
#include "src/canvas.hpp"


int main()
{
	double width = 800, height = 600, size = 8.0f, mass = 1.0, dt=0.001;
	size_t N = 10;
	Gravity particles(width,height,size,mass,N,false,true,41, Color(255,255,255,255),1.0);

	// Gravity particles(width,height,3,std::vector<double>({0.1*width,0.5*width,0.9*width}),std::vector<double>({0.1*height,0.5*height,0.9*height}),std::vector<double>({0.1,0.0,-0.1}),std::vector<double>({2.0,0.0,-2.0}),std::vector<double>({1.0,1000.0,27.0}),std::vector<double>({5.0,50.0,15.0}),std::vector<uint8_t>({255,255,0}),std::vector<uint8_t>({0,255,0}),std::vector<uint8_t>({0,255,255}),std::vector<uint8_t>({255,255,255}),false,true,1.0);
	Canvas canvas(width,height,"Particles");
	if (!canvas.CanvasCreateWindow())
	{
		std::printf("Couldn't Create Canvas... Quitting!\n"); return -1;
	}
	// populate the vector of Walker(s)
	SDL_Event event;
	bool running = true;
	while (running)
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
		particles.Draw(canvas.GetRenderer());
		for (size_t i{}; i<1000; ++i)
			particles.Update(dt);
		particles.Draw(canvas.GetRenderer());
		
		SDL_RenderPresent(canvas.GetRenderer());
		SDL_Delay(16);
	}
}
