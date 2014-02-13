/********************************************************
 *                          
 *  C o p y r i g h t   N o t i c e         
 *                          
 *  The Tics Realtime Operating System
 *
 *  Copyright (c) 1991-2004, Michael Dennis McDonnell
 *  All rights reserved.                
 *                          
 *  
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  
 *  02111-1307, USA.
 *  
 *  See the GNU General Public License here:
 *
 *  http://www.gnu.org/copyleft/gpl.html#SEC1
 *
 *  Tics Realtime contact info:
 * 
 *  Mike@TicsRealtime.com or MichaelDMcDonnell@Yahoo.com.
 *  
 *  
 ********************************************************/

/*
 * Adaptation of the original hs_msdos.c for the X32 FPGA softcore processor
 * by Michel Wilson, michel@crondor.net.
 *
 * For more information about the X32 FPGA softcore, please
 * visit http://x32.ewi.tudelft.nl/
 */


#include "tics.h"
#include "ticsext.h"
#include "x32.h"
#include <setjmp.h>

void** CurrentSp;
unsigned int TimerTicsPerPcTic;
unsigned int Tctr;
extern unsigned int MsPerTimerInt;
extern int canPreempt;
static typeTics tics;

void ticsTimerIsr(void)
{
    typeMsg * msg, * timer;
    int switchFlag;
    int     foundHigherPri;
    typeMsg * nextMsg;
    typeTcb * tcb;
    typeTcb * nextTcbToRun;

    canPreempt = FALSE;
    msg = TimerHead->next;

    switchFlag = foundHigherPri = FALSE; 

    if (msg != TimerTail && --msg->ticCountDown <= 0) {
        do {
            nextMsg = msg->next;
            removeQ(msg, FALSE);
            tcb = (typeTcb *) msg->destTcb;
            
            if (tcb->pri < ActiveTcb->pri) {
                foundHigherPri = TRUE;
            }

            if (bitIsSet(msg->flags, PERIODIC)) {
                timer = (typeMsg *) getFreeMsg(FALSE);
                * timer = * msg;
                msg->ticCountDown = msg->startTicCount;
                _startTimer(msg, FALSE);
                _sendMsg(timer, FALSE);
            } 
            else {
                _sendMsg(msg, FALSE);
            }

            msg = nextMsg;
        } while (msg->ticCountDown <= 0 && msg != TimerTail);
    }

    if (foundHigherPri == TRUE) {
        switchFlag = TRUE;
    } 

    if (bitIsSet(ActiveTcb->flags, TIMESLICE)) {
        ActiveTcb->tsCounter -= MsPerTimerInt;
        if (switchFlag == FALSE && ActiveTcb->tsCounter <= 0) {
            nextTcbToRun = (typeTcb *) ReadyHead->next->destTcb;
            if (!isEmpty(ReadyHead) &&
                    nextTcbToRun != ActiveTcb &&
                    (nextTcbToRun->pri == ActiveTcb->pri || 
                    isIdleTask(ActiveTcb))) {
                ActiveTcb->tsCounter = ActiveTcb->timeSlice;
                switchFlag = TRUE;
            }
        }
    }

    canPreempt = TRUE;

    if (switchFlag == TRUE) {
        _isrPreempt(FALSE);
    }
}

void isrRet() {
    unsigned int *fp = (unsigned int *)_get_fp();
    fp = (unsigned int *)(*(fp-1));
    if(*(fp-4) == 0) {
        canPreempt=TRUE;
        _isrRet();
    }
}

void initTimer(unsigned int timerChipCount, int diOpt)
{
    DI(diOpt);
    peripherals[PERIPHERAL_TIMER1_PERIOD] = timerChipCount;
    SET_INTERRUPT_VECTOR(INTERRUPT_TIMER1, &ticsTimerIsr);
    SET_INTERRUPT_PRIORITY(INTERRUPT_TIMER1, 1);
    ENABLE_INTERRUPT(INTERRUPT_TIMER1);
    EI(diOpt);
}

void exitTics()
{
    DISABLE_INTERRUPT(INTERRUPT_GLOBAL);
    DISABLE_INTERRUPT(INTERRUPT_TIMER1);
    exit(0);
}

void suspend(void)
{
    DISABLE_INTERRUPT(INTERRUPT_GLOBAL);

    /* Save stack pointer and stack segment registers. */
    ActiveTcb->context = (void *)_get_fp();

    /* Get the new active task tcb. */
    ActiveTcb = removeRQ(FALSE);

    if (bitIsClr(ActiveTcb->flags, NO_STACK_CHECK) &&
        (ActiveTcb->context > ActiveTcb->stackTop ||
        ActiveTcb->context < ActiveTcb->stackBottom)) {
        setError("suspend - Bad stack.\n");
    }

    /*  Restore stack pointer and stack segment for the new active task. */
    _set_fp(ActiveTcb->context);

    ENABLE_INTERRUPT(INTERRUPT_GLOBAL);
}

void initStack(typeTcb * tcb, int diOpt)
{
    unsigned int stackSizeInInts;

    if (tcb->stackSizeInBytes < MIN_STACK_SIZE) {
        setError("initStack - Stack size too small.\n");
    }

    if (tcb == NULL) {
        setError("initStack - NULL tcb.\n");
    }

    stackSizeInInts = tcb->stackSizeInBytes / sizeof(int);

    DI(diOpt);

    CurrentSp = (void *) ((int *)CurrentSp - stackSizeInInts);
    tcb->stackBottom = CurrentSp;

    if (tcb->stackBottom < tics.stackBottomAddress) {
        setError("initStack - out of stack space.\n");
    }

    tcb->stackTop = (void **) ((int *)CurrentSp + stackSizeInInts - 1);

    tcb->stackBottom[0] = (void *)0; // param
    tcb->stackBottom[1] = (void *)tcb->taskPtr; // addr
    tcb->stackBottom[2] = 0; // el
    tcb->stackBottom[3] = &(tcb->stackBottom[0]); // ap
    tcb->stackBottom[4] = &(tcb->stackBottom[1]); // lp
    tcb->stackBottom[5] = &(tcb->stackBottom[1]); // fp
    tcb->context = &(tcb->stackBottom[6]);

    EI(diOpt);
}

void doHardwareDependentInits(typeTics * tics)
{
    unsigned int stackSegment, stackPtr;

    MsPerTimerInt = (unsigned int) ((tics->uSPerTimerChipCount) / (1000L));
    CurrentSp = tics->stackTopAddress;

    initTimer(tics->timerChipCount, FALSE);
}

void x32FatalErrorHandler(char * errMsg) {
    printf("%s", errMsg);
    while(TRUE);
}

typeTics * makeTics(typeMsg * msgSpace, int msgSpaceSizeInBytes,
        unsigned char * stackSpace, int stackSpaceSizeInBytes,
        typeTcb * tcbSpace, int tcbSpaceSizeInBytes)
{
    tics.msgSpace = msgSpace;
    tics.msgSpaceSizeInBytes = msgSpaceSizeInBytes;
    tics.stackSpaceSizeInBytes = stackSpaceSizeInBytes;

    tics.stackBottomAddress = (void **) stackSpace + sizeof(int);
    tics.stackTopAddress =
    (tics.stackBottomAddress - sizeof(int)) + stackSpaceSizeInBytes - sizeof(int);

    tics.tcbSpace = tcbSpace;
    tics.tcbSpaceSizeInBytes = tcbSpaceSizeInBytes;
    tics.fatalErrorHandler = x32FatalErrorHandler;
    tics.timerChipCount = 5 * CLOCKS_PER_MS;
    tics.uSPerTimerChipCount = 5000L;

    Tics = &tics;

    return(&tics);
}


