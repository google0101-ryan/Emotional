#include <emu/cpu/EmotionEngine.h>
#include "EmotionEngine.h"

EmotionEngine::COP0::COP0(uint8_t* RDRAM, uint8_t* BIOS, uint8_t* SPR)
{
	ram = RDRAM;
	bios = BIOS;
	spr = SPR;
}

void EmotionEngine::COP0::init_tlb()
{
	if (!kernel_vtlb)
		kernel_vtlb = new uint8_t*[1024 * 1024];
	if (!sup_vtlb)
		sup_vtlb = new uint8_t*[1024 * 1024];
	if (!user_vtlb)
		user_vtlb = new uint8_t*[1024 * 1024];
	if (!vtlb_info)
		vtlb_info = new VTLB_Info[1024 * 1024];
	
	memset(kernel_vtlb, 0, 1024 * 1024 * sizeof(uint8_t*));
    memset(sup_vtlb, 0, 1024 * 1024 * sizeof(uint8_t*));
    memset(user_vtlb, 0, 1024 * 1024 * sizeof(uint8_t*));
    memset(vtlb_info, 0, 1024 * 1024 * sizeof(VTLB_Info));

	uint32_t unmapped_start = 0x80000000, unmapped_end = 0xC0000000;
	for (uint32_t i = unmapped_start; i < unmapped_end; i += 4096)
	{
		int map_index = i / 4096;
		kernel_vtlb[map_index] = get_mem_pointer(i & 0x1FFFFFFF);
		if (i < 0xA0000000)
			vtlb_info[map_index].cache_mode = CACHED;
		else
			vtlb_info[map_index].cache_mode = UNCACHED;
	}
}
uint8_t **EmotionEngine::COP0::get_vtlb_map()
{
	return kernel_vtlb;
}

void EmotionEngine::COP0::set_tlb_modified(size_t page)
{
	vtlb_info[page].modified = true;
}

void EmotionEngine::COP0::set_tlb(int index)
{
	TLB_Entry* new_entry = &tlb[index];

	unmap_tlb(new_entry);

	new_entry->is_spr = cop0_regs[2] >> 31;

	new_entry->page_mask = (cop0_regs[5] >> 13) & 0xFFF;
	new_entry->page_shift = 0;
	switch (new_entry->page_mask)
	{
	case 0x000:
		new_entry->page_size = 1024 * 4;
        new_entry->page_shift = 0;
        break;
    case 0x003:
        new_entry->page_size = 1024 * 16;
        new_entry->page_shift = 2;
        break;
    case 0x00F:
        new_entry->page_size = 1024 * 64;
        new_entry->page_shift = 4;
        break;
    case 0x03F:
        new_entry->page_size = 1024 * 256;
        new_entry->page_shift = 6;
        break;
    case 0x0FF:
        new_entry->page_size = 1024 * 1024;
        new_entry->page_shift = 8;
        break;
    case 0x3FF:
        new_entry->page_size = 1024 * 1024 * 4;
        new_entry->page_shift = 10;
        break;
    case 0xFFF:
        new_entry->page_size = 1024 * 1024 * 16;
        new_entry->page_shift = 12;
        break;
    default:
        new_entry->page_size = 0;
	}

	new_entry->asid = cop0_regs[10] & 0xFF;
	new_entry->vpn2 = cop0_regs[10] >> 13;

	new_entry->global = true;

	for (int i = 0; i < 2; i++)
	{
		uint32_t entry_lo = cop0_regs[i + 2];

		new_entry->global &= entry_lo & 0x1;
		new_entry->valid[i] = (entry_lo >> 1) & 0x1;
		new_entry->dirty[i] = (entry_lo >> 2) & 0x1;
		new_entry->cache_mode[i] = (entry_lo >> 3) & 0x7;
		new_entry->pfn[i] = (entry_lo >> 6) & 0xFFFFF;
	}

	uint32_t real_virt = new_entry->vpn2 * 2;
	real_virt >>= new_entry->page_shift;
	real_virt *= new_entry->page_size;

	for (int i = 0; i < 2; i++)
	{
		uint32_t real_phy = new_entry->pfn[i] >> new_entry->page_shift;
		real_phy *= new_entry->page_size;
	}

	map_tlb(new_entry);
}

bool EmotionEngine::COP0::get_condition(Bus * bus)
{
	uint32_t stat = bus->read<uint32_t>(0x1000E010) & 0x3FF;
	uint32_t pcr = bus->read<uint32_t>(0x1000E020) & 0x3FF;
	return ((~pcr | stat) & 0x3FF) == 0x3FF;
}

void EmotionEngine::COP0::unmap_tlb(TLB_Entry* entry)
{
	uint32_t real_vpn = entry->vpn2 * 2;
	real_vpn >>= entry->page_shift;

	uint32_t even_page = (real_vpn * entry->page_size) / 4096;
	uint32_t odd_page = ((real_vpn + 1) * entry->page_size) / 4096;

	if (entry->is_spr)
	{
		if (entry->valid[0])
		{
			for (uint32_t i = 0; i < 1024*16; i += 4096)
			{
				int map_index = i / 4096;
				kernel_vtlb[even_page + map_index] = nullptr;
				sup_vtlb[even_page + map_index] = nullptr;
				user_vtlb[even_page + map_index] = nullptr;
			}
		}
	}
	else
	{
		if (entry->valid[0])
        {
            for (uint32_t i = 0; i < entry->page_size; i += 4096)
            {
                int map_index = i / 4096;
                kernel_vtlb[even_page + map_index] = nullptr;
                sup_vtlb[even_page + map_index] = nullptr;
                user_vtlb[even_page + map_index] = nullptr;
            }
        }

        if (entry->valid[1])
        {
            for (uint32_t i = 0; i < entry->page_size; i += 4096)
            {
                int map_index = i / 4096;
                kernel_vtlb[odd_page + map_index] = nullptr;
                sup_vtlb[odd_page + map_index] = nullptr;
                user_vtlb[odd_page + map_index] = nullptr;
            }
        }
	}
}

void EmotionEngine::COP0::map_tlb(TLB_Entry *entry)
{
	uint32_t real_vpn = entry->vpn2 * 2;
    real_vpn >>= entry->page_shift;

    uint32_t even_virt_page = (real_vpn * entry->page_size) / 4096;
    uint32_t even_virt_addr = even_virt_page * 4096;
    uint32_t even_phy_addr = (entry->pfn[0] >> entry->page_shift) * entry->page_size;
    uint32_t odd_virt_page = ((real_vpn + 1) * entry->page_size) / 4096;
    uint32_t odd_virt_addr = odd_virt_page * 4096;
    uint32_t odd_phy_addr = (entry->pfn[1] >> entry->page_shift) * entry->page_size;

    if (entry->is_spr)
    {
        if (entry->valid[0])
        {
            for (uint32_t i = 0; i < 1024 * 16; i += 4096)
            {
                int map_index = i / 4096;
                kernel_vtlb[even_virt_page + map_index] = spr + i;
                sup_vtlb[even_virt_page + map_index] = spr + i;
                user_vtlb[even_virt_page + map_index] = spr + i;
                vtlb_info[even_virt_page + map_index].cache_mode = SPR;
            }
        }
    }
    else
    {
        if (entry->valid[0])
        {
			for (uint32_t i = 0; i < entry->page_size; i += 4096)
            {
                int map_index = i / 4096;
                uint8_t* mem = get_mem_pointer(even_phy_addr + i);
                kernel_vtlb[even_virt_page + map_index] = mem;

				
                if (even_virt_addr < 0x80000000)
                {
                    sup_vtlb[even_virt_page + map_index] = mem;
                    user_vtlb[even_virt_page + map_index] = mem;
                }
                else if (even_virt_addr >= 0xC0000000 && even_virt_addr < 0xE0000000)
                    sup_vtlb[even_virt_page + map_index] = mem;

                vtlb_info[even_virt_page + map_index].cache_mode = entry->cache_mode[0];
            }
        }

        if (entry->valid[1])
        {
			for (uint32_t i = 0; i < entry->page_size; i += 4096)
            {
                int map_index = i / 4096;
                uint8_t* mem = get_mem_pointer(odd_phy_addr + i);
                kernel_vtlb[odd_virt_page + map_index] = mem;

                if (odd_virt_addr < 0x80000000)
                {
                    sup_vtlb[odd_virt_page + map_index] = mem;
                    user_vtlb[odd_virt_page + map_index] = mem;
                }
                else if (odd_virt_addr >= 0xC0000000 && odd_virt_addr < 0xE0000000)
                    sup_vtlb[odd_virt_page + map_index] = mem;

                vtlb_info[odd_virt_page + map_index].cache_mode = entry->cache_mode[1];
            }
        }
    }
}

uint8_t* EmotionEngine::COP0::get_mem_pointer(uint32_t paddr)
{
	if (paddr < 0x10000000)
	{
		paddr &= (1024 * 1024 * 32) - 1;
		return ram + paddr;
	}
	
	if (paddr >= 0x1FC00000 && paddr < 0x20000000)
	{
		paddr &= (1024 * 1024 * 4) - 1;
		return bios + paddr;
	}

	return (uint8_t*)1;
}
