// (c) Copyright 2022-2023 Ryan Ilari
// This code is licensed under MIT license (see LICENSE for details)

#include "EmotionEngine.h"
#include "EEJit.h"
#include <emu/memory/Bus.h>


extern float convert(uint32_t);

namespace EmotionEngine
{
	ProcessorState state;
	bool can_dump = false;
	bool can_disassemble = false;
}

namespace EmotionEngine
{

void Reset()
{
#ifdef EE_JIT
	EEJit::Initialize();
#endif
	memset(&state, 0, sizeof(state));

	GetState()->pc = 0xBFC00000;
	GetState()->next_pc = 0xBFC00004;
	GetState()->cop0_regs[15] = 0x2E20;
}

int Clock(int cycles)
{
#ifdef EE_JIT
	GetState()->cop0_regs[9] += cycles;
	return EEJit::Clock(cycles);
#else
	#error TODO: EE Interpreter clock
#endif
}

bool IsBranch(uint32_t instr)
{
	uint8_t opcode = (instr >> 26) & 0x3F;

	switch (opcode)
	{
	case 0x00:
	{
		opcode = instr & 0x3F;
		switch (opcode)
		{
		case 0x08:
		case 0x09:
		case 0x0C:
			return true;
		default:
			return false;
		}
		break;
	}
	case 0x01:
	case 0x02:
	case 0x03:
	case 0x04:
	case 0x05:
	case 0x06:
	case 0x07:
	case 0x14:
	case 0x15:
		return true;
	case 0x10:
		opcode = (instr >> 21) & 0x3F;
		switch (opcode)
		{
		case 0x10:
		{
			opcode = instr & 0x3F;
			switch (opcode)
			{
			case 0x18:
				return true;
			default:
				return false;
			}
		}
		default:
			return false;
		}
		break;
	case 0x11:
		opcode = (instr >> 21) & 0x1F;
		switch (opcode)
		{
		case 0x8:
			return true;
		}
		break;
	default:
		return false;
	}

	return false;
}
#define BLOCKCACHE_ENABLE

void Dump()
{
	for (int i = 0; i < 32; i++)
		printf("%s\t->\t0x%lx%016lx\n", Reg(i), GetState()->regs[i].u64[1], GetState()->regs[i].u64[0]);
	printf("pc\t->\t0x%08x\n", GetState()->pc);
	printf("pc_at\t->\t0x%08x\n", GetState()->pc_at);
	printf("hi\t->\t0x%08lx\n", GetState()->hi);
	printf("lo\t->\t0x%08lx\n", GetState()->lo);
	printf("hi1\t->\t0x%08lx\n", GetState()->hi1);
	printf("lo1\t->\t0x%08lx\n", GetState()->lo1);
	printf("COP0:");
	for (int i = 0; i < 32; i++)
		printf("\tr%d\t->\t0x%08x\n", i, GetState()->cop0_regs[i]);
	for (int i = 0; i < 32; i++)
		printf("f%d\t->\t%f\n", i, convert(GetState()->fprs[i].i));
	
	printf("0x%08lx\n", offsetof(EmotionEngine::ProcessorState, cop0_regs[12]));
	
#ifdef EE_JIT
	EEJit::Dump();
#endif
}

ProcessorState* GetState()
{
	return &state;
}

void MarkDirty(uint32_t address, uint32_t size)
{
}

union COP0CAUSE
{
	uint32_t value;
	struct
	{
		uint32_t : 2;
		uint32_t excode : 5;	/* Exception Code */
		uint32_t : 3;
		uint32_t ip0_pending : 1;
		uint32_t ip1_pending : 1;
		uint32_t siop : 1;
		uint32_t : 2;
		uint32_t timer_ip_pending : 1;
		uint32_t exc2 : 3;
		uint32_t : 9;
		uint32_t ce : 2;
		uint32_t bd2 : 1;
		uint32_t bd : 1;
	};
};

union COP0Status
{
    uint32_t value;
    struct
    {
        uint32_t ie : 1; /* Interrupt Enable */
        uint32_t exl : 1; /* Exception Level */
        uint32_t erl : 1; /* Error Level */
        uint32_t ksu : 2; /* Kernel/Supervisor/User Mode bits */
        uint32_t : 5;
        uint32_t im0 : 1; /* Int[1:0] signals */
        uint32_t im1 : 1;
        uint32_t bem : 1; /* Bus Error Mask */
        uint32_t : 2;
        uint32_t im7 : 1; /* Internal timer interrupt  */
        uint32_t eie : 1; /* Enable IE */
        uint32_t edi : 1; /* EI/DI instruction Enable */
        uint32_t ch : 1; /* Cache Hit */
        uint32_t : 3;
        uint32_t bev : 1; /* Location of TLB refill */
        uint32_t dev : 1; /* Location of Performance counter */
        uint32_t : 2;
        uint32_t fr : 1; /* Additional floating point registers */
        uint32_t : 1;
        uint32_t cu : 4; /* Usability of each of the four coprocessors */
    };
};

void Exception(uint8_t code)
{
	static uint32_t exception_addr[2] = { 0x80000000, 0xBFC00200 };

	can_disassemble = false;

	if (code == 0x08)
	{
		int syscall_number = std::abs((int)EmotionEngine::GetState()->regs[3].u32[0]);
		if (syscall_number != 122)
			printf("Syscall %d\n", syscall_number);
		if (syscall_number == 0x02)
		{
			printf("SetGsCrt(%d, %d, %d)\n", EmotionEngine::GetState()->regs[4].u32[0], EmotionEngine::GetState()->regs[5].u32[0], EmotionEngine::GetState()->regs[6].u32[0]);
		}
		else if (syscall_number == 0x77)
		{
			printf("sceSifSetDma(0x%08x, %d)\n", EmotionEngine::GetState()->regs[4].u32[0], EmotionEngine::GetState()->regs[5].u32[0]);
		}
		else if (syscall_number == 0x20)
		{
			printf("CreateThread(0x%08x)\n", EmotionEngine::GetState()->regs[4].u32[0]);
		}
		else if (syscall_number == 0x2F)
		{
			printf("GetThreadID()\n");
		}
		else if (syscall_number == 0x3C)
		{
			printf("InitMainThread(0x%08x, 0x%08x, %d, 0x%08x)\n", EmotionEngine::GetState()->regs[4].u32[0], EmotionEngine::GetState()->regs[5].u32[0], EmotionEngine::GetState()->regs[6].u32[0], EmotionEngine::GetState()->regs[7].u32[0]);
		}
		else if (syscall_number == 0x3D)
		{
			printf("InitHeap(0x%08x, %d)\n", EmotionEngine::GetState()->regs[4].u32[0], EmotionEngine::GetState()->regs[5].u32[0]);
		}
		else if (syscall_number == 0x3E)
		{
			printf("EndOfHeap()\n");
		}
		else if (syscall_number == 0x40)
		{
			printf("CreateSema(0x%08x)\n", EmotionEngine::GetState()->regs[4].u32[0]);
		}
		else if (syscall_number == 0x42)
		{
			printf("SignalSema(%d)\n", EmotionEngine::GetState()->regs[4].u32[0]);
		}
		else if (syscall_number == 0x44)
		{
			printf("WaitSema(%d)\n", EmotionEngine::GetState()->regs[4].u32[0]);
		}
		else if (syscall_number == 0x64)
		{
			printf("FlushCache(%d)\n", EmotionEngine::GetState()->regs[4].u32[0]);
		}
		else if (syscall_number == 121)
		{
			printf("sceSifSetReg(0x%08x, 0x%08x)\n", EmotionEngine::GetState()->regs[4].u32[0], EmotionEngine::GetState()->regs[5].u32[0]);
		}
		else if (syscall_number == 5)
		{
			printf("_ExceptionEpilogue()\n");
		}
	}

	COP0CAUSE cause;
	COP0Status status;
	status.value = GetState()->cop0_regs[12];
	cause.value = GetState()->cop0_regs[13];

	cause.excode = code;

	uint32_t vector = 0x180;

	if (!status.exl)
	{
		GetState()->cop0_regs[14] = GetState()->pc-4;

		switch (code)
		{
		case 0:
			vector = 0x200;
			break;
		default:
			vector = 0x180;
			break;
		}

		status.exl = true;
	}

	GetState()->pc = exception_addr[status.bev] + vector;
	GetState()->next_pc = GetState()->pc + 4;

	GetState()->cop0_regs[12] = status.value;
	GetState()->cop0_regs[13] = cause.value;
}

void SetIp1Pending()
{
	COP0CAUSE cause;
	cause.value = EmotionEngine::GetState()->cop0_regs[13];
	cause.ip1_pending = true;
	EmotionEngine::GetState()->cop0_regs[13] = cause.value;
}

void ClearIp1Pending()
{
	COP0CAUSE cause;
	cause.value = EmotionEngine::GetState()->cop0_regs[13];
	cause.ip1_pending = false;
	EmotionEngine::GetState()->cop0_regs[13] = cause.value;
}

void SetIp0Pending()
{
	COP0CAUSE cause;
	cause.value = EmotionEngine::GetState()->cop0_regs[13];
	cause.ip0_pending = true;
	EmotionEngine::GetState()->cop0_regs[13] = cause.value;
}

}  // namespace EmotionEngine
