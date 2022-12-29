#include <cstddef> // size_t
#include <emu/Bus.h> // class Bus
#include <fstream> // ifstrem
#include <util/uint128.h>
#include "Bus.h"
#include <emu/cpu/EmotionEngine.h>

Bus::Bus(std::string fileName, std::string elf, bool& s)
{
	this->elf_name = elf;
    std::ifstream file(fileName, std::ios::binary | std::ios::ate);

    if (!file.is_open())
    {
        printf("[emu/Bus]: %s: Couldn't open file %s. Is it available?\n", __FUNCTION__, fileName.c_str());
        s = false;
        return;
    }

    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);

    if (size < 0x400000)
    {
        printf("[emu/Bus]: %s: Expected file of size 0x400000, got file of size 0x%06lx\n", __FUNCTION__, size);
        s = false;
        return;
    }

    file.read((char*)bios, size);

    s = true;
    
	console.open("log.txt");

    gif = new GIF();
    gs = new GraphicsSynthesizer();
    ee_timers = new EmotionTimers();
    ee_dmac = new EmotionDma(this);
    vu0 = new VectorUnit(0);
    vu1 = new VectorUnit(1);

    vif0 = new VectorInterface(vu0, 0);
    vif1 = new VectorInterface(vu1, 1);

    sif = new SubsystemInterface();

    iop_bus = new IopBus(bios, iop_ram, sif, this);
    iop = new IoProcessor(this);

	printf("[emu/Bus]: %s: Bus initialized\n", __FUNCTION__);
}

void Bus::LoadElf()
{
	struct Elf32_Ehdr 
    {
		uint8_t  e_ident[16];
		uint16_t e_type;
		uint16_t e_machine;
		uint32_t e_version;
		uint32_t e_entry;
		uint32_t e_phoff;
		uint32_t e_shoff;
		uint32_t e_flags;
		uint16_t e_ehsize;
        uint16_t e_phentsize;
        uint16_t e_phnum;
        uint16_t e_shentsize;
        uint16_t e_shnum;
        uint16_t e_shstrndx;
    };

    /* Program header */
    struct Elf32_Phdr 
    {
        uint32_t p_type;
        uint32_t p_offset;
        uint32_t p_vaddr;
        uint32_t p_paddr;
        uint32_t p_filesz;
        uint32_t p_memsz;
        uint32_t p_flags;
        uint32_t p_align;
    };

    std::ifstream reader;
    reader.open(elf_name, std::ios::in | std::ios::binary);

    if (reader.is_open())
    {
        reader.seekg(0, std::ios::end);
        int size = reader.tellg();
        uint8_t* buffer = new uint8_t[size];

        reader.seekg(0, std::ios::beg);
        reader.read((char*)buffer, size);
        reader.close();
            
        auto header = *(Elf32_Ehdr*)&buffer[0];
        printf("[CORE][ELF] Loading %s\n", elf_name.c_str());
        printf("Entry: 0x%x\n", header.e_entry);
        printf("Program header start: 0x%x\n", header.e_phoff);
        printf("Section header start: 0x%x\n", header.e_shoff);
        printf("Program header entries: %d\n", header.e_phnum);
        printf("Section header entries: %d\n", header.e_shnum);
        printf("Section header names index: %d\n", header.e_shstrndx);

        for (auto i = header.e_phoff; i < header.e_phoff + (header.e_phnum * 0x20); i += 0x20)
		{
			auto pheader = *(Elf32_Phdr*)&buffer[i];
			printf("\nProgram Header\n");
			printf("p_type: 0x%x\n", pheader.p_type);
			printf("p_offset: 0x%x\n", pheader.p_offset);
			printf("p_vaddr: 0x%x\n", pheader.p_vaddr);
			printf("p_paddr: 0x%x\n", pheader.p_paddr);
			printf("p_filesz: 0x%x\n", pheader.p_filesz);
			printf("p_memsz: 0x%x\n", pheader.p_memsz);

			int mem_w = pheader.p_paddr;
			for (auto file_w = pheader.p_offset; file_w < (pheader.p_offset + pheader.p_filesz); file_w += 4)
			{
				uint32_t word = *(uint32_t*)&buffer[file_w];
				write<uint32_t>(mem_w, word);
				mem_w += 4;
			}

			cpu->JumpToEntry(header.e_entry);
		}
	}
}

void Bus::Clock()
{
    ee_dmac->tick(16);
	vif0->tick(16);
	vif1->tick(16);
    gif->tick(16);

    iop->Clock(4);
    iop_bus->GetIopTimers()->tick(4);
    iop_bus->GetIopDma()->tick(4);
}

void Bus::TriggerDMAInterrupt()
{
	cpu->SetINT1();
}

void Bus::ClearDMAInterrupt()
{
	cpu->ClearINT1();
}

void Bus::Dump()
{
    std::ofstream file("iop_mem.dump");
    
    for (int i = 0; i < 0x200000; i++)
        file << iop_ram[i];
    
    file.flush();
    file.close();

    file.open("ee_mem.dump");

    for (int i = 0; i < 0x2000000; i++)
        file << ram[i];
    file.flush();
    file.close();

    file.open("spr_mem.dump");

    for (int i = 0; i < 0x4000; i++)
        file << scratchpad[i];
    file.flush();
    file.close();

	vu0->Dump();
	vu1->Dump();
}