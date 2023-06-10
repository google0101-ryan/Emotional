// (c) Copyright 2022-2023 Ryan Ilari
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <cstdint>
#include <emu/cpu/ee/x64/reg_alloc.h>
#include <3rdparty/xbyak/xbyak.h>
#include <emu/cpu/ee/EmotionEngine.h>
#include <vector>

namespace EE_JIT
{

struct Block;
struct IRInstruction;

class Emitter
{
private:
	Xbyak::CodeGenerator* cg;
	uint8_t* base;
	uint8_t* free_base;
	RegisterAllocator* reg_alloc;
	size_t sizeWithoutCurBlock = 0;

	void EmitSaveHostRegs();
	void EmitRestoreHostRegs();
	void EmitMov(IRInstruction i);
	void EmitPMFLO(IRInstruction i);
	void EmitPMFHI(IRInstruction i);
	void EmitMovCond(IRInstruction i);
	void EmitSLTI(IRInstruction i);
	void EmitBC(IRInstruction i);
	void EmitIncPC(IRInstruction i);
	void EmitOR(IRInstruction i);
	void EmitNOR(IRInstruction i);
	void EmitXOR(IRInstruction i);
	void EmitAND(IRInstruction i);
	void EmitJA(IRInstruction i);
	void EmitJumpImm(IRInstruction i);
	void EmitAdd(IRInstruction i);
	void EmitSub(IRInstruction i);
	void EmitMemoryStore(IRInstruction i);
	void EmitMemoryLoad(IRInstruction i);
	void EmitLDL(IRInstruction i);
	void EmitLDR(IRInstruction i);
	void EmitSDL(IRInstruction i);
	void EmitSDR(IRInstruction i);
	void EmitShift(IRInstruction i);
	void EmitShift64(IRInstruction i);
	void EmitMULT(IRInstruction i);
	void EmitDIV(IRInstruction i);
	void EmitUhOh(IRInstruction i);
	void EmitIR(IRInstruction i);
	void EmitMoveFromLo(IRInstruction i);
	void EmitMoveFromHi(IRInstruction i);
	void EmitBranchRegImm(IRInstruction i);
	void EmitUpdateCopCount(IRInstruction i);
	void EmitPOR(IRInstruction i);
	void EmitPADDUSW(IRInstruction i);
	void EmitDI(IRInstruction i);
	void EmitERET(IRInstruction i);
	void EmitSyscall(IRInstruction i);
	void EmitEI(IRInstruction i);
	void EmitPLZCW(IRInstruction i);
	void EmitMTLO(IRInstruction i);
	void EmitMTHI(IRInstruction i);
	void EmitPCPYLD(IRInstruction i);
	void EmitPSUBB(IRInstruction i);
	void EmitPADDSB(IRInstruction i);
	void EmitPNOR(IRInstruction i);
	void EmitPAND(IRInstruction i);
	void EmitPCPYUD(IRInstruction i);
	void EmitBC0T(IRInstruction i);
	void EmitBC0F(IRInstruction i);
	void EmitPCPYH(IRInstruction i);
	void EmitCFC2(IRInstruction i);
	void EmitCTC2(IRInstruction i);
	void EmitVISWR(IRInstruction i);
	void EmitQMFC2(IRInstruction i);
	void EmitQMTC2(IRInstruction i);
	void EmitVSUB(IRInstruction i);
	void EmitVSQI(IRInstruction i);
	void EmitVIADD(IRInstruction i);
	void EmitADDA(IRInstruction i);
	void EmitMADD(IRInstruction i);
	void EmitMADDS(IRInstruction i);
	void EmitCVTS(IRInstruction i);
	void EmitMULS(IRInstruction i);
	void EmitCVTW(IRInstruction i);
	void EmitDIVS(IRInstruction i);
	void EmitMOVS(IRInstruction i);
	void EmitADDS(IRInstruction i);
	void EmitSUBS(IRInstruction i);
	void EmitNEGS(IRInstruction i);
	void EmitLQC2(IRInstruction i);
	void EmitSQC2(IRInstruction i);
	void EmitVMULAX(IRInstruction i);
	void EmitVMADDAX(IRInstruction i);
	void EmitVMADDAY(IRInstruction i);
	void EmitVMADDAZ(IRInstruction i);
	void EmitVMADDW(IRInstruction i);
	void EmitVMADDZ(IRInstruction i);
	void EmitVMULAW(IRInstruction i);
	void EmitVCLIPW(IRInstruction i);
	void EmitCLES(IRInstruction i);
	void EmitCEQS(IRInstruction i);
	void EmitBC1F(IRInstruction i);
	void EmitBC1T(IRInstruction i);

	EE_JIT::JIT::EntryFunc dispatcher_entry;
public:
	Emitter();
	void Dump();
	void TranslateBlock(Block* block);

	void EnterDispatcher();

	void ResetFreeBase()
	{
		free_base = base;
		delete cg;
		cg = new Xbyak::CodeGenerator(0xffffffff, base);
	}

	uint8_t* GetFreeBase();
};

extern Emitter* emit;

}