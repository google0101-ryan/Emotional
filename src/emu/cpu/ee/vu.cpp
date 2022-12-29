#include <emu/cpu/ee/vu.h>
#include <app/Application.h>
#include <fstream>

#define _x(f) f&8
#define _y(f) f&4
#define _z(f) f&2
#define _w(f) f&1

#define _ft_ ((instr >> 16) & 0x1F)
#define _fs_ ((instr >> 11) & 0x1F)
#define _fd_ ((instr >> 6) & 0x1F)

#define _field ((instr >> 21) & 0xF)

#define _it_ (_ft_ & 0xF)
#define _is_ (_fs_ & 0xF)
#define _id_ (_fd_ & 0xF)

#define _fsf_ ((instr >> 21) & 0x3)
#define _ftf_ ((instr >> 23) & 0x3)

#define _bc_ (instr & 0x3)

#define _Imm11_		(int32_t)(instr & 0x400 ? 0xfffffc00 | (instr & 0x3ff) : instr & 0x3ff)
#define _UImm11_	(int32_t)(instr & 0x7ff)

float convert(uint32_t val)
{
    switch (val & 0x7f800000)
    {
    case 0:
        val &= 0x80000000;
        return *(float*)&val;
    case 0x7f800000:
    {
        uint32_t result = (val & 0x80000000)|0x7f7fffff;
        return *(float*)&result;
    }
    }
    return *(float*)&val;
}

void VectorUnit::special1(Opcode i)
{
    uint32_t instr = i.full;
    switch (i.r_type.func)
    {
	case 0x28:
	{
		for (int i = 0; i < 4; i++)
		{
			if (_field & (1 << (3 - i)))
			{
				float result = convert(gpr[_fs_].u[i]) + convert(gpr[_ft_].u[i]);
				SetGprF(_ft_, i, result);
			}
		}
		break;
	}
    case 0x2c:
    {
        //printf("vsub");
        for (int i = 0; i < 4; i++)
        {
            if (_field & (1 << (3 - i)))
            {
                float result = convert(gpr[_fs_].u[i]) - convert(gpr[_ft_].u[i]);
                gpr[_fd_].f[i] = result;
                //printf(" %d[%d], %d[%d], %d[%d]", _fd_, i, _fs_, i, _ft_, i);
            }
        }
        //printf("\n");
    }
    break;
    case 0x30:
        i_regs[_id_].u = i_regs[_is_].s + i_regs[_it_].s;
        //printf("viadd %d, %d, %d\n", _id_, _is_, _it_);
        break;
    case 0x3c:
    case 0x3d:
    case 0x3e:
    case 0x3f:
    {
        uint16_t op = (i.full & 0x3) | ((i.full >> 4) & 0x7C);
        switch (op)
        {
		case 0x31:
		{
			uint32_t x = gpr[_fs_].u[0];
			if (_x(_field))
				SetGprU(_ft_, 0, gpr[_fs_].u[1]);
			if (_y(_field))
				SetGprU(_ft_, 1, gpr[_fs_].u[2]);
			if (_z(_field))
				SetGprU(_ft_, 2, gpr[_fs_].u[3]);
			if (_w(_field))
				SetGprU(_ft_, 3, x);
			break;
		}
        case 0x35:
        {
            i_regs[_it_].u = _it_;
            uint32_t addr = (uint32_t)i_regs[_it_].u << 4;
            //printf("sqi 0x%08x\n", addr);
            for (int i = 0; i < 4; i++)
            {
                if (_field & (1 << (3 - i)))
                {
                    write_data<uint32_t>(addr + (i * 4), gpr[_fs_].u[i]);
                }
            }
            if (_it_)
                i_regs[_it_].u++;
        }
        break;
        case 0x3f:
        {
            i_regs[_it_].u = _is_;
            uint32_t addr = (uint32_t)i_regs[_is_].u << 4;
            //printf("vilwr 0x%08x ", addr);
            for (int i = 0; i < 4; i++)
            {
                if (_field & (1 << (3 - i)))
                {
                    uint32_t word = read_data<uint32_t>(addr + (i * 4));
                    //printf(" 0x%04x (0x%02x %d %d)", word, _field, _it_, _is_);
                    i_regs[_it_].u = word & 0xFFFF;
                    break;
                }
            }
            //printf("\n");
        }
        break;
        default:
            printf("[emu/CPU]: %s: Unknown cop2 special2 instruction 0x%08x (0x%02x)\n", __FUNCTION__, i.full, op);
            Application::Exit(1);
            break;
        }
        break;
    }
    default:
        printf("[emu/CPU]: %s: Unknown cop2 special1 instruction 0x%08x (0x%02x)\n", __FUNCTION__, i.full, i.r_type.func);
        Application::Exit(1);
    }
}

void VectorUnit::Dump()
{
    for (int i = 0; i < 32; i++)
    {
        printf("r%d:", i);
        for (int j = 0; j < 4; j++) printf(" %d -> %0.2f", j, convert(gpr[i].f[j]));
        printf("\n");
    }

	std::ofstream file("vu" + std::to_string(id) + std::string("_instr_mem.dump"));

	for (int i = 0; i < 0x4000; i++)
	{
		file << instr_mem[i];
	}

	file.close();
}