/*
 * =====================================================================================
 *
 *       Filename:  display.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2014年12月24日 14时01分04秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Adam_zju 
 *   Organization:  
 *
 * =====================================================================================
 */

#ifndef display.h
#define display.h
#include <xdc/std.h>

#include <ti/sdo/dmai/Fifo.h>
#include <ti/sdo/dmai/Pause.h>
#include <ti/sdo/dmai/Display.h>
#include <ti/sdo/dmai/Rendezvous.h>

/* Environment passed when creating the thread */
typedef struct DisplayEnv {
    Display_Output          displayOutput;
    VideoStd_Type           videoStd;
    Rendezvous_Handle       hRendezvousInit;
    Rendezvous_Handle       hRendezvousCleanup;
    Pause_Handle            hPauseProcess;
    Pause_Handle            hPausePrime;
    Fifo_Handle             hOutFifo;
    Fifo_Handle             hInFifo;
    Int                     osd;
} DisplayEnv;

/* Thread function prototype */
extern Void *displayThrFxn(Void *arg);




#endif

