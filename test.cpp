#include <algorithm>
#include <functional>
#include <vector>
#include <SDL.h>

#include <catch2/catch_all.hpp>

#include "Chip8VM.h"
#include "Chip8Renderer.h"

using namespace Chip8VM;

/**
* File containing all unit tests.
*/

TEST_CASE("Test state initialization", "[State]") {
	State state;
	REQUIRE(state.program_load_offset() == DEFAULT_PROGRAM_LOAD_OFFSET);
	REQUIRE(state.register_size() == DEFAULT_REGISTER_SIZE);
	REQUIRE(state.flag_register_index() == DEFAULT_FLAG_REGISTER);
	REQUIRE(state.font_offset() == DEFAULT_FONT_OFFSET);
	REQUIRE(state.memory().size() == DEFAULT_MEMORY_SIZE);

	std::vector<memval> ref;
	ref.resize(state.memory_size());
	{
		std::fill(ref.begin(), ref.end(), 0);
		auto begin(ref.begin());
		std::advance(begin, DEFAULT_FONT_OFFSET);

		std::copy(DEFAULT_FONT_DATA.begin(), DEFAULT_FONT_DATA.end(), begin);
	}
	REQUIRE_THAT(state.memory(), Catch::Matchers::Equals(ref));

	for (memptr i = 0; i < state.memory_size(); i++)
	{
		state.memory(i, i);
		ref[i] = i;
	}

	REQUIRE_THAT(state.memory(), Catch::Matchers::Equals(ref));
	{
		std::fill(ref.begin(), ref.end(), 0);
		auto begin(ref.begin());
		std::advance(begin, DEFAULT_FONT_OFFSET);

		std::copy(DEFAULT_FONT_DATA.begin(), DEFAULT_FONT_DATA.end(), begin);
	}
	state.reset();

	REQUIRE_THAT(state.memory(), Catch::Matchers::Equals(ref));
	REQUIRE(state.callstack().empty());
}

TEST_CASE("Test state callstack", "[State]") {
	State state;
	REQUIRE(state.callstack().size() == 0);

	state.push_callstack(1);
	REQUIRE(state.callstack().size() == 1);

	state.push_callstack(2);
	REQUIRE(state.callstack().size() == 2);

	state.push_callstack(3);
	REQUIRE(state.callstack().size() == 3);

	REQUIRE(state.pop_callstack() == 3);
	REQUIRE(state.callstack().size() == 2);

	REQUIRE(state.pop_callstack() == 2);
	REQUIRE(state.callstack().size() == 1);

	REQUIRE(state.pop_callstack() == 1);
	REQUIRE(state.callstack().size() == 0);
}

TEST_CASE("Test set and get memory.", "[State]")
{
	Rendering::DummyRenderer renderer;
	VM vm(renderer);
	State& state(vm.manipulate_state());

	for (size_t i = 0; i < state.memory_size(); i++)
	{
		state.memory(i, 0);
		REQUIRE(state.memory()[i] == 0);
	}

	for (size_t i = 0; i < state.memory_size(); i++)
	{
		REQUIRE(vm.state().memory()[i] == 0);
	}

	for (size_t i = 0; i < state.memory_size(); i++)
	{
		state.memory(i, i);
		REQUIRE(state.memory()[i] == (i & 0xFF));
	}

	for (size_t i = 0; i < state.memory_size(); i++)
	{
		REQUIRE(vm.state().memory()[i] == (i & 0xFF));
	}
}

TEST_CASE("Test register set & values", "[State]")
{
	State state;
	for (size_t i = 0; i < state.register_size(); i++)
	{
		REQUIRE(state.reg(i) == 0);
	}

	for (size_t i = 0; i < state.register_size(); i++)
	{
		state.reg(i, i);

		for (size_t j = i + 1; j < state.register_size(); j++)
		{
			REQUIRE(state.reg(j) == 0);
		}
	}

	for (size_t i = 0; i < state.register_size(); i++)
	{
		REQUIRE(state.reg(i) == i);
	}

	for (size_t i = 0; i < state.register_size(); i++)
	{
		state.reg(i, 0xFF);
	}

	for (size_t i = 0; i < state.register_size(); i++)
	{
		REQUIRE(state.reg(i) == 0xFF);
	}

	state.reset();

	for (size_t i = 0; i < state.register_size(); i++)
	{
		REQUIRE(state.reg(i) == 0);
	}
}

TEST_CASE("Test Instruction class", "[Instruction]") {

	Instruction instr(0x1234);

	REQUIRE(instr.upper_byte() == 0x12);
	REQUIRE(instr.lower_byte() == 0x34);

	REQUIRE(instr.upper_triplet() == 0x123);
	REQUIRE(instr.lower_triplet() == 0x234);

	REQUIRE(instr.prefix() == 0x1);
	REQUIRE(instr.suffix() == 0x4);

	REQUIRE(instr.nibble(3) == 0x1);
	REQUIRE(instr.nibble(2) == 0x2);
	REQUIRE(instr.nibble(1) == 0x3);
	REQUIRE(instr.nibble(0) == 0x4);

	REQUIRE_THROWS(instr.nibble(-1));
	REQUIRE_THROWS(instr.nibble(4));
}

//TEST_CASE("Test VM screen clear", "[VM]") {
//	Rendering::DummyRenderer renderer;
//	VM vm(renderer);
//
//	std::vector<int> ref(renderer.framebuffer().size());
//	std::fill(ref.begin(), ref.end(), 0);
//	std::vector< std::vector<int> >& fb(renderer.framebuffer());
//	std::fill(fb.begin(), fb.end(), 1);
//	const std::vector<memval> pgm{ 0x00, 0xE0 };
//	vm.load_program(pgm.begin(), pgm.end());
//	vm.step();
//
//	REQUIRE_THAT(fb, Catch::Matchers::Equals(ref));
//}

TEST_CASE("Test VM return instruction", "[VM]")
{
	SECTION("Check that an illegal return instruction doesn't modify the state.")
	{
		Rendering::DummyRenderer renderer;
		VM vm(renderer);

		State& state(vm.manipulate_state());

		const std::vector<memval> pgm;
		vm.load_program(pgm.begin(), pgm.end());

		REQUIRE(vm.execution_state() == VMExecutionState::RUNNING);
		state.program_counter(0x0);
		state.memory(0x1, 0xEE);
		state.memory(0x0, 0x00);

		REQUIRE_THROWS(vm.step());
		REQUIRE(state.program_counter() == 0x0);
	}

	SECTION("Check that a legal return instruction does modify the state and program counter properly.")
	{
		Rendering::DummyRenderer renderer;
		VM vm(renderer);

		State& state(vm.manipulate_state());

		const std::vector<memval> pgm;
		vm.load_program(pgm.begin(), pgm.end());

		REQUIRE(vm.execution_state() == VMExecutionState::RUNNING);
		state.program_counter(0x0);
		state.memory(0x1, 0xEE);
		state.memory(0x0, 0x00);

		state.push_callstack(0x789);

		REQUIRE(state.program_counter() == 0x0);
		REQUIRE_NOTHROW(vm.step());

		REQUIRE(state.program_counter() == 0x789);
	}

	SECTION("Check that multiple returns in sequence work properly")
	{

		Rendering::DummyRenderer renderer;
		VM vm(renderer);

		State& state(vm.manipulate_state());

		const std::vector<memval> pgm;
		vm.load_program(pgm.begin(), pgm.end());

		REQUIRE(vm.execution_state() == VMExecutionState::RUNNING);

		std::vector<memptr> stack;

		for (memptr ptr = 0; ptr < state.memory_size(); ptr += CHARS_PER_INSTRUCTION)
		{
			state.memory(ptr, 0x0);
			state.memory(ptr + 1, 0xEE);
			state.push_callstack(ptr);
			std::vector<memptr> stack;
		}

		state.program_counter(0x0);

		while (!stack.empty())
		{
			REQUIRE_NOTHROW(vm.step());
			REQUIRE(state.program_counter() == stack.back());
			stack.pop_back();
		}
	}
}

TEST_CASE("Test VM jump instruction.", "[VM]")
{
	SECTION("Test forward jump")
	{
		Rendering::DummyRenderer renderer;
		VM vm(renderer);
		State& state(vm.manipulate_state());

		const std::vector<memval> pgm;
		vm.load_program(pgm.begin(), pgm.end());

		state.memory(0, 0x1F);
		state.memory(1, 0xFF);
		state.program_counter(0x0);

		REQUIRE_NOTHROW(vm.step());
		REQUIRE(state.program_counter() == 0xFFF);
	}

	SECTION("Test forward jump")
	{
		Rendering::DummyRenderer renderer;
		VM vm(renderer);
		State& state(vm.manipulate_state());

		const std::vector<memval> pgm;
		vm.load_program(pgm.begin(), pgm.end());

		state.memory(0xFFA, 0x10);
		state.memory(0xFFB, 0x00);
		state.program_counter(0xFFA);

		REQUIRE_NOTHROW(vm.step());
		REQUIRE(state.program_counter() == 0x0);
	}

	SECTION("Test ping-pong jump")
	{
		Rendering::DummyRenderer renderer;
		VM vm(renderer);
		State& state(vm.manipulate_state());

		const std::vector<memval> pgm;
		vm.load_program(pgm.begin(), pgm.end());
		std::fill(state.memory_begin(), state.memory_end(), 0);

		state.memory(0x100, 0x1F);
		state.memory(0x101, 0xFA);

		state.memory(0xFFA, 0x11);
		state.memory(0xFFB, 0x00);

		state.program_counter(0xFFA);

		for (size_t i = 0; i < 10; i++)
		{
			REQUIRE_NOTHROW(vm.step());
			REQUIRE(((state.program_counter() == 0x100) || (state.program_counter() == 0xFFA)));
		}
	}
}

TEST_CASE("Test VM call", "[VM]")
{
	SECTION("Test simple call")
	{
		Rendering::DummyRenderer renderer;
		VM vm(renderer);
		State& state(vm.manipulate_state());

		const std::vector<memval> pgm;
		vm.load_program(pgm.begin(), pgm.end());

		state.memory(0, 0x21);
		state.memory(1, 0x00);
		state.program_counter(0x0);

		REQUIRE_NOTHROW(vm.step());
		REQUIRE(state.program_counter() == 0x100);
		REQUIRE(state.callstack().back() == 0x2);
	}

	SECTION("Test multi call")
	{
		Rendering::DummyRenderer renderer;
		VM vm(renderer);
		State& state(vm.manipulate_state());

		const std::vector<memval> pgm;
		vm.load_program(pgm.begin(), pgm.end());

		state.memory(0, 0x21);
		state.memory(1, 0x00);

		state.memory(0x100, 0x20);
		state.memory(0x101, 0x00);

		state.program_counter(0x0);

		for (size_t i = 0; i < 10; i++)
		{
			REQUIRE(state.callstack().size() == i);
			REQUIRE_NOTHROW(vm.step());
			REQUIRE(state.callstack().size() == (i + 1));

			if (state.program_counter() == 0x100)
			{
				REQUIRE(state.callstack().back() == 0x2);
			}
			else
			{
				REQUIRE(state.callstack().back() == 0x102);
			}
		}
	}

	SECTION("Test call return")
	{
		Rendering::DummyRenderer renderer;
		VM vm(renderer);
		State& state(vm.manipulate_state());

		const std::vector<memval> pgm;
		vm.load_program(pgm.begin(), pgm.end());

		for (memptr i = 0; i < 0x256; i += CHARS_PER_INSTRUCTION)
		{
			state.memory(i, 0x25);
			state.memory(i + 1, 0x12);
		}

		state.memory(0x512, 0x00);
		state.memory(0x512 + 1, 0xEE);

		while (state.program_counter() < 0x256)
		{
			const memptr pgctr(state.program_counter());
			REQUIRE(state.callstack().size() == 0);
			REQUIRE_NOTHROW(vm.step());
			REQUIRE(state.callstack().size() == 1);
			REQUIRE(state.program_counter() == 0x512);
			REQUIRE_NOTHROW(vm.step());
			REQUIRE(state.program_counter() == (pgctr + CHARS_PER_INSTRUCTION));
			REQUIRE(state.callstack().size() == 0);
		}
	}
}

TEST_CASE("Test if vx != NN then", "[VM]")
{
	SECTION("Test jump regX != 0 => true")
	{
		Rendering::DummyRenderer renderer;
		VM vm(renderer);
		State& state(vm.manipulate_state());

		const std::vector<memval> pgm;
		vm.load_program(pgm.begin(), pgm.end());


		for (size_t regidx = 0; regidx < state.register_size() && regidx < 16; regidx++)
		{
			state.program_counter(0x0);

			REQUIRE(state.reg(regidx) == 0);

			//Reg regidx equals 0
			state.memory(0, 0x30 | regidx);
			state.memory(1, 0x00);

			state.memory(2, 0x1F); // Jump to 0xFFF if untrue
			state.memory(3, 0xFF);

			state.memory(4, 0x1E); // Jump to 0xEEE if true
			state.memory(5, 0xEE);

			REQUIRE_NOTHROW(vm.step());
			REQUIRE(state.program_counter() == 0x4);
			REQUIRE_NOTHROW(vm.step());
			REQUIRE(state.program_counter() == 0xEEE);
		}
	}

	SECTION("Test jump regX != 1 => true")
	{
		Rendering::DummyRenderer renderer;
		VM vm(renderer);
		State& state(vm.manipulate_state());

		const std::vector<memval> pgm;
		vm.load_program(pgm.begin(), pgm.end());


		for (size_t regidx = 0; regidx < state.register_size() && regidx < 16; regidx++)
		{
			state.program_counter(0x0);

			state.reg(regidx, 0xAA);
			REQUIRE(state.reg(regidx) == 0xAA);

			//Reg regidx equals 0
			state.memory(0, 0x30 | regidx);
			state.memory(1, 0xAA);

			state.memory(2, 0x1F); // Jump to 0xFFF if untrue
			state.memory(3, 0xFF);

			state.memory(4, 0x1E); // Jump to 0xEEE if true
			state.memory(5, 0xEE);

			REQUIRE_NOTHROW(vm.step());
			REQUIRE(state.program_counter() == 0x4);
			REQUIRE_NOTHROW(vm.step());
			REQUIRE(state.program_counter() == 0xEEE);
		}
	}

	SECTION("Test jump regX != 0 => false")
	{
		Rendering::DummyRenderer renderer;
		VM vm(renderer);
		State& state(vm.manipulate_state());

		const std::vector<memval> pgm;
		vm.load_program(pgm.begin(), pgm.end());


		for (size_t regidx = 0; regidx < state.register_size() && regidx < 16; regidx++)
		{
			state.program_counter(0x0);
			state.reg(regidx, 1);

			REQUIRE(state.reg(regidx) == 1);

			//Reg regidx equals 0
			state.memory(0, 0x30 | regidx);
			state.memory(1, 0x00);

			state.memory(2, 0x1F); // Jump to 0xFFF if untrue
			state.memory(3, 0xFF);

			state.memory(4, 0x1E); // Jump to 0xEEE if true
			state.memory(5, 0xEE);

			REQUIRE_NOTHROW(vm.step());
			REQUIRE(state.program_counter() == 0x2);
			REQUIRE_NOTHROW(vm.step());
			REQUIRE(state.program_counter() == 0xFFF);
		}
	}

	SECTION("Test jump regX != 0xAA => false")
	{
		Rendering::DummyRenderer renderer;
		VM vm(renderer);
		State& state(vm.manipulate_state());

		const std::vector<memval> pgm;
		vm.load_program(pgm.begin(), pgm.end());


		for (size_t regidx = 0; regidx < state.register_size() && regidx < 16; regidx++)
		{
			state.program_counter(0x0);
			state.reg(regidx, 0xAA);

			REQUIRE(state.reg(regidx) == 0xAA);

			//Reg regidx equals 0
			state.memory(0, 0x30 | regidx);
			state.memory(1, 0xAA + 1);

			state.memory(2, 0x1F); // Jump to 0xFFF if untrue
			state.memory(3, 0xFF);

			state.memory(4, 0x1E); // Jump to 0xEEE if true
			state.memory(5, 0xEE);

			REQUIRE_NOTHROW(vm.step());
			REQUIRE(state.program_counter() == 0x2);
			REQUIRE_NOTHROW(vm.step());
			REQUIRE(state.program_counter() == 0xFFF);
		}
	}
}


TEST_CASE("Test if vx == NN then", "[VM]")
{
	SECTION("Test jump regX != 0 => true")
	{
		Rendering::DummyRenderer renderer;
		VM vm(renderer);
		State& state(vm.manipulate_state());

		const std::vector<memval> pgm;
		vm.load_program(pgm.begin(), pgm.end());


		for (size_t regidx = 0; regidx < state.register_size() && regidx < 16; regidx++)
		{
			state.program_counter(0x0);

			REQUIRE(state.reg(regidx) == 0);

			//Reg regidx equals 0
			state.memory(0, 0x40 | regidx);
			state.memory(1, 0x00);

			state.memory(2, 0x1F); // Jump to 0xFFF if untrue
			state.memory(3, 0xFF);

			state.memory(4, 0x1E); // Jump to 0xEEE if true
			state.memory(5, 0xEE);


			REQUIRE_NOTHROW(vm.step());
			REQUIRE(state.program_counter() == 0x2);
			REQUIRE_NOTHROW(vm.step());
			REQUIRE(state.program_counter() == 0xFFF);
		}
	}

	SECTION("Test jump regX != 1 => true")
	{
		Rendering::DummyRenderer renderer;
		VM vm(renderer);
		State& state(vm.manipulate_state());

		const std::vector<memval> pgm;
		vm.load_program(pgm.begin(), pgm.end());


		for (size_t regidx = 0; regidx < state.register_size() && regidx < 16; regidx++)
		{
			state.program_counter(0x0);

			state.reg(regidx, 0xAA);
			REQUIRE(state.reg(regidx) == 0xAA);

			//Reg regidx equals 0
			state.memory(0, 0x40 | regidx);
			state.memory(1, 0xAA);

			state.memory(2, 0x1F); // Jump to 0xFFF if untrue
			state.memory(3, 0xFF);

			state.memory(4, 0x1E); // Jump to 0xEEE if true
			state.memory(5, 0xEE);

			REQUIRE_NOTHROW(vm.step());
			REQUIRE(state.program_counter() == 0x2);
			REQUIRE_NOTHROW(vm.step());
			REQUIRE(state.program_counter() == 0xFFF);
		}
	}

	SECTION("Test jump regX != 0 => false")
	{
		Rendering::DummyRenderer renderer;
		VM vm(renderer);
		State& state(vm.manipulate_state());

		const std::vector<memval> pgm;
		vm.load_program(pgm.begin(), pgm.end());


		for (size_t regidx = 0; regidx < state.register_size() && regidx < 16; regidx++)
		{
			state.program_counter(0x0);
			state.reg(regidx, 1);

			REQUIRE(state.reg(regidx) == 1);

			//Reg regidx equals 0
			state.memory(0, 0x40 | regidx);
			state.memory(1, 0x00);

			state.memory(2, 0x1F); // Jump to 0xFFF if untrue
			state.memory(3, 0xFF);

			state.memory(4, 0x1E); // Jump to 0xEEE if true
			state.memory(5, 0xEE);

			REQUIRE_NOTHROW(vm.step());
			REQUIRE(state.program_counter() == 0x4);
			REQUIRE_NOTHROW(vm.step());
			REQUIRE(state.program_counter() == 0xEEE);
		}
	}

	SECTION("Test jump regX != 0xAA => false")
	{
		Rendering::DummyRenderer renderer;
		VM vm(renderer);
		State& state(vm.manipulate_state());

		const std::vector<memval> pgm;
		vm.load_program(pgm.begin(), pgm.end());


		for (size_t regidx = 0; regidx < state.register_size() && regidx < 16; regidx++)
		{
			state.program_counter(0x0);
			state.reg(regidx, 0xAA);

			REQUIRE(state.reg(regidx) == 0xAA);

			//Reg regidx equals 0
			state.memory(0, 0x40 | regidx);
			state.memory(1, 0xAA + 1);

			state.memory(2, 0x1F); // Jump to 0xFFF if untrue
			state.memory(3, 0xFF);

			state.memory(4, 0x1E); // Jump to 0xEEE if true
			state.memory(5, 0xEE);

			REQUIRE_NOTHROW(vm.step());
			REQUIRE(state.program_counter() == 0x4);
			REQUIRE_NOTHROW(vm.step());
			REQUIRE(state.program_counter() == 0xEEE);
		}
	}
}

TEST_CASE("Test if vx != vy then", "[VM]")
{
	SECTION("Test jump regX != regY => true")
	{
		Rendering::DummyRenderer renderer;
		VM vm(renderer);
		State& state(vm.manipulate_state());

		const std::vector<memval> pgm;
		vm.load_program(pgm.begin(), pgm.end());


		for (size_t regidx = 0; regidx < state.register_size() && regidx < 16; regidx++)
		{
			for (size_t regidy = 0; regidy < state.register_size() && regidy < 16; regidy++)
			{
				if (regidx == regidy)
				{
					continue;
				}

				REQUIRE(regidx != regidy);

				state.program_counter(0x0);

				state.reg(regidx, 0);
				state.reg(regidy, 1);

				REQUIRE(state.reg(regidx) == 0);
				REQUIRE(state.reg(regidy) == 1);

				//Reg regidx equals 0
				state.memory(0, (0x50 | regidx));
				state.memory(1, (0x00 | (regidy << 4)));

				state.memory(2, 0x1F); // Jump to 0xFFF if untrue
				state.memory(3, 0xFF);

				state.memory(4, 0x1E); // Jump to 0xEEE if true
				state.memory(5, 0xEE);


				REQUIRE(state.program_counter() == 0x0);
				REQUIRE_NOTHROW(vm.step());
				REQUIRE(state.program_counter() == 0x2);
				REQUIRE_NOTHROW(vm.step());
				REQUIRE(state.program_counter() == 0xFFF);
			}
		}
	}

	SECTION("Test jump regX != regY => false")
	{
		Rendering::DummyRenderer renderer;
		VM vm(renderer);
		State& state(vm.manipulate_state());

		const std::vector<memval> pgm;
		vm.load_program(pgm.begin(), pgm.end());


		for (size_t regidx = 0; regidx < state.register_size() && regidx < 16; regidx++)
		{
			for (size_t regidy = 0; regidy < state.register_size() && regidy < 16; regidy++)
			{
				if (regidx == regidy)
				{
					continue;
				}

				REQUIRE(regidx != regidy);

				state.program_counter(0x0);

				state.reg(regidx, 1);
				state.reg(regidy, 1);

				REQUIRE(state.reg(regidx) == 1);
				REQUIRE(state.reg(regidy) == 1);

				//Reg regidx equals 0
				state.memory(0, (0x50 | regidx));
				state.memory(1, (0x00 | (regidy << 4)));

				state.memory(2, 0x1F); // Jump to 0xFFF if untrue
				state.memory(3, 0xFF);

				state.memory(4, 0x1E); // Jump to 0xEEE if true
				state.memory(5, 0xEE);


				REQUIRE(state.program_counter() == 0x0);
				REQUIRE_NOTHROW(vm.step());
				REQUIRE(state.program_counter() == 0x4);
				REQUIRE_NOTHROW(vm.step());
				REQUIRE(state.program_counter() == 0xEEE);
			}
		}
	}
}

TEST_CASE("Test VM set register.", "[VM]")
{
	SECTION("Test set register instruction.")
	{
		Rendering::DummyRenderer renderer;
		VM vm(renderer);
		State& state(vm.manipulate_state());

		const std::vector<memval> pgm;
		vm.load_program(pgm.begin(), pgm.end());


		const std::vector<regval> values{ 0x0, 0x80, 0x1, 0x10, 0xFF };

		for (size_t i = 0; i < state.register_size(); i++)
		{
			state.reset();

			for (std::vector<regval>::const_iterator it = values.begin(); it != values.end(); ++it)
			{
				state.memory(0, 0x60 | i);
				state.memory(1, *it & 0xFF);
				state.program_counter(0);

				REQUIRE_NOTHROW(vm.step());
				REQUIRE((state.reg(i) == (*it & 0xFF)));

				for (size_t j = 0; j < state.register_size(); j++)
				{
					if (j == i) continue;
					REQUIRE(state.reg(j) == 0);
				}
			}
		}
	}
}

TEST_CASE("Test VM add register.", "[VM]")
{
	SECTION("Test add register instruction.")
	{
		Rendering::DummyRenderer renderer;
		VM vm(renderer);
		State& state(vm.manipulate_state());

		const std::vector<memval> pgm;
		vm.load_program(pgm.begin(), pgm.end());


		const std::vector<regval> values{ 0x0, 0x80, 0x1, 0x10, 0xFF };

		for (size_t i = 0; i < state.register_size(); i++)
		{
			state.reset();

			int rolling_sum = 0;
			for (std::vector<regval>::const_iterator it = values.begin(); it != values.end(); ++it)
			{
				const int value(*it);
				REQUIRE(state.reg(i) == rolling_sum);

				state.memory(0, 0x70 | i);
				state.memory(1, value & 0xFF);
				state.program_counter(0);

				rolling_sum += value;

				REQUIRE_NOTHROW(vm.step());
				REQUIRE((state.reg(i) == (rolling_sum & 0xFF)));

				for (size_t j = 0; j < state.register_size(); j++)
				{
					if (j == i) continue;
					REQUIRE(state.reg(j) == 0);
				}
			}
		}
	}
}

TEST_CASE("Test VM bitwise operations on register.", "[VM]")
{
	SECTION("Test set register operation")
	{
		Rendering::DummyRenderer renderer;
		VM vm(renderer);
		State& state(vm.manipulate_state());

		const std::vector<memval> pgm;
		vm.load_program(pgm.begin(), pgm.end());

		for (size_t i = 0; i < state.register_size(); i++)
		{
			state.reset();

			for (size_t j = 0; j < state.register_size(); j++)
			{
				if (i == j) continue;
				state.reg(i, 0xF);
				state.reg(j, 0xF0);

				state.memory(0, 0x80 | i);
				state.memory(1, j << 4);

				state.program_counter(0);
				REQUIRE_NOTHROW(vm.step());
				REQUIRE(state.reg(i) == 0xF0);
				REQUIRE(state.reg(j) == 0xF0);

				for (size_t k = j + 1; k < state.register_size(); k++)
				{
					if (k == i || k == j) continue;
					REQUIRE(state.reg(k) == 0);
				}
			}
		}
	}

	SECTION("Test or register operation")
	{
		Rendering::DummyRenderer renderer;
		VM vm(renderer);
		State& state(vm.manipulate_state());

		const std::vector<memval> pgm;
		vm.load_program(pgm.begin(), pgm.end());

		for (size_t i = 0; i < state.register_size(); i++)
		{
			state.reset();

			for (size_t j = 0; j < state.register_size(); j++)
			{
				if (i == j) continue;
				state.reg(i, 0xF);
				state.reg(j, 0xF0);

				state.memory(0, 0x80 | i);
				state.memory(1, j << 4 | 0x1);

				state.program_counter(0);
				REQUIRE_NOTHROW(vm.step());
				REQUIRE(state.reg(i) == 0xFF);
				REQUIRE(state.reg(j) == 0xF0);

				for (size_t k = j + 1; k < state.register_size(); k++)
				{
					if (k == i || k == j) continue;
					REQUIRE(state.reg(k) == 0);
				}
			}
		}
	}

	SECTION("Test and register operation")
	{
		Rendering::DummyRenderer renderer;
		VM vm(renderer);
		State& state(vm.manipulate_state());

		const std::vector<memval> pgm;
		vm.load_program(pgm.begin(), pgm.end());

		for (size_t i = 0; i < state.register_size(); i++)
		{
			state.reset();

			for (size_t j = 0; j < state.register_size(); j++)
			{
				if (i == j) continue;
				state.reg(i, 0xF);
				state.reg(j, 0xF0);

				state.memory(0, 0x80 | i);
				state.memory(1, j << 4 | 0x2);

				state.program_counter(0);
				REQUIRE_NOTHROW(vm.step());
				REQUIRE(state.reg(i) == 0x00);
				REQUIRE(state.reg(j) == 0xF0);

				for (size_t k = j + 1; k < state.register_size(); k++)
				{
					if (k == i || k == j) continue;
					REQUIRE(state.reg(k) == 0);
				}
			}
		}
	}

	SECTION("Test xor register operation")
	{
		Rendering::DummyRenderer renderer;
		VM vm(renderer);
		State& state(vm.manipulate_state());

		const std::vector<memval> pgm;
		vm.load_program(pgm.begin(), pgm.end());

		for (size_t i = 0; i < state.register_size(); i++)
		{
			state.reset();

			for (size_t j = 0; j < state.register_size(); j++)
			{
				if (i == j) continue;
				state.reg(i, 0x0F);
				state.reg(j, 0xFF);

				state.memory(0, 0x80 | i);
				state.memory(1, j << 4 | 0x3);

				state.program_counter(0);
				REQUIRE_NOTHROW(vm.step());
				REQUIRE(state.reg(i) == 0xF0);
				REQUIRE(state.reg(j) == 0xFF);

				for (size_t k = j + 1; k < state.register_size(); k++)
				{
					if (k == i || k == j) continue;
					REQUIRE(state.reg(k) == 0);
				}
			}
		}
	}

	SECTION("Test add register operation with carry")
	{
		Rendering::DummyRenderer renderer;
		VM vm(renderer);
		State& state(vm.manipulate_state());

		const std::vector<memval> pgm;
		vm.load_program(pgm.begin(), pgm.end());

		for (size_t i = 0; i < state.register_size(); i++)
		{
			state.reset();

			for (size_t j = 0; j < state.register_size(); j++)
			{
				if (i == j || i == state.flag_register_index() || j == state.flag_register_index()) continue;
				state.reg(i, 0x04);
				state.reg(j, 0xF0);

				state.memory(0, 0x80 | i);
				state.memory(1, j << 4 | 0x4);

				state.program_counter(0);
				REQUIRE_NOTHROW(vm.step());
				REQUIRE(state.reg(i) == 0xF4);
				REQUIRE(state.reg(j) == 0xF0);
				REQUIRE(state.flag_register() == 0);

				for (size_t k = j + 1; k < state.register_size(); k++)
				{
					if (k == i || k == j || k == state.flag_register_index()) continue;
					REQUIRE(state.reg(k) == 0);
				}
			}
		}

		for (size_t i = 0; i < state.register_size(); i++)
		{
			state.reset();

			for (size_t j = 0; j < state.register_size(); j++)
			{
				if (i == j || i == state.flag_register_index() || j == state.flag_register_index()) continue;
				state.reg(i, 0x80);
				state.reg(j, 0x80);

				state.memory(0, 0x80 | i);
				state.memory(1, j << 4 | 0x4);

				state.program_counter(0);
				REQUIRE_NOTHROW(vm.step());
				REQUIRE(state.reg(i) == ((0x80 + 0x80) & 0xFF));
				REQUIRE(state.reg(j) == 0x80);
				REQUIRE(state.flag_register() == 1);

				for (size_t k = j + 1; k < state.register_size(); k++)
				{
					if (k == i || k == j || k == state.flag_register_index()) continue;
					REQUIRE(state.reg(k) == 0);
				}
			}
		}
	}

	SECTION("Test sub register operation")
	{
		Rendering::DummyRenderer renderer;
		VM vm(renderer);
		State& state(vm.manipulate_state());

		const std::vector<memval> pgm;
		vm.load_program(pgm.begin(), pgm.end());

		for (size_t i = 0; i < state.register_size(); i++)
		{
			state.reset();

			for (size_t j = 0; j < state.register_size(); j++)
			{
				if (i == j || i == state.flag_register_index() || j == state.flag_register_index()) continue;
				state.reg(i, 0x80);
				state.reg(j, 0x20);

				state.memory(0, 0x80 | i);
				state.memory(1, j << 4 | 0x5);

				state.program_counter(0);
				REQUIRE_NOTHROW(vm.step());
				REQUIRE(state.reg(i) == 0x60);
				REQUIRE(state.reg(j) == 0x20);
				REQUIRE(state.flag_register() == 1);

				for (size_t k = j + 1; k < state.register_size(); k++)
				{
					if (k == i || k == j || k == state.flag_register_index()) continue;
					REQUIRE(state.reg(k) == 0);
				}
			}
		}

		for (size_t i = 0; i < state.register_size(); i++)
		{
			state.reset();

			for (size_t j = 0; j < state.register_size(); j++)
			{
				if (i == j || i == state.flag_register_index() || j == state.flag_register_index()) continue;
				state.reg(i, 0x20);
				state.reg(j, 0x80);

				state.memory(0, 0x80 | i);
				state.memory(1, j << 4 | 0x5);

				state.program_counter(0);
				REQUIRE_NOTHROW(vm.step());
				REQUIRE(state.reg(i) == ((0x20 - 0x80) & 0xFF));
				REQUIRE(state.reg(j) == 0x80);
				REQUIRE(state.flag_register() == 0);

				for (size_t k = j + 1; k < state.register_size(); k++)
				{
					if (k == i || k == j || k == state.flag_register_index()) continue;
					REQUIRE(state.reg(k) == 0);
				}
			}
		}
	}

	SECTION("Test right shift register operation")
	{
		Rendering::DummyRenderer renderer;
		VM vm(renderer);
		State& state(vm.manipulate_state());

		const std::vector<memval> pgm;
		vm.load_program(pgm.begin(), pgm.end());

		const std::vector<regval> shift_values{ 0x0, 0x1 , 0x4, 0x8 };
		for (size_t i = 0; i < state.register_size(); i++)
		{
			state.reset();

			for (size_t j = 0; j < state.register_size(); j++)
			{
				if (i == j || i == state.flag_register_index() || j == state.flag_register_index()) continue;

				state.reg(i, 0xAA);
				state.reg(j, 0);

				state.memory(0, 0x80 | i);
				state.memory(1, j << 4 | 0x6);
				state.program_counter(0);

				REQUIRE_NOTHROW(vm.step());

				REQUIRE(state.reg(i) == 0xAA);
				REQUIRE(state.reg(j) == 0);
				REQUIRE(state.flag_register() == 0);

				////////////////////////////////////////////////
				state.reg(i, 0xAA);
				state.reg(j, 1);

				state.memory(0, 0x80 | i);
				state.memory(1, j << 4 | 0x6);
				state.program_counter(0);

				REQUIRE_NOTHROW(vm.step());

				REQUIRE((state.reg(i) == (0xAA >> 1)));
				REQUIRE(state.reg(j) == 1);
				REQUIRE(state.flag_register() == 0);

				////////////////////////////////////////////////
				state.reg(i, 0xAA);
				state.reg(j, 4);

				state.memory(0, 0x80 | i);
				state.memory(1, j << 4 | 0x6);
				state.program_counter(0);

				REQUIRE_NOTHROW(vm.step());

				REQUIRE((state.reg(i) == (0xAA >> 4)));
				REQUIRE(state.reg(j) == 4);
				REQUIRE(state.flag_register() == 0);

				for (size_t k = j + 1; k < state.register_size(); k++)
				{
					if (k == i || k == j || k == state.flag_register_index()) continue;
					REQUIRE(state.reg(k) == 0);
				}

				////////////////////////////////////////////////
				state.reg(i, 0x55);
				state.reg(j, 4);

				state.memory(0, 0x80 | i);
				state.memory(1, j << 4 | 0x6);
				state.program_counter(0);

				REQUIRE_NOTHROW(vm.step());

				REQUIRE((state.reg(i) == (0x55 >> 4)));
				REQUIRE(state.reg(j) == 4);
				REQUIRE(state.flag_register() == 1);

				for (size_t k = j + 1; k < state.register_size(); k++)
				{
					if (k == i || k == j || k == state.flag_register_index()) continue;
					REQUIRE(state.reg(k) == 0);
				}

				////////////////////////////////////////////////
				state.reg(i, 0x55);
				state.reg(j, 1);

				state.memory(0, 0x80 | i);
				state.memory(1, j << 4 | 0x6);
				state.program_counter(0);

				REQUIRE_NOTHROW(vm.step());

				REQUIRE((state.reg(i) == (0x55 >> 1)));
				REQUIRE(state.reg(j) == 1);
				REQUIRE(state.flag_register() == 1);

				for (size_t k = j + 1; k < state.register_size(); k++)
				{
					if (k == i || k == j || k == state.flag_register_index()) continue;
					REQUIRE(state.reg(k) == 0);
				}
			}
		}
	}

	SECTION("Test rsub register operation")
	{
		Rendering::DummyRenderer renderer;
		VM vm(renderer);
		State& state(vm.manipulate_state());

		const std::vector<memval> pgm;
		vm.load_program(pgm.begin(), pgm.end());

		for (size_t i = 0; i < state.register_size(); i++)
		{
			state.reset();

			for (size_t j = 0; j < state.register_size(); j++)
			{
				if (i == j || i == state.flag_register_index() || j == state.flag_register_index()) continue;
				state.reg(i, 0x20);
				state.reg(j, 0x80);

				state.memory(0, 0x80 | i);
				state.memory(1, j << 4 | 0x7);

				state.program_counter(0);
				REQUIRE_NOTHROW(vm.step());
				REQUIRE(state.reg(i) == 0x60);
				REQUIRE(state.reg(j) == 0x80);
				REQUIRE(state.flag_register() == 0);

				for (size_t k = j + 1; k < state.register_size(); k++)
				{
					if (k == i || k == j || k == state.flag_register_index()) continue;
					REQUIRE(state.reg(k) == 0);
				}
			}
		}

		for (size_t i = 0; i < state.register_size(); i++)
		{
			state.reset();

			for (size_t j = 0; j < state.register_size(); j++)
			{
				if (i == j || i == state.flag_register_index() || j == state.flag_register_index()) continue;
				state.reg(i, 0x80);
				state.reg(j, 0x20);

				state.memory(0, 0x80 | i);
				state.memory(1, j << 4 | 0x7);

				state.program_counter(0);
				REQUIRE_NOTHROW(vm.step());
				REQUIRE(state.reg(i) == ((0x20 - 0x80) & 0xFF));
				REQUIRE(state.reg(j) == 0x20);
				REQUIRE(state.flag_register() == 1);

				for (size_t k = j + 1; k < state.register_size(); k++)
				{
					if (k == i || k == j || k == state.flag_register_index()) continue;
					REQUIRE(state.reg(k) == 0);
				}
			}
		}
	}

	SECTION("Test left shift register operation")
	{
		Rendering::DummyRenderer renderer;
		VM vm(renderer);
		State& state(vm.manipulate_state());

		const std::vector<memval> pgm;
		vm.load_program(pgm.begin(), pgm.end());

		const std::vector<regval> shift_values{ 0x0, 0x1 , 0x4, 0x8 };
		for (size_t i = 0; i < state.register_size(); i++)
		{
			state.reset();

			for (size_t j = 0; j < state.register_size(); j++)
			{
				if (i == j || i == state.flag_register_index() || j == state.flag_register_index()) continue;

				state.reg(i, 0xAA);
				state.reg(j, 0);

				state.memory(0, 0x80 | i);
				state.memory(1, j << 4 | 0xE);
				state.program_counter(0);

				REQUIRE_NOTHROW(vm.step());

				REQUIRE(state.reg(i) == 0xAA);
				REQUIRE(state.reg(j) == 0);
				REQUIRE(state.flag_register() == 1);

				////////////////////////////////////////////////
				state.reg(i, 0xAA);
				state.reg(j, 1);

				state.memory(0, 0x80 | i);
				state.memory(1, j << 4 | 0xE);
				state.program_counter(0);

				REQUIRE_NOTHROW(vm.step());

				REQUIRE(state.reg(i) == ((0xAA << 1) & 0xFF));
				REQUIRE(state.reg(j) == 1);
				REQUIRE(state.flag_register() == 1);

				////////////////////////////////////////////////
				state.reg(i, 0xAA);
				state.reg(j, 4);

				state.memory(0, 0x80 | i);
				state.memory(1, j << 4 | 0xE);
				state.program_counter(0);

				REQUIRE_NOTHROW(vm.step());

				REQUIRE((state.reg(i) == ((0xAA << 4) & 0xFF)));
				REQUIRE(state.reg(j) == 4);
				REQUIRE(state.flag_register() == 1);

				for (size_t k = j + 1; k < state.register_size(); k++)
				{
					if (k == i || k == j || k == state.flag_register_index()) continue;
					REQUIRE(state.reg(k) == 0);
				}

				////////////////////////////////////////////////
				state.reg(i, 0x55);
				state.reg(j, 4);

				state.memory(0, 0x80 | i);
				state.memory(1, j << 4 | 0xE);
				state.program_counter(0);

				REQUIRE_NOTHROW(vm.step());

				REQUIRE(state.reg(i) == ((0x55 << 4) & 0xFF));
				REQUIRE(state.reg(j) == 4);
				REQUIRE(state.flag_register() == 0);

				for (size_t k = j + 1; k < state.register_size(); k++)
				{
					if (k == i || k == j || k == state.flag_register_index()) continue;
					REQUIRE(state.reg(k) == 0);
				}

				////////////////////////////////////////////////
				state.reg(i, 0x55);
				state.reg(j, 1);

				state.memory(0, 0x80 | i);
				state.memory(1, j << 4 | 0xE);
				state.program_counter(0);

				REQUIRE_NOTHROW(vm.step());

				REQUIRE((state.reg(i) == ((0x55 << 1) & 0xFF)));
				REQUIRE(state.reg(j) == 1);
				REQUIRE(state.flag_register() == 0);

				for (size_t k = j + 1; k < state.register_size(); k++)
				{
					if (k == i || k == j || k == state.flag_register_index()) continue;
					REQUIRE(state.reg(k) == 0);
				}
			}
		}
	}
}

TEST_CASE("Test if vx == vy then", "[VM]")
{
	SECTION("Test jump regX == regY => false")
	{
		Rendering::DummyRenderer renderer;
		VM vm(renderer);
		State& state(vm.manipulate_state());

		const std::vector<memval> pgm;
		vm.load_program(pgm.begin(), pgm.end());


		for (size_t regidx = 0; regidx < state.register_size() && regidx < 16; regidx++)
		{
			for (size_t regidy = 0; regidy < state.register_size() && regidy < 16; regidy++)
			{
				if (regidx == regidy)
				{
					continue;
				}

				REQUIRE(regidx != regidy);

				state.program_counter(0x0);

				state.reg(regidx, 0);
				state.reg(regidy, 1);

				REQUIRE(state.reg(regidx) == 0);
				REQUIRE(state.reg(regidy) == 1);

				//Reg regidx equals 0
				state.memory(0, (0x90 | regidx));
				state.memory(1, (0x00 | (regidy << 4)));

				state.memory(2, 0x1F); // Jump to 0xFFF if untrue
				state.memory(3, 0xFF);

				state.memory(4, 0x1E); // Jump to 0xEEE if true
				state.memory(5, 0xEE);


				REQUIRE(state.program_counter() == 0x0);
				REQUIRE_NOTHROW(vm.step());
				REQUIRE(state.program_counter() == 0x4);
				REQUIRE_NOTHROW(vm.step());
				REQUIRE(state.program_counter() == 0xEEE);
			}
		}
	}

	SECTION("Test jump regX == regY => true")
	{
		Rendering::DummyRenderer renderer;
		VM vm(renderer);
		State& state(vm.manipulate_state());

		const std::vector<memval> pgm;
		vm.load_program(pgm.begin(), pgm.end());

		for (size_t regidx = 0; regidx < state.register_size() && regidx < 16; regidx++)
		{
			for (size_t regidy = 0; regidy < state.register_size() && regidy < 16; regidy++)
			{
				if (regidx == regidy)
				{
					continue;
				}

				REQUIRE(regidx != regidy);

				state.program_counter(0x0);

				state.reg(regidx, 1);
				state.reg(regidy, 1);

				REQUIRE(state.reg(regidx) == 1);
				REQUIRE(state.reg(regidy) == 1);

				//Reg regidx equals 0
				state.memory(0, (0x90 | regidx));
				state.memory(1, (0x00 | (regidy << 4)));

				state.memory(2, 0x1F); // Jump to 0xFFF if untrue
				state.memory(3, 0xFF);

				state.memory(4, 0x1E); // Jump to 0xEEE if true
				state.memory(5, 0xEE);


				REQUIRE(state.program_counter() == 0x0);
				REQUIRE_NOTHROW(vm.step());
				REQUIRE(state.program_counter() == 0x2);
				REQUIRE_NOTHROW(vm.step());
				REQUIRE(state.program_counter() == 0xFFF);
			}
		}
	}
}

TEST_CASE("Test set index register.", "[State]")
{
	Rendering::DummyRenderer renderer;
	VM vm(renderer);
	State& state(vm.manipulate_state());

	const std::vector<memval> pgm;
	vm.load_program(pgm.begin(), pgm.end());

	for (size_t i = 0; i < 0xFFF; i += 0x20)
	{
		state.reset();

		state.memory(0, 0xA0 | ((i >> 8) & 0xF));
		state.memory(1, (i & 0xFF));

		state.program_counter(0);
		REQUIRE(state.index_register() == 0);
		REQUIRE_NOTHROW(vm.step());
		REQUIRE(state.index_register() == i);
	}
}

TEST_CASE("Test jump with V0 register.", "[State]")
{
	Rendering::DummyRenderer renderer;
	VM vm(renderer);
	State& state(vm.manipulate_state());

	const std::vector<memval> pgm;
	vm.load_program(pgm.begin(), pgm.end());

	for (size_t i = 0; i < 0xFFF; i += 0x20)
	{
		state.reset();

		state.memory(0, 0xB0 | ((i >> 8) & 0xF));
		state.memory(1, (i & 0xFF));

		state.program_counter(0);
		REQUIRE_NOTHROW(vm.step());
		REQUIRE(state.program_counter() == i);
	}

	for (size_t i = 0; i < 0x256; i += (0x256 / 4))
	{
		for (size_t v0val = 0; v0val < 0xFF; v0val += (0xFF / 5))
		{
			state.reset();

			state.reg(0, v0val);

			state.memory(0, 0xB0 | ((i >> 8) & 0xF));
			state.memory(1, (i & 0xFF));

			state.program_counter(0);
			REQUIRE_NOTHROW(vm.step());
			REQUIRE(state.reg(0) == v0val);
			REQUIRE(state.program_counter() == (i + v0val));
		}
	}
}

TEST_CASE("Is key in register not pressed?.", "[VM]")
{
	Rendering::DummyRenderer renderer;
	VM vm(renderer);
	State& state(vm.manipulate_state());

	const std::vector<memval> pgm;
	vm.load_program(pgm.begin(), pgm.end());

	for (size_t regidx = 0; regidx < state.register_size(); regidx++)
	{
		for (size_t keyid = 0; keyid < 16; keyid++)
		{
			state.program_counter(0x0);

			//Reg regidx equals 0
			state.memory(0, 0xE0 | regidx);
			state.memory(1, 0x9E);

			state.memory(2, 0x1F); // Jump to 0xFFF if untrue
			state.memory(3, 0xFF);

			state.memory(4, 0x1E); // Jump to 0xEEE if true
			state.memory(5, 0xEE);

			state.reg(regidx, keyid);
			vm.keydown(keyid);

			REQUIRE_NOTHROW(vm.step());
			REQUIRE(state.program_counter() == 0x4);
			REQUIRE_NOTHROW(vm.step());
			REQUIRE(state.program_counter() == 0xEEE);
		}
	}

	for (size_t regidx = 0; regidx < state.register_size(); regidx++)
	{
		for (size_t keyid = 0; keyid < 16; keyid++)
		{
			state.program_counter(0x0);

			//Reg regidx equals 0
			state.memory(0, 0xE0 | regidx);
			state.memory(1, 0x9E);

			state.memory(2, 0x1F); // Jump to 0xFFF if untrue
			state.memory(3, 0xFF);

			state.memory(4, 0x1E); // Jump to 0xEEE if true
			state.memory(5, 0xEE);

			state.reg(regidx, ~keyid);
			vm.keydown(keyid);

			REQUIRE_NOTHROW(vm.step());
			REQUIRE(state.program_counter() == 0x2);
			REQUIRE_NOTHROW(vm.step());
			REQUIRE(state.program_counter() == 0xFFF);
		}
	}
}

TEST_CASE("Is key in register pressed?.", "[VM]")
{
	Rendering::DummyRenderer renderer;
	VM vm(renderer);
	State& state(vm.manipulate_state());

	const std::vector<memval> pgm;
	vm.load_program(pgm.begin(), pgm.end());

	for (size_t regidx = 0; regidx < state.register_size(); regidx++)
	{
		for (size_t keyid = 0; keyid < 16; keyid++)
		{
			state.program_counter(0x0);

			//Reg regidx equals 0
			state.memory(0, 0xE0 | regidx);
			state.memory(1, 0xA1);

			state.memory(2, 0x1F); // Jump to 0xFFF if untrue
			state.memory(3, 0xFF);

			state.memory(4, 0x1E); // Jump to 0xEEE if true
			state.memory(5, 0xEE);

			state.reg(regidx, keyid);
			vm.keydown(keyid);

			REQUIRE_NOTHROW(vm.step());
			REQUIRE(state.program_counter() == 0x2);
			REQUIRE_NOTHROW(vm.step());
			REQUIRE(state.program_counter() == 0xFFF);
		}
	}

	for (size_t regidx = 0; regidx < state.register_size(); regidx++)
	{
		for (size_t keyid = 0; keyid < 16; keyid++)
		{
			state.program_counter(0x0);

			//Reg regidx equals 0
			state.memory(0, 0xE0 | regidx);
			state.memory(1, 0xA1);

			state.memory(2, 0x1F); // Jump to 0xFFF if untrue
			state.memory(3, 0xFF);

			state.memory(4, 0x1E); // Jump to 0xEEE if true
			state.memory(5, 0xEE);

			state.reg(regidx, ~keyid);
			vm.keydown(keyid);

			REQUIRE_NOTHROW(vm.step());
			REQUIRE(state.program_counter() == 0x4);
			REQUIRE_NOTHROW(vm.step());
			REQUIRE(state.program_counter() == 0xEEE);
		}
	}
}


TEST_CASE("Test F Prefix instructions.", "[VM]")
{
	Rendering::DummyRenderer renderer;
	VM vm(renderer);
	State& state(vm.manipulate_state());

	const std::vector<memval> pgm;
	vm.load_program(pgm.begin(), pgm.end());

	SECTION("Test set register to delay register value.")
	{
		for (size_t i = 0; i < 0xF; i++)
		{
			state.reset();

			state.program_counter(0x0);
			state.timer(0xAB);

			state.memory(0, 0xF0 | i);
			state.memory(1, 0x07);

			REQUIRE_NOTHROW(vm.step());
			REQUIRE(state.reg(i) == 0xAB);

			for (size_t j = 0; j < 0xF; j++)
			{
				if (i == j) continue;
				REQUIRE(state.reg(j) == 0);
			}
		}
	}

	SECTION("Wait for key and store in register.")
	{
		for (size_t i = 0; i < 0xF; i++)
		{
			for (size_t key = 0; key < 0xF; key++)
			{
				vm.reset();
				state.reset();

				state.program_counter(0x0);
				state.memory(0, 0xF0 | i);
				state.memory(1, 0x0A);

				REQUIRE_NOTHROW(vm.step());
				REQUIRE(vm.execution_state() == VMExecutionState::WAIT_FOR_KEY);

				for (size_t cycle = 0; cycle < 256; cycle++)
				{
					REQUIRE(vm.execution_state() == VMExecutionState::WAIT_FOR_KEY);
					REQUIRE_NOTHROW(vm.step());
				}

				vm.keydown(key);
				REQUIRE_NOTHROW(vm.step());

				REQUIRE(state.reg(i) == key);

				for (size_t j = 0; j < 0xF; j++)
				{
					if (i == j) continue;
					REQUIRE(state.reg(j) == 0);
				}
			}
		}
	}

	SECTION("Set delay register.")
	{
		for (size_t i = 0; i < 0xFF; i += 0x40)
		{
			for (size_t regidx = 0; regidx < state.register_size(); regidx++)
			{
				vm.reset();
				state.reset();

				state.program_counter(0x0);
				state.memory(0, 0xF0 | regidx);
				state.memory(1, 0x15);

				state.reg(regidx, i);
				REQUIRE_NOTHROW(vm.step());

				REQUIRE(state.timer() == i);

				for (size_t j = 0; j < 0xF; j++)
				{
					if (regidx == j) continue;
					REQUIRE(state.reg(j) == 0);
				}
			}
		}
	}

	SECTION("Set buzzer register.")
	{
		for (size_t i = 0; i < 0xFF; i += 0x40)
		{
			for (size_t regidx = 0; regidx < state.register_size(); regidx++)
			{
				vm.reset();
				state.reset();

				state.program_counter(0x0);
				state.memory(0, 0xF0 | regidx);
				state.memory(1, 0x18);

				state.reg(regidx, i);
				REQUIRE_NOTHROW(vm.step());

				REQUIRE(state.sound_timer() == i);

				for (size_t j = 0; j < 0xF; j++)
				{
					if (regidx == j) continue;
					REQUIRE(state.reg(j) == 0);
				}
			}
		}
	}

	SECTION("Add register to index register.")
	{
		for (size_t i = 0; i < 0xFF; i += 0x40)
		{
			for (size_t regidx = 0; regidx < state.register_size(); regidx++)
			{
				vm.reset();
				state.reset();

				state.program_counter(0x0);
				state.memory(0, 0xF0 | regidx);
				state.memory(1, 0x1E);

				state.reg(regidx, i);
				REQUIRE_NOTHROW(vm.step());

				REQUIRE(state.index_register() == i);

				for (size_t j = 0; j < 0xF; j++)
				{
					if (regidx == j) continue;
					REQUIRE(state.reg(j) == 0);
				}
			}
		}
	}

	SECTION("Add register to index register.")
	{
		for (size_t i = 0; i < 0xF; i += 0x40)
		{
			vm.reset();
			state.reset();

			state.program_counter(0x0);
			state.memory(0, 0xF0 | i);
			state.memory(1, 0x29);

			REQUIRE_NOTHROW(vm.step());

			REQUIRE(state.index_register() == (state.font_offset() + (i * 5)));
		}
	}

	SECTION("Add register to index register.")
	{
		for (size_t i = 0; i < 0xF; i += 0x40)
		{
			vm.reset();
			state.reset();

			state.program_counter(0x0);
			state.memory(0, 0xF0 | i);
			state.memory(1, 0x33);
			state.index_register(0x512);

			REQUIRE_NOTHROW(vm.step());

			REQUIRE(state.memory()[0x512] == (i % 1000) / 100);
			REQUIRE(state.memory()[0x512 + 1] == (i % 100) / 10);
			REQUIRE(state.memory()[0x512 + 2] == (i % 10));

			REQUIRE(state.index_register() == 0x512);
		}
	}

	SECTION("Store registers in memory.")
	{
		for (size_t i = 0; i <= 0xF; i++)
		{
			vm.reset();

			for (size_t regidx = 0; regidx < state.register_size(); regidx++)
			{
				state.reg(regidx);
			}

			for (size_t j = 0; j < state.memory_size(); j++) state.memory(j, 0);

			state.memory(0, 0xF0 | i);
			state.memory(1, 0x55);
			state.index_register(0x512);

			REQUIRE_NOTHROW(vm.step());

			REQUIRE(state.index_register() == (0x512 + i + 1));

			for (size_t j = 0x512; j <= 0x512 + i; j++)
			{
				REQUIRE(((int)state.memory()[j]) == state.reg(j - 0x512));
			}

			for (size_t j = 0x512 + i + 1; j < state.memory_size(); j++)
			{
				REQUIRE(state.memory()[j] == 0);
			}

		}
	}

	SECTION("Store memory in registers.")
	{
		for (size_t i = 0; i <= 0xF; i++)
		{
			vm.reset();

			for (size_t regidx = 0; regidx < state.register_size(); regidx++)
			{
				state.reg(regidx);
			}

			for (size_t j = 0; j < state.memory_size(); j++) state.memory(j, 0);
			for (size_t j = 0; j < state.register_size(); j++) state.memory(0x512 + j, j + 1);

			state.memory(0, 0xF0 | i);
			state.memory(1, 0x65);
			state.index_register(0x512);

			REQUIRE_NOTHROW(vm.step());

			for (size_t j = 0; j <= i; j++) REQUIRE(state.reg(j) == (j + 1));
			for (size_t j = i + 1; j < state.register_size(); j++) REQUIRE(state.reg(j) == 0);

			REQUIRE(state.index_register() == (0x512 + i + 1));
		}
	}
}

TEST_CASE("Test renderer.", "[Rendering]")
{
	Chip8VM::Rendering::DummyRenderer renderer;
	Chip8VM::VM vm(renderer);

	for (size_t x = 0; x < 64; x += 8)
	{
		for (size_t y = 0; y < 32; y += 8)
		{
			REQUIRE(renderer.set_pixel(x, y, 1) == 0);
			renderer.update();
		}
	}

	for (size_t x = 0; x < 64; x += 8)
	{
		for (size_t y = 0; y < 32; y += 8)
		{
			REQUIRE(renderer.set_pixel(x, y, 0) == 0);
			renderer.update();
		}
	}


	for (size_t x = 0; x < 64; x += 8)
	{
		for (size_t y = 0; y < 32; y += 8)
		{
			REQUIRE(renderer.set_pixel(x, y, 1) == 1);
			renderer.update();
		}
	}

	for (size_t x = 0; x < 64; x += 8)
	{
		for (size_t y = 0; y < 32; y += 8)
		{
			REQUIRE(renderer.set_pixel(x, y, 0) == 0);
			renderer.update();
		}
	}
}

TEST_CASE("Test renderer 2.", "[Rendering]")
{
	Chip8VM::Rendering::SDLRenderer renderer(64, 32);
	Chip8VM::VM vm(renderer);
	State& state(vm.manipulate_state());

	for (size_t x = 0; x < 64; x++)
	{
		for (size_t y = 0; y < 32; y++)
		{
			renderer.set_pixel(x, y, 1);
		}
	}
	
	renderer.update();

	for (size_t x = 0; x < 64; x += 2)
	{
		for (size_t y = 0; y < 32; y += 2)
		{
			renderer.set_pixel(x, y, 1);

		}
	}

	renderer.update();
}
