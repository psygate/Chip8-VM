// Chip8VM.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <iostream>
#include <vector>
#include <iterator>
#include <algorithm>
#include <functional>
#include <exception>
#include <stdexcept>
#include <cassert>
#include <cstdint>
#include <climits>
#include <random>

#include "Chip8Renderer.h"

#ifndef assert

#endif

namespace Chip8VM
{
	static_assert(CHAR_BIT == 8);
	static_assert(INT_MIN < 0);
	static_assert(INT_MAX >= 0xFFFF);
	static_assert(sizeof(int) >= 2);

	// Memory values type.
	typedef unsigned char memval;
	typedef int memptr; static_assert(sizeof(memptr) >= 2); // atleast 16bit

	// Register value type.
	typedef unsigned char regval;
	const regval MAX_REGISTER_VALUE = UCHAR_MAX;
	const regval MIN_REGISTER_VALUE = 0;
	const regval HALF_CHAR_MASK = 0xF;
	const regval CHAR_MASK = 0xFF;
	const regval TRIPLET_MASK = 0xFFF;
	const int HALF_CHAR_SIZE = CHAR_BIT / 2;
	// Program counter increase per cycle.
	const memptr CHARS_PER_INSTRUCTION = 2;


	typedef int instr_value; static_assert(sizeof(instr_value) >= 2);

	// Default values for state construction. These are the Chip8 defaults.
	const size_t DEFAULT_REGISTER_SIZE = 16;
	const size_t DEFAULT_FLAG_REGISTER = 0xF;
	const size_t DEFAULT_PROGRAM_LOAD_OFFSET = 0x200;
	const size_t DEFAULT_MEMORY_SIZE = 4096;
	const size_t DEFAULT_CALLSTACK_SIZE = 4096;
	const size_t DEFAULT_FONT_OFFSET = 0x50;
	const size_t SPRITE_CHARS = 5;

	// Default font sprite data. Each sprite is 5 bytes in size.
	const std::vector<memval> DEFAULT_FONT_DATA = {
		0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
		0x20, 0x60, 0x20, 0x20, 0x70, // 1
		0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
		0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
		0x90, 0x90, 0xF0, 0x10, 0x10, // 4
		0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
		0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
		0xF0, 0x10, 0x20, 0x40, 0x40, // 7
		0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
		0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
		0xF0, 0x90, 0xF0, 0x90, 0x90, // A
		0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
		0xF0, 0x80, 0x80, 0x80, 0xF0, // C
		0xE0, 0x90, 0x90, 0x90, 0xE0, // D
		0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
		0xF0, 0x80, 0xF0, 0x80, 0x80  // F
	};

	/**
	 * @brief State of the Chip8 CPU.
	*/
	struct State
	{
	private:
		std::vector<memval> _memory;
		memptr _pc; //program counter
		memptr _i; //Index register
		std::vector<memptr> _callstack; // callstack

		regval _sound_timer;
		regval _timer;

		std::vector<regval> _regs; // Registers

		size_t _flag_register;
		size_t _program_load_offset;
		size_t _font_offset;

		size_t _memory_size;
		size_t _callstack_size;
		size_t _callstack_ptr;
		size_t _register_size;

		/**
		 * @brief Initialize the memory to its default values.
		*/
		void memory_init();
	public:
		/**
		 * @brief Constructs the state to its default state with default values.
		*/
		State();
		State(const State& state) = default;
		State(State&& state) = default;
		virtual ~State() = default;

		/**
		 * @brief Resets the state to its default values, erasing memory completely.
		*/
		void reset();

		/**
		 * @brief Clears registers setting them to their default values.
		*/
		void clear_registers();
		/**
		 * @brief Creates a string represenation of the state.
		 * @return String representing the state.
		*/
		std::string to_string() const;

		size_t program_load_offset() const;
		size_t memory_size() const;
		const std::vector<memval>& memory() const;
		void memory(const memptr index, const memval value);

		memptr program_counter() const;
		void program_counter(const memptr value);

		const std::vector<memptr>& callstack() const;
		/**
		 * @brief Removes the last element from the callstack and returns it.
		 * @return Last return address.
		*/
		memptr pop_callstack();
		/**
		 * @brief Adds the value as the last element on the callstack.
		 * @param value Return address.
		*/
		void push_callstack(const memptr value);

		regval reg(const size_t index) const;
		void reg(const size_t index, const regval register_value);

		std::vector<memval>::iterator memory_begin();
		std::vector<memval>::iterator memory_end();

		std::vector<memval>::const_iterator memory_cbegin() const;
		std::vector<memval>::const_iterator memory_cend() const;

		size_t flag_register_index() const;
		size_t flag_register() const;
		void flag_register(const regval flags);

		memptr index_register() const;
		void index_register(const memptr index);

		regval timer() const;
		void timer(const regval value);

		regval sound_timer() const;
		void sound_timer(const regval value);

		memptr font_offset() const;
		void font_offset(const size_t value);

		size_t register_size() const;
	};

	/**
	 * @brief Represents a single instruction of the vm.
	*/
	struct Instruction
	{
	public:
		Instruction(const instr_value instruction);
		/**
		 * @brief Raw instruction value.
		*/
		const instr_value instruction;

		/**
		 * @brief Returns the upper byte (high value byte) of the instruction. NNXX, NN = high order.
		 * @return High order byte of the instruction.
		*/
		instr_value upper_byte() const;

		/**
		 * @brief Returns the lower byte (low value byte) of the instruction. XXNN, NN = low order.
		 * @return Low order byte of the instruction.
		*/
		instr_value lower_byte() const;

		/**
		 * @brief Returns the high order triplet of the instruciton. NNNX, NNN = high order
		 * @return High order triplet of the instruction.
		*/
		instr_value upper_triplet() const;

		/**
		 * @brief Returns the low order triplet of the instruciton. XNNN, NNN = high order
		 * @return Low order triplet of the instruction.
		*/
		instr_value lower_triplet() const;

		/**
		 * @brief Returns the highest order nibble of the instruction. NXXX, N = highest order nibble.
		 * @return Highest order nibble of the instruction.
		*/
		instr_value prefix() const;

		/**
		 * @brief Returns the lowest order nibble of the instruction. XXXN, N = highest order nibble.
		 * @return Lowest order nibble of the instruction.
		*/
		instr_value suffix() const;

		/**
		 * @brief Returns the requested nibble.
		 * @param index Index of the nibble, starting at 0. Maximum nibble is sizeof(instr_value) - 1. (3 for default chip8)
		 * @return Nibble value.
		*/
		instr_value nibble(const int index) const;
	};

	/**
	 * @brief Describes the VM execution state.
	*/
	enum class VMExecutionState
	{
		INIT, // Initializing
		RUNNING, // Running / Ready to run
		WAIT_FOR_KEY, //Parked, waiting for key input.
	};

	/**
	 * @brief State delta. This class is used to update the VM state transactionally.
	*/
	struct VMCycleState
	{
	private:
		VMExecutionState _execution_state;
		memptr _next_program_counter_value;
		size_t _key_target_register;

	public:
		VMCycleState(const VMExecutionState execution_state, const memptr current_program_counter_value, const memptr next_program_counter_value, const Instruction instruction);
		VMCycleState(const VMCycleState& state) = delete;
		virtual ~VMCycleState() = default;
		/**
		 * @brief CUrrent program counter value. Points at current instruction in memory.
		*/
		const memptr current_program_counter_value;
		const Instruction instruction;

		/**
		 * @brief Return the current value of the NEXT progam counter value.
		 * @return Next program counter value.
		*/
		memptr next_program_counter_value() const;
		/**
		 * @brief Set the value of the next program counter value. (Default CHARS_PER_INSTRUCTION)
		 * @param value 
		*/
		void next_program_counter_value(const memptr value);

		VMExecutionState execution_state() const;
		/**
		 * @brief Set the execution state for the next cycle.
		 * @param new_state New execution state for the next cycle.
		*/
		void execution_state(const VMExecutionState new_state);

		/**
		 * @brief Current register where the key press will be stored.
		 * @return Register to store the key press in.
		*/
		size_t key_target_register() const;
		/**
		 * @brief Set the register to store a key press in.
		 * @param target_reg Set the register where a key press should be stored.
		*/
		void key_target_register(const size_t target_reg);
	};

	/**
	 * @brief VM class to execute a Chip8VM.
	*/
	class VM
	{
	private:
		//The last state of the VM, used for updates & logging.
		VMExecutionState _last_state;
		VMExecutionState _execution_state;
		State _state;
		// Pressed key value.
		int _key_value;
		// Used to store the register where a pressed key value should go.
		size_t _key_target_register;

		std::default_random_engine generator;
		std::uniform_int_distribution<int> dist;

		Rendering::Renderer& renderer;

		// Specific instruction functions to avoid scope pollution.
		void execute_prefix0_instruction(VMCycleState& state);
		void execute_prefix1_instruction(VMCycleState& state);
		void execute_prefix2_instruction(VMCycleState& state);
		void execute_prefix3_instruction(VMCycleState& state);
		void execute_prefix4_instruction(VMCycleState& state);
		void execute_prefix5_instruction(VMCycleState& state);
		void execute_prefix6_instruction(VMCycleState& state);
		void execute_prefix7_instruction(VMCycleState& state);
		void execute_prefix8_instruction(VMCycleState& state);
		void execute_prefix9_instruction(VMCycleState& state);
		void execute_prefixA_instruction(VMCycleState& state);
		void execute_prefixB_instruction(VMCycleState& state);
		void execute_prefixC_instruction(VMCycleState& state);
		void execute_prefixD_instruction(VMCycleState& state);
		void execute_prefixE_instruction(VMCycleState& state);
		void execute_prefixF_instruction(VMCycleState& state);

		// True if any key is currently pressed.
		bool key_pressed() const;
	public:
		VM() = default;
		VM(const State& state, Rendering::Renderer& renderer);
		VM(Rendering::Renderer& renderer);
		VM(State&& state, Rendering::Renderer& renderer);
		VM(const VM& vm) = default;
		VM(VM&& vm) = default;
		virtual ~VM() = default;

		/**
		 * @brief Execute a single step of the VM. (decode, fetch, execute, ...)
		*/
		void step();
		/**
		 * @brief Tick the timer registers of the VM. (Generally decrement by 1 until 0)
		*/
		void update_timers();
		/**
		 * @brief Resets the VM completely, erasing any state and returning to the default state.
		*/
		void reset();

		/**
		 * @brief Fetches and decodes the next instruction.
		 * @param index Index of the instruction to fetch and decode. (Must be within memory limits.)
		 * @return Decoded instruction.
		*/
		Instruction fetch_and_decode(const memptr index);
		/**
		 * @brief Executes a specific instruction.
		 * @param state VMCycleState to use for updates.
		*/
		void execute_instruction(VMCycleState& state);
		/**
		 * @brief Sets the renderer of the current VM.
		 * @param renderer 
		*/
		void set_renderer(Rendering::Renderer& renderer);

		VMExecutionState execution_state() const;

		const State& state() const;
		State& manipulate_state();

		/**
		 * @brief Set the value of the key being pressed. (Must be 0 - 0xF for Chip8VM)
		 * @param keyvalue Value of the key being pressed.
		*/
		void keydown(const int keyvalue);
		/**
		 * @brief Clears the pressed key value.
		*/
		void keyup();

		/**
		 * @brief Loads a program into the VM state starting at the program load offset.
		 * @tparam Iterator Iterator type to use for iteration. Must be useable with std::distance
		 * @param begin Start of data.
		 * @param end End of data.
		*/
		template<typename Iterator>
		void load_program(const Iterator& begin, const Iterator& end)
		{
			if (std::distance(begin, end) + _state.program_load_offset() > _state.memory_size())
			{
				throw std::exception("Program too big for memory.");
			}

			_state.reset();
			std::vector<memval>::iterator it(_state.memory_begin());
			std::advance(it, _state.program_load_offset());

			std::copy(begin, end, it);
			assert(_state.memory().size() == _state.memory_size());
			_state.program_counter(_state.program_load_offset());
		}
		/**
		 * @brief String representation of the vm.
		 * @return String representation of the vm.
		*/
		std::string to_string() const;
	};

	/**
	 * @brief Throws an illegal opcode exception with a formatted error message.
	 * @param reason Reason string for the error.
	 * @param op_code Opcode of the illegal instruction.
	*/
	void raise_illegal_opcode_error(const char* reason, const instr_value op_code);

	/**
	 * @brief Throws a memory access error (Out of bounds access, protected access error)
	 * @param reason Reason string for the error.
	 * @param value Memory location where the access happened.
	*/
	void raise_memory_access_error(const char* reason, const memptr value);
	
	/**
	 * @brief Throws a not implemented error.
	*/
	void not_yet_implemented();

	/**
	 * @brief Throws an unimplemented opcode error. This is not an illegal opcode, but an unsupported one.
	 * @param instruction Instruction value where the error happened.
	*/
	void unimplemented_opcode(const instr_value instruction);
	
	/**
	 * @brief Throws an error on illegal register access. (Out of bounds for register file, invalid id, ...)
	 * @param reason Reason string for the error.
	 * @param index Index of the requested register.
	 * @param regs_size Maximum index of available registers.
	*/
	void raise_register_access_error(const char* reason, const size_t index, const size_t regs_size);
	
	/**
	 * @brief Throws an error indicating a memory alignment issue.
	 * @param reason Reason string for the error.
	 * @param memory_idx Access index of memory used when the exception was triggered.
	*/
	void raise_memory_alignment_error(const char* reason, const size_t memory_idx);
	
	/**
	 * @brief Thrown if the VM enters a state that isn't implemented.
	 * @param state State of the VM that triggered the error.
	*/
	void raise_unimplemented_state_error(const VMExecutionState state);

	class IllegalInstructionException : public std::runtime_error
	{
	public:
		IllegalInstructionException(const char* reason);
		IllegalInstructionException(const std::string reason);
	};

	class MemoryAccessError : public std::runtime_error
	{
	public:
		MemoryAccessError(const char* reason);
		MemoryAccessError(const std::string reason);
	};

	class MemoryAlignmentError : public std::runtime_error
	{
	public:
		MemoryAlignmentError(const char* reason);
		MemoryAlignmentError(const std::string reason);
	};

	class RegisterAccessError : public std::runtime_error
	{
	public:
		RegisterAccessError(const char* reason);
		RegisterAccessError(const std::string reason);
	};
};
