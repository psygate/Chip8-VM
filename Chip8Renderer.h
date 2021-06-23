#pragma once

#include <vector>
#include <memory>

#include <SDL.h>

namespace Chip8VM::Rendering {
	class Renderer
	{
	public:
		/**
		 * @brief Clears the frame buffer (set to black)
		*/
		virtual void clear() = 0;
		/**
		 * @brief Sets a pixel @x and @y, if x or y are larger than the frame buffer, this will wrap by x mod width and y mod height.
		 * @param x X-Coord
		 * @param y Y-Coord
		 * @param value Either 1 or 0.
		 * @return 1 if a pixel was turned off by the current set_pixel call.
		*/
		virtual int set_pixel(const size_t x, const size_t y, const int value) = 0;
		/**
		 * @brief Flush the frame buffer to display / console, wherever the Renderer points to.
		*/
		virtual void update() = 0;
	};

	class DummyRenderer : public Renderer
	{
	public:
		//Framebuffer width
		const size_t fbuffer_width;
		//Framebuffer height
		const size_t fbuffer_height;

		/**
		 * @brief Create a dummy renderer with a frame buffer and 64x32 display.
		*/
		DummyRenderer();

		virtual ~DummyRenderer();
		virtual void clear();
		virtual int set_pixel(const size_t x, const size_t y, const int value);
		virtual void update() override;

	protected:
		int** _framebuffer;
	};

	class SDLRenderer : public DummyRenderer
	{
	private:
		SDL_Window* window;
		SDL_Renderer* renderer;
	public:
		/**
		 * @brief Create a new dummy renderer with these dimensions for the framebuffer.
		 * @param xscale Multiply width of default framebuffer by this amount.
		 * @param yscale Multiply height of default framebuffer by this amount.
		*/
		SDLRenderer(const size_t xscale, const size_t yscale);
		virtual ~SDLRenderer();
		virtual void update() override;

		/**
		 * @brief Scale the display by this value.
		*/
		const float x_scale;
		/**
		 * @brief Scale the display by this value.
		*/
		const float y_scale;
		/**
		 * @brief Final canvas width
		*/
		const size_t width;
		/**
		 * @brief Final canvas height.
		*/
		const size_t height;
	};

	/**
	 * @brief A simple renderer that renders to console.
	*/
	class ConsoleRenderer : public DummyRenderer
	{
	public:
		virtual void update() override;
	};

	/**
	 * @brief Renders to multiple targets.
	*/
	class MultiRenderer : public Renderer
	{
	private:
		std::vector<Renderer *> renderers;
	public:
		void add_renderer(Renderer* renderer);

		virtual void clear();
		virtual int set_pixel(const size_t x, const size_t y, const int value);
		virtual void update() override;
	};
}