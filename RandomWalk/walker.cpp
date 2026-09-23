#include "src/walker.hpp"
#include <cstdio>
#include <vector>
#include <string>

inline constexpr const char* HELP = {"-h"};
inline constexpr const char* help = {"--help"};
inline constexpr const char* SWMS = {"-wms"};
inline constexpr const char* swms = {"--walker-move-style"};
inline constexpr const char* str8 = {"Straight"};
inline constexpr const char* diag = {"Diagonal"};
inline constexpr const char* stdi = {"StraightDiagonal"};
inline constexpr const char* stwc = {"StraightWCenter"};
inline constexpr const char* diwc = {"DiagonalWCenter"};
inline constexpr const char* sdwc = {"StraightDiagonalWCenter"};
inline constexpr const char* stco = {"StraightContinuous"};
inline constexpr const char* dico = {"DiagonalContinuous"};
inline constexpr const char* sdco = {"StraightDiagonalContinuous"};
inline constexpr const char* STR8 = {"s"};
inline constexpr const char* DIAG = {"d"};
inline constexpr const char* STDI = {"o"};
inline constexpr const char* STWC = {"+"};
inline constexpr const char* DIWC = {"x"};
inline constexpr const char* SDWC = {"q"};
inline constexpr const char* STCO = {"S"};
inline constexpr const char* DICO = {"D"};
inline constexpr const char* SDCO = {"Q"};
inline constexpr const char* SETW = {"-W"};
inline constexpr const char* SETH = {"-H"};
inline constexpr const char* SETN = {"-N"};

void printhelp()
{
	std::printf("Usage: The following flags are available.\n");
	std::printf("\t--no-trail OR -NT: Set trailing off.\n");
	std::printf("\t--aperture OR -A: Set the size of each walker (it expects a double-precision floating-point value).\n");
	std::printf("\t-TL OR --trail-length: Set the fading criterion for the trail.\n");
	std::printf("\t-N: Set the number of random walkers, i.e. `-N 10000` will set 10000 walkers.\n");
	std::printf("\t-W: Set the 'width' of the canvas, e.g. `-W 800`.\n");
	std::printf("\t-H: Set the 'height' of the canvas, e.g. `-H 600`.\n");
	std::printf("\t-wms OR --walker-move-style: Set the `style` of the movement of the random walkers.\n");
	std::printf("------------------------- Options for -wms are:\n");
	std::printf("\t\tStraight OR s: Random Walkers move in one of the four MAIN directions.\n");
	std::printf("\t\tDiagonal OR d: Random Walkers move in one of the four DIAGONAL directions.\n");
	std::printf("\t\tStraightDiagonal OR o: Random Walkers move in either MAIN or DIAGONAL directions.\n");
	std::printf("\t\tStraightWCenter OR +: Like \"--Straight\" but with added probability of staying still.\n");
	std::printf("\t\tDiagonalWCenter OR x: Like \"--Diagonal\" but with added probability of staying still.\n");
	std::printf("\t\tStraightDiagonalWCenter OR q: Like \"--StraightDiagonal\" but with dded probability of staying still.\n");
	std::printf("\t\tStraightContinuous OR S: Like \"--Straight\" but with a continuous step size between [-1.0,1.0).\n");
	std::printf("\t\tDiagonalContinuous OR D: Like \"--Diagonal\" but with a continuous step size between [-1.0,1.0).\n");
	std::printf("\t\tStraightDiagonalContinuous OR Q: Like \"--StraightDiagonal\" but with a continuous step size between [-1.0,1.0).\n");
	std::printf("------------------------- \n");
	std::printf("\t-h OR --help: Show this help.\n");
}

void setwms(int i, int argc, char** argv, WalkerMoveStyle& wms, uint16_t& wmsCount)
{
	std::string_view nextarg = argv[i];
	if (argc>i)
	{
		if (nextarg == str8 || nextarg == STR8)
		{
			wms = WalkerMoveStyle::Straight;
			wmsCount++;
		}
		else if (nextarg == diag || nextarg == DIAG)
		{
			wms = WalkerMoveStyle::Diagonal;
			wmsCount++;
		}
		else if (nextarg == stdi || nextarg == STDI)
		{
			wms = WalkerMoveStyle::StraightDiagonal;
			wmsCount++;
		}
		else if (nextarg == stwc || nextarg == STWC)
		{
			wms = WalkerMoveStyle::StraightWCenter;
			wmsCount++;
		}
		else if (nextarg == diwc || nextarg == DIWC)
		{
			wms = WalkerMoveStyle::DiagonalWCenter;
			wmsCount++;
		}
		else if (nextarg == sdwc || nextarg == SDWC)
		{
			wms = WalkerMoveStyle::StraightDiagonalWCenter;
			wmsCount++;
		}
		else if (nextarg == stco || nextarg == STCO)
		{
			wms = WalkerMoveStyle::StraightContinuus;
			wmsCount++;
		}
		else if (nextarg == dico || nextarg == DICO)
		{
			wms = WalkerMoveStyle::DiagonalContinuous;
			wmsCount++;
		}
		else if (nextarg == sdco || nextarg == SDCO)
		{
			wms = WalkerMoveStyle::StraightDiagonalContinuous;
			wmsCount++;
		}
		else
		{
			std::printf("No valid argument was provided (on flag %d which says \"%s\"), see \"--help\" OR \"-H\"",i,argv[i]);
		}
	}
	else
	{
		std::printf("Reached EOF!");
	}
}

inline void Run(bool trail, uint64_t width, uint64_t height, uint64_t numWalkers, double size, WalkerMoveStyle wms,uint64_t maxAge);

int main(int argc, char* argv[])
{
	bool trailing = true;
	WalkerMoveStyle wms = WalkerMoveStyle::Straight;
	uint64_t width = 900;
	uint64_t height = 600;
	uint64_t maxAge = 500;
	uint64_t numWalkers = 200;
	uint16_t wmsCount = 0;
	double size = 5.0;
	for (int i{1}; i<argc; ++i)
	{
		std::string_view arg = argv[i];
		if (arg == help || arg == HELP)
		{
			printhelp();
			return 0;
		}
		else if ((arg == swms || arg == SWMS) && wmsCount==0)
		{
			if (wmsCount==0)
			{
				++i;
				setwms(i,argc,argv,wms,wmsCount);
			}
			else
			{
				++i;
				std::printf("Have set WalkerMoveStyle before! Ignoring this (flag %d : %s) and corresponding value (flag %d : %s)",i-1,argv[i-1],i,argv[i]);
			}
		}
		else if (arg == SETW)
		{
			++i;
			if (argc>i)
			{
				width = std::stoul(argv[i]);
			}
			else
			{
				std::printf("Reached EOF!\n");
			}
		}
		else if (arg == SETH)
		{
			++i;
			if (argc>i)
			{
				height = std::stoul(argv[i]);
			}
			else
			{
				std::printf("Reached EOF!\n");
			}
		}
		else if (arg == SETN)
		{
			++i;
			if (argc>i)
			{
				numWalkers = std::stoul(argv[i]);
			}
			else
			{
				std::printf("Reached EOF!\n");
			}
		}
		else if (arg == "--no-trail" || arg == "-NT")
		{
			trailing = false;
		}
		else if (arg == "--trail-length" || arg == "-TL")
		{
			++i;
			if (argc>i)
			{
				maxAge = std::stoul(argv[i]);
			}
			else
			{
				std::printf("Reached EOF!\n");
			}
		}
		else if (arg == "--aperture" || arg == "-A")
		{
			++i;
			if (argc>i)
			{
				size = std::stod(argv[i]);
			}
			else
			{
				std::printf("Reached EOF!\n");
			}
		}
		else
		{
			std::printf("Undefined flag (on flag %d which says \"%s\")! ignoring...\n",i,argv[i]);
		}
	}
	Run(trailing, width, height, numWalkers, size, wms, maxAge);
}

inline void RunTrail(uint64_t width, uint64_t height, uint64_t numWalkers, double size, WalkerMoveStyle wms,uint64_t maxAge)
{
	// Walker walker(color,450.0,450.0,5.0,wms);
	Canvas canvas(width,height,"Walkers");
	if (!canvas.CanvasCreateWindow())
	{
		std::printf("Couldn't Create Canvas... Quitting!\n"); return;
	}
	// populate the vector of Walker(s)
	std::vector<Walker> walkers; walkers.reserve(numWalkers);
	for (uint64_t i{}; i<numWalkers; ++i)
		walkers.emplace_back(Walker(Color(RNG::uid256(RNG::rngr),RNG::uid256(RNG::rngr),RNG::uid256(RNG::rngr),255),width/2.0,height/2.0,size,wms));
	TrailManager trails(width,height,maxAge);
	bool running = true;
	SDL_Event event;
	while (running)
	{
		SDL_SetRenderDrawColor(canvas.GetRenderer(),5,5,5,255);
		SDL_RenderClear(canvas.GetRenderer());
		while (SDL_PollEvent(&event))
		{
			if (event.type==SDL_EVENT_QUIT)
			{
				running = false;
			}
		}
		trails.Step();
		for (uint64_t i{}; i<walkers.size(); ++i)
		{
			walkers[i].Step(width,height);
			trails.RecordWalker(i, walkers[i].GetX(), walkers[i].GetY(), walkers[i].GetSize(), walkers[i].GetR(), walkers[i].GetG(), walkers[i].GetB());
			// walkers[i].Draw(canvas.GetRenderer());
		}
		trails.Draw(canvas.GetRenderer());
		SDL_RenderPresent(canvas.GetRenderer());
		SDL_Delay(16);
	}
}
inline void RunNoTrail(uint64_t width, uint64_t height, uint64_t numWalkers, double size, WalkerMoveStyle wms,uint64_t maxAge)
{
	// Walker walker(color,450.0,450.0,5.0,wms);
	Canvas canvas(width,height,"Walkers");
	if (!canvas.CanvasCreateWindow())
	{
		std::printf("Couldn't Create Canvas... Quitting!\n"); return;
	}
	// populate the vector of Walker(s)
	std::vector<Walker> walkers; walkers.reserve(numWalkers);
	for (uint64_t i{}; i<numWalkers; ++i)
		walkers.emplace_back(Walker(Color(RNG::uid256(RNG::rngr),RNG::uid256(RNG::rngr),RNG::uid256(RNG::rngr),255),width/2.0,height/2.0,size,wms));
	// TrailManager trails(width,height,maxAge);
	bool running = true;
	SDL_Event event;
	while (running)
	{
		SDL_SetRenderDrawColor(canvas.GetRenderer(),5,5,5,255);
		SDL_RenderClear(canvas.GetRenderer());
		while (SDL_PollEvent(&event))
		{
			if (event.type==SDL_EVENT_QUIT)
			{
				running = false;
			}
		}
		// trails.Step();
		for (uint64_t i{}; i<walkers.size(); ++i)
		{
			walkers[i].Step(width,height);
			// trails.RecordWalker(i, walkers[i].GetX(), walkers[i].GetY(), walkers[i].GetSize(), walkers[i].GetR(), walkers[i].GetG(), walkers[i].GetB());
			walkers[i].Draw(canvas.GetRenderer());
		}
		// trails.Draw(canvas.GetRenderer());
		SDL_RenderPresent(canvas.GetRenderer());
		SDL_Delay(16);
	}
}
inline void Run(bool trail, uint64_t width, uint64_t height, uint64_t numWalkers, double size, WalkerMoveStyle wms,uint64_t maxAge)
{
	if (trail) return RunTrail(width,height,numWalkers,size,wms,maxAge);
	else return RunNoTrail(width,height,numWalkers,size,wms,maxAge);
}
