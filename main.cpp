#define SDL_MAIN_HANDLED
#define CATCH_CONFIG_RUNNER

#include <map>
#include <fstream>
#include <iterator>
#include <algorithm>
#include <stdexcept>
#include <exception>

#include <SDL.h>
#include <catch2/catch_all.hpp>

#include "Chip8Renderer.h"
#include "Chip8VM.h"

using namespace std;

#define NDEBUG

// Keycode to Chip8 key translation table.
static map<SDL_Keycode, int> KEY_MAP = {
	{49, 0x1},
	{50, 0x2},
	{51, 0x3},
	{52, 0xC},
	{113, 0x4},
	{119, 0x5},
	{101, 0x6},
	{114, 0xD},
	{97, 0x7},
	{115, 0x8},
	{100, 0x9},
	{102, 0xE},
	{121, 0xA},
	{120, 0x0},
	{99, 0xB},
	{118, 0xF},
};

std::vector<Chip8VM::memval> load_cavern()
{
	std::vector<Chip8VM::memval> buffer;
	std::ifstream input("cavern.ch8", std::ios::binary);

	if (!input.good())
	{
		throw std::runtime_error("Couldnt open cavern.ch8.");
	}

	std::copy(
		std::istreambuf_iterator<char>(input),
		std::istreambuf_iterator<char>(),
		std::back_inserter(buffer)
	);

	return buffer;
}

std::vector<Chip8VM::memval> load_chipaquarium()
{
	std::vector<Chip8VM::memval> buffer;
	std::ifstream input("chipquarium.ch8", std::ios::binary);

	if (!input.good())
	{
		throw std::runtime_error("Couldnt open chipquarium.ch8.");
	}

	std::copy(
		std::istreambuf_iterator<char>(input),
		std::istreambuf_iterator<char>(),
		std::back_inserter(buffer)
	);

	return buffer;
}

std::vector<Chip8VM::memval> load_test_rom()
{
	std::vector<Chip8VM::memval> buffer;
	std::ifstream input("test_opcode.ch8", std::ios::binary);

	if (!input.good())
	{
		throw std::runtime_error("Couldnt open chipquarium.ch8.");
	}

	std::copy(
		std::istreambuf_iterator<char>(input),
		std::istreambuf_iterator<char>(),
		std::back_inserter(buffer)
	);

	return buffer;
}

std::vector<Chip8VM::memval> load_rom_file(const std::string& filename)
{
	std::vector<Chip8VM::memval> buffer;
	std::ifstream input(filename, std::ios::binary);

	if (!input.good())
	{
		throw std::runtime_error("Couldnt open chipquarium.ch8.");
	}

	std::copy(
		std::istreambuf_iterator<char>(input),
		std::istreambuf_iterator<char>(),
		std::back_inserter(buffer)
	);

	return buffer;
}

static int main_loop(
	const size_t cycle_rate,
	const size_t timer_rate,
	const size_t frame_rate,
	Chip8VM::VM& vm,
	Chip8VM::Rendering::Renderer& renderer
)
{
	bool quit = false;
	SDL_Event ev;

	Uint32 timer_start = SDL_GetTicks();
	Uint32 step_start = SDL_GetTicks();
	Uint32 frame_start = SDL_GetTicks();

	for (;;)
	{
		Uint32 now = SDL_GetTicks();

		if (SDL_PollEvent(&ev))
		{
			switch (ev.type)
			{
			case SDL_QUIT:
				quit = true;
				break;
			case SDL_KEYDOWN:
				if (KEY_MAP.contains(ev.key.keysym.sym))
				{
					vm.keydown(KEY_MAP[ev.key.keysym.sym]);
				}
				break;
			case SDL_KEYUP:
				if (KEY_MAP.contains(ev.key.keysym.sym))
				{
					vm.keyup();
				}
				break;
			}
		}

		if (quit) break;

		if (now - step_start >= 1000 / cycle_rate)
		{
			vm.step();
			step_start = now;
		}

		if (now - timer_start >= 1000 / timer_rate)
		{
			vm.update_timers();
			timer_start = now;
		}

		if (now - frame_start >= 1000 / frame_rate)
		{
			renderer.update();
			frame_start = now;
		}
	}

	return 0;
}

static void usage(const char* pgm_name)
{
	if (pgm_name != nullptr)
	{
		std::cout << pgm_name << " Usage:" << std::endl;
		std::cout << pgm_name << " <file path to rom>" << std::endl;
	}
}

static void parse_cmd_args(int argc, char** argv, std::string& filename)
{
	if (argc == 0)
	{
		usage(nullptr);
	}
	else if (argc == 2)
	{
		filename = argv[1];
		return;
	}
	else
	{
		usage(argv[0]);
	}

	filename = "";
}

int main(int argc, char** argv)
{
	std::string filename;
	parse_cmd_args(argc, argv, filename);

	if (filename == "")
	{
		return 0;
	}

	std::cout << "Loading ROM: " << std::string(filename) << std::endl;

	//Run unit tests
	int result = Catch::Session().run(argc, argv);
	if (result != 0) return result;

	vector<Chip8VM::memval> pgm(load_rom_file(filename));

	// Cycles per second of the Chip8 VM to target.
	const size_t TARGET_CYCLE_RATE = 500;
	// Cycles per second timers are decreased.
	const size_t TARGET_TIMER_RATE = 60;
	// Cycles per second the framebuffer / renderers are updated.
	const size_t TARGET_FRAME_RATE = 60;

	Chip8VM::Rendering::SDLRenderer sdl(4, 4);
	Chip8VM::Rendering::ConsoleRenderer console;
	Chip8VM::Rendering::MultiRenderer renderer;
	renderer.add_renderer(&sdl);
	renderer.add_renderer(&console);

	Chip8VM::VM vm(renderer);

	vm.load_program(pgm.cbegin(), pgm.cend());

	int retcode(
		main_loop( 
			TARGET_CYCLE_RATE,
			TARGET_TIMER_RATE,
			TARGET_FRAME_RATE,
			vm,
			renderer
		)
	);

	return 0;
}
