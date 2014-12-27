/*
 * =====================================================================================
 *
 *       Filename:  capture.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2014年12月24日 14时50分08秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Adam_zju 
 *   Organization:  
 *
 * =====================================================================================
 */

#ifndef capture.h
#define capture.h

#include <xdc/std.h>

#include <ti/sdo/dmai/Fifo.h>
#include <ti/sdo/dmai/Pause.h>
#include <ti/sdo/dmai/Capture.h>
#include <ti/sdo/dmai/Rendezvous.h>

/* Environment passed when creating the thread */
typedef struct CaptureEnv {
    Rendezvous_Handle       hRendezvousInit;
    Rendezvous_Handle       hRendezvousCapStd;
    Rendezvous_Handle       hRendezvousCleanup;
    Rendezvous_Handle       hRendezvousPrime;
    Pause_Handle            hPauseProcess;
    Fifo_Handle             hOutFifo;
    Fifo_Handle             hInFifo;
    Capture_Input           videoInput;
    Int32                   imageWidth;
    Int32                   imageHeight;
    Int                     deinterlace;
} CaptureEnv;

/* Thread function prototype */
extern Void *captureThrFxn(Void *arg);



#endif

