/*
 * =====================================================================================
 *
 *       Filename:  display.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2014年12月24日 13时58分20秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Adam_zju 
 *   Organization:  
 *
 * =====================================================================================
 */
#include <stdio.h>
#include <string.h>

#include <xdc/std.h>

#include <ti/sdo/dmai/Framecopy.h>
#include <ti/sdo/dmai/Fifo.h>
#include <ti/sdo/dmai/Pause.h>
#include <ti/sdo/dmai/Blend.h>
#include <ti/sdo/dmai/Display.h>
#include <ti/sdo/dmai/BufferGfx.h>
#include <ti/sdo/dmai/Rendezvous.h>

#include "display.h"
#include "../demo.h"

/* Video display is triple buffered */
#define NUM_DISPLAY_BUFS         3

/******************************************************************************
 * displayThrFxn
 ******************************************************************************/
Void *displayThrFxn(Void *arg)
{
    DisplayEnv             *envp       = (DisplayEnv *) arg;
    Display_Attrs           dAttrs     = Display_Attrs_DM6446_DM355_VID_DEFAULT;
    Framecopy_Attrs         fcAttrs    = Framecopy_Attrs_DEFAULT;
    Display_Handle          hDisplay   = NULL;
    Framecopy_Handle        hFc        = NULL;
    Void                   *status     = THREAD_SUCCESS;
    Uns                     frameCnt   = 0;
    Int32                   clipWidth  = 0;
    Int32                   clipHeight = 0;
    Int32                   srcWidth   = 0;
    Int32                   srcHeight  = 0;
    Int32                   dstWidth   = 0;
    Int32                   dstHeight  = 0;
    Int32                   dstX       = 0;
    Int32                   dstY       = 0;
    Buffer_Handle           hSrcBuf, hDstBuf;
    BufferGfx_Dimensions    srcDim, dstDim;
    Int                     fifoRet;

    /* Create the display device instance */
    dAttrs.numBufs = NUM_DISPLAY_BUFS;
    dAttrs.videoStd = envp->videoStd;
    dAttrs.videoOutput = envp->displayOutput;
    hDisplay = Display_create(NULL, &dAttrs);

    if (hDisplay == NULL) {
        ERR("Failed to create display device\n");
        cleanup(THREAD_FAILURE);
    }

    /* Create the frame copy job */
    fcAttrs.accel = TRUE;
    hFc = Framecopy_create(&fcAttrs);

    if (hFc == NULL) {
        ERR("Failed to create frame copy job\n");
        cleanup(THREAD_FAILURE);
    }

    /* Signal that initialization is done and wait for other threads */
    Rendezvous_meet(envp->hRendezvousInit);

    while (!gblGetQuit()) {
        /* Pause processing? */
        Pause_test(envp->hPauseProcess);

        /* Pause for priming? */
        Pause_test(envp->hPausePrime);

        fifoRet = Fifo_get(envp->hInFifo, &hSrcBuf);

        if (fifoRet < 0) {
            ERR("Failed to get buffer from video thread\n");
            cleanup(THREAD_FAILURE);
        }

        /* Did the video thread flush the fifo? */
        if (fifoRet == Dmai_EFLUSH) {
            cleanup(THREAD_SUCCESS);
        }

        BufferGfx_getDimensions(hSrcBuf, &srcDim);

        /* Get a buffer from the display device driver */
        if (Display_get(hDisplay, &hDstBuf) < 0) {
            ERR("Failed to get display buffer\n");
            cleanup(THREAD_FAILURE);
        }

        BufferGfx_getDimensions(hDstBuf, &dstDim);

        /* Alter the position of the image to center it */
        if (srcDim.width != clipWidth || srcDim.height != clipHeight) {
            clipWidth = srcDim.width;
            clipHeight = srcDim.height;

            /* Clamp the width and center if clip is wider than screen */
            if (srcDim.width > dstDim.width) {
                dstWidth   = dstDim.width;
                srcWidth   = dstDim.width;
                dstX       = 0;
            }
            else {
                dstWidth   = srcDim.width;
                srcWidth   = srcDim.width;
                dstX       = ((dstDim.width - srcDim.width) / 2) & ~0xf;
            }

            /* Clamp the height if clip is higher than screen */
            if (srcDim.height > dstDim.height) {
                dstHeight  = dstDim.height;
                srcHeight  = dstDim.height;
                dstY       = 0;
            }
            else {
                dstHeight  = srcDim.height;
                srcHeight  = srcDim.height;
                dstY       = (dstDim.height - srcDim.height) / 2;
            }

            dstDim.x       = dstX;
            dstDim.y       = dstY;
            dstDim.width   = dstWidth;
            dstDim.height  = dstHeight;

            BufferGfx_setDimensions(hDstBuf, &dstDim);

            srcDim.width    = srcWidth;
            srcDim.height   = srcHeight;

            BufferGfx_setDimensions(hSrcBuf, &srcDim);

            if (Framecopy_config(hFc, hSrcBuf, hDstBuf) < 0) {
                ERR("Failed to configure frame copy job\n");
                cleanup(THREAD_FAILURE);
            }
        }

        srcDim.width    = srcWidth;
        srcDim.height   = srcHeight;
        BufferGfx_setDimensions(hSrcBuf, &srcDim);

        dstDim.width    = dstWidth;
        dstDim.height   = dstHeight;
        dstDim.x        = dstX;
        dstDim.y        = dstY;
        BufferGfx_setDimensions(hDstBuf, &dstDim);

        /* Copy the decoded buffer from the video thread to the display. */
        if (Framecopy_execute(hFc, hSrcBuf, hDstBuf) < 0) {
            ERR("Failed to execute frame copy job\n");
            cleanup(THREAD_FAILURE);
        }

        BufferGfx_resetDimensions(hSrcBuf);
        BufferGfx_resetDimensions(hDstBuf);

        /* Send buffer back to the video thread */
        if (Fifo_put(envp->hOutFifo, hSrcBuf) < 0) {
            ERR("Failed to send buffer to video thread\n");
            cleanup(THREAD_FAILURE);
        }

        /* Incremement statistics for the user interface */
        gblIncFrames();

        /* Give a filled buffer back to the display device driver */
        if (Display_put(hDisplay, hDstBuf) < 0) {
            ERR("Failed to put display buffer\n");
            cleanup(THREAD_FAILURE);
        }

        frameCnt++;
    }

cleanup:
    /* Make sure the other threads aren't waiting for us */
    Rendezvous_force(envp->hRendezvousInit);
    Pause_off(envp->hPauseProcess);
    Pause_off(envp->hPausePrime);
    Fifo_flush(envp->hOutFifo);

    /* Meet up with other threads before cleaning up */
    Rendezvous_meet(envp->hRendezvousCleanup);

    /* Clean up the thread before exiting */
    if (hFc) {
        Framecopy_delete(hFc);
    }

    if (hDisplay) {
        Display_delete(hDisplay);
    }

    return status;
}
