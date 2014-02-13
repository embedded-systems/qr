/********************************************************
 *							
 *	C o p y r i g h t   N o t i c e			
 *							
 *	The Tics Realtime Operating System
 *
 * 	Copyright (c) 1991-2004, Michael Dennis McDonnell
 *	All rights reserved.				
 *							
 *	
 * 	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version 2
 *	of the License, or (at your option) any later version.
 *	
 *	This program is distributed in the hope that it will be useful,
 * 	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *	
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 * 	Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  
 *  02111-1307, USA.
 *	
 *  See the GNU General Public License here:
 *
 *  http://www.gnu.org/copyleft/gpl.html#SEC1
 *
 *	Tics Realtime contact info:
 * 
 *  Mike@TicsRealtime.com or MichaelDMcDonnell@Yahoo.com.
 *	
 *	
 ********************************************************/


/********************************************************
 *							
 *	I n c l u d e s					
 *							
 ********************************************************/


#include "tics.h"

#include "x32.h"

/********************************************************
 *							
 *	G l o b a l s					
 *							
 ********************************************************/

int TicsReadyFlag;
unsigned int MsPerTimerInt;
int canPreempt;
unsigned long SeqNum;
typeTcb * ActiveTcb, * ActiveCoopTcb, * IdleTaskTcb, * CoopTcb;
typeMsg * TimerHead, * TimerTail;
typeMsg * ReadyHead, * ReadyTail;
typeMsg ActualReadyHead, ActualReadyTail; 
typeMsg ActualTimerHead, ActualTimerTail;
typeTcb ActualReadyHeadTcb,  ActualReadyTailTcb;
void (* FatalErrorHandler)(char * errMsg);
typeMem ActualTcbMemPool;
typeMem ActualMsgMemPool;
typeMem * TcbMemPool;
typeMem * MsgMemPool;
typeTics * Tics;


/********************************************************
 *							
 *	E x t e r n a l s
 *
 ********************************************************/



/*********************************************************
 *							
 *	F u n c t i o n   D e f i n i t i o n		
 *							
 *							
 *	P u r p o s e					
 *							
 *	This function returns a pointer to a free msg.	
 *							
 *							
 *	A r g u m e n t s				
 *							
 *	diOpt - If TRUE, interrupts will be disabled 	
 *	when required. This argument should always be 	
 *	TRUE unless interrupts are already disabled 	
 *	when this function is called.			
 *							
 *							
 *	R e t u r n   V a l u e				
 *							
 *	Returns a pointer to the free msg. 		
 *							
 *							
 *	N o t e s					
 *							
 *	The free list is maintained as a singly linked	
 *	list of typeFreeMsg's. The caller must cast 	
 *	the msg to the desired type. Each time a free	
 *	msg is issued it is assigned a unique sequence	
 *	number.						
 *
 ********************************************************/

typeMsg * getFreeMsg(int diOpt)
{
	typeMsg * msg;
	
	DI(diOpt);
	msg = (typeMsg *) _getMem(MsgMemPool, FALSE);
	msg->seqNum = ++SeqNum;
	EI(diOpt);

	return(msg);
}


/*********************************************************
 *							
 *	F u n c t i o n   D e f i n i t i o n		
 *							
 *							
 *	P u r p o s e					
 *							
 *	This function adds a msg to to the free list.	
 *
 *							
 *	A r g u m e n t s				
 *							
 *	msgToFree - Pointer to msg to free.		
 *							
 *	diOpt - If TRUE, interrupts will be disabled 	
 *	when required. This argument should always be 	
 *	TRUE unless interrupts are already disabled 	
 *	when this function is called. 			
 *							
 *							
 *	N o t e s					
 *							
 *	Each time a  msg is added back to the free 	
 *	list, it is assigned a unique number.		
 *							
 ********************************************************/

void putFreeMsg(typeMsg * msg, int diOpt)
{
	DI(diOpt);
	msg->seqNum = ++SeqNum;
	_putMem(MsgMemPool, msg, FALSE);
	EI(diOpt);
}


/********************************************************
 *							
 *	F u n c t i o n   D e f i n i t i o n		
 *							
 *							
 *	P u r p o s e					
 *							
 *	This function initializes a msg.		
 *							
 *							
 *	A r g u m e n t s				
 *							
 *	msg - Pointer to msg to initialize.		
 *							
 *	pri - priority of the msg.			
 *							
 *							
 *	N o t e s					
 *							
 *							
 ********************************************************/

void initMsg(typeMsg * msg, int pri)
{
	msg->iData = 0;
	msg->next = msg->prev = NULL;
	msg->pri = pri;
	msg->flags = 0;
	msg->msgQNum = 0;
	msg->msgNum = 0;
	msg->pData = NULL;
	msg->lData = 0L;
	msg->destName = NULL;
	msg->senderTcb = actualActiveTcb();
}


/********************************************************
 *							
 *	F u n c t i o n   D e f i n i t i o n		
 *							
 *							
 *	P u r p o s e					
 *							
 *	This function adds a timer to the timer queue.	
 *							
 *							
 *	A r g u m e n t s				
 *							
 *	msg - Pointer to msg to send on timeout.	
 *							
 *	diOpt - If TRUE, interrupts are disabled when	
 *	required. This argument should always be TRUE	
 *	unless interrupts are already disabled when 	
 *	this function is called.			
 *							
 *							
 *	R e t u r n   V a l u e				
 *							
 *	Returns a pointer to the msg that was		
 *	created and placed in the timer queue.		
 *							
 *							
 *	N o t e s					
 *							
 *	Timers are maintained in a sorted list with the 
 *	shortest timer at the front. Timeout counts	
 *	are stored differentially. For example, timers	
 *	of 10 ms, 20 ms, and 30 ms are stored in 3	
 *	separate msgs as 10 ms, 10 ms, and 10 ms. 	
 *	This means that only the first timer 	 	
 *	requires decrementing per timer interrupt, 	
 *	which greatly increases efficiency.		
 *							
 ********************************************************/

typeMsg * _startTimer(typeMsg * msg, int diOpt)
{
	typeMsg * tp;
	long count;

	if (msg == NULL || 
	msg->startTicCount < MIN_TIMER || 
	msg->startTicCount > MAX_TIMER || msg->msgQNum < 0)
	{
		setError("_startTimer - Bad msg.\n");
	}

	if (bitIsClr(msg->flags, CALL_BACK)) {
		if (msg->destTcb == NULL || msg->msgQNum >= msg->destTcb->maxNumMsgQueues) {
			setError("_startTimer - Bad msg.\n");
		}
	}


	DI(diOpt);

	msg->readyMsg = NULL;
	msg->readySeqNum = 0;

	tp = TimerHead->next;
	count = msg->startTicCount;

	while ( tp != TimerTail && (count - tp->ticCountDown) >= 0) {
		count -= tp->ticCountDown;
		tp = tp->next;
    }

	msg->ticCountDown = count;

	if (tp != TimerTail) tp->ticCountDown -= count;

	setBit(msg->flags, TIMER);
	addQ(tp->prev, msg, FALSE);

	EI(diOpt);

	return(msg);
}


/********************************************************
 *							
 *	F u n c t i o n   D e f i n i t i o n		
 *							
 *							
 *	P u r p o s e					
 *							
 *	This function initializes a linked list.	
 *							
 *							
 *	A r g u m e n t s				
 *							
 *	head	- Pointer to the head of the list.	
 *							
 *	tail	- Pointer to the tail of the list.	
 *							
 *							
 *	N o t e s					
 *							
 *	Interrupts must be disabled prior to call.	
 *							
 ********************************************************/

void initList(typeMsg * head, typeMsg * tail)
{
	head->next = head->prev = tail;
	tail->next = tail->prev = head;
	head->pri = HEAD_PRI;
	tail->pri = TAIL_PRI;
}


/********************************************************
 *							
 *	F u n c t i o n   D e f i n i t i o n		
 *							
 *							
 *	P u r p o s e					
 *							
 *	This function creates and initializes a task control
 *  block.
 *							
 *							
 *	A r g u m e n t s				
 *							
 *	taskPtr - Pointer to the task (which is a C	
 *	function).					
 *							
 *	taskNum - User supplied task number.		
 *							
 *	diOpt - If TRUE, interrupts will be disabled 	
 *	when required. This argument should always be 	
 *	TRUE unless interrupts are already disabled 	
 *	when this function is called. 			
 *							
 *							
 *	R e t u r n   V a l u e				
 *							
 *	Returns a pointer to the tcb, or NULL if an	
 *	error occurred.					
 *							
 *							
 *	N o t e s					
 *							
 *	Task control/communication is accomplished	
 *	with the task control block.			
 *							
 ********************************************************/

typeTcb * _makeTask(typeTask taskPtr, int taskNum, int diOpt)
{
	typeTcb * tcb;

	DI(diOpt);

	tcb = (typeTcb *) _getMem(TcbMemPool, FALSE);

	initTcb(tcb, taskPtr, taskNum, DEF_PRI, DEF_NUM_MSG_QUEUES, 
	DEF_NUM_MAILBOXES, DEF_TCB_FLAGS, FALSE);

	EI(diOpt);

	return(tcb);
}


/********************************************************
 *							
 *	F u n c t i o n   D e f i n i t i o n		
 *							
 *							
 *	P u r p o s e					
 *							
 *	Add a msg to a queue according to its		
 *	priority.					
 *							
 *							
 *	A r g u m e n t s				
 *							
 *	head - Pointer to head of queue.		
 *							
 *	newMsg - Msg to add to queue.			
 *							
 *	diOpt - If TRUE, interrupts will be disabled 	
 *	when required. This argument should always be 	
 *	TRUE unless interrupts are already disabled 	
 *	when this function is called.			
 *							
 *							
 *	N o t e s					
 *							
 *	The list is traversed from the highest priority	
 *	msg (head) and newMsg inserted accordingly.	
 *	However, since most of the time tasks are 	
 *	at the same priority, a check is first made to	
 *	see if the msg can be inserted at the end	
 *	of the list.					
 *							
 ********************************************************/

void addPQ(typeMsg * head, typeMsg * newMsg, int diOpt)
{
	int		newMsgPri;
	typeMsg	* msg, * lastMsg;

	DI(diOpt);

	lastMsg = head->prev->prev;
	newMsgPri = newMsg->pri;

	if (newMsgPri >= lastMsg->pri) {
		addQ(lastMsg, newMsg, FALSE);
		EI(diOpt);
		return;
	}

	msg = head;

	for (;;) {
		if (msg->next->pri > newMsgPri) {
			addQ(msg, newMsg, FALSE);
			break;
		}
		msg = msg->next;
	}

	EI(diOpt);
}


/********************************************************
 *							
 *	F u n c t i o n   D e f i n i t i o n		
 *							
 *							
 *	P u r p o s e					
 *							
 *	Inserts a msg into a queue.			
 *							
 *							
 *	A r g u m e n t s				
 *							
 *	msg - newMsg will be added after this msg.	
 *							
 *	newMsg - Pointer to msg to add.			
 *							
 *	diOpt - If TRUE, interrupts will be disabled 	
 *	when required. This argument should always be 	
 *	TRUE unless interrupts are already disabled 	
 *	when this function is called.			
 *							
 ********************************************************/

void addQ(typeMsg * msg, typeMsg * newMsg, int diOpt)
{
	DI(diOpt);

	if (msg == NULL || newMsg == NULL) {
		EI(diOpt);
		setError("addQ - Attempt to add a NULL msg.\n");
	}

	newMsg->next = msg->next;
	msg->next->prev = newMsg;
	newMsg->prev = msg;
	msg->next = newMsg;

	EI(diOpt);
}


/********************************************************
 *							
 *	F u n c t i o n   D e f i n i t i o n		
 *							
 *							
 *	P u r p o s e					
 *							
 *	Removes a msg from a queue.			
 *							
 *							
 *	A r g u m e n t s				
 *							
 *	msg - Pointer to the msg to remove.		
 *							
 *	diOpt - If TRUE, interrupts will be disabled 	
 *	when required. This argument should always be 	
 *	TRUE unless interrupts are already disabled 	
 *	when this function is called.			
 *							
 *							
 ********************************************************/

void * removeQ(typeMsg * msg, int diOpt)
{
	DI(diOpt);

	if (msg == NULL || isHead(msg) || isTail(msg)) {
		EI(diOpt);
		setError("removeQ - Attempt to remove a NULL or head/tail msg.\n");
	}

	msg->next->prev = msg->prev;
	msg->prev->next = msg->next;
	msg->next = NULL;

	EI(diOpt);

	return(msg);
}


/********************************************************
 *							
 *	F u n c t i o n   D e f i n i t i o n		
 *							
 *							
 *	P u r p o s e					
 *							
 *	Removes the next ready task tcb from the Ready	
 *	Queue.						
 *							
 *							
 *	A r g u m e n t s				
 *							
 *	diOpt - If TRUE, interrupts will be disabled 	
 *	when required. This argument should always be 	
 *	TRUE unless interrupts are already disabled 	
 *	when this function is called.			
 *							
 *							
 *	N o t e s					
 *							
 *	The Ready Queue is a doubly linked list of	
 *	typeMsg's. The msg field destTcb points		
 *	to the tcb of the task to run. The msg is 	
 *	removed from the Ready Queue, and the 		
 *	associated tcb removed. The typeMsg is then	
 *	returned to the free list, and the tcb is	
 *	returned to the caller.				
 *							
 ********************************************************/

typeTcb * removeRQ(int diOpt)
{
	typeMsg * msg;
	typeTcb * tcb;

	DI(diOpt);

	msg = (typeMsg *) removeQ(ReadyHead->next, FALSE);
	tcb = msg->destTcb;
	putFreeMsg(msg, FALSE);

	EI(diOpt);

	return(tcb);
}


/********************************************************
 *							
 *	F u n c t i o n   D e f i n i t i o n		
 *							
 *							
 *	P u r p o s e					
 *							
 *	Add a msg to the the Ready Queue.		
 *							
 *							
 *	A r g u m e n t s				
 *							
 *	tcb - Pointer to tcb to add to Ready Queue.	
 *							
 *	diOpt - If TRUE, interrupts will be disabled 	
 *	when required. This argument should always be 	
 *	TRUE unless interrupts are already disabled 	
 *	when this function is called.			
 *							
 *							
 *	R e t u r n   V a l u e				
 *							
 *	Returns a pointer to the msg added to the 	
 *	Ready Queue.					
 *							
 *							
 *	N o t e s					
 *							
 *	The Ready Queue is a doubly linked list of	
 *	typeMsg's. The typeMsg field destTcb points		
 *	to the tcb of the task to run. The msg is 	
 *	created and msg->destTcb is set to point to	
 *	tcb. The msg is then added in priority order	
 *	to the Ready Queue.				
 *							
 ********************************************************/

typeMsg * addRQ(typeTcb * tcb, int diOpt)
{
	typeMsg * msg;

	DI(diOpt);

	if (isCoop(tcb)) {
		setError("addRQ - Attempt to schedule coop task.\n");
	}

	msg = (typeMsg *) getFreeMsg(FALSE);
	
	initMsg(msg, tcb->pri);
	msg->destTcb = tcb;

	addPQ(ReadyHead, msg, FALSE);

	EI(diOpt);

	return(msg);
}


/********************************************************
 *							
 *	F u n c t i o n   D e f i n i t i o n		
 *							
 *							
 *	P u r p o s e					
 *							
 *	This task runs when there are now other tasks to run.
 *	It runs at the lowest possible priority.
 *							
 *							
 *	N o t e s					
 *							
 *	There must always be at least one task running.	
 *	The idle task will run when no other tasks	
 *	are ready to run. When a task becomes ready	
 *	to run it preempts the idle task, since the	
 *	idle task's priority is lower than the lowest	
 *	user priority.					
 *							
 ********************************************************/

void idleTask(void)
{
	while (TRUE) {
		if (!isTail(ReadyHead->next)) {
			preempt(TRUE);
		}
	}
}


/********************************************************
 *							
 *	F u n c t i o n   D e f i n i t i o n		
 *							
 *							
 *	P u r p o s e					
 *							
 *	Send a msg.					
 *							
 *							
 *	A r g u m e n t s				
 *							
 *	msg - Msg to send.				
 *							
 *	diOpt - If TRUE, interrupts are disabled when	
 *	required. This argument should always be TRUE	
 *	unless interrupts are already disabled when 	
 *	this function is called.			
 *							
 *	preeOpt - TRUE if immediate preemption is 	
 *	desired. Current task will suspend and msg	
 *	destination task will run if it is of a higher	
 *	priority.					
 *							
 *							
 *	N o t e s					
 *							
 *	USE MACRO sendMsg - see Tics.h.
 *
 *  Msg queues are attached to the task's tcb	
 *	structure. _sendMsg adds a msg to 		
 *	the desired destination task's queue according	
 *	to its priority. 				
 *							
 *	After the msg is placed into the queue, 	
 *	the destination task is added to the ready	
 *	queue so that it can run and receive the msg.	
 *							
 ********************************************************/

typeMsg * _sendMsg(typeMsg * msg, int diOpt)
{
	int msgQNum;
	typeMsg * headPtr, * readyMsg;
	typeTcb * destTcb;

	if (msg == NULL || msg->destTcb == NULL) {
		setError("_sendMsg - NULL msg or NULL dest tcb.\n");
	}

	destTcb = actualTcbForMsg(msg->destTcb);

	msgQNum = msg->msgQNum;
	if (msgQNum >= destTcb->maxNumMsgQueues) {
		setError("_sendMsg - Bad queue.\n");
	}
	
        DI(diOpt);

	headPtr = destTcb->msgTable[msgQNum];
	if (headPtr == NULL) {
		setError("_sendMsg - No message queue found.\n");
	}

	addPQ(headPtr, msg, FALSE);
	msg->readyMsg = NULL;
	msg->readySeqNum = 0;

	if (bitIsClr(msg->flags, NO_MSG_NOTIFY)) {
		readyMsg = addRQ(destTcb, FALSE);
		msg->readyMsg = readyMsg;
		msg->readySeqNum = readyMsg->seqNum;
		if (canPreempt && destTcb->pri < ActiveTcb->pri) {
			preempt(FALSE);
		}
	}

	EI(diOpt);

	return(msg);
}


/********************************************************
 *							
 *	F u n c t i o n   D e f i n i t i o n		
 *							
 *							
 *	P u r p o s e					
 *							
 *	Wake up a suspended task (system use only). Users
 *	should send a msg to wake up a task.
 *
 *	XXX seems not to be used anywhere? (MW)
 *							
 *							
 *	A r g u m e n t s				
 *							
 *	tcb - tcb of task to wakeup.			
 *							
 *	diOpt - If TRUE, interrupts are disabled when	
 *	required. This argument should always be TRUE	
 *	unless interrupts are already disabled when 	
 *	this function is called.			
 *							
 *	preeOpt - TRUE if immediate preemption is 	
 *	desired. Current task will suspend and msg	
 *	destination task will run if it is of a higher	
 *	priority.					
 *							
 ********************************************************/

void _wakeup(typeTcb * tcb, int diOpt)
{
	DI(diOpt);

	addRQ(tcb, FALSE);

	if (canPreempt && ActiveTcb->pri > tcb->pri) {
		preempt(FALSE);
    }

    EI(diOpt);
}


/********************************************************
 *							
 *	F u n c t i o n   D e f i n i t i o n		
 *							
 *							
 *	P u r p o s e					
 *							
 *	Receive a msg.					
 *							
 *							
 *	A r g u m e n t s				
 *							
 *	tcb - Tcb to remove msg from.			
 *							
 *	msgQNum - Msg queue number to remove msg	
 *	from.						
 *							
 *	msgNum - Msg number of msg. If ANY_MSG, 	
 *	then remove first msg in queue.			
 *							
 *	diOpt - If TRUE, interrupts are disabled when	
 *	required. This argument should always be TRUE	
 *	unless interrupts are already disabled when 	
 *	this function is called.			
 *							
 *	remOpt - If TRUE, then msg is removed		
 *	from msg queue. Otherwise, the msg remains	
 *	in the msg queue, and a pointer to the msg	
 *	in the queue is returned.			
 *							
 *							
 *	R e t u r n   V a l u e				
 *							
 *	Returns a pointer to the msg, or		
 *	NULL if the msg is not in the queue.		
 *							
 *							
 *	N o t e s					
 *							
 *							
 *	USE MACRO rcvMsg - see Tics.h.
 *
 *	Msg queues are attached to the task's tcb	
 *	structure. _rcvMsg removes a designated msg	
 *	from a designated queue. _rcvMsg must also 	
 *	remove superfluous msgs from the ready queue.	
 *	Consider a task that has 2 msgs in its queue, 	
 *	which means there are 2 corresponding entries 	
 *	in the ready queue. When the task runs, the	
 *	first entry in the ready queue is deleted.	
 *	If the task removes both msgs from its msg 	
 *	queue, the remaining msg in the ready queue	
 *	is superfluous and must be removed. Whether 	
 *	the correspnding msg is in the ready queue	
 *	is determined by cross referencing the unique	
 *	sequence number.				
 *							
 ********************************************************/

typeMsg * _rcvMsg(typeTcb * tcb, int msgQNum, int msgNum, int diOpt,
int remOpt)
{
	typeMsg * msg;

	if (msgQNum >= tcb->maxNumMsgQueues) {
		setError("_rcvMsg - Attempt to get msg from a non-existent queue.\n");
	}

	DI(diOpt);

	msg = tcb->msgTable[msgQNum]->next;

	if (msgNum != ANY_MSG) {
		while (!isTail(msg)) {
			if (msg->msgNum == msgNum) break;
			msg = msg->next;
		}
	}

	if (isTail(msg)) {
		EI(diOpt);
		return(NULL);
	}

	if (remOpt == TRUE) {
		removeQ(msg, FALSE);
		msg->seqNum = ++SeqNum;
		if (msg->readyMsg != NULL && 
		msg->readySeqNum == msg->readyMsg->seqNum) {
			removeQ(msg->readyMsg, FALSE);
			putFreeMsg(msg->readyMsg, FALSE);
		}
	}

	EI(diOpt);

	return(msg);
}


/********************************************************
 *							
 *	F u n c t i o n   D e f i n i t i o n		
 *							
 *							
 *	P u r p o s e					
 *							
 *	Wait for a designated msg to arrive.		
 *							
 *							
 *	A r g u m e n t s				
 *							
 *	msgQNum - Msg queue number to remove msg	
 *	from.						
 *							
 *	msgNum - Msg number of msg. If ANY_MSG,  	
 *	then remove first msg in queue.			
 *							
 *	diOpt - If TRUE, interrupts are disabled when	
 *	required. This argument should always be TRUE	
 *	unless interrupts are already disabled when 	
 *	this function is called.			
 *							
 *	remOpt - If TRUE, then msg is removed		
 *	from msg queue. Otherwise, the msg remains	
 *	in the msg queue, and a pointer to the msg	
 *	in the queue is returned.			
 *							
 *							
 *	R e t u r n   V a l u e				
 *							
 *	Returns a pointer to the msg.			
 *							
 *							
 *	N o t e s					
 *							
 *							
 *	USE MACRO waitMsg - see Tics.h.
 *
 *	This is the general form of waitMsg. It calls	
 *	_rcvMsg in an attempt to get the desired 	
 *	msg. If it is not in the queue, the task is	
 *	suspended. The WAITING flag is set so that	
 *	time-slicing, if enabled, will not run this	
 *	task even if it is due to be time-sliced in.	
 *							
 ********************************************************/

typeMsg * _waitMsg(int msgQNum, int msgNum, int diOpt, int remOpt)
{
	typeMsg * msg;

	DI(diOpt);
	setBit(ActiveTcb->flags, WAITING);
	while ((msg = _rcvMsg(ActiveTcb, msgQNum, msgNum, FALSE, remOpt)) == NULL) {
		suspend();
        }
	clrBit(ActiveTcb->flags, WAITING);
	EI(diOpt);

	return(msg);
}


/********************************************************
 *							
 *	F u n c t i o n   D e f i n i t i o n		
 *							
 *							
 *	P u r p o s e					
 *							
 *	Suspends program execution for the desired	
 *	number of milliseconds.				
 *							
 *							
 *	A r g u m e n t s				
 *							
 *	count - Time to pause in milliseconds.		
 *							
 *							
 *	N o t e s					
 *							
 *	pause issues a startTimer for the desired	
 *	number of milliseconds, then waits for		
 *	the msg. 					
 *							
 ********************************************************/

void pause(long count)
{
	typeMsg * msg;

	msg = makeTimer(count);
	if (msg == NULL) setError("pause - makeTimer error.\n");
		
	msg->msgNum = PAUSE_DONE;
	if(startTimer(msg) == NULL) setError("pause - startTimer error.\n");

	freeMsg(waitMsg(PAUSE_DONE));
}


/********************************************************
 *							
 *	F u n c t i o n   D e f i n i t i o n		
 *							
 *							
 *	P u r p o s e					
 *							
 *	Handle fatal errors.				
 *							
 *							
 *	A r g u m e n t s				
 *							
 *	errMsg - Pointer to error msg string.		
 *							
 *							
 *	N o t e s					
 *							
 *	This function is called whenever a fatal error	
 *	occurs. Interrupts are disabled and the		
 *	error handler pointed to by FatalErrorHandler	
 *	is called. 					
 *							
 ********************************************************/

void setError(char * errMsg)
{
	DI(TRUE);
	FatalErrorHandler(errMsg);
}


/********************************************************
 *							
 *	F u n c t i o n   D e f i n i t i o n		
 *							
 *							
 *	P u r p o s e					
 *							
 *	Called when a task attempts to perform	
 *	a return.					
 *							
 *							
 *	N o t e s					
 *							
 *	Normal tasks must never issue a return. Once	
 *	entered, tasks must remain in an infinite loop.	
 *	This function is called whenever a normal task	
 *	performs a return. 				
 *							
 *							
 ********************************************************/

void taskExitErrorHandler(void)
{
	setError("taskExitErrorHandler - Attempt to return from a task.\n");
}


/********************************************************
 *							
 *	F u n c t i o n   D e f i n i t i o n		
 *							
 *							
 *	P u r p o s e					
 *							
 *	Cancel a msg.					
 *							
 *							
 *	A r g u m e n t s				
 *							
 *	msg - Pointer to msg to cancel.			
 *							
 *	seqNum - The squence number of the 		
 *	msg to cancel.					
 *							
 *							
 *	R e t u r n   V a l u e				
 *							
 *	Returns YES for success, NO if error.		
 *							
 *							
 *	N o t e s					
 *							
 *	A pointer to the msg to cancel is not 		
 *	enough information to delete the msg.		
 *	The msg may have been added back into the free 	
 *	list, for example. seqNum is checked against	
 *	the sequence number of the msg to cancel. 	
 *	If they match, then the msg is removed from 	
 *	its queue. If there is a corresponding entry 	
 *	in the ready queue, it is also deleted. 	
 *	Note also that since the timer list contains 	
 *	relative timers, timerCountDown must be 	
 *	adjusted if the message	to be deleted is a 	
 *	timer msg.					
 *							
 *							
 ********************************************************/

int cancelMsg(typeMsg * msg, long seqNum)
{
	typeMsg * ptr;

	DI(TRUE);
	if (msg->seqNum == (unsigned long) seqNum) {
		if (bitIsSet(msg->flags, TIMER) && 
		!isHeadOrTail(msg->next)) {
			msg->next->ticCountDown += msg->ticCountDown;
		}
		ptr = (typeMsg *) removeQ(msg, FALSE);
		if (ptr == NULL) {
			EI(TRUE);
			return(FALSE);
		}
		if (ptr->readyMsg != NULL) {
			if (ptr->readySeqNum == ptr->readyMsg->seqNum) {
			  putFreeMsg(removeQ(ptr->readyMsg, FALSE), FALSE);
			}
		}
		putFreeMsg(ptr, FALSE);
		EI(TRUE);
		return(TRUE);
	}
	else {
		EI(TRUE);
		return(FALSE);
	}
}


/********************************************************
 *							
 *	F u n c t i o n   D e f i n i t i o n		
 *							
 *							
 *	P u r p o s e					
 *							
 *	Make and initialize a msg.			
 *							
 *							
 *	A r g u m e n t s				
 *							
 *	tcb - Pointer to destination tcb.		
 *							
 *	msgNum - Msg number.				
 *							
 *	diOpt - If TRUE, interrupts are disabled when	
 *	required. This argument should always be TRUE	
 *	unless interrupts are already disabled when 	
 *	this function is called.			
 *							
 *							
 *	R e t u r n   V a l u e				
 *							
 *	Pointer to msg.					
 *							
 ********************************************************/

typeMsg * _makeMsg(typeTcb * tcb,  int msgNum, int diOpt)
{
	typeMsg * msg;

	msg = (typeMsg *) getFreeMsg(diOpt);
	if (msg == NULL || tcb == NULL || 
	msgNum < MIN_MSG || msgNum > MAX_MSG) {
		setError("_makeMsg - Bad msg.\n");
	}

	initMsg(msg, DEF_PRI);
	msg->msgNum = msgNum;
	msg->destTcb = tcb;

	return(msg);
}


/********************************************************
 *							
 *	F u n c t i o n   D e f i n i t i o n		
 *							
 *							
 *	P u r p o s e					
 *							
 *	Make and initialize a timer msg.		
 *							
 *	USE MACRO makeTimer - see Tics.h.
 *
 *							
 *							
 *	A r g u m e n t s				
 *							
 *	startTicCount - Number of tics for timer. 	
 *							
 *	diOpt - If TRUE, interrupts are disabled when	
 *	required. This argument should always be TRUE	
 *	unless interrupts are already disabled when 	
 *	this function is called.			
 *							
 *							
 *	R e t u r n   V a l u e				
 *							
 *	Pointer to timer msg.				
 *							
 ********************************************************/

typeMsg * _makeTimer(long startTicCount, int diOpt)
{
	typeMsg * msg;

	if (startTicCount < MIN_TIMER || 
	startTicCount > MAX_TIMER) {
		setError("_makeTimer - Bad count.\n");
        }

	msg = (typeMsg *) getFreeMsg(diOpt);
	if (msg == NULL) {
		setError("_makeTimer - Out of memory.\n");
	}

	initMsg(msg, DEF_PRI);
	msg->msgNum = TIMEOUT;
	msg->destTcb = actualActiveTcb();
	msg->startTicCount = msg->ticCountDown = startTicCount;

	return(msg);
}


/********************************************************
 *							
 *	F u n c t i o n   D e f i n i t i o n		
 *							
 *							
 *	P u r p o s e					
 *							
 *	Task to manage access to critcal regions.	
 *							
 *							
 *	N o t e s					
 *							
 *	Manages access to critcal regions. Once a task	
 *	"enters" the region with an enterRegion(tcb)	
 *	call, it has exclusive use until it issues an	
 *	exitRegion(tcb). Multiple instances of this	
 *	task may be issued for different regions.	
 *	The "enterDos" and "exitDos" macros use this	
 *	task as described below.			
 *							
 *	Non-reentrant MS-DOS & BIOS cannot be preempted 
 *	& therefore cannot be accessed when timeslicing	
 *	is enabled, or when tasks are run at different	
 *	priorities. I.e., whenever a task can be 	
 *	preempted, access to DOS must be acquired by	
 *	the protocol described below. 			
 *							
 *	requestingTask		regionMgr		
 *	--------------		---------		
 *	    |			 |			
 *	    |------REQUEST------>|			
 * 	    |<-----GRANT---------|			
 *		  ... use critical region ...		
 *	    |------RELEASE------>|			
 *							
 *							
 ********************************************************/

void regionMgr()
{
	typeMsg * msgPtr;
	typeTcb * senderTcb;

	while (TRUE) {          
		msgPtr = waitMsg(REQUEST);
		senderTcb = msgPtr->senderTcb;
		putFreeMsg(msgPtr, TRUE);
		msgPtr = makeMsg(senderTcb, GRANT);
		msgPtr->pri = HI_SYS_PRI;
		sendMsg(msgPtr);
		freeMsg(waitMsg(RELEASE));
	}
}


/********************************************************
 *							
 *	F u n c t i o n   D e f i n i t i o n		
 *							
 *							
 *	P u r p o s e					
 *							
 *	Acquire and release critical regions.		
 *							
 *							
 *	tcb - Before enterRegion and exitRegion can be	
 *	used, an instance of task regionMgr must be	
 *	made and started. tcb points to the tcb for 	
 *	the instance of task regionMgr that manages     
 *	the region. Note that Tics automatically 	
 *	creates an instance of task regionMgr called 	
 *	Dos in startTics. This region can be used to 	
 *	enter and exit DOS regions when required.	
 *							
 *							
 ********************************************************/

void enterRegion(typeTcb * tcb)
{
	if (sendMsg(makeMsg(tcb, REQUEST)) == NULL) {
		setError("enterRegion error");
	}

	freeMsg(waitMsg(GRANT));
}

void exitRegion(typeTcb * tcb)
{
  if (sendMsg(makeMsg(tcb, RELEASE)) == NULL) {
	setError("exitRegion error");
  }
}

/********************************************************
 *							
 *	F u n c t i o n   D e f i n i t i o n		
 *							
 *							
 *	P u r p o s e					
 *							
 *	Initialize a task's tcb and enter the task 	
 *	into the Ready Queue.				
 *							
 *							
 *	A r g u m e n t s				
 *							
 *	tcb - Pointer to tcb.				
 *							
 *	diOpt - If TRUE, interrupts are disabled when	
 *	required. This argument should always be TRUE	
 *	unless interrupts are already disabled when 	
 *	this function is called.			
 *							

 *							
 *	R e t u r n   V a l u e				
 *							
 *	Pointer to the tcb.     			
 *							
 *							
 *	N o t e s					
 *							
 *							
 *	USE MACRO startTask - see Tics.h.
 *
 *	Cooperative tasks are not added to the Ready 	
 *	Queue. (A task without a stack cannot be 	
 *	preempted.) 					
 *							
 *							
 ********************************************************/

typeTcb * _startTask(typeTcb * tcb, int diOpt)
{
	int i;
	typeMsg * headPtr, * tailPtr;

	if (tcb == NULL) { 
		setError("Tics - _startTask - NULL task pointer.\n");
	}

	if (tcb->stackSizeInBytes < MIN_STACK_SIZE) {
		setError("Tics - _startTask - Stack is too small.\n");
	}

	if (tcb->maxNumMsgQueues <= 0) {
		setError("Tics - _startTask - Bad msgQ number.\n");
	}

	DI(diOpt);

	for (i = 0; i < tcb->maxNumMsgQueues; i++) {
		if (!isCoop(tcb) && !isHeadOrTail(tcb)) {
			headPtr = (typeMsg *) getFreeMsg(FALSE);
			tailPtr = (typeMsg *) getFreeMsg(FALSE);
			initMsg(headPtr, HEAD_PRI);
			initMsg(tailPtr, TAIL_PRI);
			initList(headPtr, tailPtr);
			tcb->msgTable[i] = headPtr;
		}
	}

	if (!isCoop(tcb)) {
		initStack(tcb, FALSE);
		addRQ(tcb, FALSE);
	}

	EI(diOpt);

	return(tcb);
}


/********************************************************
 *							
 *	F u n c t i o n   D e f i n i t i o n		
 *							
 *							
 *	P u r p o s e					
 *							
 *	Initialize a mail box.				
 *							
 *							
 *	A r g u m e n t s				
 *							
 *	mail - Pointer to the mailbox to initialize.	
 *							
 *							
 *	N o t e s					
 *							
 *							
 ********************************************************/

void initMail(typeMail * mail)
{
	mail->flags = 0; 
	mail->iData = 0; 
	mail->lData = 0L;
	mail->pData = NULL; 
	mail->mailBoxNum = 0; 
	mail->destTcb = NULL;
	mail->senderTcb = NULL;
}

/********************************************************
 *							
 *	F u n c t i o n   D e f i n i t i o n		
 *							
 *							
 *	P u r p o s e					
 *							
 *	Default fatal error handler.			
 *							
 *							
 *	A r g u m e n t s				
 *							
 *	errMsg - Error message.				
 *							
 *							
 *	N o t e s					
 *							
 *	Called when a fatal error occurs.		
 *	The user may specify his own fatal error	
 *	handler when calling startTics.			
 *							
 *							
 ********************************************************/

void defaultFatalErrorHandler(char * errMsg)
{
	/* Replace code below with your own error handling code. */
    while (TRUE) ;
}


/********************************************************
 *							
 *	F u n c t i o n   D e f i n i t i o n		
 *							
 *							
 *	P u r p o s e					
 *							
 *	Dispatches msgs to cooperative tasks.		
 *							
 *							
 *	N o t e s					
 *							
 *	Cooperative tasks are simply functions. 	
 *	When cooperative tasks send msgs to each other 	
 *	the messages are actually sent to the to 	
 *	task coopMgr. coopMgr then calls the 		
 *	cooperative task with a function call and 	
 *	passes the msg as an argument. 			
 *							
 *	This function and the associated macros		
 *	enter and exit region must be called with 	
 *	interrupts enabled.				
 *							
 ********************************************************/

void coopMgr(void)
{
	typeMsg * msg;
	typeTask2 taskPtr;

	while (TRUE) {          
		msg = waitMsg(ANY_MSG);
		taskPtr = (typeTask2) msg->destTcb->taskPtr;
		ActiveCoopTcb = msg->destTcb;
		if (taskPtr != NULL) taskPtr(msg);
		freeMsg(msg);
	}
}

/********************************************************
 *							
 *	F u n c t i o n   D e f i n i t i o n		
 *							
 *							
 *	P u r p o s e					
 *							
 *	Forward a msg.					
 *							
 *							
 *	N o t e s					
 *							
 *							
 ********************************************************/

 void forwardMsg(typeTcb * tcb, typeMsg * msg)
 {
    msg->destTcb = tcb;
    sendMsg(msg);
 }


/********************************************************
 *							
 *	F u n c t i o n   D e f i n i t i o n		
 *							
 *							
 *	P u r p o s e					
 *							
 *	Reply to a msg using the same msg.		
 *							
 *							
 *	N o t e s					
 *							
 *							
 ********************************************************/

 void replyMsg(typeMsg * msg, int msgNum)
 {
    msg->msgNum = msgNum;
    msg->destTcb = msg->senderTcb;
    msg->senderTcb = ActiveTcb;
    sendMsg(msg);
 }


/********************************************************
 *							
 *	F u n c t i o n   D e f i n i t i o n		
 *							
 *							
 *	P u r p o s e					
 *							
 *	Yield to let other tasks run.			
 *							
 *							
 *	N o t e s					
 *							
 *							
 ********************************************************/

void yield(void)
{
    sendMsg(makeMsg(ActiveTcb, DO_YIELD));
    suspend();
    freeMsg(waitMsg(DO_YIELD));
}


/********************************************************
 *							
 *	F u n c t i o n   D e f i n i t i o n		
 *							
 *							
 *	P u r p o s e					
 *							
 *	Send mail.					
 *							
 *							
 *	A r g u m e n t s				
 *							
 *	mail - Pointer to destination mail structure.	
 *							
 *	diOpt - If TRUE, interrupts are disabled when	
 *	required. This argument should always be TRUE	
 *	unless interrupts are already disabled when 	
 *	this function is called.			
 *							
 *	preeOpt - TRUE if immediate preemption is 	
 *	desired. Current task will suspend and the	
 *	destination task will run if it is of a higher	
 *	priority.					
 *							
 *							
 *	N o t e s					
 *							
 *							
 *	USE MACRO sendMail - see Tics.h.
 *
 *							
 ********************************************************/

void _sendMail(typeMail * mail, int diOpt)
{
	typeMsg * readyMsg;

	if (mail == NULL || mail->destTcb == NULL || 
	mail->mailBoxNum >= LEN_MAILBOXES) {
		setError("Tics - sendMail error.\n");
	}

	DI(diOpt);

	if (_haveMail(mail->destTcb, mail->mailBoxNum) && 
	bitIsSet(mail->flags, OVER_WRITE_DISABLE)) {
		setError("Tics - sendMail error.\n");
	}

	if (!_haveMail(mail->destTcb, mail->mailBoxNum)) {
		setMailBit(mail->destTcb, mail->mailBoxNum);
		readyMsg = addRQ(mail->destTcb, FALSE);
		mail->readyMsg = readyMsg;
		mail->readySeqNum = readyMsg->seqNum;
		if (canPreempt && mail->destTcb->pri < ActiveTcb->pri) {
			preempt(FALSE);
		}
	}

	EI(diOpt);
}


/********************************************************
 *							
 *	F u n c t i o n   D e f i n i t i o n		
 *							
 *							
 *	P u r p o s e					
 *							
 *	Return a pointer to the indicated mail struct.	
 *							
 *							
 *	A r g u m e n t s				
 *							
 *	tcb - The tcb that contains the mailbox.	
 *							
 *	mBoxNum - The mailbox number.			
 *							
 *							
 *	N o t e s					
 *							
 *							
 ********************************************************/

typeMail * makeMail(typeTcb * tcb,  int mailBoxNum)
{
	typeMail * mail;

	if (tcb == NULL || mailBoxNum >= LEN_MAILBOXES) {
		setError("Tics - makeMail error.\n");
	}

	mail = &tcb->mailBoxes[mailBoxNum];

	mail->mailBoxNum = mailBoxNum;
	mail->destTcb = tcb;
	mail->senderTcb = ActiveTcb;

	return(mail);
}


/********************************************************
 *							
 *	F u n c t i o n   D e f i n i t i o n		
 *							
 *							
 *	P u r p o s e					
 *							
 *	Waits for mail for a specific mailbox.		
 *							
 *							
 *	A r g u m e n t s				
 *							
 *	mBoxNum - The mailbox number.			
 *							
 *	diOpt - If TRUE, interrupts will be disabled 	
 *	when required. This argument should always be 	
 *	TRUE unless interrupts are already disabled 	
 *	when this function is called. 			
 *							
 *							
 *	N o t e s					
 *							
 *							
 *	USE MACRO waitMail - see Tics.h.
 *
 *							
 ********************************************************/

typeMail * _waitMail(int mailBoxNum, int diOpt)
{
	typeMail * mail;

	if (mailBoxNum >= LEN_MAILBOXES) {
		setError("Tics - _waitMail error.\n");
	}

	DI(diOpt);
	setBit(ActiveTcb->flags, WAITING);
	while ((mail = _rcvMail(ActiveTcb, mailBoxNum, FALSE)) == NULL) {
		suspend();
        }
	clrBit(ActiveTcb->flags, WAITING);
	EI(diOpt);

	return(mail);
}


/********************************************************
 *							
 *	F u n c t i o n   D e f i n i t i o n		
 *							
 *							
 *	P u r p o s e					
 *							
 *	Extracts mail for a specific mailbox.		
 *							
 *							
 *	A r g u m e n t s				
 *							
 *	tcb - The tcb that contains the mailbox.	
 *							
 *	mBoxNum - The mailbox number.			
 *							
 *	diOpt - If TRUE, interrupts will be disabled 	
 *	when required. This argument should always be 	
 *	TRUE unless interrupts are already disabled 	
 *	when this function is called. 			
 *							
 *							
 *	N o t e s					
 *							
 *							
 *	USE MACRO rcvMail - see Tics.h.
 *
 *							
 ********************************************************/

typeMail * _rcvMail(typeTcb * tcb, int mailBoxNum, int diOpt)
{
	typeMail * mail;

	if (tcb == NULL || mailBoxNum >= LEN_MAILBOXES) {
		setError("Tics - _waitMail error.\n");
	}

	DI(diOpt);
	if (!_haveMail(tcb, mailBoxNum)) {
		mail = NULL;
	}
	else {
		mail = &tcb->mailBoxes[mailBoxNum];
		if (mail->readyMsg != NULL && 
		mail->readySeqNum == mail->readyMsg->seqNum) {
			removeQ(mail->readyMsg, FALSE);
			putFreeMsg(mail->readyMsg, FALSE);
		}
	}
	EI(diOpt);

	return(mail);
}


/********************************************************
 *							
 *	F u n c t i o n   D e f i n i t i o n		
 *							
 *							
 *	P u r p o s e					
 *							
 *	Wait for a message or timeout, whichever	
 *	occurs first.					
 *							
 *							
 *	A r g u m e n t s				
 *							
 *	msgNum - Message number to wait for.		
 *							
 *	timeout - If the message is not received in	
 *	"timeout" milliseconds, this function returns	
 *	a message with message number TIMEOUT. 		
 *	Otherwise, the expected message is returned.	
 *							
 *							
 *	N o t e s					
 *							
 *							
 ********************************************************/

typeMsg * waitTimedMsg(int msgNum, long timeout)
{
	long timerSeqNum;
	typeMsg * timer, * msg;

	timer = makeTimer(timeout);
	timerSeqNum = timer->seqNum;
	startTimer(timer);
		
	while (TRUE) {
		msg = _rcvMsg(ActiveTcb, MSGQ, msgNum, TRUE, TRUE);
		if (msg != NULL) {
			cancelTimer(timer, timerSeqNum);
			return(msg);
		} else {
			msg = _rcvMsg(ActiveTcb, MSGQ, TIMEOUT, TRUE, TRUE);
			if (msg != NULL) return(msg);
		}
		suspend();
	}
    return NULL;
}


/********************************************************
 *							
 *	F u n c t i o n   D e f i n i t i o n		
 *							
 *							
 *	P u r p o s e					
 *							
 *	Make a memory pool.				
 *							
 *							
 *	A r g u m e n t s
 *
 *	mem - Ptr to typeMem struct to manage pool.
 *							
 *	memPtr - Pointer to contiguous memory space,	
 *	provided by caller, that will be used by the	
 *	memory management system.			
 *							
 *	poolSize - The number of bytes in the memory	
 *	space pointed to by memPtr.			
 *							
 *	blkSize - The number of bytes contained in 	
 *	each memory block.				
 *							
 *	diOpt - If TRUE, interrupts will be disabled 	
 *	when required. This argument should always be 	
 *	TRUE unless interrupts are already disabled 	
 *	when this function is called. 			
 *							
 *							
 *	N o t e s					
 *							
 *							
 *	USE MACRO makeMem - see Tics.h.
 *
 *	Returns a pointer to the start of the memory	
 *	pool. A pointer to this pointer is used as 	
 *	an argument to the _getMem and _putMem 		
 *	functions.					
 *							
 ********************************************************/

typeMem * _makeMem(typeMem * memMgr, void * memPtr, int poolSize, int blkSize, int diOpt)
{
	int i, n;
	typeMem * ptr;

	n = poolSize / blkSize;
	ptr = (typeMem *) memPtr;

	if (ptr == NULL || n <= MIN_POOL_SIZE) {
		setError("Tics - makeMem error.\n");
	}

	memMgr->seqNum = 0;
	memMgr->numItemsLeftInPool = n;
	memMgr->numItemsOriginallyInPool = n;
	memMgr->lowWaterMark = DEF_MEM_POOL_LOW_WATER_MARK;
	memMgr->next = ptr;
	
	DI(diOpt);

	for (i = 0; i < n - 1; i++) {
		ptr->next = (typeMem *) (((char *) ptr) + blkSize);
		ptr = ptr->next;
	}
	ptr->next = NULL;
		
	EI(diOpt);

	return(memMgr);
}


/********************************************************
 *							
 *	F u n c t i o n   D e f i n i t i o n		
 *							
 *							
 *	P u r p o s e					
 *							
 *	Get a pointer to a memory block from the 	
 *	memory pool.					
 *							
 *							
 *	A r g u m e n t s				
 *							
 *	memPtr - Pointer to pointer to contiguous 	
 *	memory space, provided by caller, in the	
 *	makeMemPool call.				
 *							
 *	diOpt - If TRUE, interrupts will be disabled 	
 *	when required. This argument should always be 	
 *	TRUE unless interrupts are already disabled 	
 *	when this function is called. 			
 *							
 *							
 *	N o t e s					
 *							
 *							
 *	USE MACRO getMem - see Tics.h.
 *
 *							
 ********************************************************/

void * _getMem(typeMem * memMgr, int diOpt)
{
	typeMem * ptr;

	DI(diOpt);

	if (memMgr->next == NULL) {
		EI(diOpt);
		setError("Tics - _getMem error.\n");
	}

	if (memMgr->numItemsLeftInPool <= memMgr->lowWaterMark) {
		EI(diOpt);
		setError("Tics - _getMem out of memory.\n");
	}

	ptr = memMgr->next;

	memMgr->next = ptr->next;

	memMgr->numItemsLeftInPool--;
	
	EI(diOpt);

	return(ptr);

}

/********************************************************
 *							
 *	F u n c t i o n   D e f i n i t i o n		
 *							
 *							
 *	P u r p o s e					
 *							
 *	Free a memory block back to the memory pool.	
 *							
 *							
 *	A r g u m e n t s				
 *							
 *	memPtr - Pointer to pointer to contiguous 	
 *	memory space, provided by caller, in the	
 *	makeMemPool call.				
 *							
 *	blkPtr - Pointer to memory block to free.	
 *							
 *	diOpt - If TRUE, interrupts will be disabled 	
 *	when required. This argument should always be 	
 *	TRUE unless interrupts are already disabled 	
 *	when this function is called. 			
 *							
 *							
 *	N o t e s					
 *							
 *							
 *	USE MACRO putMem - see Tics.h.
 *
 *							
 ********************************************************/

void _putMem(typeMem * memMgr, void * blkPtr, int diOpt)
{
	DI(diOpt);

	if (memMgr->next == NULL || blkPtr == NULL) {
		EI(diOpt);
		setError("Tics - _putMem error.\n");
	}

	((typeMem *) blkPtr)->next = memMgr->next;

	memMgr->next = (typeMem *) blkPtr;

	if (memMgr->next == NULL) {
		EI(diOpt);
		setError("Tics - _putMem error.\n");
	}

	memMgr->numItemsLeftInPool++;

	EI(diOpt);

}


/********************************************************
 *							
 *	F u n c t i o n   D e f i n i t i o n		
 *							
 *							
 *	P u r p o s e					
 *							
 *	This function initializes a tcb.		
 *							
 *							
 *	A r g u m e n t s				
 *							
 *	tcb - Pointer to the tcb structure to init.	
 *							
 *	taskPtr - Pointer to the task (which is a C	
 *	function).					
 *							
 *	taskNum - User supplied task number.		
 *							
 *      pri - The task priority.			
 *							
 *	nMsgQueues - Number of msg queues connected to	
 *	the task.					
 *							
 *	nMailBoxes - Number of mailboxes connected to	
 *	the task.					
 *							
 *	flags - Flag options. See tcb flags in		
 *	Tics header file.				
 *							
 *	diOpt - If TRUE, interrupts will be disabled 	
 *	when required. This argument should always be 	
 *	TRUE unless interrupts are already disabled 	
 *	when this function is called.			
 *							
 *							
 *	N o t e s					
 *							
 *	Task control/communication is accomplished	
 *	with the task control block (tcb).		
 *							
 ********************************************************/

void initTcb(typeTcb * tcb, typeTask taskPtr, int taskNum, int pri, 
	    int nMsgQueues, int nMailBoxes, int flags, int diOpt)
{
	int i;

	if (nMsgQueues >  LEN_MSG_TABLE || tcb == NULL ||
	taskPtr == NULL) {
		if (pri != HEAD_PRI && pri != TAIL_PRI) {		/* I.e., if this is a head or tail tcb, don't flag an error. */
			setError("initTcb - Parameter error.\n");
		}
	}

        DI(diOpt);

	tcb->timeSlice = DEF_TIME_SLICE;
	tcb->tsCounter = DEF_TIME_SLICE;
	tcb->flags = flags;
	tcb->mailFlags = 0;
	tcb->taskNum = taskNum;
	tcb->taskName = NULL;
	tcb->maxNumMsgQueues = nMsgQueues;
	tcb->maxNumMailBoxes = nMailBoxes;
	tcb->pri = pri;
	tcb->stackSizeInBytes = DEF_STACK_SIZE_IN_BYTES;
	tcb->taskPtr = (typeTask2)taskPtr;

	for (i = 0; i < nMsgQueues; i++) {
		tcb->msgTable[i] = NULL;
	}

	for (i = 0; i < nMailBoxes; i++) {
		initMail(&tcb->mailBoxes[i]);
	}

	EI(diOpt);
}

/********************************************************
 *
 *	F u n c t i o n   D e f i n i t i o n		
 *							
 *							
 *	P u r p o s e					
 *							
 *	This function initializes the Tics system	
 *	and associated data structures.			
 *							
 *							
 *	A r g u m e n t s				
 *							
 *	tics - Pointer to the typeTics data struct 	
 *							
 *	R e t u r n   V a l u e				
 *							
 *	Pointer to a typeTics structure.		
 *							
 *							
 *	N o t e s					
 *							
 *	This function initializes the ready queue, 	
 *	the free list, the timer and keyboard isr's, 	
 *	and starts the tasks: idleTask,			
 *	regionMgr, and coopMgr. idleTask runs		
 *	when no other tasks are ready to run. 		
 *	regionMgr manages access to protected regions.	
 *	coopMgr dispatches msgs to tasks which are 	
 *	started with the COOP flag set.			
 *							
 ********************************************************/

typeTics * startTics(typeTics * tics)
{
	FatalErrorHandler = defaultFatalErrorHandler;

	TicsReadyFlag = FALSE;

	if (tics->msgSpace == NULL) {
		setError("startTics - NULL msg space.\n");
	}

	if (((tics->msgSpaceSizeInBytes) / sizeof(typeMsg)) < MIN_NUM_MSGS) {
		setError("startTics - msg space is too small.\n");
	}

	if (tics->fatalErrorHandler != NULL) {
		FatalErrorHandler = tics->fatalErrorHandler;
	}

	DI(TRUE);

    canPreempt = TRUE;

    doHardwareDependentInits(tics);

	SeqNum = 0L;

	ReadyHead = &ActualReadyHead;
	ReadyTail = &ActualReadyTail;
	TimerHead = &ActualTimerHead;
	TimerTail = &ActualTimerTail;
	initList(ReadyHead, ReadyTail);
	initList(TimerHead, TimerTail);

	initTcb(&ActualReadyHeadTcb, NULL, 0, HEAD_PRI, 0, 0, 0, FALSE);
	initTcb(&ActualReadyTailTcb, NULL, 0, TAIL_PRI, 0, 0, 0, FALSE);

	ReadyHead->destTcb = ActiveTcb = &ActualReadyHeadTcb;
	ReadyTail->destTcb = &ActualReadyTailTcb;

	/* Allocate msg pool. */
	MsgMemPool = _makeMem(&ActualMsgMemPool, Tics->msgSpace, Tics->msgSpaceSizeInBytes,
		sizeof(typeMsg), FALSE);

	/* Allocate tcb pool. */
	TcbMemPool = _makeMem(&ActualTcbMemPool, Tics->tcbSpace, Tics->tcbSpaceSizeInBytes,
		sizeof(typeTcb), FALSE);

	IdleTaskTcb = _makeTask(idleTask, IDLE_TASK_NUM, FALSE);
	IdleTaskTcb->pri = IDLE_PRI;
	IdleTaskTcb->flags |= NO_STACK_CHECK;
        IdleTaskTcb->taskName = "idle";

	_startTask(IdleTaskTcb, FALSE);

	CoopTcb = _makeTask(coopMgr, COOP_TASK_NUM, FALSE);
	CoopTcb->flags |= NO_STACK_CHECK;
        CoopTcb->taskName = "coopmgr";
	_startTask(CoopTcb, FALSE);
	

	TicsReadyFlag = TRUE;

	EI(TRUE);

	return(tics);

}


