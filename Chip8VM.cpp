// Chip8VM.cpp : Defines the entry point for the application.
//

#include <cstdint>
#include <memory>
#include <string>
#include <sstream>
#include <iomanip>
#include <exception>
#include <stdexcept>
#include <algorithm>
#include <iterator>
#include <random>

#include "Chip8VM.h"


//Look ahead value for outputting the memory of the vm.
//Consecutive values that occur more than MIN_LOOK_AHEAD times will be compressed.
#define MIN_LOOK_AHEAD 8

namespace Chip8VM
{
	/**
	 * @brief Clears a vector ensuring the size is the provided size.
	 * @tparam T Vector type.
	 * @param vec Vector to initialize.
	 * @param size Size of the vector to use.
	*/
	template<typename T>
	void init_zero(std::vector<T>& vec, const size_t size)
	{
		vec.resize(size);
		std::fill(vec.begin(), vec.end(), 0);
	}

	State::State()
		:
		_memory_size(DEFAULT_MEMORY_SIZE),
		_register_size(DEFAULT_REGISTER_SIZE),
		_callstack_size(DEFAULT_CALLSTACK_SIZE),
		_flag_register(DEFAULT_FLAG_REGISTER),
		_program_load_offset(DEFAULT_PROGRAM_LOAD_OFFSET),
		_font_offset(DEFAULT_FONT_OFFSET),

		_memory(),
		_callstack(),
		_regs(),

		_pc(0),
		_i(0),
		_sound_timer(0),
		_timer(0),
		_callstack_ptr(0)
	{
		init_zero(_memory, _memory_size);
		init_zero(_regs, _register_size);
		_callstack.reserve(_callstack_size);
		memory_init();
	}

	void State::reset()
	{
		init_zero(_memory, _memory_size);
		init_zero(_regs, _register_size);
		_callstack.clear();

		_pc = 0;
		_i = 0;
		_callstack_ptr = 0;
		_sound_timer = 0;
		_timer = 0;

		memory_init();
	}

	void State::clear_registers()
	{
		std::fill(_regs.begin(), _regs.end(), 0);
	}

	void State::memory_init()
	{
		if (_font_offset + DEFAULT_FONT_DATA.size() > _memory.size())
		{
			throw std::exception("Font data out of bounds for memory.");
		}

		std::fill(_memory.begin(), _memory.end(), 0);
		std::copy(DEFAULT_FONT_DATA.begin(), DEFAULT_FONT_DATA.end(), _memory.begin() + _font_offset);
	}

	std::string State::to_string() const
	{
		std::stringstream stream;

		stream << "State {"
			<< "memory[" << _memory_size << "] {";

		for (std::vector<memval>::const_iterator it = _memory.cbegin(); it != _memory.cend();) {
			const memval& curval(*it);

			static_assert(sizeof(long) >= sizeof(memval));
			stream << "0x" << std::hex << static_cast<long>(curval);

			std::vector<memval>::const_iterator look_ahead_it = _memory.cbegin();
			std::advance(look_ahead_it, std::distance(_memory.cbegin(), it));

			size_t lookahead_count = 0;
			for (; look_ahead_it != _memory.cend(); ++look_ahead_it)
			{
				if (*look_ahead_it == curval)
				{
					lookahead_count++;
				}
				else
				{
					break;
				}
			}

			if (lookahead_count > MIN_LOOK_AHEAD)
			{
				stream << " (x" << std::dec << (lookahead_count + 1) << ")";
				it = look_ahead_it;
			}

			if (it != _memory.cend())
			{
				stream << ", ";
				it++;
			}
		}

		stream << "}"
			<< "}";

		return stream.str();
	}

	size_t State::program_load_offset() const
	{
		return _program_load_offset;
	}

	size_t State::memory_size() const
	{
		return _memory_size;
	}

	const std::vector<memval>& State::memory() const
	{
		return _memory;
	}

	void State::memory(const memptr index, const memval value)
	{
		if (index < 0 || index > _memory_size)
		{
			raise_memory_access_error("Memory write out of bounds. ", index);
		}

		_memory[index] = value;
	}

	std::vector<memval>::iterator State::memory_begin()
	{
		return _memory.begin();
	}

	std::vector<memval>::iterator State::memory_end()
	{
		return _memory.end();
	}

	std::vector<memval>::const_iterator State::memory_cbegin() const
	{
		return _memory.cbegin();
	}

	std::vector<memval>::const_iterator State::memory_cend() const
	{
		return _memory.cend();
	}

	memptr State::program_counter() const
	{
		return _pc;
	}

	void State::program_counter(const memptr value)
	{
		if (value > memory_size())
		{
			raise_memory_access_error("Cannot set program counter to value.", value);
		}

		_pc = value;
	}

	size_t State::register_size() const
	{
		return _register_size;
	}

	const std::vector<memptr>& State::callstack() const
	{
		return _callstack;
	}

	memptr State::pop_callstack()
	{
		if (_callstack.empty())
		{
			throw std::runtime_error("Can't pop empty stack.");
		}

		const memptr value(_callstack.back());
		_callstack.pop_back();

		return value;
	}

	void State::push_callstack(const memptr value)
	{
		if (value < 0 || value > memory_size())
		{
			raise_memory_access_error("Value pushed onto the stack is out of range.", value);
		}

		_callstack.push_back(value);
	}

	regval State::reg(const size_t index) const
	{
		if (index < 0 || index > register_size())
		{
			raise_register_access_error("No register for index.", index, register_size());
		}

		assert(index >= 0 && index < _regs.size());
		return _regs[index];
	}

	void State::reg(const size_t index, const regval register_value)
	{
		assert(index >= 0 && index <= _regs.size());
		_regs[index] = register_value;
	}

	size_t State::flag_register() const
	{
		assert(_flag_register <= _regs.size());
		return _regs[_flag_register];
	}

	void State::flag_register(const regval flags)
	{
		assert(_flag_register <= _regs.size());
		_regs[_flag_register] = flags;
	}

	memptr State::index_register() const
	{
		return _i;
	}

	void State::index_register(const memptr index)
	{
		assert(index >= 0 && index <= _memory_size);
		_i = index;
	}

	regval State::timer() const
	{
		return _timer;
	}

	void State::timer(const regval value)
	{
		_timer = value;
	}

	regval State::sound_timer() const
	{
		return _sound_timer;
	}

	void State::sound_timer(const regval value)
	{
		_sound_timer = value;
	}

	memptr State::font_offset() const
	{
		return _font_offset;
	}

	void State::font_offset(const size_t value)
	{
		_font_offset = value;
	}


	////////////// VM

	VM::VM(const State& state, Rendering::Renderer& renderer)
		:
		_state(state),
		renderer(renderer),
		generator(),
		dist(MIN_REGISTER_VALUE, MAX_REGISTER_VALUE),
		_execution_state(VMExecutionState::RUNNING),
		_last_state(VMExecutionState::INIT)
	{

	}

	VM::VM(Rendering::Renderer& renderer)
		:
		_state(),
		renderer(renderer),
		generator(),
		dist(MIN_REGISTER_VALUE, MAX_REGISTER_VALUE),
		_execution_state(VMExecutionState::RUNNING),
		_last_state(VMExecutionState::INIT)
	{

	}

	VM::VM(State&& state, Rendering::Renderer& renderer)
		:
		_state(std::move(state)),
		renderer(renderer),
		generator(),
		dist(MIN_REGISTER_VALUE, MAX_REGISTER_VALUE),
		_execution_state(VMExecutionState::RUNNING),
		_last_state(VMExecutionState::INIT)
	{

	}

	State& VM::manipulate_state()
	{
		return _state;
	}

	const State& VM::state() const
	{
		return _state;
	}

	void VM::reset()
	{
		_state.reset();

		_execution_state = VMExecutionState::RUNNING;
		_last_state = VMExecutionState::INIT;
		_key_target_register = 0;
		_key_value = -1;
	}

	size_t State::flag_register_index() const
	{
		return _flag_register;
	}


	VMExecutionState VM::execution_state() const
	{
		return _execution_state;
	}


	Instruction VM::fetch_and_decode(const memptr index)
	{
		static_assert(sizeof(instr_value) >= sizeof(memval) * 2);
		if (index + 1 >= _state.memory_size())
		{
			raise_memory_access_error("Reading instruction is out of bounds.", index + 1);
		}

		const instr_value(_state.memory()[index] << CHAR_BIT | _state.memory()[index + 1]);

		return { _state.memory()[index] << CHAR_BIT | _state.memory()[index + 1] };
	}

	void VM::execute_prefix0_instruction(VMCycleState& state)
	{
		const Instruction& instruction(state.instruction);
		//  00E0 clear
		//	00EE return Exit a subroutine
		switch (instruction.lower_triplet())
		{
			//This isn't exactly official, but it does... make sense.
		case 0x0000:
			//NOP
			break;
		case 0x00E0:
			renderer.clear();
			break;
		case 0x00EE:
			state.next_program_counter_value(_state.pop_callstack());
			break;
		default:
			raise_illegal_opcode_error("Illegal 0 opcode instruction.", instruction.instruction);
		}
	}

	void VM::execute_prefix1_instruction(VMCycleState& state)
	{
		const Instruction& instruction(state.instruction);

		//	1NNN jump NNN
		state.next_program_counter_value(instruction.lower_triplet());
	}

	void VM::execute_prefix2_instruction(VMCycleState& state)
	{
		//	2NNN NNN Call a subroutine
		const Instruction& instruction(state.instruction);
		//TODO move callstack handling into state
		_state.push_callstack(_state.program_counter() + CHARS_PER_INSTRUCTION);
		state.next_program_counter_value(instruction.lower_triplet());
	}

	void VM::execute_prefix3_instruction(VMCycleState& state)
	{
		const Instruction& instruction(state.instruction);
		//	3XNN if vx != NN then
		const size_t reg_index(instruction.nibble(2));
		const instr_value reg_val(_state.reg(reg_index));
		const instr_value cmp_value(instruction.lower_byte());

		//Skip next instruction if the values are equal. Only execute next instruction if values are unequal.
		if (reg_val == cmp_value)
		{
			state.next_program_counter_value(state.next_program_counter_value() + CHARS_PER_INSTRUCTION);
		}
	}


	void VM::execute_prefix4_instruction(VMCycleState& state)
	{
		const Instruction& instruction(state.instruction);
		//	4XNN if vx == NN then
		const size_t reg_index(instruction.nibble(2));
		const instr_value reg_val(_state.reg(reg_index));
		const instr_value cmp_value(instruction.lower_byte());

		//Skip next instruction if the values are equal. Only execute next instruction if values are unequal.
		if (reg_val != cmp_value)
		{
			state.next_program_counter_value(state.next_program_counter_value() + CHARS_PER_INSTRUCTION);
		}
	}


	void VM::execute_prefix5_instruction(VMCycleState& state)
	{
		const Instruction& instruction(state.instruction);
		//	5XY0 if vx != vy then
		if (instruction.nibble(0) != 0)
		{
			raise_illegal_opcode_error("5XY0 instruction lowest nibble isn't 0.", instruction.instruction);
		}

		const size_t reg_index_x(instruction.nibble(2));
		const size_t reg_index_y(instruction.nibble(1));

		const size_t reg_val_x(_state.reg(reg_index_x));
		const size_t reg_val_y(_state.reg(reg_index_y));


		if (reg_val_x == reg_val_y)
		{
			state.next_program_counter_value(state.next_program_counter_value() + CHARS_PER_INSTRUCTION);
		}
	}


	void VM::execute_prefix6_instruction(VMCycleState& state)
	{
		const Instruction& instruction(state.instruction);
		//	6XNN vx : = NN
		const size_t reg_index(instruction.nibble(2));
		const regval value(instruction.lower_byte());

		_state.reg(reg_index, value);
	}


	void VM::execute_prefix7_instruction(VMCycleState& state)
	{
		const Instruction& instruction(state.instruction);
		//	7XNN vx += NN
		const size_t reg_index(instruction.nibble(2));
		const regval old_value(_state.reg(reg_index));

		const regval value(instruction.lower_byte());

		_state.reg(reg_index, value + old_value);
	}


	void VM::execute_prefix8_instruction(VMCycleState& state)
	{
		const Instruction& instruction(state.instruction);

		const size_t reg_index_x(instruction.nibble(2));
		const size_t reg_index_y(instruction.nibble(1));

		const size_t reg_val_x(_state.reg(reg_index_x));
		const size_t reg_val_y(_state.reg(reg_index_y));

		switch (instruction.nibble(0))
		{
		case 0:
			//	8XY0 vx : = vy
			_state.reg(reg_index_x, reg_val_y);
			break;
		case 1:
			//	8XY1 vx |= vy Bitwise OR
			_state.reg(reg_index_x, reg_val_x | reg_val_y);
			break;
		case 2:
			//	8XY2 vx &= vy Bitwise AND
			_state.reg(reg_index_x, reg_val_x & reg_val_y);
			break;
		case 3:
			//	8XY3 vx ^= vy Bitwise XOR
			_state.reg(reg_index_x, reg_val_x ^ reg_val_y);
			break;
		case 4:
			//	8XY4 vx += vy vf = 1 on carry
			static_assert(sizeof(int) > sizeof(regval));


			if (static_cast<int>(reg_val_x) + static_cast<int>(reg_val_y) > MAX_REGISTER_VALUE)
			{
				_state.flag_register(1);
			}
			else
			{
				_state.flag_register(0);
			}
			_state.reg(reg_index_x, reg_val_x + reg_val_y);
			break;
		case 5:
			//	8XY5 vx -= vy vf = 0 on borrow
			if (reg_val_x < reg_val_y)
			{
				_state.flag_register(0);
			}
			else
			{
				//TODO check if this is correct behaviour
				_state.flag_register(1);
			}
			_state.reg(reg_index_x, reg_val_x - reg_val_y);
			break;
		case 6:
			//	8XY6 vx >>= vy vf = old least signicant bit
			_state.flag_register(reg_val_x & 1);
			_state.reg(reg_index_x, reg_val_x >> reg_val_y);
			break;
		case 7:
			//	8XY7 vx = vy - vx vf = 0 on borrow
			static_assert(sizeof(int) > sizeof(regval));

			if (reg_val_y < reg_val_x)
			{
				_state.flag_register(1);
			}
			else
			{
				//TODO check if this is correct behaviour
				_state.flag_register(0);
			}
			_state.reg(reg_index_x, reg_val_y - reg_val_x);
			break;
		case 0xE:
			//	8XYE vx <<= vy vf = old most signicant bit
			static_assert(sizeof(regval) == 1 && CHAR_BIT == 8);
			if (reg_val_x & 0x80)
			{
				_state.flag_register(1);
			}
			else
			{
				_state.flag_register(0);
			}

			_state.reg(reg_index_x, reg_val_x << reg_val_y);
			break;
		default:
			raise_illegal_opcode_error("8XYB instruction has illegal B value. ", instruction.instruction);
		}
	}


	void VM::execute_prefix9_instruction(VMCycleState& state)
	{
		const Instruction& instruction(state.instruction);

		//	9XY0 if vx == vy then
		if (instruction.nibble(0) != 0)
		{
			raise_illegal_opcode_error("9XY0 instruction lowest nibble isn't 0.", instruction.instruction);
		}

		const size_t reg_index_x(instruction.nibble(2));
		const size_t reg_index_y(instruction.nibble(1));

		const regval reg_value_x(_state.reg(reg_index_x));
		const regval reg_value_y(_state.reg(reg_index_y));

		if (reg_value_x != reg_value_y)
		{
			//this should ultimately increase the program counter by 2 this cycle combined with the normal increase.
			state.next_program_counter_value(state.next_program_counter_value() + CHARS_PER_INSTRUCTION);
		}
	}


	void VM::execute_prefixA_instruction(VMCycleState& state)
	{
		const Instruction& instruction(state.instruction);
		//	ANNN i : = NNN
		_state.index_register(instruction.lower_triplet());
	}


	void VM::execute_prefixB_instruction(VMCycleState& state)
	{
		const Instruction& instruction(state.instruction);
		//	BNNN jump0 NNN Jump to address NNN + v0
		// Jumps to the address NNN plus V0. 
		memptr offset(_state.reg(0));
		offset += instruction.lower_triplet();

		state.next_program_counter_value(offset);
	}


	void VM::execute_prefixC_instruction(VMCycleState& state)
	{
		const Instruction& instruction(state.instruction);
		//	CXNN vx : = random NN Random number 0 - 255 AND NN
		// Sets VX to the result of a bitwise and operation on a random number (Typically: 0 to 255) and NN. 
		const regval rnd_val(dist(generator));
		const size_t reg_index(instruction.nibble(2));
		const regval and_value(instruction.lower_byte());

		_state.reg(reg_index, and_value & rnd_val);
	}


	void VM::execute_prefixD_instruction(VMCycleState& state)
	{
		//	DXYN sprite vx vy N vf = 1 on collision
		// Draws a sprite at coordinate (VX, VY) that has a width of 8 pixels and a height of N+1 pixels. 
		// Each row of 8 pixels is read as bit-coded starting from memory location I; 
		// I value does not change after the execution of this instruction. 
		// As described above, VF is set to 1 if any screen pixels are flipped from set to unset 
		// when the sprite is drawn, and to 0 if that does not happen 
		const Instruction& instruction(state.instruction);

		const regval offset_x(_state.reg(instruction.nibble(2)));
		const regval offset_y(_state.reg(instruction.nibble(1)));

		const regval height(instruction.nibble(0));

		int modified = 0;

		if (height + 1 + _state.index_register() > _state.memory_size())
		{
			throw MemoryAccessError("Draw index out of bounds.");
		}

		for (size_t row = 0; row < height; row++)
		{
			int mem_val(_state.memory()[row + _state.index_register()]);

			for (size_t col = 0; col < CHAR_BIT; col++)
			{
				const int pixel_value = (mem_val & 0x80) != 0 ? 1 : 0;
				modified |= renderer.set_pixel(offset_x + col, offset_y + row, pixel_value);
				mem_val <<= 1;
			}
		}

		_state.flag_register(modified);
	}


	void VM::execute_prefixE_instruction(VMCycleState& state)
	{
		//	EX9E if vx - key then Is a key not pressed ?
		//	EXA1 if vx key then Is a key pressed ?

		const Instruction& instruction(state.instruction);

		const size_t reg_index(instruction.nibble(2));
		const regval reg_value(_state.reg(reg_index));

		switch (instruction.lower_byte())
		{
		case 0x9E:
		{
			// Skip the following instruction if the key corresponding to the hex value 
			// currently stored in register VX is pressed
			if (key_pressed() && _key_value == reg_value)
			{
				state.next_program_counter_value(state.next_program_counter_value() + CHARS_PER_INSTRUCTION);
			}
		}
		break;
		case 0xA1:
		{
			// Skip the following instruction if the key corresponding to the hex value
			// currently stored in register VX is not pressed
			if (key_pressed() && _key_value != reg_value)
			{
				state.next_program_counter_value(state.next_program_counter_value() + CHARS_PER_INSTRUCTION);
			}
		}
		break;
		default:
			raise_illegal_opcode_error("Illegal 0xE prefix instruction.", instruction.instruction);
		}
	}


	void VM::execute_prefixF_instruction(VMCycleState& state)
	{
		//	FX07 vx : = delay
		//	FX0A vx : = key Wait for a keypress
		//	FX15 delay : = vx
		//	FX18 buzzer : = vx
		//	FX1E i += vx
		//	FX29 i : = hex vx Set i to a hex character
		//	FX33 bcd vx Decode vx into binary - coded decimal
		//	FX55 save vx Save v0 - vx to i through(i + x)
		//	FX65 load vx Load v0 - vx from i through(i + x)

		const Instruction& instruction(state.instruction);
		const regval data(instruction.nibble(2));
		const regval suffix(instruction.lower_byte());

		switch (suffix)
		{
		case 0x07:
			// Store the current value of the delay timer in register VX 
			_state.reg(data, _state.timer());
			break;
		case 0x0A:
			//Wait for a keypressand store the result in register VX
			state.key_target_register(data);
			state.execution_state(VMExecutionState::WAIT_FOR_KEY);
			break;
		case 0x15:
			// FX15 	Set the delay timer to the value of register VX
			_state.timer(_state.reg(data));
			break;
		case 0x18:
			// FX18 	Set the sound timer to the value of register VX
			_state.sound_timer(_state.reg(data));
			break;
		case 0x1E:
			// FX1E 	Add the value stored in register VX to register I
			_state.index_register(_state.index_register() + _state.reg(data));
			break;
		case 0x29:
			// FX29 	Set I to the memory address of the sprite data corresponding to the hexadecimal digit stored in register VX
		{
			const regval digit(_state.reg(data));
			if (digit < 0 || digit > 16)
			{
				std::stringstream msg;
				msg << "Hexadecimal sprite requested by FX29, but register value is out of bounds. (Register value: "
					<< std::hex << std::setw(2) << std::setfill('0') << digit << ")";

				throw std::runtime_error(msg.str());
			}

			const memptr digit_offset(_state.font_offset() + SPRITE_CHARS * digit);
			_state.index_register(digit_offset);
			break;
		}
		case 0x33:
		{
			// FX33 	Store the binary - coded decimal equivalent of the value stored in register VX at addresses I, I + 1, and I + 2
			if (_state.index_register() + 2 >= _state.memory_size())
			{
				raise_memory_access_error("FX33 instruction out of bounds.", _state.index_register() + 2);
			}

			const regval reg_value(_state.reg(data));
			const memptr i(_state.index_register());

			_state.memory(i, (reg_value % 1000) / 100);
			_state.memory(i + 1, (reg_value % 100) / 10);
			_state.memory(i + 2, (reg_value % 10));

			break;
		}
		case 0x55:
		{
			// FX55 	Store the values of registers V0 to VX inclusive in memory starting at address I
			// I is set to I + X + 1 after operation²
			if (_state.index_register() + data > _state.memory_size())
			{
				raise_memory_access_error("Out of bounds access.", _state.index_register() + data);
			}

			for (size_t i = 0; i <= data; i++)
			{
				_state.memory(_state.index_register() + i, _state.reg(i));
			}

			_state.index_register(_state.index_register() + data + 1);
			break;
		}
		case 0x65:
		{
			// FX65 	Fill registers V0 to VX inclusive with the values stored in memory starting at address I
			// I is set to I + X + 1 after operation²			
			if (_state.index_register() + data > _state.memory_size())
			{
				raise_memory_access_error("Out of bounds access.", _state.index_register() + data);
			}

			for (size_t i = 0; i <= data; i++)
			{
				_state.reg(i, _state.memory()[_state.index_register() + i]);
			}

			_state.index_register(_state.index_register() + data + 1);

			break;
		}
		default:
			raise_illegal_opcode_error("FXNN instruction with illegal suffix opcode: ", instruction.instruction);
		}
	}


	void VM::execute_instruction(VMCycleState& state)
	{
		//if (state.current_program_counter_value & 0x1)
		//{
		//	raise_memory_alignment_error("Instruciton counter misaligned.", state.current_program_counter_value);
		//}

		switch (state.instruction.prefix())
		{
		case 0x0:
			execute_prefix0_instruction(state);
			break;
		case 0x1:
			execute_prefix1_instruction(state);
			break;
		case 0x2:
			execute_prefix2_instruction(state);
			break;
		case 0x3:
			execute_prefix3_instruction(state);
			break;
		case 0x4:
			execute_prefix4_instruction(state);
			break;
		case 0x5:
			execute_prefix5_instruction(state);
			break;
		case 0x6:
			execute_prefix6_instruction(state);
			break;
		case 0x7:
			execute_prefix7_instruction(state);
			break;
		case 0x8:
			execute_prefix8_instruction(state);
			break;
		case 0x9:
			execute_prefix9_instruction(state);
			break;
		case 0xA:
			execute_prefixA_instruction(state);
			break;
		case 0xB:
			execute_prefixB_instruction(state);
			break;
		case 0xC:
			execute_prefixC_instruction(state);
			break;
		case 0xD:
			execute_prefixD_instruction(state);
			break;
		case 0xE:
			execute_prefixE_instruction(state);
			break;
		case 0xF:
			execute_prefixF_instruction(state);
			break;
		}
	}

	void VM::step()
	{
		switch (_execution_state)
		{
		case VMExecutionState::RUNNING:
		{
			VMCycleState state = {
				_execution_state,
				_state.program_counter(),
				_state.program_counter() + CHARS_PER_INSTRUCTION,
				fetch_and_decode(_state.program_counter())
			};

			execute_instruction(state);

			_state.program_counter(state.next_program_counter_value() % _state.memory_size());
			_last_state = _execution_state;
			_execution_state = state.execution_state();

			if (_execution_state == VMExecutionState::WAIT_FOR_KEY)
			{
				_key_target_register = state.key_target_register();
			}

			renderer.update();
		}
		break;
		case VMExecutionState::WAIT_FOR_KEY:
		{

			if (_last_state != VMExecutionState::WAIT_FOR_KEY)
			{
				std::cout << "Parking wait for key." << std::endl;
			}

			_last_state = _execution_state;


			if (key_pressed())
			{
				_execution_state = VMExecutionState::RUNNING;
				_state.reg(_key_target_register, _key_value);
				step();
			}
		}
		break;
		default:
			raise_unimplemented_state_error(_execution_state);
		}
	}

	bool VM::key_pressed() const
	{
		return _key_value >= 0;
	}

	void VM::keydown(const int keyvalue)
	{
		assert(keyvalue >= 0 && keyvalue <= 0xF);
		_key_value = keyvalue;
	}

	void VM::keyup()
	{
		_key_value = -1;
	}

	std::string VM::to_string() const
	{
		return _state.to_string();
	}

	// VMCycleState
	VMCycleState::VMCycleState(const VMExecutionState execution_state, const memptr current_program_counter_value, const memptr next_program_counter_value, const Instruction instruction)
		:
		_execution_state(execution_state),
		current_program_counter_value(current_program_counter_value),
		_next_program_counter_value(next_program_counter_value),
		instruction(instruction),
		_key_target_register(0)
	{

	}

	size_t VMCycleState::key_target_register() const
	{
		return _key_target_register;
	}

	void VMCycleState::key_target_register(const size_t target_reg)
	{
		_key_target_register = target_reg;
	}

	void VM::update_timers()
	{
		if (_state.sound_timer() > 0)
		{
			_state.sound_timer(_state.sound_timer() - 1);
		}

		if (_state.timer() > 0)
		{
			_state.timer(_state.timer() - 1);
		}
	}

	VMExecutionState VMCycleState::execution_state() const
	{
		return _execution_state;
	}

	void VMCycleState::execution_state(const VMExecutionState new_state)
	{
		_execution_state = new_state;
	}

	memptr VMCycleState::next_program_counter_value() const
	{
		return _next_program_counter_value;
	}

	void VMCycleState::next_program_counter_value(const memptr value)
	{

		//if (value & 0x1)
		//{
		//	raise_memory_alignment_error("Program counter misaligned.", value);
		//}

		_next_program_counter_value = value;
	}

	// Instruction

	Instruction::Instruction(const instr_value instruction)
		:
		instruction(instruction)
	{

	}

	instr_value Instruction::upper_byte() const
	{
		return (instruction >> CHAR_BIT) & CHAR_MASK;
	}

	instr_value Instruction::lower_byte() const
	{
		return instruction & CHAR_MASK;
	}


	instr_value Instruction::upper_triplet() const
	{
		return (instruction >> HALF_CHAR_SIZE) & 0xFFF;
	}

	instr_value Instruction::lower_triplet() const
	{
		return instruction & 0xFFF;
	}


	instr_value Instruction::prefix() const
	{
		return nibble(3);
	}

	instr_value Instruction::suffix() const
	{
		return nibble(0);
	}


	instr_value Instruction::nibble(const int index) const
	{
		if (index < 0 || index > 3) throw std::runtime_error("Nibble index out of range.");
		return (instruction >> (index * HALF_CHAR_SIZE)) & HALF_CHAR_MASK;
	}


	// Exceptions

	IllegalInstructionException::IllegalInstructionException(const char* reason)
		:
		std::runtime_error(reason)
	{

	}

	IllegalInstructionException::IllegalInstructionException(const std::string reason)
		:
		std::runtime_error(reason)
	{

	}

	void raise_illegal_opcode_error(const char* reason, const instr_value op_code)
	{
		std::stringstream errmsg;
		errmsg << reason << std::hex << std::setw(4) << std::setfill('0') << op_code;
		throw IllegalInstructionException(errmsg.str());
	}

	MemoryAccessError::MemoryAccessError(const char* reason)
		:
		std::runtime_error(reason)
	{

	}

	MemoryAccessError::MemoryAccessError(const std::string reason)
		:
		std::runtime_error(reason)
	{

	}

	RegisterAccessError::RegisterAccessError(const char* reason)
		:
		std::runtime_error(reason)
	{

	}
	RegisterAccessError::RegisterAccessError(const std::string reason)
		:
		std::runtime_error(reason)
	{

	}

	MemoryAlignmentError::MemoryAlignmentError(const char* reason)
		:
		std::runtime_error(reason)
	{

	}
	MemoryAlignmentError::MemoryAlignmentError(const std::string reason)
		:
		std::runtime_error(reason)
	{

	}

	void raise_memory_access_error(const char* reason, const memptr value)
	{
		std::stringstream errmsg;
		errmsg << reason << std::hex << std::setw(4) << std::setfill('0') << value;
		throw MemoryAccessError(errmsg.str());
	}

	void not_yet_implemented()
	{
		throw std::runtime_error("Not yet implemented.");
	}

	void unimplemented_opcode(const instr_value instruction)
	{
		std::stringstream msg;
		msg << "Unimplemented opcode: " << std::hex << std::setw(4) << std::setfill('0') << (int)instruction;

		throw std::runtime_error(msg.str());
	}

	void raise_register_access_error(const char* reason, const size_t index, const size_t regs_size)
	{
		std::stringstream msg;
		msg << reason << std::dec << index << "/" << regs_size;

		throw RegisterAccessError(msg.str());
	}

	void raise_memory_alignment_error(const char* reason, const size_t memory_idx)
	{
		std::stringstream msg;
		msg << reason << std::dec << memory_idx << "/" << std::hex << std::setw(4) << std::setfill('0') << memory_idx;

		throw MemoryAlignmentError(msg.str());
	}

	void raise_unimplemented_state_error(const VMExecutionState state)
	{
		std::stringstream msg;
		msg << "Unimplemented VMExecutionState: ";

		switch (state)
		{
		case VMExecutionState::RUNNING:
			msg << "RUNNING";
			break;
		case VMExecutionState::WAIT_FOR_KEY:
			msg << "WAIT_FOR_KEY";
			break;
		case VMExecutionState::INIT:
			msg << "INIT";
			break;
		default:
			msg << "<UNKOWN ?? TO STRING NOT IMPLEMENTED>";
			break;
		}

		throw std::runtime_error(msg.str());
	}
}


