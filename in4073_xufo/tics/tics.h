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


#include <stdio.h>
#include <stdlib.h>


 /********************************************************
 *							
 *	M i s c .   d e f i n e s			
 *							
 ********************************************************/

#define MIN_POOL_SIZE 		5
#define LEN_MSG_TABLE		1
#define LEN_MAILBOXES		3
#define LEN_INTS			4
#define LEN_LONGS			2
#define LEN_CHARS			16
#define DEF_MS_PER_TIMER_INTERRUPT 10
#define MIN_NUM_MSGS		3 * (4 * DEF_NUM_MSG_QUEUES) + 20
#define MIN_STACK_SIZE		256
#define MIN_STACK_LEVEL		1000
#define DEF_STACK_SIZE_IN_BYTES	1024
#define DEF_TCB_FLAGS		0
#define DEF_MEM_POOL_LOW_WATER_MARK 5
#define DEF_NUM_MSG_QUEUES	1
#define DEF_NUM_MAILBOXES	LEN_MAILBOXES
#define LOW_SYS_PRI			LOW_USER_PRI
#define HI_SYS_PRI			10
#define LOW_USER_PRI		100
#define HI_USER_PRI			20
#define DEF_PRI				((LOW_USER_PRI + HI_USER_PRI) / 2)
#define MAX_PRI				(LOW_USER_PRI + 100)
#define MIN_PRI				HI_SYS_PRI
#define IDLE_TASK_NUM		10000
#define ACQDF_TASK_NUM		10001
#define COOP_TASK_NUM		10002
#define ACQDF_PRI			HI_SYS_PRI
#define IDLE_PRI			LOW_USER_PRI + 1
#define HEAD_PRI			HI_SYS_PRI - 1
#define TAIL_PRI			IDLE_PRI + 1
#define MAX_TIMER			0x0fffffffL
#define MIN_TIMER			1
#define MIN_MSG				-999
#define MAX_MSG				32000
#define DEF_TIME_SLICE		50
#define TRUE				1
#define FALSE				0
#define	PC_5MS 				5966
#define	PC_10MS 			(2 * PC_5MS)
#define	uS_PER_PC_TIC 		55000L
#define	TIMERQ				0
#define MSGQ			TIMERQ


/********************************************************
 *							
 *	S y s t e m   M s g  I d s			
 *							
 ********************************************************/

#define FIRST_MSG		-1
#define ANY_MSG			FIRST_MSG
#define TIMEOUT			-2
#define REQUEST			-3
#define GRANT			-4
#define RELEASE			-5
#define DONE			-6
#define PAUSE_DONE		-7
#define WAKEUP			-8
#define GO				-9
#define DO_YIELD		-10


/********************************************************
 *							
 *	T c b   F l a g s 				
 *							
 *	These flag bits are contained in each Task 	
 *	Control Block structure (typeTcb) in the 	
 *	"flags" field. They specify options for the 	
 *	task instance.					
 *							
 ********************************************************/

#define TIMESLICE		1 /* Set if task can be preempted in order
						   * to run another task.
			               */

#define WAITING			2 /* Set if waiting for msg. Disables	
						   * time-slicing, even if time		
						   * slicing is turned on.
			               */

#define PREEMPT_DOS		4 /* Preempt and run this tcb even if 	
						   * in DOS.
			               */

#define COOP			8 /* Set if task has no stack.		*/

#define NO_STACK_CHECK	16 /* Set to disable stack checking each	
							* time a task switch occurs.
			                */

#define TERMINATE	    32 /* Set to terminate task on next 	
							* task switch. (Not implemented in 3.1)
			                */


/********************************************************
 *							
 *	Q u e u e   M s g   F l a g s 			
 *							
 *	These flag bits are contained in each message	
 *	structure (typeMsg) in the "flags" field. 	
 *	They specify options for the message. 		
 *							
 ********************************************************/

#define	NO_MSG_NOTIFY		1 /* TRUE if receiver task is not to 	
							   * be scheduled when msg is put into 	
							   * queue.
			                   */
#define PERIODIC			2 /* Set for periodic timer. */

#define TIMER				4 /* System use only. States that msg  	
							   * is a timer msg.
			                   */
#define CALL_BACK			8 /* Indicates that a timer function ptr
							   * in the timer msg is to be called
			                   * directly from the timer isr.
                               * (Not implmented in version 3.1)
			                   */
 

/********************************************************
 *							
 *	M a i l b o x   F l a g s 			
 *							
 *	These flag bits are contained in each mailbox	
 *	structure (typeMail) in the "flags" field. 	
 *	They specify options for the mailbox. Do not	
 *	confuse these flags with the "mailFlags" field	
 *	of the task control block, which indicate 	
 *	if mail is available for a particular mailbox.	
 *							
 ********************************************************/

#define OVER_WRITE_DISABLE	1	/* TRUE to disable mail over-writes */



/********************************************************
 *							
 *	M a c r o s					
 *							
 ********************************************************/

#define LOW(x) 				(0x00ff & x)
#define HIGH(x) 			((0xff00 & x) >> 8)

#define ticsMax(a,b) ((a > b) ? a : b)

#define DI(x)	 if (x == TRUE) DISABLE_INTERRUPT(INTERRUPT_GLOBAL)
#define EI(x)	 if (x == TRUE) ENABLE_INTERRUPT(INTERRUPT_GLOBAL)

#define bitIsSet(word, mask) (word & mask)
#define bitIsClr(word, mask) (!bitIsSet(word, mask))
#define setBit(word, mask) word |= mask
#define clrBit(word, mask) word &=  ~mask

/* Move a bit field defined by 1's in mask from src to dest. */
#define moveBitField(dest, src, mask) dest = ((dest & ~mask) | (src & mask))

#define MS_TO_TICS(numMs) (numMs / MsPerTimerInt)

#define isIdleTask(xtcb) (xtcb->pri == IDLE_PRI)

#define preempt(opt) addRQ(ActiveTcb, opt); suspend()

#define _isrPreempt(opt) addRQ(ActiveTcb, opt); suspend()

#define _isrRet() \
if (!isTail(ReadyHead->next) && ReadyHead->next->pri < ActiveTcb->pri) { \
  _isrPreempt(FALSE); \
}

#define isrEnter() canPreempt=FALSE

#define isHead(x) (x->pri == HEAD_PRI)

#define isTail(x) (x->pri == TAIL_PRI)

#define isHeadOrTail(x) (isHead(x) || isTail(x))

#define isEmpty(x) (isTail(x->next))

#define freeMsg(mp) putFreeMsg(mp, TRUE)

#define _freeMsg(mp,diOpt) putFreeMsg(mp, diOpt)

#define makeMsg(tcb, msgNum) _makeMsg(tcb, msgNum, TRUE)

#define startTimer(msg) \
_startTimer(msg, TRUE)

#define send(tcb, msgNum) \
sendMsg(makeMsg(tcb, msgNum))

#define timer(count) \
startTimer(makeTimer(count))

#define startTask(tcb) _startTask(tcb, TRUE)

#define start(taskPtr, taskNum) \
startTask(makeTask(taskPtr, taskNum))

#define sendMsg(msgPtr) \
_sendMsg(msgPtr, TRUE)

#define waitMsg(msgNum) _waitMsg(MSGQ, msgNum, TRUE, TRUE)

#define cancelTimer(msg, seqNum) cancelMsg(msg, seqNum)

#define rcvMsg(msgNum) _rcvMsg(ActiveTcb, MSGQ, msgNum, TRUE, TRUE)

#define MAIL_BIT ((unsigned long) 1L)
#define MAIL_MASK ((unsigned long) 0xffffffffL)

#define sendMail(mail) \
_sendMail(mail, TRUE)

#define setMailBit(tcb, mBoxNum) \
(setBit(tcb->mailFlags, (MAIL_BIT << mBoxNum)))

#define clrMailBit(tcb, mBoxNum) \
(clrBit(tcb->mailFlags, (MAIL_BIT << mBoxNum)))

#define _haveMail(tcb, mBoxNum) \
(bitIsSet(tcb->mailFlags, (MAIL_BIT << mBoxNum)))

#define haveMail(mBoxNum) _haveMail(ActiveTcb, mBoxNum)

#define freeMail(mailPtr) clrMailBit(ActiveTcb, mailPtr->mailBoxNum)

#define rcvMail(mBoxNum) _rcvMail(ActiveTcb, mBoxNum, TRUE)

#define waitMail(mBoxNum) _waitMail(mBoxNum, TRUE)

#define wakeup(tcb) _wakeup(tcb, TRUE)

#define makeTask(taskPtr, taskNum) \
_makeTask((typeTask) taskPtr, taskNum, TRUE)

#define makeTimer(count) _makeTimer(MS_TO_TICS(count), TRUE)

#define isCoop(tcb) (bitIsSet(tcb->flags, COOP))

#define actualActiveTcb() (ActiveTcb == CoopTcb ? ActiveCoopTcb : ActiveTcb)

#define actualTcbForMsg(tcb) (isCoop(tcb) ? CoopTcb : tcb)

#define makeMem(memPtr,poolSize,blkSize) \
_makeMem(memPtr, poolSize, blkSize, TRUE)


#define noStack(tcb) (bitIsSet(tcb->flags, NO_STACK))


/********************************************************
 *							
 *	T y p e d e f s					
 *							
 ********************************************************/

typedef void (* typeTask)(void);
typedef void (* typeCallBack)(void);
typedef void (* typeIsr)(void);

typedef struct structMsg {
	unsigned long seqNum;
	struct structMsg * next, * prev, * readyMsg;
	int	flags;
	int	pri;
	unsigned long readySeqNum;
	struct structTcb * senderTcb;
	struct structTcb * destTcb;
	char	* destName;
	int	msgQNum;
	int	msgNum;
	int	iData;
	long	lData;
	void 	* pData;
	typeCallBack callBackFunction;
	int	ints[LEN_INTS];
	long	longs[LEN_LONGS];
	char	chars[LEN_CHARS];
	long	startTicCount;
	long	ticCountDown;
} typeMsg;

typedef void (* typeTask2)(typeMsg *);

typedef struct structMail {
	int	flags;
	int	mailBoxNum;
	int	iData;
	long	lData;
	void	* pData;
	struct structTcb * senderTcb, * destTcb;
	struct structMsg * readyMsg;
	unsigned long readySeqNum;
} typeMail;

typedef struct structTcb {
	unsigned long 	seqNum;
	int		flags;
	int		pri;
	unsigned long	mailFlags;
	void		* ptr;
	typeTask2 		 taskPtr;
	char		* taskName;
    void       ** context;
	void       ** stackBottom;
	void       ** stackTop;
	int		taskNum;
	int		stackSizeInBytes;
	int		timeSlice;
	int		tsCounter;
	int		maxNumMsgQueues;
	int		maxNumMailBoxes;
	typeMsg		* msgTable[LEN_MSG_TABLE];
	typeMail	mailBoxes[LEN_MAILBOXES];
} typeTcb;


typedef struct structTics {
	typeMsg * msgSpace;	
	int msgSpaceSizeInBytes;	
	int stackSpaceSizeInBytes;
	typeTcb * tcbSpace;
	int tcbSpaceSizeInBytes;
	int timerChipCount;
	long uSPerTimerChipCount;
	void **stackTopAddress, **stackBottomAddress;
	void (* fatalErrorHandler)(char * errMsg);
} typeTics;

typedef struct structMem {
	unsigned long 	seqNum;
	int numItemsLeftInPool;
	int numItemsOriginallyInPool;
	int lowWaterMark;			/* If we get lower than this number of items in the pool - error. */
	struct structMem * next; 	/* Pool is maintained as a linked list. */
}typeMem;


typedef int typeStack;


/********************************************************
 *							
 *	P r o t o t y p e s				
 *							
 ********************************************************/

void * removeQ(typeMsg * msg, int options);
void initMsg(typeMsg * msg, int pri);
void taskSwitch(void);
void exitTics(void);
void initTimer(unsigned int timerChipCount, int options);
void pause(long timeDelayInMs);
void setError(char * errMsg);
void addPQ(typeMsg * head, typeMsg * newMsg, int options);
typeMsg * addRQ(typeTcb * tcb, int options);
void addQ(typeMsg * msg, typeMsg * newMsg, int options);
typeTics * makeTics(typeMsg * msgSpace, int msgSpaceSizeInBytes,
		unsigned char * stackSpace, int stactkSpaceSizeInBytes,
		typeTcb * tcbSpace, int tcbSpaceSizeInBytes);
typeTics * startTics(typeTics * tics);
void regionMgr();
int cancelTimer(typeMsg * msg, long timerNum);
void initKeyboard(int options);
int canTimeslice(void);
int cancelMsg(typeMsg * msg, long msgNum);
void ticsPcExit(int exitCode);
typeTcb * _startTask(typeTcb * tcb, int diOpt);
typeMsg * _waitMsg(int msgQNum, int msgNum, int diOpt, int remOpt);
typeMsg * _makeTimer(long startTicCount, int diOpt);
void _sendMail(typeMail * mail, int diOpt);
typeMail * _waitMail(int mailBoxNum, int diOpt);
typeMail * _rcvMail(typeTcb * tcb, int mailBoxNum, int diOpt);
typeTcb * _makeTask(typeTask taskPtr, int taskNum, int options);
typeMsg * _makeMsg(typeTcb * tcb,  int msgNum, int diOpt);
typeMsg * _rcvMsg(typeTcb * tcb, int msgQNum, int msgNum, int diOpt,
int rcvOpt);
void initTcb(typeTcb * tcb, typeTask taskPtr, int taskNum, int pri, 
	    int nMsgQueues, int nMailBoxes, int flags, int diOpt);
typeMsg * _sendMsg(typeMsg * msg, int options);
typeMsg * _startTimer(typeMsg * msg, int options);
void enterRegion(typeTcb * tcb);
void exitRegion(typeTcb * tcb);
void ticsSched(void);
void initStack(typeTcb * tcb, int diOpt);
void defaultFatalErrorHandler(char * errMsg);
void initList(typeMsg * head, typeMsg * tail);
typeTcb * removeRQ(int options);
void _wakeup(typeTcb * tcb, int diOpt);
void idleTask(void);
void addPQ(typeMsg * head, typeMsg * newMsg, int options);
void taskExitErrorHandler(void);
void coopMgr(void);
void taskExitErrorHandler(void);
void initMail(typeMail * mail);
typeMail * makeMail(typeTcb * tcb,  int mailBoxNum);
typeMem * _makeMem(typeMem * mem, void * memPtr, int poolSize, int blkSize, int diOpt);
void * _getMem(typeMem * memMgr, int diOpt);
void _putMem(typeMem * memMgr, void * blkPtr, int diOpt);
typeMsg * waitTimedMsg(int msgNum, long timeout);
void forwardMsg(typeTcb * tcb, typeMsg * msg);
 void reply(typeMsg * msg, int msgNum);
int getCurrentStackPointer(void);
void setSoftwareInterruptVector(typeIsr ticsSoftwareInterruptIsr);
void ticsSoftwareInterruptIsr(void);
void initTimerInTimerCounts(int timerNum, int intervalInTimerCounts);
void initTimerInMicroSeconds(int timerNum, int intervalInMicroSeconds);
void initTimerInMs(int timerNum, int intervalInTimerCounts);
void forwardMsg(typeTcb * tcb, typeMsg * msg);
void replyMsg(typeMsg * msg, int msgNum);
typeMsg * getFreeMsg(int diOpt);
void putFreeMsg(typeMsg * msgToFree, int diOpt);
void suspend(void);
void yield(void);
void doHardwareDependentInits(typeTics * tics);
void ticsTimerIsr(void);
void ticsKeyboardIsr(void);
void ctrlCIsr(void);
void isrRet(void);


extern int canPreempt;
