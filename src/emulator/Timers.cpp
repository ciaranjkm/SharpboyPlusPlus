#include "Timers.h"
#include "Emulator.h"

Timers::Timers(std::shared_ptr<Emulator> emulator_ptr) {
	this->emulator_ptr = emulator_ptr;
	internal_div = 0x0000;
	if (this->emulator_ptr != nullptr) {
		initialised = true;
	}
	else {
		initialised = false;
	}
}

Timers::~Timers() {
	this->emulator_ptr.reset();
	this->emulator_ptr = nullptr;

	printf("[SB] Shutting down TIMERS object\n");
}

void Timers::reset_timers() {
	bool using_boot_rom = emulator_ptr->is_using_boot_rom();

	internal_div = 0x0000;
	tac = 0x00;
	tima = 0x00;
	tma = 0x00;

	if (using_boot_rom) {
		return;
	}

	internal_div = 0x18;

	return;
}

bool Timers::is_timers_initialised() {
	return initialised;
}

void Timers::timers_tick() {
	internal_div++;

	//select timer bit from tac
	int bit_selected = timer_input_bit(tac);
	bool div_bit_selected = (internal_div & (1 << bit_selected)) != 0x00;
	bool timer_enabled = (tac & 0x4) != 0x00;

	//find falling edge result
	bool and_result = div_bit_selected && timer_enabled;

	//inc timer if edge case
	if (previous_and_result && !and_result) {
		tima++;
		if (tima == 0x00) {
			reload_tima = true;
			tima_delay = DEFAULT_TIMA_DELAY;
		}

		previous_and_result = and_result;
	}

	if (reload_tima) {
		tima_delay--;

		//after 1 m cycle load tma into tma
		if (tima_delay == 4) {
			tima = tma;
		}

		//complete reload of tima and trigger interrupt, load with new tma incase of a new write on t cycle 2 of m cycle 2
		if (tima_delay == 2) {
			tima = tma;
			emulator_ptr->trigger_interrupt(int_TIMER);
		}

		//when tima delay is complete turn off tima reload
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
	case io_DIV: return internal_div >> 8;
	case io_TIMA: return tima;
	case io_TMA: return tima;
	case io_TAC: return tac;
	default: return 0xff;
	}
}

void Timers::io_instant_write(const byte& timer_io, const byte& value) {
	switch (timer_io) {
	case io_DIV: 
		internal_div = 0x0000; 
		return;

	case io_TMA: 
		tma = value; 
		return;

	case io_TIMA: 
		if (reload_tima) {
			if (tima_delay < 8 && tima_delay >= 4) {
				stop_tima_reload();
				written_tima = true;
				tima = value;
				return;
			}
			else if (tima_delay < 5 && tima_delay >= 0) {
				return;
			}
		}

		written_tima = true;
		tima = value;
		return;

	case io_TAC: 
		tac = value; 
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