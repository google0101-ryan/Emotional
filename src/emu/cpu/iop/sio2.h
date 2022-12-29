#pragma once

#include <cstdint>
#include <queue>

class IopBus;

class SIO2
{
private:
	union
	{
		uint32_t data;
		struct
		{
			uint32_t start : 1,
			resume : 1,
			reset : 1,
			reset_fifo : 1,
			timeout_enable : 1,
			error_proceed : 1,
			: 1,
			unknown : 1,
			error_irq_enable : 1,
			tx_irq_enable : 1,
			: 2,
			: 18,
			ps1 : 1,
			dir : 1;
		};
	} sio2_ctrl;

	uint32_t send1[4];
	uint32_t send2[4];
	uint32_t send3[16];

	struct SIO2Command
	{
		/* Index in SEND3 array */
		uint32_t index;
		uint32_t size;
		bool running = false;
	};

	SIO2Command command = {};
	enum class SIO2Peripheral
	{
		None = 0x0,
		Controller = 0x1,
		Multitap = 0x21,
		Infrared = 0x61,
		MemCard = 0x81
	};

	SIO2Peripheral cur_dev;
	bool just_started = false;

	std::queue<uint8_t> fifo;
	uint32_t recv1;

	IopBus* bus;

	void process_command(uint8_t command);
public:
	SIO2(IopBus* iop);

	uint32_t read(uint32_t addr);
	void write(uint32_t addr, uint32_t data);

	uint32_t read_serial()
	{
		uint8_t value = fifo.front();
		fifo.pop();
		return value;
	}

	void set_send1(int index, uint32_t value);
	void set_send2(int index, uint32_t value);
	void set_send3(int index, uint32_t value);
};