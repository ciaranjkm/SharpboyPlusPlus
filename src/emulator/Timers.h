#pragma once

#include "_definitions.h"
#include <memory>

class Emulator;

class Timers {
public:
	Timers(std::shared_ptr<Emulator> emulator_ptr);
	~Timers();

	void reset_timers();

	bool is_timers_initialised();
	void timers_tick();

	void stop_tima_reload();

	byte read_timer_io(const byte& timer_io);
	void write_timer_io(const byte& timer_io, const byte& value);

private:
	std::shared_ptr<Emulator> emulator_ptr;
	bool initialised;

	ushort internal_div = 0x0000;
	byte tac = 0x00;
	byte tima = 0x00;
	byte tma = 0x00;

	bool previous_and_result = false;
	bool reload_tima = false;
	int tima_delay = 0;
	bool written_tima = false;

	const int DEFAULT_TIMA_DELAY = 8;
private:
	bool tac_enabled(const byte& tac);
	int timer_input_bit(const byte& tac);
};