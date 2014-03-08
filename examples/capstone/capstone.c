/*****************************************************************************
* Model: capstone.qm
* File:  ./capstone.c
*
* This code has been generated by QM tool (see state-machine.com/qm).
* DO NOT EDIT THIS FILE MANUALLY. All your changes will be lost.
*
* This program is open source software: you can redistribute it and/or
* modify it under the terms of the GNU General Public License as published
* by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
* or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
* for more details.
*****************************************************************************/
/* @(/2/1) .................................................................*/
#include "qpn_port.h"
#include "bsp.h"
#include "capstone.h"
#include "alarm.h"

#ifndef NDEBUG
Q_DEFINE_THIS_FILE
#endif

/* Pelican class declaration -----------------------------------------------*/
/* @(/1/0) .................................................................*/
typedef struct CapstoneTag {
/* protected: */
    QActive super;

/* private: */
    uint8_t heartbeat_led_sel;
    int32_t depth_in_mm;
    char depth_units[2];
    uint8_t dt_tts_sel;
    int32_t ascent_rate_in_mm_per_sec;
    uint32_t start_dive_time_in_ticks;
    uint32_t dive_time_in_ticks;
    uint32_t tts_in_ticks;
    uint32_t gas_in_cylinder_in_cl;
    uint32_t consumed_gas_in_cl;
} Capstone;

/* private: */
static void Capstone_display_depth(Capstone * const me);
static void Capstone_display_pressure(Capstone * const me);
static void Capstone_display_assent(Capstone * const me);

/* protected: */
static QState Capstone_initial(Capstone * const me);
static QState Capstone_always(Capstone * const me);
static QState Capstone_surfaced(Capstone * const me);
static QState Capstone_adding_gas(Capstone * const me);
static QState Capstone_diving(Capstone * const me);


/* Global objects ----------------------------------------------------------*/
Capstone AO_Capstone;

/* Capstone class definition -----------------------------------------------*/
/* @(/1/4) .................................................................*/
void Capstone_ctor(void) {
    QActive_ctor(&AO_Capstone.super, Q_STATE_CAST(&Capstone_initial));
}
/* @(/1/0) .................................................................*/
/* @(/1/0/10) ..............................................................*/
static void Capstone_display_depth(Capstone * const me) {
    int32_t displayed_depth;

    if (me->depth_units[0] == 'm') {                        /* show meters? */
        displayed_depth = (me->depth_in_mm + 1000/2) / 1000;
    }
    else {                                                     /* show feet */
        displayed_depth = (me->depth_in_mm * 328 + 100000/2) / 100000;
    }

    if (displayed_depth > 999) {             /* clamp the depth to 3 digits */
        displayed_depth = 999;
    }

    BSP_lcdStr(LCD_DEPTH_X + 4,  LCD_DEPTH_Y, bin2dec3(displayed_depth));
    BSP_lcdStr(LCD_DEPTH_UNITS_X, LCD_DEPTH_Y, me->depth_units);
}
/* @(/1/0/11) ..............................................................*/
static void Capstone_display_pressure(Capstone * const me) {
    uint32_t cylinder_pressure_in_bar =
             1 + (me->gas_in_cylinder_in_cl / CYLINDER_VOLUME_IN_CL);
    BSP_progressBar(LCD_CP_X, LCD_CP_Y,
            (cylinder_pressure_in_bar * LCD_PRESSURE_LIMIT
                 / FULL_SCALE_CYLINDER_PRESSURE),
            LCD_PRESSURE_LIMIT, PROGRESS_BAR_LEN);
}
/* @(/1/0/12) ..............................................................*/
static void Capstone_display_assent(Capstone * const me) {
    if (me->ascent_rate_in_mm_per_sec > 0) {                  /* ascending? */
            BSP_progressBar(LCD_AR_X, LCD_AR_Y,
                (me->ascent_rate_in_mm_per_sec * LCD_ASCENT_RATE_LIMIT)
                    / FULL_SCALE_ASCENT_RATE,
                LCD_ASCENT_RATE_LIMIT, PROGRESS_BAR_LEN);
    }
    else {                          /* descending--show empty ascending bar */
            BSP_progressBar(LCD_AR_X, LCD_AR_Y, 0, LCD_ASCENT_RATE_LIMIT,
                PROGRESS_BAR_LEN);
    }
}
/* @(/1/0/13) ..............................................................*/
/* @(/1/0/13/0) */
static QState Capstone_initial(Capstone * const me) {
    me->depth_units[0]        = 'm';                              /* meters */
    me->depth_units[1]        = '\0';                     /* zero terminate */
    me->gas_in_cylinder_in_cl = 0;
    me->heartbeat_led_sel = 0;
    me->dt_tts_sel            = 0;

    BSP_lcdStr(LCD_DEPTH_X, LCD_DEPTH_Y, "Dpt");
    return Q_TRAN(&Capstone_surfaced);
}
/* @(/1/0/13/1) ............................................................*/
static QState Capstone_always(Capstone * const me) {
    QState status_;
    switch (Q_SIG(me)) {
        /* @(/1/0/13/1/0) */
        case HEARTBEAT_SIG: {
            BSP_ADCstart();
            BSP_ledOn(ADC_LED);
            if (me->heartbeat_led_sel) {
                BSP_ledOn(HEARTBEAT_LED);
            }
            else {
                BSP_ledOff(HEARTBEAT_LED);
            }
            me->heartbeat_led_sel = !me->heartbeat_led_sel;
            QActive_arm((QActive *)me, HEARTBEAT_TOUT);
            status_ = Q_HANDLED();
            break;
        }
        /* @(/1/0/13/1/1) */
        case DT_TTS_SIG: {
            if (me->dt_tts_sel) {
                BSP_lcdStr(LCD_TTS_X, LCD_TTS_Y, "TTS");
                BSP_lcdStr(LCD_TTS_X + 3, LCD_TTS_Y,
                    ticks2min_sec(me->tts_in_ticks));
            }
            else {
                BSP_lcdStr(LCD_TTS_X, LCD_TTS_Y, "Div");
                BSP_lcdStr(LCD_TTS_X + 3, LCD_TTS_Y,
                    ticks2min_sec(me->dive_time_in_ticks));
            }
            me->dt_tts_sel = !me->dt_tts_sel;
            status_ = Q_HANDLED();
            break;
        }
        /* @(/1/0/13/1/2) */
        case BTN2_DOWN_SIG: {
            if (me->depth_units[0] == 'm') {
                me->depth_units[0] = 'f';
            }
            else {
                me->depth_units[0] = 'm';
            }
            Capstone_display_depth(me);
            status_ = Q_HANDLED();
            break;
        }
        default: {
            status_ = Q_SUPER(&QHsm_top);
            break;
        }
    }
    return status_;
}
/* @(/1/0/13/1/3) ..........................................................*/
static QState Capstone_surfaced(Capstone * const me) {
    QState status_;
    switch (Q_SIG(me)) {
        /* @(/1/0/13/1/3) */
        case Q_ENTRY_SIG: {
            BSP_ledOn(SURFACE_LED);
            me->depth_in_mm = 0;
            Capstone_display_depth(me);
            me->dive_time_in_ticks = 0;
            me->tts_in_ticks = 0;
            me->ascent_rate_in_mm_per_sec = 0;
            Capstone_display_assent(me);
            status_ = Q_HANDLED();
            break;
        }
        /* @(/1/0/13/1/3) */
        case Q_EXIT_SIG: {
            BSP_ledOff(SURFACE_LED);
            status_ = Q_HANDLED();
            break;
        }
        /* @(/1/0/13/1/3/0) */
        case BTN1_UP_SIG: {
            Capstone_display_pressure(me);
            status_ = Q_HANDLED();
            break;
        }
        /* @(/1/0/13/1/3/1) */
        case BTN1_DOWN_SIG: {
            status_ = Q_TRAN(&Capstone_adding_gas);
            break;
        }
        /* @(/1/0/13/1/3/2) */
        case ASCENT_RATE_ADC_SIG: {
            BSP_ledOff(ADC_LED);

            me->ascent_rate_in_mm_per_sec =
                ASCENT_RATE_MM_PER_MIN((uint16_t)Q_PAR(me));

            me->depth_in_mm -=
                depth_change_in_mm(me->ascent_rate_in_mm_per_sec);

            /* @(/1/0/13/1/3/2/0) */
            if (me->ascent_rate_in_mm_per_sec >= 0) {
                me->ascent_rate_in_mm_per_sec = 0;
                status_ = Q_HANDLED();
            }
            /* @(/1/0/13/1/3/2/1) */
            else {
                status_ = Q_TRAN(&Capstone_diving);
            }
            break;
        }
        default: {
            status_ = Q_SUPER(&Capstone_always);
            break;
        }
    }
    return status_;
}
/* @(/1/0/13/1/3/3) ........................................................*/
static QState Capstone_adding_gas(Capstone * const me) {
    QState status_;
    switch (Q_SIG(me)) {
        /* @(/1/0/13/1/3/3) */
        case Q_ENTRY_SIG: {
            QActive_arm((QActive *)me, BSP_TICKS_PER_SEC/10);
            status_ = Q_HANDLED();
            break;
        }
        /* @(/1/0/13/1/3/3) */
        case Q_EXIT_SIG: {
            QActive_disarm((QActive *)me);
            Capstone_display_pressure(me);
            status_ = Q_HANDLED();
            break;
        }
        /* @(/1/0/13/1/3/3/0) */
        case BTN1_UP_SIG: {
            status_ = Q_TRAN(&Capstone_surfaced);
            break;
        }
        /* @(/1/0/13/1/3/3/1) */
        case Q_TIMEOUT_SIG: {
            if (me->gas_in_cylinder_in_cl + GAS_INCREMENT_IN_CL
                            <= (CYLINDER_VOLUME_IN_CL * FULL_SCALE_CYLINDER_PRESSURE))
            {
                me->gas_in_cylinder_in_cl += GAS_INCREMENT_IN_CL;/* add gas */
                Capstone_display_pressure(me);
            }
            else {                                  /* the cylinder is full */
                BSP_lcdStr(LCD_CP_X + 2, LCD_CP_Y, "FULL");
            }
            QActive_arm((QActive *)me, BSP_TICKS_PER_SEC/10);
            status_ = Q_HANDLED();
            break;
        }
        default: {
            status_ = Q_SUPER(&Capstone_surfaced);
            break;
        }
    }
    return status_;
}
/* @(/1/0/13/1/4) ..........................................................*/
static QState Capstone_diving(Capstone * const me) {
    QState status_;
    switch (Q_SIG(me)) {
        /* @(/1/0/13/1/4) */
        case Q_ENTRY_SIG: {
            me->start_dive_time_in_ticks = BSP_get_ticks();
            status_ = Q_HANDLED();
            break;
        }
        /* @(/1/0/13/1/4) */
        case Q_EXIT_SIG: {
            QActive_post((QActive *)&AO_AlarmMgr, ALARM_SILENCE_SIG, ALL_ALARMS);
            status_ = Q_HANDLED();
            break;
        }
        /* @(/1/0/13/1/4/0) */
        case ASCENT_RATE_ADC_SIG: {
                        BSP_ledOff(ADC_LED);

                        me->ascent_rate_in_mm_per_sec =
                            ASCENT_RATE_MM_PER_MIN((uint16_t)Q_PAR(me));

                                        /* integrate the depth based on the ascent rate */
                        me->depth_in_mm -=
                            depth_change_in_mm(me->ascent_rate_in_mm_per_sec);
            /* @(/1/0/13/1/4/0/0) */
            if (me->depth_in_mm > 0) {
                                uint32_t consumed_gas_in_cl = gas_rate_in_cl(me->depth_in_mm);

                                if (me->gas_in_cylinder_in_cl > consumed_gas_in_cl) {
                                    me->gas_in_cylinder_in_cl -= consumed_gas_in_cl;
                                }
                                else {
                                    me->gas_in_cylinder_in_cl = 0;
                                }

                                me->dive_time_in_ticks = BSP_get_ticks()
                                                         - me->start_dive_time_in_ticks;

                                me->tts_in_ticks = me->depth_in_mm * (60 * BSP_TICKS_PER_SEC)
                                                   / ASCENT_RATE_LIMIT;

                                Capstone_display_depth(me);
                                Capstone_display_assent(me);
                                Capstone_display_pressure(me);

                                                           /* check the OUT_OF_AIR_ALARM... */
                                if (me->gas_in_cylinder_in_cl <
                                    gas_to_surface_in_cl(me->depth_in_mm) + GAS_SAFETY_MARGIN)
                                {
                                    QActive_post((QActive *)&AO_AlarmMgr, ALARM_REQUEST_SIG, OUT_OF_AIR_ALARM);
                                }
                                else {
                                    QActive_post((QActive *)&AO_AlarmMgr, ALARM_SILENCE_SIG, OUT_OF_AIR_ALARM);
                                }

                                                          /* check the ASCENT_RATE_ALARM... */
                                if (me->ascent_rate_in_mm_per_sec > ASCENT_RATE_LIMIT) {
                                    QActive_post((QActive *)&AO_AlarmMgr, ALARM_REQUEST_SIG, ASCENT_RATE_ALARM);
                                }
                                else {
                                    QActive_post((QActive *)&AO_AlarmMgr, ALARM_SILENCE_SIG, ASCENT_RATE_ALARM);
                                }

                                                                /* check the DEPTH_ALARM... */
                                if (me->depth_in_mm > MAXIMUM_DEPTH_IN_MM) {
                                    QActive_post((QActive *)&AO_AlarmMgr, ALARM_REQUEST_SIG, DEPTH_ALARM);
                                }
                                else {
                                    QActive_post((QActive *)&AO_AlarmMgr, ALARM_SILENCE_SIG, DEPTH_ALARM);
                                }

                                return Q_HANDLED();
                status_ = Q_HANDLED();
            }
            /* @(/1/0/13/1/4/0/1) */
            else {
                status_ = Q_TRAN(&Capstone_surfaced);
            }
            break;
        }
        default: {
            status_ = Q_SUPER(&Capstone_always);
            break;
        }
    }
    return status_;
}

