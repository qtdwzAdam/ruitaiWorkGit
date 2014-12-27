/*
 * =====================================================================================
 *
 *       Filename:  video.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2014年12月24日 16时58分20秒
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

#include <ti/sdo/ce/Engine.h>
#include <ti/sdo/ce/osal/Memory.h>

#include <ti/sdo/dmai/Fifo.h>
#include <ti/sdo/dmai/Pause.h>
#include <ti/sdo/dmai/BufTab.h>
#include <ti/sdo/dmai/VideoStd.h>
#include <ti/sdo/dmai/BufferGfx.h>
#include <ti/sdo/dmai/Rendezvous.h>
#include <ti/sdo/dmai/ce/Vdec2.h>
#include <ti/sdo/dmai/ce/Venc1.h>

#include "video.h"
#include "../demo.h"

#define CAPTURE_PIPE_SIZE       2
#define DISPLAY_PIPE_SIZE       2
#define NUM_VIDEO_BUFS          CAPTURE_PIPE_SIZE + DISPLAY_PIPE_SIZE

/* The masks to use for knowing when a buffer is free */
#define CODEC_FREE              0x1
#define DISPLAY_FREE            0x2

/******************************************************************************
 * encodedecode
 ******************************************************************************/
static Int encodedecode(Venc1_Handle hVe1, Vdec2_Handle hVd2,
                        Buffer_Handle hVidBuf, Buffer_Handle hEncBuf,
                        Fifo_Handle displayFifo)
{
    Buffer_Handle hOutBuf, hFreeBuf;
    Int           ret;

    /* Make sure the whole buffer is used for input and output */
    BufferGfx_resetDimensions(hVidBuf);

    /* Encode the video buffer */
    if (Venc1_process(hVe1, hVidBuf, hEncBuf) < 0) {
        ERR("Failed to encode video buffer\n");
        return FAILURE;
    }

    if (Buffer_getNumBytesUsed(hEncBuf) == 0) {
        ERR("Encoder created 0 sized output frame\n");
        return FAILURE;
    }

    /* Update global data for user interface */
    gblIncVideoBytesProcessed(Buffer_getNumBytesUsed(hEncBuf));

    /* Decode the video buffer */
    ret = Vdec2_process(hVd2, hEncBuf, hVidBuf);

    if (ret != Dmai_EOK) {
        ERR("Failed to decode video buffer\n");
        return FAILURE;
    }

    /* Send display frames to display thread */
    hOutBuf = Vdec2_getDisplayBuf(hVd2);
    while (hOutBuf) {
        if (Fifo_put(displayFifo, hOutBuf) < 0) {
            ERR("Failed to send buffer to display thread\n");
            return FAILURE;
        }

        hOutBuf = Vdec2_getDisplayBuf(hVd2);
    }

    /* Free up released frames */
    hFreeBuf = Vdec2_getFreeBuf(hVd2);
    printf ( "\nhFreeBuf- pre = %d\n\n", hFreeBuf );
    while (hFreeBuf) {
        Buffer_freeUseMask(hFreeBuf, CODEC_FREE);
        hFreeBuf = Vdec2_getFreeBuf(hVd2);
        printf ( "hFreeBuf = %d\n", hFreeBuf );
    }

    return SUCCESS;
}

/******************************************************************************
 * resizeBufTab
******************************************************************************/
static Int resizeBufTab(Vdec2_Handle hVd2, Int displayBufs)
{
    BufTab_Handle hBufTab = Vdec2_getBufTab(hVd2);
    Int numBufs, numCodecBuffers, numExpBufs;
    Buffer_Handle hBuf;
    Int32 frameSize;

    /* How many buffers can the codec keep at one time? */
    numCodecBuffers = Vdec2_getMinOutBufs(hVd2);
    printf ( "numCodecBuffers = %d \n", numCodecBuffers);

    if (numCodecBuffers < 0) {
        ERR("Failed to get buffer requirements\n");
        return FAILURE;
    }

    /*
     * Total number of frames needed are the number of buffers the codec
     * can keep at any time, plus the number of frames in the display pipe.
     */
    numBufs = numCodecBuffers + displayBufs;

    /* Get the size of output buffers needed from codec */
    frameSize = Vdec2_getOutBufSize(hVd2);

    /*
     * Get the first buffer of the BufTab to determine buffer characteristics.
     * All buffers in a BufTab share the same characteristics.
     */
    hBuf = BufTab_getBuf(hBufTab, 0);

    /* Do we need to resize the BufTab? */
    if (numBufs > BufTab_getNumBufs(hBufTab) ||
        frameSize < Buffer_getSize(hBuf)) {

        /* Should we break the current buffers in to many smaller buffers? */
        if (frameSize < Buffer_getSize(hBuf)) {

            /* First undo any previous chunking done */
            BufTab_collapse(Vdec2_getBufTab(hVd2));

            /*
             * Chunk the larger buffers of the BufTab in to smaller buffers
             * to accomodate the codec requirements.
             */
            numExpBufs = BufTab_chunk(hBufTab, numBufs, frameSize);

            if (numExpBufs < 0) {
                ERR("Failed to chunk %d bufs size %ld to %d bufs size %ld\n",
                    BufTab_getNumBufs(hBufTab), Buffer_getSize(hBuf),
                    numBufs, frameSize);
                return FAILURE;
            }

            /*
             * Did the current BufTab fit the chunked buffers,
             * or do we need to expand the BufTab (numExpBufs > 0)?
             */
            if (BufTab_expand(hBufTab, numExpBufs) < 0) {
                ERR("Failed to expand BufTab with %d buffers\n",
                    numExpBufs);
                return FAILURE;
            }
        }
        else {
            /* Just expand the BufTab with more buffers */
            if (BufTab_expand(hBufTab, numCodecBuffers) < 0) {
                ERR("Failed to expand BufTab with %d buffers\n",
                    numCodecBuffers);
                return FAILURE;
            }
        }
    }

    return numBufs;
}

/******************************************************************************
 * videoThrFxn
 ******************************************************************************/
Void *videoThrFxn(Void *arg)
{
    VideoEnv               *envp                 = (VideoEnv *) arg;
    Void                   *status               = THREAD_SUCCESS;
    VIDDEC2_Params          defaultDecParams     = Vdec2_Params_DEFAULT;
    VIDDEC2_DynamicParams   defaultDecDynParams  = Vdec2_DynamicParams_DEFAULT;
    VIDENC1_Params          defaultEncParams     = Venc1_Params_DEFAULT;
    VIDENC1_DynamicParams   defaultEncDynParams  = Venc1_DynamicParams_DEFAULT;
    BufferGfx_Attrs         gfxAttrs             = BufferGfx_Attrs_DEFAULT;
    Buffer_Attrs            bAttrs               = Buffer_Attrs_DEFAULT;
    Vdec2_Handle            hVd2                 = NULL;
    Venc1_Handle            hVe1                 = NULL;
    BufTab_Handle           hBufTab              = NULL;
    Engine_Handle           hEngine              = NULL;
    Buffer_Handle           hEncBuf              = NULL;
    Int                     ret                  = Dmai_EOK;
    Int                     numBufs              = 0;
    Buffer_Handle           hVidBuf, hDispBuf, hDstBuf;
    VIDDEC2_Params         *decParams;
    VIDDEC2_DynamicParams  *decDynParams;
    VIDENC1_Params         *encParams;
    VIDENC1_DynamicParams  *encDynParams;
    Int32                   bufSize;
    Int                     fifoRet;
    Int                     bufIdx;

    /* Use supplied params if any, otherwise use defaults */
    decParams = envp->decParams ? envp->decParams : &defaultDecParams;
    decDynParams = envp->decDynParams ? envp->decDynParams :
                                        &defaultDecDynParams;

    encParams = envp->encParams ? envp->encParams : &defaultEncParams;
    encDynParams = envp->encDynParams ? envp->encDynParams :
                                        &defaultEncDynParams;

    gblSetImageWidth(envp->imageWidth);
    gblSetImageHeight(envp->imageHeight);

    gfxAttrs.colorSpace     = ColorSpace_UYVY;
    gfxAttrs.dim.width      = envp->imageWidth;
    gfxAttrs.dim.height     = envp->imageHeight;
    gfxAttrs.dim.lineLength = BufferGfx_calcLineLength(gfxAttrs.dim.width,
                                                       gfxAttrs.colorSpace);

    if (envp->passThrough) {
        bufSize = gfxAttrs.dim.lineLength * gfxAttrs.dim.height;

        /* Only the display thread can own a buffer since no codecs are used */
        gfxAttrs.bAttrs.useMask = DISPLAY_FREE;
    }
    else {
        /* Open the codec engine */
        hEngine = Engine_open(envp->engineName, NULL, NULL);
        printf ( "engineName is : %d\n" , envp->engineName);

        if (hEngine == NULL) {
            ERR("Failed to open codec engine %s\n", envp->engineName);
            cleanup(THREAD_FAILURE);
        }

        /* Set the resolution to match the specified resolution */
        encParams->maxWidth          = envp->imageWidth;
        encParams->maxHeight         = envp->imageHeight;
        encParams->inputChromaFormat = XDM_YUV_422ILE;

        /* Set up codec parameters depending on bit rate */
        if (envp->videoBitRate < 0) {
            /* Variable bit rate */
            encParams->rateControlPreset = IVIDEO_NONE;

            /*
             * If variable bit rate use a bogus bit rate value (> 0)
             * since it will be ignored.
             */
            encParams->maxBitRate        = 2000000;
        }
        else {
            /* Constant bit rate */
            encParams->rateControlPreset = IVIDEO_LOW_DELAY;
            encParams->maxBitRate        = envp->videoBitRate;
        }

        encDynParams->targetBitRate = encParams->maxBitRate;
        encDynParams->inputWidth    = encParams->maxWidth;
        encDynParams->inputHeight   = encParams->maxHeight;

        /* Create the video encoder */
        hVe1 = Venc1_create(hEngine, envp->videoEncoder,
                            encParams, encDynParams);
        if (hVe1 == NULL) {
            ERR("Failed to create video encoder: %s\n", envp->videoEncoder);
            cleanup(THREAD_FAILURE);
        }

        decParams->maxWidth          = encParams->maxWidth;
        decParams->maxHeight         = encParams->maxHeight;
        decParams->forceChromaFormat = XDM_YUV_422ILE;

        /* Create the video decoder */
        hVd2 = Vdec2_create(hEngine, envp->videoDecoder,
                            decParams, decDynParams);

        if (hVd2 == NULL) {
            ERR("Failed to create video decoder: %s\n", envp->videoDecoder);
            cleanup(THREAD_FAILURE);
        }

        /* Which output buffer size does the codec require? */
        bufSize = Vdec2_getOutBufSize(hVd2);

        /* Allocate buffer for encoded data */
        hEncBuf = Buffer_create(Vdec2_getInBufSize(hVd2), &bAttrs);

        if (hEncBuf == NULL) {
            ERR("Failed to allocate buffer for encoded data\n");
            cleanup(THREAD_FAILURE);
        }

        /* A buffer can be owned by either the codec or the display thread */
        gfxAttrs.bAttrs.useMask = CODEC_FREE | DISPLAY_FREE;
    }

    /* Allocate video buffers */
    hBufTab = BufTab_create(NUM_VIDEO_BUFS, bufSize,
                            BufferGfx_getBufferAttrs(&gfxAttrs));

    if (hBufTab == NULL) {
        ERR("Failed to create BufTab for display pipe\n");
        cleanup(THREAD_FAILURE);
    }

    if (!envp->passThrough) {
        /* The codec is going to use this BufTab for output buffers */
        Vdec2_setBufTab(hVd2, hBufTab);
    }

    /* Prime the capture pipe with buffers */
    for (bufIdx = 0; bufIdx < NUM_VIDEO_BUFS; bufIdx++) {
        hDstBuf = BufTab_getFreeBuf(hBufTab);

        if (hDstBuf == NULL) {
            ERR("Failed to get free buffer from BufTab\n");
            BufTab_print(hBufTab);
            cleanup(THREAD_FAILURE);
        }

        if (Fifo_put(envp->hCaptureInFifo, hDstBuf) < 0) {
            ERR("Failed to send buffer to display thread\n");
            cleanup(THREAD_FAILURE);
        }
    }

    /* Make sure the display thread is stopped when it's unlocked */
    Pause_on(envp->hPausePrime);

    /* Signal that initialization is done and wait for other threads */
    Rendezvous_meet(envp->hRendezvousInit);

    /* Process one buffer to figure out buffer requirements */
    fifoRet = Fifo_get(envp->hCaptureOutFifo, &hVidBuf);

    if (fifoRet < 0) {
        ERR("Failed to get buffer from video thread\n");
        cleanup(THREAD_FAILURE);
    }

    /* Did the capture thread flush the fifo? */
    if (fifoRet == Dmai_EFLUSH) {
        cleanup(THREAD_SUCCESS);
    }

    if (!envp->passThrough) {
        /*
         * Encode and decode the buffer from the capture thread and
         * send any display buffers to the display thread.
         */
        ret = encodedecode(hVe1, hVd2, hVidBuf, hEncBuf, envp->hDisplayInFifo);

        if (ret < 0) {
            cleanup(THREAD_FAILURE);
        }

        /*
         * Resize the BufTab after the first frame has been processed.
         * This because the codec may not know it's buffer requirements
         * before the first frame has been decoded.
         */
        numBufs = resizeBufTab(hVd2, NUM_VIDEO_BUFS);

        /* Send any additional buffers to the capture thread */
        for (bufIdx = NUM_VIDEO_BUFS; bufIdx < numBufs; bufIdx++) {
            /* Get a free buffer from the BufTab */
            hDstBuf = BufTab_getFreeBuf(hBufTab);

            if (hDstBuf == NULL) {
                ERR("Failed to get free buffer from BufTab\n");
                BufTab_print(hBufTab);
                cleanup(THREAD_FAILURE);
            }

            if (Fifo_put(envp->hCaptureInFifo, hDstBuf) < 0) {
                ERR("Failed to send buffer to display thread\n");
                cleanup(THREAD_FAILURE);
            }
        }
    }
    else {
        /* Send the buffer through to the display thread unmodified */
        if (Fifo_put(envp->hDisplayInFifo, hVidBuf) < 0) {
            ERR("Failed to send buffer to display thread\n");
            cleanup(THREAD_FAILURE);
        }
    }

    /* Prime the display thread */
    for (bufIdx=0; bufIdx < numBufs - CAPTURE_PIPE_SIZE  - 1; bufIdx++) {
        if (ret != Dmai_EFIRSTFIELD) {
            /* Get a video buffer from the capture thread */
            fifoRet = Fifo_get(envp->hCaptureOutFifo, &hVidBuf);

            if (fifoRet < 0) {
                ERR("Failed to get buffer from video thread\n");
                cleanup(THREAD_FAILURE);
            }

            /* Did the capture thread flush the fifo? */
            if (fifoRet == Dmai_EFLUSH) {
                cleanup(THREAD_SUCCESS);
            }
        }

        if (!envp->passThrough) {
            /*
             * Encode and decode the buffer from the capture thread and
             * send any display buffers to the display thread.
             */
            ret = encodedecode(hVe1, hVd2, hVidBuf, hEncBuf,
                               envp->hDisplayInFifo);

            if (ret < 0) {
                cleanup(THREAD_FAILURE);
            }
        }
        else {
            /* Send the buffer through to the display thread unmodified */
            if (Fifo_put(envp->hDisplayInFifo, hVidBuf) < 0) {
                ERR("Failed to send buffer to display thread\n");
                cleanup(THREAD_FAILURE);
            }
        }
    }

    /* Release the display thread, it is now fully primed */
    Pause_off(envp->hPausePrime);

    /* Main loop */
    while (!gblGetQuit()) {
        /* Get a buffer from the capture thread */
        fifoRet = Fifo_get(envp->hCaptureOutFifo, &hVidBuf);

        if (fifoRet < 0) {
            ERR("Failed to get buffer from capture thread\n");
            cleanup(THREAD_FAILURE);
        }

        /* Did the capture thread flush the fifo? */
        if (fifoRet == Dmai_EFLUSH) {
            cleanup(THREAD_SUCCESS);
        }

        if (!envp->passThrough) {
            /*
             * Encode and decode the buffer from the capture thread and
             * send any display buffers to the display thread.
             */
            ret = encodedecode(hVe1, hVd2, hVidBuf, hEncBuf,
                               envp->hDisplayInFifo);

            if (ret < 0) {
                cleanup(THREAD_FAILURE);
            }
        }
        else {
            /* Send the buffer through to the display thread unmodified */
            if (Fifo_put(envp->hDisplayInFifo, hVidBuf) < 0) {
                ERR("Failed to send buffer to display thread\n");
                cleanup(THREAD_FAILURE);
            }
        }

        /* Get a buffer from the display thread to send to the capture thread */
        if (ret != Dmai_EFIRSTFIELD) {
            do {
                fifoRet = Fifo_get(envp->hDisplayOutFifo, &hDispBuf);

                if (fifoRet < 0) {
                    ERR("Failed to get buffer from video thread\n");
                    cleanup(THREAD_FAILURE);
                }

                /* Did the display thread flush the fifo? */
                if (fifoRet == Dmai_EFLUSH) {
                    cleanup(THREAD_SUCCESS);
                }

                /* The display thread is no longer using the buffer */
                Buffer_freeUseMask(BufTab_getBuf(hBufTab,
                                                 Buffer_getId(hDispBuf)),
                                                 DISPLAY_FREE);

                /* Get a free buffer */
                hDstBuf = BufTab_getFreeBuf(hBufTab);

                if (hDstBuf == NULL) {
                    ERR("Failed to get free buffer from BufTab\n");
                    BufTab_print(hBufTab);
                    cleanup(THREAD_FAILURE);
                }

                /*
                 * Reset the dimensions of the buffer in case the decoder has
                 * changed them due to padding.
                 */
                BufferGfx_resetDimensions(hDstBuf);

                /* Send a buffer to the capture thread */
                if (Fifo_put(envp->hCaptureInFifo, hDstBuf) < 0) {
                    ERR("Failed to send buffer to capture thread\n");
                    cleanup(THREAD_FAILURE);
                }

            } while (Fifo_getNumEntries(envp->hDisplayOutFifo) > 0);
        }
    }

cleanup:
    /* Make sure the other threads aren't waiting for us */
    Rendezvous_force(envp->hRendezvousInit);
    Pause_off(envp->hPauseProcess);
    Pause_off(envp->hPausePrime);
    Fifo_flush(envp->hDisplayInFifo);

    /* Meet up with other threads before cleaning up */
    Rendezvous_meet(envp->hRendezvousCleanup);

    /* Clean up the thread before exiting */
    if (hEncBuf) {
        Buffer_delete(hEncBuf);
    }

    if (hVd2) {
        Vdec2_delete(hVd2);
    }

    if (hVe1) {
        Venc1_delete(hVe1);
    }

    if (hEngine) {
        Engine_close(hEngine);
    }

    if (hBufTab) {
        BufTab_delete(hBufTab);
    }

    return status;
}

