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
 *	E x t e r n a l s		
 *							
 ********************************************************/

extern int TicsReadyFlag;
extern int TicsInKeyboardCount;
extern void** CurrentSp;
extern unsigned long SeqNum;
extern unsigned int TimerTicsPerPcTic;
extern long uSPerTimerChipTic;
extern unsigned int MsPerTimerInt;
extern typeTcb * ActiveTcb, * ActiveCoopTcb, * IdleTaskTcb, * Dos, * CoopTcb;
extern typeMsg * TimerHead, * TimerTail;
extern typeMsg * ReadyHead, * ReadyTail;
extern typeMsg ActualReadyHead, ActualReadyTail;
extern typeMsg ActualTimerHead, ActualTimerTail;
extern typeTcb ActualReadyHeadTcb,  ActualReadyTailTcb;
extern void (* FatalErrorHandler)(char * errMsg);
extern typeMem ActualTcbMemPool;
extern typeMem ActualMsgMemPool;
extern typeMem * TcbMemPool;
extern typeMem * MsgMemPool;
extern unsigned int Tctr;
extern typeTics * Tics;
