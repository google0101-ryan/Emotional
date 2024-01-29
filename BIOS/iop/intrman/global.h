#pragma once

extern int CpuSuspendIntr(int* state);
extern int RegisterIntrHandler(int irq, int mode, int (*func)(void*), void* arg);
extern int EnableIntr(int irq);
extern int CpuResumeIntr(int state);
extern int CpuEnableIntr();