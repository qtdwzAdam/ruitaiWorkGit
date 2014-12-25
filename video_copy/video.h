/*
 * =====================================================================================
 *
 *       Filename:  video.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2014年12月24日 14时50分54秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Adam_zju 
 *   Organization:  
 *
 * =====================================================================================
 */

#ifndef video.h
#define video.h

#include <xdc/std.h>

#include <ti/sdo/dmai/Fifo.h>
#include <ti/sdo/dmai/Pause.h>
#include <ti/sdo/dmai/Rendezvous.h>

/* Environment passed when creating the thread */
typedef struct VideoEnv {
    Rendezvous_Handle       hRendezvousInit;
    Rendezvous_Handle       hRendezvousCleanup;
    Pause_Handle            hPauseProcess;
    Pause_Handle            hPausePrime;
    Fifo_Handle             hDisplayInFifo;
    Fifo_Handle             hDisplayOutFifo;
    Fifo_Handle             hCaptureInFifo;
    Fifo_Handle             hCaptureOutFifo;
    Char                   *videoEncoder;
    Void                   *encParams;
    Void                   *encDynParams;
    Char                   *videoDecoder;
    Void                   *decParams;
    Void                   *decDynParams;
    Char                   *engineName;
    Int32                   imageWidth;
    Int32                   imageHeight;
    Int                     videoBitRate;
    Int                     passThrough;
} VideoEnv;

/* Thread function prototype */
extern Void *videoThrFxn(Void *arg);



#endif

