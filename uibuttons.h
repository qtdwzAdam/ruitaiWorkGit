/*
 * uibuttons.h
 *
 * ============================================================================
 * Copyright (c) Texas Instruments Inc 2009
 *
 * Use of this software is controlled by the terms and conditions found in the
 * license agreement under which this software has been supplied or provided.
 * ============================================================================
 */

#ifndef _UIBUTTONS_H
#define _UIBUTTONS_H

#include <xdc/std.h>

#include <ti/sdo/dmai/Buffer.h>

#include <ti/sdo/simplewidget/Font.h>

typedef enum {
    /* States for the control button, including original state */
    UIButtons_State_CTRL = 0,
    UIButtons_State_CTRLPLAY,
    UIButtons_State_CTRLPAUSE,
    UIButtons_State_CTRLREC,
    UIButtons_State_CTRLSTOP,

    /* States for the navigation button, including original state */
    UIButtons_State_NAV,
    UIButtons_State_NAVOK,
    UIButtons_State_NAVLEFT,
    UIButtons_State_NAVRIGHT,
    UIButtons_State_NAVUP,
    UIButtons_State_NAVDOWN,

    /* States for the info button, including original state */
    UIButtons_State_INFO,
    UIButtons_State_INFOPRESSED,

    /* States for the wrong button pressed, including original state */
    UIButtons_State_NOWRONG,
    UIButtons_State_WRONG,

    UIButtons_State_CNT 
} UIButtons_State;

typedef struct UIButtons_ButtonCoords {
    Int32   x;
    Int32   y;
} UIButtons_ButtonCoords;

typedef struct UIButtons_Attrs {
    UIButtons_ButtonCoords    *ctrlButtonCoords;
    UIButtons_ButtonCoords    *navButtonCoords;
    UIButtons_ButtonCoords    *infoButtonCoords;
    UIButtons_ButtonCoords    *wrongButtonCoords;
} UIButtons_Attrs;

extern Int UIButtons_createButtons(UIButtons_Attrs *attrs);

extern Int UIButtons_deleteButtons(Void);

extern Int UIButtons_drawButtons(Buffer_Handle hBuf);

extern Int UIButtons_pressButton(UIButtons_State btnState, Buffer_Handle hBuf);

#endif /* _UIBUTTONS_H */
