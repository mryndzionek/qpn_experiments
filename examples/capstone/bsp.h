#ifndef _bsp_h
#define _bsp_h
                                                          /* CPU clock [Hz] */
#define F_CPU                16000000UL

                                              /* Sys timer tick per seconds */
#define BSP_TICKS_PER_SEC    50
enum BSP_LedSignal {
    UP,DOWN
};

void BSP_signalLeds(enum BSP_LedSignal sig);

#define BSP_showState(state_) ((void)0)

#endif                                                             /* bsp_h */

