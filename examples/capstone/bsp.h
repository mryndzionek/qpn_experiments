#ifndef _bsp_h
#define _bsp_h
                                                          /* CPU clock [Hz] */
#define F_CPU                16000000UL

#define BSP_TICKS_PER_SEC    40

void BSP_init(void);
void BSP_ledOn(uint8_t led_num);
void BSP_ledOff(uint8_t led_num);
void BSP_lcdStr(uint8_t x, uint8_t y, char const *str);
void BSP_progressBar(uint8_t x, uint8_t y, uint8_t progress,
                     uint8_t maxprogress, uint8_t length);
uint32_t BSP_get_ticks(void);
void BSP_ADCstart(void);
#define BSP_showState(state_) ((void)0)

#endif                                                             /* bsp_h */

