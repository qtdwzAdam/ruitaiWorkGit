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

#include <ti/sdo/ce/video/viddec.h>
#include <ti/sdo/ce/video/videnc.h>

#include "video.h"
#include "../demo.h"

#define CAPTURE_PIPE_SIZE       2
#define DISPLAY_PIPE_SIZE       2
#define NUM_VIDEO_BUFS          CAPTURE_PIPE_SIZE + DISPLAY_PIPE_SIZE

/* The masks to use for knowing when a buffer is free */
#define CODEC_FREE              0x1
#define DISPLAY_FREE            0x2
/******************************************************************************
 * videoThrFxn
 ******************************************************************************/
Void *videoThrFxn(Void *arg)
{
}

