/*
 * =====================================================================================
 *
 *       Filename:  capture.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2014年12月25日 15时13分07秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Adam_zju 
 *   Organization:  
 *
 * =====================================================================================
 */

#include <xdc/std.h>

#include <ti/sdo/dmai/Fifo.h>
#include <ti/sdo/dmai/Pause.h>
#include <ti/sdo/dmai/Smooth.h>
#include <ti/sdo/dmai/Capture.h>
#include <ti/sdo/dmai/VideoStd.h>
#include <ti/sdo/dmai/Framecopy.h>
#include <ti/sdo/dmai/BufferGfx.h>
#include <ti/sdo/dmai/Rendezvous.h>

#include "capture.h"
#include "../demo.h"
#include "../ui.h"

/* Triple buffering for the capture driver */
#define NUM_CAPTURE_BUFS         3

/******************************************************************************
 * captureThrFxn
 ******************************************************************************/
Void *captureThrFxn(Void *arg)
{
    CaptureEnv          *envp           = (CaptureEnv *) arg;
    Void                *status         = THREAD_SUCCESS;
    Capture_Attrs        cAttrs         = Capture_Attrs_DM6446_DM355_DEFAULT;
    Framecopy_Attrs      fcAttrs        = Framecopy_Attrs_DEFAULT;
    Smooth_Attrs         smAttrs        = Smooth_Attrs_DEFAULT;
    Capture_Handle       hCapture       = NULL;
    Framecopy_Handle     hFc            = NULL;
    Smooth_Handle        hSmooth        = NULL;
    BufferGfx_Dimensions dim;
    VideoStd_Type        videoStd;
    Buffer_Handle        hDstBuf, hCapBuf;
    Int32                width, height;
    Int                  fifoRet;

    printf ( "Inside the captureThrFxn\n" );
    /* Create capture device driver instance */
    cAttrs.numBufs = NUM_CAPTURE_BUFS;
    cAttrs.videoInput = envp->videoInput;
    cAttrs.smoothPad = envp->deinterlace ? TRUE : FALSE;

    if (Capture_detectVideoStd(NULL, &videoStd, &cAttrs) < 0) {
        ERR("Failed to detect video standard, video input connected?\n");
        cleanup(THREAD_FAILURE);
    }

    /* We only support D1 input */
    if (videoStd != VideoStd_D1_NTSC &&
        videoStd != VideoStd_D1_PAL) {

        ERR("Need D1 input to this demo\n");
        cleanup(THREAD_FAILURE);
    }

    if (envp->imageWidth > 0 && envp->imageHeight > 0) {
        if (VideoStd_getResolution(videoStd, &width, &height) < 0) {
            ERR("Failed to calculate resolution of video standard\n");
            cleanup(THREAD_FAILURE);
        }

        if (width < envp->imageWidth && height < envp->imageHeight) {
            ERR("User resolution (%ldx%ld) larger than detected (%ldx%ld)\n",
                envp->imageWidth, envp->imageHeight, width, height);
            cleanup(THREAD_FAILURE);
        }

        /* Crop the center of the captured image */
        cAttrs.cropX      = (width - envp->imageWidth) / 2 & ~1;
        cAttrs.cropY      = (height - envp->imageHeight) / 2;
        cAttrs.cropWidth  = envp->imageWidth;
        cAttrs.cropHeight = envp->imageHeight;
    }
    else {
        /* Calculate the dimensions of a video standard given a color space */
        if (BufferGfx_calcDimensions(videoStd,
                                     ColorSpace_UYVY, &dim) < 0) {
            ERR("Failed to calculate Buffer dimensions\n");
            cleanup(THREAD_FAILURE);
        }

        envp->imageWidth = dim.width;
        envp->imageHeight = dim.height;
    }

    /* Report the video standard and image size back to the main thread */
    Rendezvous_meet(envp->hRendezvousCapStd);

    hCapture = Capture_create(NULL, &cAttrs);

    if (hCapture == NULL) {
        ERR("Failed to create capture device\n");
        cleanup(THREAD_FAILURE);
    }

    /* Get a buffer from the video thread */
    fifoRet = Fifo_get(envp->hInFifo, &hDstBuf);

    if (fifoRet < 0) {
        ERR("Failed to get buffer from video thread\n");
        cleanup(THREAD_FAILURE);
    }

    if (fifoRet == Dmai_EFLUSH) {
        cleanup(THREAD_SUCCESS);
    }

    /* Create frame copy job or smooth job */
    if (envp->deinterlace) {
        hSmooth = Smooth_create(&smAttrs);

        if (hSmooth == NULL) {
            ERR("Failed to create frame copy job\n");
            cleanup(THREAD_FAILURE);
        }

        /* Configure frame copy job */
        if (Smooth_config(hSmooth,
                          BufTab_getBuf(Capture_getBufTab(hCapture), 0),
                          hDstBuf) < 0) {
            ERR("Failed to configure smooth job\n");
            cleanup(THREAD_FAILURE);
        }
    }
    else {
        fcAttrs.accel = TRUE;
        hFc = Framecopy_create(&fcAttrs);

        if (hFc == NULL) {
            ERR("Failed to create frame copy job\n");
            cleanup(THREAD_FAILURE);
        }

        /* Configure frame copy job */
        if (Framecopy_config(hFc,
                             BufTab_getBuf(Capture_getBufTab(hCapture), 0),
                             hDstBuf) < 0) {
            ERR("Failed to configure frame copy job\n");
            cleanup(THREAD_FAILURE);
        }
    }

    /* Signal that initialization is done and wait for other threads */
    Rendezvous_meet(envp->hRendezvousInit);

    while (!gblGetQuit()) {
        /* Get a buffer from the capture driver to encode */
        if (Capture_get(hCapture, &hCapBuf) < 0) {
            ERR("Failed to get capture buffer\n");
            cleanup(THREAD_FAILURE);
        }

        /* Copy captured frame into destination buffer */
        if (envp->deinterlace) {
            if (Smooth_execute(hSmooth, hCapBuf, hDstBuf) < 0) {
                ERR("Failed to execute smooth job\n");
                cleanup(THREAD_FAILURE);
            }
        }
        else {
            if (Framecopy_execute(hFc, hCapBuf, hDstBuf) < 0) {
                ERR("Failed to execute frame copy job\n");
                cleanup(THREAD_FAILURE);
            }
        }

        /* Send color converted buffer to video thread for encoding */
        if (Fifo_put(envp->hOutFifo, hDstBuf) < 0) {
            ERR("Failed to send buffer to display thread\n");
            cleanup(THREAD_FAILURE);
        }

        /* Return a buffer to the capture driver */
        if (Capture_put(hCapture, hCapBuf) < 0) {
            ERR("Failed to put capture buffer\n");
            cleanup(THREAD_FAILURE);
        }

        /* Pause processing? */
        Pause_test(envp->hPauseProcess);

        /* Get a buffer from the video thread */
        fifoRet = Fifo_get(envp->hInFifo, &hDstBuf);

        if (fifoRet < 0) {
            ERR("Failed to get buffer from video thread\n");
            cleanup(THREAD_FAILURE);
        }

        /* Did the video thread flush the fifo? */
        if (fifoRet == Dmai_EFLUSH) {
            cleanup(THREAD_SUCCESS);
        }
    }

cleanup:
    /* Make sure the other threads aren't waiting for us */
    Rendezvous_force(envp->hRendezvousCapStd);
    Rendezvous_force(envp->hRendezvousInit);
    Pause_off(envp->hPauseProcess);
    Fifo_flush(envp->hOutFifo);

    /* Meet up with other threads before cleaning up */
    Rendezvous_meet(envp->hRendezvousCleanup);

    /* Clean up the thread before exiting */
    if (hFc) {
        Framecopy_delete(hFc);
    }

    if (hSmooth) {
        Smooth_delete(hSmooth);
    }

    if (hCapture) {
        Capture_delete(hCapture);
    }

    return status;
}

