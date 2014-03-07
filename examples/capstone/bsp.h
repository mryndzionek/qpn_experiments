#ifndef _bsp_h
#define _bsp_h
                                                          /* CPU clock [Hz] */
#define F_CPU                16000000UL

                                              /* Sys timer tick per seconds */
#define BSP_TICKS_PER_SEC    50
enum BSP_LedSignal {
    UP,DOWN
};

void BSP_init(void);
void BSP_signalLeds(enum BSP_LedSignal sig);
void BSP_progressBar(uint8_t progress, uint8_t maxprogress, uint8_t length);
uint8_t BSP_readADC(uint8_t channel);

#define BSP_showState(state_) ((void)0)

#endif                                                             /* bsp_h */

