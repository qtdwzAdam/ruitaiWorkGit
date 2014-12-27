/*
 * codecs.c
 *
 * ============================================================================
 * Copyright (c) Texas Instruments Inc 2009
 *
 * Use of this software is controlled by the terms and conditions found in the
 * license agreement under which this software has been supplied or provided.
 * ============================================================================
 */

#include <xdc/std.h>

#include "../demo.h"

/*
 * NULL terminated list of video encoders in the engine to use in the demo.
 * Only the first codec will be used in this demo.
 */
static Codec videoEncoders[] = {
    {
        "videnc_copy",        /* String name of codec for CE to locate it */
        "H.264 BP Video", /* The string to show on the UI for this codec */
        NULL,             /* No file extensions as no files used in this demo */
        NULL,             /* Use default params */
        NULL              /* Use default dynamic params */
    },
    { NULL }
};

/*
 * NULL terminated list of video decoders in the engine to use in the demo.
 * Only the first codec will be used in this demo, and should be able to
 * decode what's been encoded.
 */
static Codec videoDecoders[] = {
    {
        "viddec_copy",        /* String name of codec for CE to locate it */
        "H.264 BP Video", /* Should match the string given for encode */
        NULL,             /* No file extensions as no files used in this demo */
        NULL,
        NULL
    },
    { NULL }
};

/* Declaration of the production engine and encoders shipped with the DVSDK */
static Engine encodeDecodeEngine = {
    "video_copy",     /* Engine string name used by CE to find the engine */
    NULL,               /* Speech decoders in engine (not supported) */
    NULL,               /* Audio decoders in engine (not supported) */
    videoDecoders,      /* NULL terminated list of video decoders in engine */
    NULL,               /* Speech encoders in engine (not supported) */
    NULL,               /* Audio encoders in engine (Not supported) */
    videoEncoders       /* NULL terminated list of video encoders in engine */
};

/*
 * This assignment selects which engine will be used by the demo. Note that
 * this file can contain several engine declarations, but this declaration
 * determines which one to use.
 */
Engine *engine = &encodeDecodeEngine;
