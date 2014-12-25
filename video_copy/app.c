#include <xdc/std.h>
#include <ti/sdo/ce/Engine.h>
#include <ti/sdo/ce/osal/Memory.h>
#include <ti/sdo/ce/video/viddec.h>
#include <ti/sdo/ce/video/videnc.h>
#include <ti/sdo/ce/trace/gt.h>

#include <ti/sdo/dmai/Dmai.h>
#include <ti/sdo/dmai/Fifo.h>
#include <ti/sdo/dmai/Pause.h>
#include <ti/sdo/dmai/Capture.h>
#include <ti/sdo/dmai/BufferGfx.h>
#include <ti/sdo/dmai/Rendezvous.h>


#include "display.h"
#include "capture.h"
#include "video.h"
#include "../demo.h"
#include "../ctrl.h"

#include <string.h>  /* for memset */

#include <stdio.h>
#include <stdlib.h>

/* The levels of initialization */
#define LOGSINITIALIZED         0x1
#define DISPLAYTHREADCREATED    0x2
#define CAPTURETHREADCREATED    0x4
#define VIDEOTHREADCREATED      0x8

/* Thread priorities */
#define CAPTURE_THREAD_PRIORITY sched_get_priority_max(SCHED_FIFO) - 1
#define VIDEO_THREAD_PRIORITY   sched_get_priority_max(SCHED_FIFO) - 1
#define DISPLAY_THREAD_PRIORITY sched_get_priority_max(SCHED_FIFO)

typedef struct Args Args;
struct Args {
    Display_Output          displayOutput;
    VideoStd_Type           videoStd;
    Char                   *videoStdString;
    Int                     videoBitRate;
    Capture_Input           videoInput;
    Int                     passThrough;
    Int                     keyboard;
    Int                     time;
    Int                     osd;
    Int                     interface;
    Int                     deinterlace;
    Int32                   imageWidth;
    Int32                   imageHeight;
};

#define DEFAULT_ARGS { Display_Output_COMPOSITE, VideoStd_D1_NTSC, "D1 NTSC", \
                       -1, Capture_Input_COMPOSITE, FALSE, FALSE, \
                       FOREVER, FALSE, FALSE, TRUE, 0, 0 }

/* Global variable declarations for this application */
GlobalData gbl = GBL_DATA_INIT;


/*
 * If an XDAIS algorithm _may_ use DMA, buffers provided to it need to be
 * aligned on a cache boundary.
 */

#ifdef CACHE_ENABLED

/*
 * If buffer alignment isn't set on the compiler's command line, set it here
 * to a default value.
 */
#ifndef BUFALIGN
#define BUFALIGN 128
#endif
#else

/* Not a cached system, no buffer alignment constraints */
#define BUFALIGN Memory_DEFAULTALIGNMENT

#endif


#define NSAMPLES    1024  /* must be multiple of 128 for cache/DMA reasons */
#define IFRAMESIZE  (NSAMPLES * sizeof(Int8))  /* raw frame (input) */
#define EFRAMESIZE  (NSAMPLES * sizeof(Int8))  /* encoded frame */
#define OFRAMESIZE  (NSAMPLES * sizeof(Int8))  /* decoded frame (output) */
static XDAS_Int8 *inBuf;
static XDAS_Int8 *encodedBuf;
static XDAS_Int8 *outBuf;

static String progName     = "app";
static String decoderName  = "viddec_copy";
static String encoderName  = "videnc_copy";
static String engineName   = "video_copy";

static String usage = "%s: input-file output-file\n";

static Void encode_decode(VIDENC_Handle enc, VIDDEC_Handle dec, FILE *in,
    FILE *out);

extern GT_Mask curMask;

/*
 *  ======== createInFileIfMissing ========
 */
static void createInFileIfMissing( char *inFileName )
{
    int i;
    FILE *f = fopen(inFileName, "rb");
    if (f == NULL) {
        printf( "Input file '%s' not found, generating one.\n", inFileName );
        f = fopen( inFileName, "wb" );
        for (i = 0; i < 1024; i++) {
            fwrite( &i, sizeof( i ), 1, f );
        }
    }
    fclose( f );
}

/*
 *  ======== smain ========
 */
Int smain(Int argc, String argv[])
{
    Engine_Handle ce = NULL;
    VIDDEC_Handle dec = NULL;
    VIDENC_Handle enc = NULL;
    FILE *in = NULL;
    FILE *out = NULL;
    String inFile, outFile;
    Memory_AllocParams allocParams;

    Args                    args                = DEFAULT_ARGS;
    Uns                     initMask            = 0;
    Int                     status              = EXIT_SUCCESS;
    Pause_Attrs             pAttrs              = Pause_Attrs_DEFAULT;
    Rendezvous_Attrs        rzvAttrs            = Rendezvous_Attrs_DEFAULT;
    Fifo_Attrs              fAttrs              = Fifo_Attrs_DEFAULT;
    Rendezvous_Handle       hRendezvousCapStd   = NULL;
    Rendezvous_Handle       hRendezvousInit     = NULL;
    Rendezvous_Handle       hRendezvousend  = NULL;
    Pause_Handle            hPauseProcess       = NULL;
    Pause_Handle            hPausePrime         = NULL;
    UI_Handle               hUI                 = NULL;
    struct sched_param      schedParam;
    Int                     numThreads;
    pthread_t               videoThread;
    pthread_t               displayThread;
    pthread_t               captureThread;
    pthread_attr_t          attr;
    VideoEnv                videoEnv;
    CaptureEnv              captureEnv;
    DisplayEnv              displayEnv;
    CtrlEnv                 ctrlEnv;
    Void                   *ret;

    /* Zero out the thread environments */
    Dmai_clear(videoEnv);
    Dmai_clear(captureEnv);
    Dmai_clear(displayEnv);
    Dmai_clear(ctrlEnv);

    /* Parse the arguments given to the app and set the app environment */
//    parseArgs(argc, argv, &args);

    printf("Encodedecode demo started.\n");

    /* Initialize the mutex which protects the global data */
    pthread_mutex_init(&gbl.mutex, NULL);

    /* Set the priority of this whole process to max (requires root) */
//    setpriority(PRIO_PROCESS, 0, -20);

    /* Initialize Davinci Multimedia Application Interface */
    Dmai_init();

    /* Initialize the logs. Must be done after CERuntime_init() */
//    TraceUtil_start(engine->engineName);

    initMask |= LOGSINITIALIZED;

    /* Set up the user interface */
//    hUI = uiSetup(&args);

//    if (hUI == NULL) {
//        end(EXIT_FAILURE);
//    }
    /* reset, load, and start DSP Engine */
    if ((ce = Engine_open(engine->engineName, NULL, NULL)) == NULL) {
        fprintf(stderr, "%s: error: can't open engine %s\n",
            progName, engineName);
        goto end;
    }

    /* allocate and initialize video decoder on the engine */
    dec = VIDDEC_create(ce, decoderName, NULL);
    if (dec == NULL) {
        printf( "App-> ERROR: can't open codec %s\n", decoderName);
        goto end;
    }
    printf ( "successed in create the viddec \n" );

    /* allocate and initialize video encoder on the engine */
    enc = VIDENC_create(ce, encoderName, NULL);
    if (enc == NULL) {
        fprintf(stderr, "%s: error: can't open codec %s\n",
            progName, encoderName);
        goto end;
    }

    /* Create the Pause objects */
    hPauseProcess = Pause_create(&pAttrs);
    hPausePrime = Pause_create(&pAttrs);

    if (hPauseProcess == NULL || hPausePrime == NULL) {
        ERR("Failed to create Pause objects\n");
        end(EXIT_FAILURE);
    }

    numThreads = 4;

    /* Create the objects which synchronizes the thread init and end */
    hRendezvousCapStd  = Rendezvous_create(2, &rzvAttrs);
    hRendezvousInit = Rendezvous_create(numThreads, &rzvAttrs);
    hRendezvousend = Rendezvous_create(numThreads, &rzvAttrs);

    if (hRendezvousCapStd  == NULL || hRendezvousInit == NULL || 
        hRendezvousend == NULL) {
        ERR("Failed to create Rendezvous objects\n");
        end(EXIT_FAILURE);
    }



    if (argc <= 1) {
        inFile = "./in.dat";
        outFile = "./out.dat";
        createInFileIfMissing(inFile);
    }
    else if (argc == 3) {
        progName = argv[0];
        inFile = argv[1];
        outFile = argv[2];
    }
    else {
        fprintf(stderr, usage, argv[0]);
        exit(1);
    }

    GT_0trace(curMask, GT_1CLASS, "App-> Application started.\n");

    /* allocate input, encoded, and output buffers */
    allocParams.type = Memory_CONTIGPOOL;
    allocParams.flags = Memory_NONCACHED;
    allocParams.align = BUFALIGN;
    allocParams.seg = 0;

    inBuf = (XDAS_Int8 *)Memory_alloc(IFRAMESIZE, &allocParams);
    encodedBuf = (XDAS_Int8 *)Memory_alloc(EFRAMESIZE, &allocParams);
    outBuf = (XDAS_Int8 *)Memory_alloc(OFRAMESIZE, &allocParams);

    if ((inBuf == NULL) || (encodedBuf == NULL) || (outBuf == NULL)) {
        goto end;
    }

    /* open file streams for input and output */
    if ((in = fopen(inFile, "rb")) == NULL) {
        printf("App-> ERROR: can't read file %s\n", inFile);
        goto end;
    }
    if ((out = fopen(outFile, "wb")) == NULL) {
        printf("App-> ERROR: can't write to file %s\n", outFile);
        goto end;
    }

    
    /* use engine to encode, then decode the data */
    encode_decode(enc, dec, in, out);

end:
    /* teardown the codecs */
    if (enc) {
        VIDENC_delete(enc);
    }
    if (dec) {
        VIDDEC_delete(dec);
    }

    /* close the engine */
    if (ce) {
        Engine_close(ce);
    }

    /* close the files */
    if (in) {
        fclose(in);
    }
    if (out) {
        fclose(out);
    }

    /* free buffers */
    if (inBuf) {
        Memory_free(inBuf, IFRAMESIZE, &allocParams);
    }
    if (encodedBuf) {
        Memory_free(encodedBuf, EFRAMESIZE, &allocParams);
    }
    if (outBuf) {
        Memory_free(outBuf, OFRAMESIZE, &allocParams);
    }

    GT_0trace(curMask, GT_1CLASS, "app done.\n");
    return (0);
}

/*
 *  ======== encode_decode ========
 */
static Void encode_decode(VIDENC_Handle enc, VIDDEC_Handle dec, FILE *in,
    FILE *out)
{
    Int                         n;
    Int32                       status;

    VIDDEC_InArgs               decInArgs;
    VIDDEC_OutArgs              decOutArgs;
    VIDDEC_DynamicParams        decDynParams;
    VIDDEC_Status               decStatus;

    VIDENC_InArgs               encInArgs;
    VIDENC_OutArgs              encOutArgs;
    VIDENC_DynamicParams        encDynParams;
    VIDENC_Status               encStatus;

    XDM_BufDesc                 inBufDesc;
    XDAS_Int8                  *src[XDM_MAX_IO_BUFFERS];
    XDAS_Int32                  inBufSizes[XDM_MAX_IO_BUFFERS];

    XDM_BufDesc                 encodedBufDesc;
    XDAS_Int8                  *encoded[XDM_MAX_IO_BUFFERS];
    XDAS_Int32                  encBufSizes[XDM_MAX_IO_BUFFERS];

    XDM_BufDesc                 outBufDesc;
    XDAS_Int8                  *dst[XDM_MAX_IO_BUFFERS];
    XDAS_Int32                  outBufSizes[XDM_MAX_IO_BUFFERS];

    /* clear and initialize the buffer descriptors */
    memset(src,     0, sizeof(src[0])     * XDM_MAX_IO_BUFFERS);
    memset(encoded, 0, sizeof(encoded[0]) * XDM_MAX_IO_BUFFERS);
    memset(dst,     0, sizeof(dst[0])     * XDM_MAX_IO_BUFFERS);

    src[0]     = inBuf;
    encoded[0] = encodedBuf;
    dst[0]     = outBuf;

    inBufDesc.numBufs = encodedBufDesc.numBufs = outBufDesc.numBufs = 1;

    inBufDesc.bufSizes      = inBufSizes;
    encodedBufDesc.bufSizes = encBufSizes;
    outBufDesc.bufSizes     = outBufSizes;

    inBufSizes[0] = encBufSizes[0] = outBufSizes[0] = NSAMPLES;

    inBufDesc.bufs      = src;
    encodedBufDesc.bufs = encoded;
    outBufDesc.bufs     = dst;

    /* initialize all "sized" fields */
    encInArgs.size    = sizeof(encInArgs);
    decInArgs.size    = sizeof(decInArgs);
    encOutArgs.size   = sizeof(encOutArgs);
    decOutArgs.size   = sizeof(decOutArgs);
    encDynParams.size = sizeof(encDynParams);
    decDynParams.size = sizeof(decDynParams);
    encStatus.size    = sizeof(encStatus);
    decStatus.size    = sizeof(decStatus);

    /*
     * Query the encoder and decoder.
     * This app expects the encoder to provide 1 buf in and get 1 buf out,
     * and the buf sizes of the in and out buffer must be able to handle
     * NSAMPLES bytes of data.
     */
    status = VIDENC_control(enc, XDM_GETSTATUS, &encDynParams, &encStatus);
    if (status != VIDENC_EOK) {
        /* failure, report error and exit */
        GT_1trace(curMask, GT_7CLASS, "encode control status = 0x%x\n", status);
        return;
    }

    /* Validate this encoder codec will meet our buffer requirements */
    if ((inBufDesc.numBufs < encStatus.bufInfo.minNumInBufs) ||
        (IFRAMESIZE < encStatus.bufInfo.minInBufSize[0]) ||
        (encodedBufDesc.numBufs < encStatus.bufInfo.minNumOutBufs) ||
        (EFRAMESIZE < encStatus.bufInfo.minOutBufSize[0])) {

        /* failure, report error and exit */
        GT_0trace(curMask, GT_7CLASS,
            "Error:  encoder codec feature conflict\n");
        return;
    }

    status = VIDDEC_control(dec, XDM_GETSTATUS, &decDynParams, &decStatus);
    if (status != VIDDEC_EOK) {
        /* failure, report error and exit */
        GT_1trace(curMask, GT_7CLASS, "decode control status = 0x%x\n", status);
        return;
    }

    /* Validate this decoder codec will meet our buffer requirements */
    if ((encodedBufDesc.numBufs < decStatus.bufInfo.minNumInBufs) ||
        (EFRAMESIZE < decStatus.bufInfo.minInBufSize[0]) ||
        (outBufDesc.numBufs < decStatus.bufInfo.minNumOutBufs) ||
        (OFRAMESIZE < decStatus.bufInfo.minOutBufSize[0])) {

        /* failure, report error and exit */
        GT_0trace(curMask, GT_7CLASS,
            "App-> ERROR: decoder does not meet buffer requirements.\n");
        return;
    }

    /*
     * Read complete frames from in, encode, decode, and write to out.
     */
    for (n = 0; fread(inBuf, IFRAMESIZE, 1, in) == 1; n++) {

#ifdef CACHE_ENABLED
#ifdef xdc_target__isaCompatible_64P
        /*
         *  fread() on this processor is implemented using CCS's stdio, which
         *  is known to write into the cache, not physical memory.  To meet
         *  xDAIS DMA Rule 7, we must writeback the cache into physical
         *  memory.  Also, per DMA Rule 7, we must invalidate the buffer's
         *  cache before providing it to any xDAIS algorithm.
         */
        Memory_cacheWbInv(inBuf, IFRAMESIZE);
#else
#error Unvalidated config - add appropriate fread-related cache maintenance
#endif
        /* Per DMA Rule 7, our output buffer cache lines must be cleaned */
        Memory_cacheInv(encodedBuf, EFRAMESIZE);
#endif

        GT_1trace(curMask, GT_1CLASS, "App-> Processing frame %d...\n", n);

        /* encode the frame */
        status = VIDENC_process(enc, &inBufDesc, &encodedBufDesc, &encInArgs,
            &encOutArgs);

        GT_2trace(curMask, GT_2CLASS,
            "App-> Encoder frame %d process returned - 0x%x)\n",
            n, status);

#ifdef CACHE_ENABLED
        /* Writeback this outBuf from the previous call.  Also, as encodedBuf
         * is an inBuf to the next process call, we must invalidate it also, to
         * clean buffer lines.
         */
        Memory_cacheWbInv(encodedBuf, EFRAMESIZE);

        /* Per DMA Rule 7, our output buffer cache lines must be cleaned */
        Memory_cacheInv(outBuf, OFRAMESIZE);
#endif

        if (status != VIDENC_EOK) {
            GT_3trace(curMask, GT_7CLASS,
                "App-> Encoder frame %d processing FAILED, status = 0x%x, "
                "extendedError = 0x%x\n", n, status, encOutArgs.extendedError);
            break;
        }

        /* decode the frame */
        status = VIDDEC_process(dec, &encodedBufDesc, &outBufDesc, &decInArgs,
           &decOutArgs);

        GT_2trace(curMask, GT_2CLASS,
            "App-> Decoder frame %d process returned - 0x%x)\n",
            n, status);

        if (status != VIDDEC_EOK) {
            GT_3trace(curMask, GT_7CLASS,
                "App-> Decoder frame %d processing FAILED, status = 0x%x, "
                "extendedError = 0x%x\n", n, status, decOutArgs.extendedError);
            break;
        }

#ifdef CACHE_ENABLED
        /* Writeback the outBuf. */
        Memory_cacheWb(outBuf, OFRAMESIZE);
#endif
        /* write to file */
        fwrite(dst[0], OFRAMESIZE, 1, out);
    }

    GT_1trace(curMask, GT_1CLASS, "%d frames encoded/decoded\n", n);
}
/*
 *  @(#) ti.sdo.ce.examples.apps.video_copy; 1, 0, 0,55; 6-19-2010 19:54:01; /db/atree/library/trees/ce/ce-o16x/src/
 */

