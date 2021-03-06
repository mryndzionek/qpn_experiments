/*****************************************************************************
* Model: dcf77.qm
* File:  ./led_pulser.c
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
/*${AOs::.::led_pulser.c} ..................................................*/
#include "qpn_port.h"
#include "bsp.h"
#include "led_pulser.h"
#include "phase_detector.h"

/*Q_DEFINE_THIS_FILE*/

#define LED_PULSE_MS             (BSP_TICKS_PER_SEC / 2)

/* LEDPulser class declaration -----------------------------------------------*/
/*${AOs::LEDPulser} ........................................................*/
typedef struct LEDPulser {
/* protected: */
    QMActive super;
} LEDPulser;

/* protected: */
static QState LEDPulser_initial(LEDPulser * const me);
static QState LEDPulser_IDLE  (LEDPulser * const me);
static QMState const LEDPulser_IDLE_s = {
    (QMState const *)0, /* superstate (top) */
    Q_STATE_CAST(&LEDPulser_IDLE),
    Q_ACTION_CAST(0), /* no entry action */
    Q_ACTION_CAST(0), /* no exit action */
    Q_ACTION_CAST(0)  /* no intitial tran. */
};
static QState LEDPulser_ACTIVE  (LEDPulser * const me);
static QState LEDPulser_ACTIVE_e(LEDPulser * const me);
static QState LEDPulser_ACTIVE_x(LEDPulser * const me);
static QMState const LEDPulser_ACTIVE_s = {
    (QMState const *)0, /* superstate (top) */
    Q_STATE_CAST(&LEDPulser_ACTIVE),
    Q_ACTION_CAST(&LEDPulser_ACTIVE_e),
    Q_ACTION_CAST(&LEDPulser_ACTIVE_x),
    Q_ACTION_CAST(0)  /* no intitial tran. */
};


/* Global objects ----------------------------------------------------------*/
LEDPulser AO_LEDPulser;

/* Blink class definition --------------------------------------------------*/
/*${AOs::LEDPulser_ctor} ...................................................*/
void LEDPulser_ctor(void) {
    QMActive_ctor(&AO_LEDPulser.super, Q_STATE_CAST(&LEDPulser_initial));
}
/*${AOs::LEDPulser} ........................................................*/
/*${AOs::LEDPulser::SM} ....................................................*/
static QState LEDPulser_initial(LEDPulser * const me) {
    static QMTranActTable const tatbl_ = { /* transition-action table */
        &LEDPulser_IDLE_s,
        {
            Q_ACTION_CAST(0) /* zero terminator */
        }
    };
    /* ${AOs::LEDPulser::SM::initial} */
    return QM_TRAN_INIT(&tatbl_);
}
/*${AOs::LEDPulser::SM::IDLE} ..............................................*/
/* ${AOs::LEDPulser::SM::IDLE} */
static QState LEDPulser_IDLE(LEDPulser * const me) {
    QState status_;
    switch (Q_SIG(me)) {
        /* ${AOs::LEDPulser::SM::IDLE::LED_PULSE} */
        case LED_PULSE_SIG: {
            static struct {
                QMState const *target;
                QActionHandler act[2];
            } const tatbl_ = { /* transition-action table */
                &LEDPulser_ACTIVE_s, /* target state */
                {
                    Q_ACTION_CAST(&LEDPulser_ACTIVE_e), /* entry */
                    Q_ACTION_CAST(0) /* zero terminator */
                }
            };
            status_ = QM_TRAN(&tatbl_);
            break;
        }
        default: {
            status_ = QM_SUPER();
            break;
        }
    }
    return status_;
}
/*${AOs::LEDPulser::SM::ACTIVE} ............................................*/
/* ${AOs::LEDPulser::SM::ACTIVE} */
static QState LEDPulser_ACTIVE_e(LEDPulser * const me) {
    QActive_arm((QActive *)me, LED_PULSE_MS);
    BSP_LEDPulse(Q_PAR(me));
    return QM_ENTRY(&LEDPulser_ACTIVE_s);
}
/* ${AOs::LEDPulser::SM::ACTIVE} */
static QState LEDPulser_ACTIVE_x(LEDPulser * const me) {
    QActive_disarm((QActive *)me);
    BSP_LEDPulse(3);
    return QM_EXIT(&LEDPulser_ACTIVE_s);
}
/* ${AOs::LEDPulser::SM::ACTIVE} */
static QState LEDPulser_ACTIVE(LEDPulser * const me) {
    QState status_;
    switch (Q_SIG(me)) {
        /* ${AOs::LEDPulser::SM::ACTIVE::Q_TIMEOUT} */
        case Q_TIMEOUT_SIG: {
            static struct {
                QMState const *target;
                QActionHandler act[2];
            } const tatbl_ = { /* transition-action table */
                &LEDPulser_IDLE_s, /* target state */
                {
                    Q_ACTION_CAST(&LEDPulser_ACTIVE_x), /* exit */
                    Q_ACTION_CAST(0) /* zero terminator */
                }
            };
            status_ = QM_TRAN(&tatbl_);
            break;
        }
        default: {
            status_ = QM_SUPER();
            break;
        }
    }
    return status_;
}

