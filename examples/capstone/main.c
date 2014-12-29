/*****************************************************************************
* Model: capstone.qm
* File:  ./main.c
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
/*${.::main.c} .............................................................*/
#include "qpn_port.h"   /* QP-nano port */
#include "bsp.h"        /* Board Support Package (BSP) */
#include "capstone.h"
#include "alarm.h"
/*..........................................................................*/
static QEvt l_capstoneQueue[5];
static QEvt l_alarmQueue[5];

/* QF_active[] array defines all active object control blocks --------------*/
QActiveCB const Q_ROM Q_ROM_VAR QF_active[] = {
    { (QActive *)0,           (QEvt *)0,      0                            },
    { (QActive *)&AO_Capstone, l_capstoneQueue, Q_DIM(l_capstoneQueue)     },
    { (QActive *)&AO_AlarmMgr, l_alarmQueue, Q_DIM(l_alarmQueue)           }

};

/* make sure that the QF_active[] array matches QF_MAX_ACTIVE in qpn_port.h */
Q_ASSERT_COMPILE(QF_MAX_ACTIVE == Q_DIM(QF_active) - 1);

/*..........................................................................*/
int main(void)
{
    Capstone_ctor();
    AlarmMgr_ctor();
    BSP_init();      /* initialize the board */

    return QF_run(); /* transfer control to QF-nano */
}
