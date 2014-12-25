/*
 * uibuttons.c
 *
 * ============================================================================
 * Copyright (c) Texas Instruments Inc 2009
 *
 * Use of this software is controlled by the terms and conditions found in the
 * license agreement under which this software has been supplied or provided.
 * ============================================================================
 */
#include <string.h>

#include <xdc/std.h>

#include <ti/sdo/dmai/Buffer.h>
#include <ti/sdo/simplewidget/Font.h>
#include <ti/sdo/simplewidget/Png.h>
#include <ti/sdo/simplewidget/Text.h>

#include "demo.h"
#include "uibuttons.h"

typedef struct Btn {
    Png_Handle        hPng;
    Int               x;
    Int               y;
} Btn;

static Btn btns[UIButtons_State_CNT];

/* File names for the different button states */
static Char *pngFilename[UIButtons_State_CNT] = {
    "data/pics/Buttons_SQ.png",
    "data/pics/Play_SQ_SEL.png",
    "data/pics/Pause_SQ_SEL.png",
    "data/pics/Rec_SQ_SEL.png",
    "data/pics/Stop_SQ_SEL.png",

    "data/pics/Button_DIAG.png",
    "data/pics/Center_Diag_SEL.png",
    "data/pics/Left_Diag_SEL.png",
    "data/pics/Right_Diag_SEL.png",
    "data/pics/Top_Diag_SEL.png",
    "data/pics/Bottom_Diag_SEL.png",

    "data/pics/INFO_RND.png",
    "data/pics/Info_RND_SEL.png",

    "data/pics/nowrongbutton.png",
    "data/pics/wrongbutton.png"
};

static Int buttonsCreated = 0;

/******************************************************************************
 * UIButtons_createButtons
 ******************************************************************************/
Int UIButtons_createButtons(UIButtons_Attrs *attrs)
{
    Int i;

    if (buttonsCreated++) {
        return SUCCESS;
    }

    memset(&btns, 0, sizeof(btns));

    for (i = 0; i < UIButtons_State_CNT; i++) {
        switch (i) {
            case UIButtons_State_CTRL:
            case UIButtons_State_CTRLPLAY:
            case UIButtons_State_CTRLPAUSE:
            case UIButtons_State_CTRLREC:
            case UIButtons_State_CTRLSTOP:
                if (attrs->ctrlButtonCoords) {
                    btns[i].hPng = Png_create(pngFilename[i]);

                    if (btns[i].hPng == NULL) {
                        ERR("Failed to create png image\n");
                        return FAILURE;
                    }

                    btns[i].x = attrs->ctrlButtonCoords->x;
                    btns[i].y = attrs->ctrlButtonCoords->y;
                }
                break;
            case UIButtons_State_NAV:
            case UIButtons_State_NAVOK:
            case UIButtons_State_NAVLEFT:
            case UIButtons_State_NAVRIGHT:
            case UIButtons_State_NAVUP:
            case UIButtons_State_NAVDOWN:
                if (attrs->navButtonCoords) {
                    btns[i].hPng = Png_create(pngFilename[i]);

                    if (btns[i].hPng == NULL) {
                        ERR("Failed to create png image\n");
                        return FAILURE;
                    }

                    btns[i].x = attrs->navButtonCoords->x;
                    btns[i].y = attrs->navButtonCoords->y;
                }
                break;
            case UIButtons_State_INFO:
            case UIButtons_State_INFOPRESSED:
                if (attrs->infoButtonCoords) {
                    btns[i].hPng = Png_create(pngFilename[i]);

                    if (btns[i].hPng == NULL) {
                        ERR("Failed to create png image\n");
                        return FAILURE;
                    }

                    btns[i].x = attrs->infoButtonCoords->x;
                    btns[i].y = attrs->infoButtonCoords->y;
                }
                break;
            case UIButtons_State_NOWRONG:
            case UIButtons_State_WRONG:
                if (attrs->wrongButtonCoords) {
                    btns[i].hPng = Png_create(pngFilename[i]);

                    if (btns[i].hPng == NULL) {
                        ERR("Failed to create png image\n");
                        return FAILURE;
                    }

                    btns[i].x = attrs->wrongButtonCoords->x;
                    btns[i].y = attrs->wrongButtonCoords->y;
                }
                break;
            default:
                /* Satisfy compiler, should never happen */
                break;
        }
    }

    return SUCCESS;
}

/******************************************************************************
 * UIButtons_deleteButtons
 ******************************************************************************/
Int UIButtons_deleteButtons(Void)
{
    Int i;

    if (!buttonsCreated) {
        return SUCCESS;
    }

    if (--buttonsCreated == 0) {
        for (i = 0; i < UIButtons_State_CNT; i++) {
            if (btns[i].hPng) {
                Png_delete(btns[i].hPng);
            }
        }
    }

    return SUCCESS;
}

/******************************************************************************
 * UIButtons_pressButton
 ******************************************************************************/
Int UIButtons_pressButton(UIButtons_State state, Buffer_Handle hBuf)
{
    if (btns[state].hPng) {
        Png_show(btns[state].hPng, btns[state].x, btns[state].y, hBuf);
    }

    return SUCCESS;
}

/******************************************************************************
 * UIButtons_drawButtons
 ******************************************************************************/
Int UIButtons_drawButtons(Buffer_Handle hBuf)
{
    if (btns[UIButtons_State_CTRL].hPng) {
        Png_show(btns[UIButtons_State_CTRL].hPng,
                 btns[UIButtons_State_CTRL].x,
                 btns[UIButtons_State_CTRL].y,
                 hBuf);
    }

    if (btns[UIButtons_State_NAV].hPng) {
        Png_show(btns[UIButtons_State_NAV].hPng,
                 btns[UIButtons_State_NAV].x,
                 btns[UIButtons_State_NAV].y,
                 hBuf);
    }

    if (btns[UIButtons_State_INFO].hPng) {
        Png_show(btns[UIButtons_State_INFO].hPng,
                 btns[UIButtons_State_INFO].x,
                 btns[UIButtons_State_INFO].y,
                 hBuf);
    }

    if (btns[UIButtons_State_NOWRONG].hPng) {
        Png_show(btns[UIButtons_State_NOWRONG].hPng,
                 btns[UIButtons_State_NOWRONG].x,
                 btns[UIButtons_State_NOWRONG].y,
                 hBuf);
    }

    return SUCCESS;
}
