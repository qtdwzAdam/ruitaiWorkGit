/*
 * ctrl.c
 *
 * ============================================================================
 * Copyright (c) Texas Instruments Inc 2009
 *
 * Use of this software is controlled by the terms and conditions found in the
 * license agreement under which this software has been supplied or provided.
 * ============================================================================
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>

#include <xdc/std.h>

#include <ti/sdo/ce/Engine.h>

#include <ti/sdo/dmai/Ir.h>
#include <ti/sdo/dmai/Cpu.h>
#include <ti/sdo/dmai/Pause.h>
#include <ti/sdo/dmai/Rendezvous.h>

#include "ctrl.h"
#include "demo.h"
#include "ui.h"

/* How often to poll for new IR commands */
#define REMOTECONTROLLATENCY 300000

/* Keyboard command prompt */
#define COMMAND_PROMPT       "Command [ 'help' for usage ] > "

/* Maximum length of a keyboard command */
#define MAX_CMD_LENGTH       80

/* Structure containing the state of the OSD */
typedef struct OsdData {
    Int           time;
    ULong         firstTime;
    ULong         prevTime;
    Int           samplingFrequency;
    Int           imageWidth;
    Int           imageHeight;
} OsdData;

/* Initial values of osd data structure */
#define OSD_DATA_INIT        { -1, 0, 0, 0, 0, 0 }

/******************************************************************************
 * drawDynamicData
 ******************************************************************************/
static Void drawDynamicData(Engine_Handle hEngine, Cpu_Handle hCpu,
                            UI_Handle hUI, OsdData *op)
{
    Char                  tmpString[20];
    struct timeval        tv;
    struct tm            *timePassed;
    time_t                spentTime;
    ULong                 newTime;
    ULong                 deltaTime;
    Int                   armLoad;
    Int                   dspLoad;
    Int                   fps;
    Int                   videoKbps;
    Int                   soundKbps;
    Float                 fpsf;
    Float                 videoKbpsf;
    Float                 soundKbpsf;
    Int                   imageWidth;
    Int                   imageHeight;
    Int                   freq;
    Int                   khz;
    Int                   decimal;

    op->time = -1;

    if (gettimeofday(&tv, NULL) == -1) {
        ERR("Failed to get os time\n");
        return;
    }

    newTime = tv.tv_sec * 1000 + tv.tv_usec / 1000;
    if (!op->firstTime) {
        op->firstTime = newTime;
        op->prevTime = newTime;
        return;
    }

    /* Only update user interface every second */
    deltaTime = newTime - op->prevTime;
    if (deltaTime <= 1000) {
        return;
    }

    op->prevTime = newTime;

    spentTime = (newTime - op->firstTime) / 1000;
    if (spentTime <= 0) {
        return;
    }

    op->time     = spentTime;

    /* Calculate the frames per second */
    fpsf         = gblGetAndResetFrames() * 1000.0 / deltaTime;
    fps          = fpsf + 0.5;

    /* Calculate the video bit rate */
    videoKbpsf   = gblGetAndResetVideoBytesProcessed() * 8.0 / deltaTime;
    videoKbps    = videoKbpsf + 0.5;

    /* Calculate the audio or speech bit rate */
    soundKbpsf   = gblGetAndResetSoundBytesProcessed() * 8.0 / deltaTime;
    soundKbps    = soundKbpsf + 0.5;

    /* Get the local ARM cpu load */
    if (Cpu_getLoad(hCpu, &armLoad) < 0) {
        armLoad = 0;
        ERR("Failed to get ARM CPU load\n");
    }

    /* Get the DSP load */
    dspLoad = Engine_getCpuLoad(hEngine);

    if (dspLoad < 0) {
        dspLoad = 0;
        ERR("Failed to get DSP CPU load\n");
    }

    timePassed = localtime(&spentTime);
    if (timePassed == NULL) {
        return;
    }

    /* Update the UI */
    sprintf(tmpString, "%.2d:%.2d:%.2d", timePassed->tm_hour,
                                         timePassed->tm_min,
                                         timePassed->tm_sec);

    UI_updateValue(hUI, UI_Value_Time, tmpString);

    freq = gblGetSamplingFrequency();

    if (freq != op->samplingFrequency) {
        khz = freq / 1000;
        decimal = (freq % 1000) / 100; /* Only save one decimal */

        if (decimal) {
            sprintf(tmpString, "%d.%d KHz samp rate", khz, decimal);
        }
        else {
            sprintf(tmpString, "%d KHz samp rate", khz);
        }

        UI_updateValue(hUI, UI_Value_SoundFrequency, tmpString);
    }

    imageWidth = gblGetImageWidth();
    imageHeight = gblGetImageHeight();

    if (imageWidth != op->imageWidth ||
        imageHeight != op->imageHeight) {

        sprintf(tmpString, "%dx%d", imageWidth, imageHeight);

        UI_updateValue(hUI, UI_Value_ImageResolution, tmpString);

        op->imageWidth = imageWidth;
        op->imageHeight = imageHeight;
    } 

    sprintf(tmpString, "%d%%", armLoad);
    UI_updateValue(hUI, UI_Value_ArmLoad, tmpString);

    sprintf(tmpString, "%d%%", dspLoad);
    UI_updateValue(hUI, UI_Value_DspLoad, tmpString);

    sprintf(tmpString, "%d fps", fps);
    UI_updateValue(hUI, UI_Value_Fps, tmpString);

    sprintf(tmpString, "%d kbps", videoKbps);
    UI_updateValue(hUI, UI_Value_VideoKbps, tmpString);

    sprintf(tmpString, "%d kbps", soundKbps);
    UI_updateValue(hUI, UI_Value_SoundKbps, tmpString);

    UI_update(hUI);
}

/******************************************************************************
 * keyAction
 ******************************************************************************/
static Int keyAction(Ir_Key key, UI_Handle hUI, Pause_Handle hPauseProcess)
{
    switch(key) {
        case Ir_Key_OK:              
        case Ir_Key_PLAY:
        case Ir_Key_RECORD:
            Pause_off(hPauseProcess);
            UI_pressButton(hUI, UIButtons_State_CTRLPLAY);
            break;

        case Ir_Key_PAUSE:
            Pause_on(hPauseProcess);
            UI_pressButton(hUI, UIButtons_State_CTRLPAUSE);
            break;

        case Ir_Key_STOP:
            gblSetQuit();
            UI_pressButton(hUI, UIButtons_State_CTRLSTOP);
            break;

        case Ir_Key_VOLINC:
            UI_incTransparency(hUI);
            UI_pressButton(hUI, UIButtons_State_NAVRIGHT);
            break;

        case Ir_Key_VOLDEC:
            UI_decTransparency(hUI);
            UI_pressButton(hUI, UIButtons_State_NAVLEFT);
            break;

        case Ir_Key_INFOSELECT:
            UI_toggleVisibility(hUI);
            break;

        default:
            UI_pressButton(hUI, UIButtons_State_WRONG);
            break;
    }

    return SUCCESS;
}

/******************************************************************************
 * getKbdCommand
 ******************************************************************************/
Int getKbdCommand(Ir_Key *keyPtr)
{
    struct timeval tv;
    fd_set         fds;
    Int            ret;
    Char           string[MAX_CMD_LENGTH];

    FD_ZERO(&fds);
    FD_SET(fileno(stdin), &fds);

    /* Timeout of 0 means polling, we don't want to block */
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    ret = select(FD_SETSIZE, &fds, NULL, NULL, &tv);

    if (ret == -1) {
        ERR("Select failed on stdin\n");
        return FAILURE;
    }

    if (ret == 0) {
        return SUCCESS;
    }

    if (FD_ISSET(fileno(stdin), &fds)) {
        if (fgets(string, MAX_CMD_LENGTH, stdin) == NULL) {
            return FAILURE;
        }

        /* Remove the end of line */
        strtok(string, "\n");

        /* Assign corresponding Ir key */
        if (strcmp(string, "play") == 0) {
            *keyPtr = Ir_Key_PLAY;
        }
        else if (strcmp(string, "pause") == 0) {
            *keyPtr = Ir_Key_PAUSE;
        }
        else if (strcmp(string, "stop") == 0) {
            *keyPtr = Ir_Key_STOP;
        }
        else if (strcmp(string, "inc") == 0) {
            *keyPtr = Ir_Key_VOLDEC;
        }
        else if (strcmp(string, "dec") == 0) {
            *keyPtr = Ir_Key_VOLINC;
        }
        else if (strcmp(string, "hide") == 0) {
            *keyPtr = Ir_Key_INFOSELECT;
        }
        else if (strcmp(string, "help") == 0) {
            printf("\nAvailable commands:\n"
                   "    play  - Play / record video and sound\n"
                   "    pause - Pause video and sound\n"
                   "    stop  - Quit demo\n"
                   "    inc   - Increase OSD transparency\n"
                   "    dec   - Decrease OSD transparency\n"
                   "    hide  - Show / hide the OSD\n"
                   "    help  - Show this help screen\n\n");
        }
        else {
            printf("Unknown command: [ %s ]\n", string);
        }

        if (*keyPtr != Ir_Key_STOP) {
            printf(COMMAND_PROMPT);
            fflush(stdout);
        }
        else {
            printf("\n");
        }
    }

    return SUCCESS;
}

/******************************************************************************
 * ctrlThrFxn
 ******************************************************************************/
Void *ctrlThrFxn(Void *arg)
{
    CtrlEnv                *envp                = (CtrlEnv *) arg;
    Void                   *status              = THREAD_SUCCESS;
    OsdData                 osdData             = OSD_DATA_INIT;
    Ir_Attrs                irAttrs             = Ir_Attrs_DEFAULT;
    Cpu_Attrs               cpuAttrs            = Cpu_Attrs_DEFAULT;
    Engine_Handle           hEngine             = NULL;
    Ir_Handle               hIr                 = NULL;
    Cpu_Handle              hCpu                = NULL;
    Ir_Key                  key;

    /* Open the codec engine */
    hEngine = Engine_open(envp->engineName, NULL, NULL);

    if (hEngine == NULL) {
        ERR("Failed to open codec engine %s\n", envp->engineName);
        cleanup(THREAD_FAILURE);
    }

    /* Create the infra red interface to obtain IR key presses */
    hIr = Ir_create(&irAttrs);

    if (hIr == NULL) {
        ERR("Failed to create Ir Object\n");
        cleanup(THREAD_FAILURE);
    }

    /* Create the Cpu object to obtain ARM cpu load */
    hCpu = Cpu_create(&cpuAttrs);

    if (hCpu == NULL) {
        ERR("Failed to create Cpu Object\n");
    }

    /* Signal that initialization is done and wait for other threads */
    Rendezvous_meet(envp->hRendezvousInit);

    if (envp->keyboard) {
        printf(COMMAND_PROMPT);
        fflush(stdout);
    }

    while (!gblGetQuit()) {
        /* Update the dynamic data, either on the OSD or on the console */
        drawDynamicData(hEngine, hCpu, envp->hUI, &osdData);

        /* Has the demo timelimit been hit? */
        if (envp->time > FOREVER && osdData.time >= envp->time) {
            cleanup(THREAD_SUCCESS);
        }

        /* See if an IR remote key has been pressed */
        if (Ir_getKey(hIr, &key) < 0) {
            ERR("Failed to get IR value.\n");
            cleanup(THREAD_FAILURE);
        }

        if (envp->keyboard) {
            /* Poll for a key press */
            if (getKbdCommand(&key) == FAILURE) {
                cleanup(THREAD_FAILURE);
            }
        }

        /*
         * If an IR key had been pressed or a keyboard command
         * has been issued, service it.
         */
        if (key != 0) {
            if (keyAction(key, envp->hUI, envp->hPauseProcess) == FAILURE) {
                cleanup(THREAD_FAILURE);
            }
        }

        /* Wait a while before polling the Ir and keyboard again */
        usleep(REMOTECONTROLLATENCY);
    }

cleanup:
    /* Make sure the other threads aren't waiting for us */
    Rendezvous_force(envp->hRendezvousInit);
    Pause_off(envp->hPauseProcess);

    /* Meet up with other threads before cleaning up */
    Rendezvous_meet(envp->hRendezvousCleanup);

    /* Clean up the thread before exiting */
    if (hCpu) {
        Cpu_delete(hCpu);
    }

    if (hIr) {
        Ir_delete(hIr);
    }

    if (hEngine) {
        Engine_close(hEngine);
    }

    return status;
}
