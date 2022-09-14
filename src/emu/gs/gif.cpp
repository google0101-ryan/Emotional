#include <emu/gs/gif.h>

void GIF::process_packed(uint128_t qword)
{
    int curr_reg = tag.nregs - reg_count;
    uint64_t regs = tag.regs;
    uint32_t desc = (regs >> 4 * curr_reg) & 0xf;

    switch (desc)
    {
    case 0:
        printf("PRIM: Write 0x%08lx\n", qword.u128 & 0x7ff);
        break;
    case 1:
        printf("rgba: %d, %d, %d, %d\n", qword.u128 & 0xFF, (qword.u128 >> 32) & 0xFF, (qword.u128 >> 64) & 0xFF, (qword.u128 >> 96) & 0xFF);
        break;
    case 2:
        printf("stq\n");
        break;
    case 3:
        printf("uv\n");
        break;
    case 4:
        printf("%s\n", (qword.u128 >> 111) & 1 ? "xyz3f" : "xyz2f");
        break;
    case 5:
        printf("%s\n", (qword.u128 >> 111) & 1 ? "xyz3" : "xyz2");
        break;
    case 10:
        printf("fog\n");
        break;
    case 14:
        printf("A+D\n");
        break;
    case 15:
        printf("GIF nop\n");
        break;
    }

    reg_count--;
}

void GIF::tick(int cycles)
{

    while (!fifo.empty() && cycles--)
    {
        if (!data_count)
        {
            tag = *(GIFTag*)&fifo.front();
            fifo.pop();

            data_count = tag.nloop;
            reg_count = tag.nregs;
        }
        else
        {
            uint128_t qword = fifo.front();
            fifo.pop();

            switch (tag.format)
            {
            case 0:
                process_packed(qword);
                if (!reg_count)
                {
                    data_count--;
                    reg_count = tag.nregs;
                }
                break;
            }
        }
    }
}