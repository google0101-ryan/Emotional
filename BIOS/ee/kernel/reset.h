#pragma once

#define RESET_DMAC 1
#define RESET_VU1 2
#define RESET_VIF 4
#define RESET_GIF 8
#define RESET_VU0 16
#define RESET_IPU 64

#define RESET_ALL (RESET_DMAC | RESET_VU1 | RESET_VIF | RESET_GIF | RESET_VU0 | RESET_IPU)