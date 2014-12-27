/*
 * demo.h
 *
 * ============================================================================
 * Copyright (c) Texas Instruments Inc 2009
 *
 * Use of this software is controlled by the terms and conditions found in the
 * license agreement under which this software has been supplied or provided.
 * ============================================================================
 */

#ifndef _DEMO_H
#define _DEMO_H

/* Standard Linux headers */
#include <stdio.h>
#include <pthread.h>

/* Error message */
#define ERR(fmt, args...) fprintf(stderr, "Error: " fmt, ## args)

/* Function error codes */
#define SUCCESS             0
#define FAILURE             -1

/* Thread error codes */
#define THREAD_SUCCESS      (Void *) 0
#define THREAD_FAILURE      (Void *) -1

typedef struct Codec {
    /* String name of codec for CE to locate it */
    Char    *codecName;

    /* The string to show on the UI for this codec */ 
    Char    *uiString;

    /* NULL terminated list of file extensions */
    Char   **fileExtensions;

    /* Params to use with codec, if NULL defaults are used */
    Void    *params;

    /* Dynamic params to use with codec, if NULL defaults are used */
    Void    *dynParams;
} Codec;

typedef struct Engine {
    /* Engine string name used by CE to find the engine */
    Char    *engineName;

    /* Speech decoders in engine */
    Codec   *speechDecoders;

    /* Audio decoders in engine */
    Codec   *audioDecoders;

    /* Video decoders in engine */
    Codec   *videoDecoders;

    /* Speech encoders in engine */
    Codec   *speechEncoders;

    /* Audio encoders in engine */
    Codec   *audioEncoders;

    /* Video encoders in engine */
    Codec   *videoEncoders;
} Engine;

/* The engine exported from codecs.c for use in main.c */
extern Engine *engine;

/* Global data structure */
typedef struct GlobalData {
    Int             quit;                /* Global quit flag */
    Int             frames;              /* Video frame counter */
    Int             videoBytesProcessed; /* Video bytes processed counter */
    Int             soundBytesProcessed; /* Sound bytes processed counter */
    Int             samplingFrequency;   /* Sound sampling frequency */
    Int             imageWidth;          /* Width of clip */
    Int             imageHeight;         /* Height of clip */
    pthread_mutex_t mutex;               /* Mutex to protect the global data */
} GlobalData;

#define GBL_DATA_INIT { 0 }

/* Global data */
extern GlobalData gbl;

/* Functions to protect the global data */
static inline Int gblGetQuit(void)
{
    Int quit;

    pthread_mutex_lock(&gbl.mutex);
    quit = gbl.quit;
    pthread_mutex_unlock(&gbl.mutex);

    return quit;
}

static inline Void gblSetQuit(void)
{
    pthread_mutex_lock(&gbl.mutex);
    gbl.quit = TRUE;
    pthread_mutex_unlock(&gbl.mutex);
}

static inline Int gblGetAndResetFrames(void)
{
    Int frames;

    pthread_mutex_lock(&gbl.mutex);
    frames = gbl.frames;
    gbl.frames = 0;
    pthread_mutex_unlock(&gbl.mutex);

    return frames;
}

static inline Void gblIncFrames(void)
{
    pthread_mutex_lock(&gbl.mutex);
    gbl.frames++;
    pthread_mutex_unlock(&gbl.mutex);
}

static inline Int gblGetAndResetVideoBytesProcessed(void)
{
    Int videoBytesProcessed;

    pthread_mutex_lock(&gbl.mutex);
    videoBytesProcessed = gbl.videoBytesProcessed;
    gbl.videoBytesProcessed = 0;
    pthread_mutex_unlock(&gbl.mutex);

    return videoBytesProcessed;
}

static inline Void gblIncVideoBytesProcessed(Int videoBytesProcessed)
{
    pthread_mutex_lock(&gbl.mutex);
    gbl.videoBytesProcessed += videoBytesProcessed;
    pthread_mutex_unlock(&gbl.mutex);
}

static inline Int gblGetAndResetSoundBytesProcessed(void)
{
    Int soundBytesProcessed;

    pthread_mutex_lock(&gbl.mutex);
    soundBytesProcessed = gbl.soundBytesProcessed;
    gbl.soundBytesProcessed = 0;
    pthread_mutex_unlock(&gbl.mutex);

    return soundBytesProcessed;
}

static inline Void gblIncSoundBytesProcessed(Int soundBytesProcessed)
{
    pthread_mutex_lock(&gbl.mutex);
    gbl.soundBytesProcessed += soundBytesProcessed;
    pthread_mutex_unlock(&gbl.mutex);
}

static inline Int gblGetSamplingFrequency(void)
{
    Int samplingFrequency;

    pthread_mutex_lock(&gbl.mutex);
    samplingFrequency = gbl.samplingFrequency;
    pthread_mutex_unlock(&gbl.mutex);

    return samplingFrequency;
}

static inline Void gblSetSamplingFrequency(Int samplingFrequency)
{
    pthread_mutex_lock(&gbl.mutex);
    gbl.samplingFrequency = samplingFrequency;
    pthread_mutex_unlock(&gbl.mutex);
}

static inline Int gblGetImageWidth(void)
{
    Int imageWidth;

    pthread_mutex_lock(&gbl.mutex);
    imageWidth = gbl.imageWidth;
    pthread_mutex_unlock(&gbl.mutex);

    return imageWidth;
}

static inline Void gblSetImageWidth(Int imageWidth)
{
    pthread_mutex_lock(&gbl.mutex);
    gbl.imageWidth = imageWidth;
    pthread_mutex_unlock(&gbl.mutex);
}

static inline Int gblGetImageHeight(void)
{
    Int imageHeight;

    pthread_mutex_lock(&gbl.mutex);
    imageHeight = gbl.imageHeight;
    pthread_mutex_unlock(&gbl.mutex);

    return imageHeight;
}

static inline Void gblSetImageHeight(Int imageHeight)
{
    pthread_mutex_lock(&gbl.mutex);
    gbl.imageHeight = imageHeight;
    pthread_mutex_unlock(&gbl.mutex);
}

/* Cleans up cleanly after a failure */
#define cleanup(x)                                  \
    status = (x);                                   \
    gblSetQuit();                                   \
    goto cleanup

#endif /* _DEMO_H */
