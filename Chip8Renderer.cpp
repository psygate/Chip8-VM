#include <stdexcept>
#include <iostream>
#include <sstream>
#include <cassert>
#include <memory>
#include <algorithm>

#include <SDL.h>

#include "Chip8Renderer.h"

/**
 * @brief Allocate a new framebuffer pointer.
 * @param width Width of the framebuffer (must be > 0)
 * @param height Height of the framebuffer (must be > 0)
 * @return A heap allocated pointer to an array with atleast size width * height.
*/
static int** new_framebuffer(const size_t width, const size_t height)
{
	int** arr = new int* [width];

	for (size_t i = 0; i < width; i++)
	{
		arr[i] = new int[height];
	}

	return arr;
}

/**
 * @brief Deallocate the framebuffer.
 * @param buffer Buffer to release.
 * @param width Width of the framebuffer (must be > 0)
 * @param height Height of the framebuffer (must be > 0)
*/
static void destroy_framebuffer(int** buffer, const size_t width, const size_t height)
{
	for (size_t i = 0; i < width; i++)
	{
		delete[] buffer[i];
	}

	delete[] buffer;
}

namespace Chip8VM::Rendering {
	DummyRenderer::DummyRenderer()
		:
		fbuffer_width(64),
		fbuffer_height(32),
		_framebuffer(new_framebuffer(fbuffer_width, fbuffer_height))
	{
		for (size_t x = 0; x < fbuffer_width; x++)
		{
			for (size_t y = 0; y < fbuffer_height; y++)
			{
				_framebuffer[x][y] = 0;
			}
		}

		clear();
	}

	DummyRenderer::~DummyRenderer()
	{
		destroy_framebuffer(_framebuffer, fbuffer_width, fbuffer_height);
	}


	void DummyRenderer::clear()
	{
		for (size_t x = 0; x < fbuffer_width; x++)
		{
			for (size_t y = 0; y < fbuffer_height; y++)
			{
				_framebuffer[x][y] = 0;
			}
		}
	}

	int DummyRenderer::set_pixel(const size_t x, const size_t y, const int value)
	{
		if (x < 0 || y < 0)
		{
			throw std::runtime_error("Out of bounds pixel access.");
		}
		else if (value != 0 && value != 1)
		{
			throw std::runtime_error("Value for pixel set out of bounds.");
		}

		const size_t dx(x % fbuffer_width);
		const size_t dy(y % fbuffer_height);
		const int old_value(_framebuffer[dx][dy]);

		_framebuffer[dx][dy] = value ^ old_value;

		return (old_value & value) & 0x1;
	}

	void DummyRenderer::update()
	{
		//Do nothing.
	}



	SDLRenderer::SDLRenderer(const size_t xscale, const size_t yscale)
		:
		DummyRenderer(),
		window(nullptr),
		renderer(nullptr),
		x_scale(xscale),
		y_scale(yscale),
		width(fbuffer_width* xscale),
		height(fbuffer_height* yscale)
	{
		if (fbuffer_width > width || fbuffer_height > height)
		{
			throw std::runtime_error("Frame buffer dimensions are out of range.");
		}

		window = SDL_CreateWindow("Chip8VM", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, 0);
		renderer = SDL_CreateRenderer(window, -1, 0);

		//Set the draw color of renderer to green
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

		//Clear the renderer with the draw color
		SDL_RenderClear(renderer);

		//Update the renderer which will show the renderer cleared by the draw color which is green
		SDL_RenderPresent(renderer);
	}

	SDLRenderer::~SDLRenderer()
	{
		SDL_DestroyRenderer(renderer);
		SDL_DestroyWindow(window);

		window = nullptr;
		renderer = nullptr;
	}


	static std::string as_hex(const std::vector<int>& buffer)
	{
		std::stringstream str;

		for (std::vector<int>::const_iterator it = buffer.cbegin(); it != buffer.cend(); it++)
		{
			str << *it;
		}

		return str.str();
	}

	void SDLRenderer::update()
	{
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);

		for (size_t x = 0; x < fbuffer_width; x++)
		{
			for (size_t y = 0; y < fbuffer_height; y++)
			{
				const SDL_Rect rect = {
					.x = static_cast<int>(x * x_scale),
					.y = static_cast<int>(y * y_scale),
					.w = static_cast<int>(x_scale),
					.h = static_cast<int>(y_scale)
				};

				if (_framebuffer[x][y] == 1)
				{
					SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
					SDL_RenderFillRect(renderer, &rect);
				}
			}
		}

		SDL_RenderPresent(renderer);
	}

	void ConsoleRenderer::update()
	{
		std::stringstream stream;

		for (size_t y = 0; y < fbuffer_height; y++)
		{
			for (size_t x = 0; x < fbuffer_width; x++)
			{
				if (_framebuffer[x][y] == 1)
				{
					stream << '#';
				}
				else
				{
					stream << ' ';
				}
			}

			stream << std::endl;
		}

		std::cout << stream.str() << std::endl;
	}

	void MultiRenderer::add_renderer(Renderer* renderer)
	{
		renderers.push_back(renderer);
	}

	void MultiRenderer::clear()
	{
		std::for_each(renderers.begin(), renderers.end(), [](auto renderer) { renderer->clear(); });
	}

	int MultiRenderer::set_pixel(const size_t x, const size_t y, const int value)
	{
		int ret = 0;
		std::for_each(renderers.begin(), renderers.end(), [&](auto renderer) { ret |= renderer->set_pixel(x, y, value); });
		
		return ret;
	}


	void MultiRenderer::update()
	{
		std::for_each(renderers.begin(), renderers.end(), [](auto renderer) { renderer->update(); });
	}
}
