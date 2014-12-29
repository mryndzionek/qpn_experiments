#ifndef bsp_h
#define bsp_h

#include <stdbool.h>

/* CPU clock [Hz] */
#define F_CPU                16000000UL

/* Sys timer tick per seconds */
#define BSP_TICKS_PER_SEC    (1000)

void BSP_init(void);
void BSP_binning(uint16_t par);
void BSP_decoding(uint16_t par);
bool BSP_convolution(void);
void BSP_dispLocking(void);
void BSP_dispSyncing(uint16_t data);
void BSP_dispDecoding(uint16_t data);
void BSP_dispClear(void);

#define BSP_showState(state_) ((void)0)

#endif                                                             /* bsp_h */
