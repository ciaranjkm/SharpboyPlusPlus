#include "Timers.h"
#include "Emulator.h"

Timers::Timers(std::shared_ptr<Emulator> emulator) {
	initialised = true;

	if (emulator == nullptr) {
		return;
	}

	this->emulator = emulator;
	io.internal_div = 0x0000;
}

Timers::~Timers() {
	this->emulator.reset();
	this->emulator = nullptr;

	printf("[SB] Shutting down TIMERS object\n");
}

void Timers::reset_timers() {
	bool using_boot_rom = emulator->is_using_boot_rom();

	io.internal_div = 0x0000;
	io.tac = 0x00;
	io.tima = 0x00;
	io.tma = 0x00;

	if (using_boot_rom) {
		return;
	}

	io.internal_div = 0x18;

	return;
}

bool Timers::is_timers_initialised() {
	return initialised;
}

void Timers::timers_tick() {
	io.internal_div++;

	int bit_selected = timer_input_bit(io.tac);
	bool div_bit_selected = (io.internal_div & (1 << bit_selected)) != 0x00;
	bool timer_enabled = (io.tac & 0x4) != 0x00;

	bool and_result = div_bit_selected && timer_enabled;

	if (previous_and_result && !and_result) {
		io.tima++;
		if (io.tima == 0x00) {
			reload_tima = true;
			tima_delay = DEFAULT_TIMA_DELAY;
		}

		previous_and_result = and_result;
	}

	if (reload_tima) {
		tima_delay--;

		if (tima_delay == 4) {
			io.tima = io.tma;
		}

		if (tima_delay == 2) {
			io.tima = io.tma;
			emulator->trigger_interrupt(int_TIMER);
		}

		if (tima_delay == 0) {
			reload_tima = false;
			tima_delay = -1;
		}
	}

	previous_and_result = and_result;
}

void Timers::stop_tima_reload() {
	reload_tima = false;
	tima_delay = -1;
}

byte Timers::read_timer_io(const byte& timer_io) {
	switch (timer_io) {
	case io_DIV: return io.internal_div >> 8;
	case io_TIMA: return io.tima;
	case io_TMA: return io.tima;
	case io_TAC: return io.tac;
	default: return 0xff;
	}
}

void Timers::io_instant_write(const byte& timer_io, const byte& value) {
	switch (timer_io) {
	case io_DIV: 
		io.internal_div = 0x0000;
		return;

	case io_TMA: 
		io.tma = value;
		return;

	case io_TIMA: 
		if (reload_tima) {
			if (tima_delay < 8 && tima_delay >= 4) {
				stop_tima_reload();
				written_tima = true;
				io.tima = value;
				return;
			}
			else if (tima_delay < 5 && tima_delay >= 0) {
				return;
			}
		}

		written_tima = true;
		io.tima = value;
		return;

	case io_TAC: 
		io.tac = value;
		return;

	default: 
		return;
	}
}

bool Timers::tac_enabled(const byte& tac) {
	return tac & 0x04;
}

int Timers::timer_input_bit(const byte& tac) {
	switch (tac & 0x03) {
	case 0b00: return 9; // 4096 Hz
	case 0b01: return 3; // 262144 Hz
	case 0b10: return 5; // 65536 Hz
	case 0b11: return 7; // 16384 Hz
	}
	return 9; // default fallback
}