#pragma once

#include <cstdint>
#include <cstdio>
#include <cstdlib>

class CDVD
{
private:
    uint8_t n_status = 0x40;
	uint8_t s_status = 0x40;
	uint8_t S_command;
	uint8_t S_command_params[16];
	uint8_t S_outdata[16];
	uint8_t S_params;
	uint8_t S_out_params;

	void handle_command(uint8_t command)
	{
		s_status &= ~0x40;
		S_command = command;
		switch (command)
		{
		case 0x15:
			prepare_S_outdata(1);
			S_outdata[0] = 5;
			break;
		case 0x40:
			prepare_S_outdata(1);
			S_outdata[0] = 0;
			break;
		case 0x41:
			prepare_S_outdata(16);
			for (int i = 0; i < 16; i++)
				S_outdata[i] = 0;
			break;
		case 0x43:
			prepare_S_outdata(1);
			S_outdata[0] = 0;
			break;
		default:
			printf("Unimplemented S command 0x%02x\n", command);
			exit(1);
		}
	}

	void prepare_S_outdata(int amount)
	{
		if (amount > 16)
		{
			printf("That's too many numbers\n");
			exit(1);
		}
		S_out_params = amount;
		s_status &= 0x40;
		S_params = 0;
	}
public:
	void write(uint32_t addr, uint32_t data)
	{
        switch (addr)
        {
		case 0x1f402016:
			handle_command(data);
			break;
		case 0x1f402017:
			S_command_params[S_params] = data;
			S_params++;
			break;
        default:
            printf("[emu/CDVD]: %s: Write to unknown address 0x%08x\n", __FUNCTION__, addr);
            exit(1);
		}
	}

    uint32_t read(uint32_t addr)
    {
        switch (addr)
        {
        case 0x1f402005:
            return n_status;
		case 0x1f402018:
		{
			if (S_out_params <= 0)
				return 0;
			uint8_t value = S_outdata[S_params];
			S_params++;
			S_out_params--;
			if (S_out_params == 0)
			{
				s_status |= 0x40;
				S_params = 0;
			}
			return value;
		}
		case 0x1f402016:
			return S_command;
		case 0x1f402017:
			return s_status;
        default:
            printf("[emu/CDVD]: %s: Read from unknown address 0x%08x\n", __FUNCTION__, addr);
            exit(1);
        }
    }
};