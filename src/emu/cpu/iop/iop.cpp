#include <emu/cpu/iop/iop.h>
#include <emu/Bus.h>
#include <app/Application.h>

IoProcessor::IoProcessor(Bus *bus)
    : bus(bus)
{
    memset(regs, 0, sizeof(regs));
    Cop0 = {};

    Cop0.regs[15] = 0x1f;

    pc = 0xbfc00000;
    next_pc = pc + 4;
}

void IoProcessor::Clock(int cycles)
{
    for (int cycle = cycles; cycle > 0; cycle--)
    {
        if (singleStep)
            getc(stdin);
        if (pc == 0x12C48 || pc == 0x1420C || pc == 0x1430C)
            bus->IopPrint(regs[5], regs[6]);

        Opcode instr;
        instr.full = bus->read_iop<uint32_t>(pc);

        AdvancePC();

        if (instr.full == 0)
        {
            //printf("nop\n");
            goto handle_load_delay;
        }

        switch (instr.r_type.opcode)
        {
        case 0x00:
        {
            switch (instr.r_type.func)
            {
            case 0x00:
                sll(instr);
                break;
            case 0x02:
                srl(instr);
                break;
            case 0x03:
                sra(instr);
                break;
            case 0x04:
                sllv(instr);
                break;
            case 0x06:
                srlv(instr);
                break;
            case 0x08:
                jr(instr);
                break;
            case 0x09:
                jalr(instr);
                break;
            case 0x0C:
                syscall(instr);
                break;
            case 0x10:
                mfhi(instr);
                break;
            case 0x11:
                mthi(instr);
                break;
            case 0x12:
                mflo(instr);
                break;
            case 0x13:
                mtlo(instr);
                break;
            case 0x18:
                mult(instr);
                break;
            case 0x19:
                multu(instr);
                break;
            case 0x1B:
                divu(instr);
                break;
            case 0x20:
                add(instr);
                break;
            case 0x21:
                addu(instr);
                break;
            case 0x23:
                subu(instr);
                break;
            case 0x24:
                op_and(instr);
                break;
            case 0x25:
                op_or(instr);
                break;
            case 0x26:
                op_xor(instr);
                break;
            case 0x27:
                op_nor(instr);
                break;
            case 0x2A:
                slt(instr);
                break;
            case 0x2B:
                sltu(instr);
                break;
            default:
                printf("[emu/IOP]: %s: Unknown special 0x%08x (0x%02x)\n", __FUNCTION__, instr.full, instr.r_type.func);
                Application::Exit(1);
            }
        }
        break;
        case 0x01:
            bcond(instr);
            break;
        case 0x02:
            j(instr);
            break;
        case 0x03:
            jal(instr);
            break;
        case 0x04:
            beq(instr);
            break;
        case 0x05:
            bne(instr);
            break;
        case 0x06:
            blez(instr);
            break;
        case 0x07:
            bgtz(instr);
            break;
        case 0x08:
            addi(instr);
            break;
        case 0x09:
            addiu(instr);
            break;
        case 0x0A:
            slti(instr);
            break;
        case 0x0B:
            sltiu(instr);
            break;
        case 0x0C:
            andi(instr);
            break;
        case 0x0D:
            ori(instr);
            break;
        case 0x0F:
            lui(instr);
            break;
        case 0x10:
            cop0(instr);
            break;
        case 0x20:
            lb(instr);
            break;
        case 0x21:
            lh(instr);
            break;
        case 0x23:
            lw(instr);
            break;
        case 0x24:
            lbu(instr);
            break;
        case 0x25:
            lhu(instr);
            break;
        case 0x28:
            sb(instr);
            break;
        case 0x29:
            sh(instr);
            break;
        case 0x2B:
            sw(instr);
            break;
        default:
            printf("[emu/IOP]: %s: Unknown instruction 0x%08x (0x%02x)\n", __FUNCTION__, instr.full, instr.r_type.opcode);
            Application::Exit(1);
        }

handle_load_delay:
        regs[load_delay.reg] = load_delay.data;
        load_delay = next_load_delay;
        next_load_delay.reg = 0;

        regs[write_back.reg] = write_back.data;
        write_back.reg = 0;
        regs[0] = 0;

        if (singleStep)
            Dump();
    }

    if (IntPending())
    {
        printf("Found interrupt, might want to handle that\n");
        exit(1);
    }
}

void IoProcessor::AdvancePC()
{
    pc = next_pc;
    next_pc += 4;
}

void IoProcessor::Dump()
{
    for (int i = 0; i < 32; i++)
        printf("[emu/IOP]: %s: %s\t->\t0x%08x\n", __FUNCTION__, Reg(i), regs[i]);
    printf("[emu/IOP]: %s: pc\t->\t0x%08x\n", __FUNCTION__, pc - 4);
}

bool IoProcessor::IntPending()
{
    IopBus* iopBus = bus->GetBus();
    bool pending = iopBus->GetICtrl() && (iopBus->GetIStat() & iopBus->GetIMask());
    Cop0.cause.IP = (Cop0.cause.IP & ~0x4) | (pending << 2);

    bool enabled = Cop0.status.IEc && (Cop0.status.Im & Cop0.cause.IP);
    return pending && enabled;
}