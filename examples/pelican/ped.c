/*****************************************************************************
* Model: pelican.qm
* File:  ./ped.c
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
/*${.::ped.c} ..............................................................*/
#include "qpn_port.h"
#include "bsp.h"
#include "pelican.h"

/*Q_DEFINE_THIS_FILE*/

enum PedTimeouts {                             /* various timeouts in ticks */
    N_ATTEMPTS = 10,                      /* number of PED_WAITING attempts */
    WAIT_TOUT = BSP_TICKS_PER_SEC * 3,  /* wait between posting PED_WAITING */
    OFF_TOUT  = BSP_TICKS_PER_SEC * 8    /* wait time after posting OFF_SIG */
};

/* Peld class declaration --------------------------------------------------*/
/*${components::Ped} .......................................................*/
typedef struct Ped {
/* protected: */
    QActive super;

/* private: */
    uint8_t retryCtr;
} Ped;

/* protected: */
static QState Ped_initial(Ped * const me);
static QState Ped_off(Ped * const me);
static QState Ped_wait(Ped * const me);


/* Global objects ----------------------------------------------------------*/
Ped AO_Ped;                 /* the single instance of the Ped active object */

/* Pelican class definition ------------------------------------------------*/
/*${components::Ped_ctor} ..................................................*/
void Ped_ctor(void) {
    QActive_ctor(&AO_Ped.super, Q_STATE_CAST(&Ped_initial));
}
/*${components::Ped} .......................................................*/
/*${components::Ped::SM} ...................................................*/
static QState Ped_initial(Ped * const me) {
    /* ${components::Ped::SM::initial} */
    return Q_TRAN(&Ped_off);
}
/*${components::Ped::SM::off} ..............................................*/
static QState Ped_off(Ped * const me) {
    QState status_;
    switch (Q_SIG(me)) {
        /* ${components::Ped::SM::off} */
        case Q_ENTRY_SIG: {
            BSP_showState("off");
            QActive_arm((QActive *)me, OFF_TOUT);
            QActive_post((QActive *)&AO_Pelican, OFF_SIG, 0);
            status_ = Q_HANDLED();
            break;
        }
        /* ${components::Ped::SM::off} */
        case Q_EXIT_SIG: {
            QActive_post((QActive *)&AO_Pelican, ON_SIG, 0);
            status_ = Q_HANDLED();
            break;
        }
        /* ${components::Ped::SM::off::Q_TIMEOUT} */
        case Q_TIMEOUT_SIG: {
            me->retryCtr = N_ATTEMPTS;
            status_ = Q_TRAN(&Ped_wait);
            break;
        }
        default: {
            status_ = Q_SUPER(&QHsm_top);
            break;
        }
    }
    return status_;
}
/*${components::Ped::SM::wait} .............................................*/
static QState Ped_wait(Ped * const me) {
    QState status_;
    switch (Q_SIG(me)) {
        /* ${components::Ped::SM::wait} */
        case Q_ENTRY_SIG: {
            BSP_showState("wait");
            QActive_post((QActive *)&AO_Pelican, PEDS_WAITING_SIG, 0);
            QActive_arm((QActive *)me, WAIT_TOUT);
            status_ = Q_HANDLED();
            break;
        }
        /* ${components::Ped::SM::wait::Q_TIMEOUT} */
        case Q_TIMEOUT_SIG: {
            --me->retryCtr;
            /* ${components::Ped::SM::wait::Q_TIMEOUT::[me->retryCtr>0]} */
            if (me->retryCtr > 0) {
                status_ = Q_TRAN(&Ped_wait);
            }
            /* ${components::Ped::SM::wait::Q_TIMEOUT::[else]} */
            else {
                status_ = Q_TRAN(&Ped_off);
            }
            break;
        }
        default: {
            status_ = Q_SUPER(&QHsm_top);
            break;
        }
    }
    return status_;
}

