#pragma once

#include <cstdint>

class IopBus;

class Timer
{
private:
    union TimerMode 
	{
		uint64_t value;
		struct 
		{
			uint64_t gate_enable : 1;
			uint64_t gate_mode : 2;
			uint64_t reset_on_intr : 1;
			uint64_t compare_intr : 1;
			uint64_t overflow_intr : 1;
			uint64_t repeat_intr : 1; /* if unset, bit 10 is set to 0 after interrupt occurs. */
			uint64_t levl : 1;
			uint64_t external_signal : 1;
			uint64_t tm2_prescaler : 1;
		
	        uint64_t intr_enabled : 1;
			uint64_t compare_intr_raised : 1;
			uint64_t overflow_intr_raised : 1;
			uint64_t tm4_prescalar : 1;
			uint64_t tm5_prescalar : 1;
			uint64_t : 49;
		};
	};

	struct
    {
        uint64_t counter;
        TimerMode mode;
        uint64_t target;
    } timers[6];

	IopBus* bus;
public:
	Timer(IopBus* bus)
	: bus(bus) {}

    void tick(uint32_t cycles);

    uint32_t read(uint32_t addr);
    void write(uint32_t addr, uint32_t data);
};