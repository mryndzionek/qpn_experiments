#ifndef _bsp_h
#define _bsp_h
                                                          /* CPU clock [Hz] */
#define F_CPU                16000000UL

                                              /* Sys timer tick per seconds */
#define BSP_TICKS_PER_SEC    100

/* street signals ..........................................................*/
enum BSP_CarsSignal {
    CARS_RED, CARS_YELLOW, CARS_GREEN, CARS_BLANK
};
enum BSP_PedsSignal {
    PEDS_DONT_WALK, PEDS_BLANK, PEDS_WALK
};
void BSP_init(void);
void BSP_signalCars(enum BSP_CarsSignal sig);
void BSP_signalPeds(enum BSP_PedsSignal sig);

#define BSP_showState(state_) ((void)0)

#endif                                                             /* bsp_h */

