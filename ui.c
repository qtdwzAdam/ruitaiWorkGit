/*
 * ui.c
 *
 * ============================================================================
 * Copyright (c) Texas Instruments Inc 2009
 *
 * Use of this software is controlled by the terms and conditions found in the
 * license agreement under which this software has been supplied or provided.
 * ============================================================================
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <xdc/std.h>

#include <ti/sdo/dmai/Buffer.h>
#include <ti/sdo/dmai/Display.h>

#include <ti/sdo/simplewidget/Font.h>
#include <ti/sdo/simplewidget/Text.h>
#include <ti/sdo/simplewidget/Screen.h>

#include "demo.h"
#include "uibuttons.h"
#include "ui.h"

#define HIGHLIGHTEDBUTTONLATENCY    300000

#define COL_1                   40
#define COL_2                   280
#define COL_3                   430

#define FONT_NAME               "data/fonts/decker.ttf"
#define FONT_SIZE               20

#define MIN_TRANSPARENCY        0
#define MAX_TRANSPARENCY        0x77
#define NORMAL_TRANSPARENCY     0x55
#define INC_TRANSPARENCY        0x11

#define MAX_STRING_LENGTH       50

#define yScale(std, y) std == VideoStd_D1_NTSC ? y : y * 12 / 10

typedef enum {
    UI_BufType_WORK = 0,
    UI_BufType_DISPLAY,
    UI_BufType_NUMTYPES
} UI_Buftype;

typedef struct UI_Object {
    Display_Handle      hOsd;
    Display_Handle      hAttr;
    Font_Handle         hFont;
    UInt8               transparency;
    Bool                visible;
    Int                 osd;
    VideoStd_Type       videoStd;
} UI_Object;

typedef struct UI_String_Object {
    Int                 y;
    Int                 valid;
    Char                col1String[MAX_STRING_LENGTH];
    Char                col2String[MAX_STRING_LENGTH];
} UI_String_Object;

static UI_String_Object stringAttrs[UI_Num_Values] = {
    { UI_Row_1,  FALSE, "ARM Load:"         }, /* ARM load */
    { UI_Row_2,  FALSE, "DSP Load:"         }, /* DSP load */
    { UI_Row_3,  FALSE, "Video fps:"        }, /* Video FPS */
    { UI_Row_4,  FALSE, "Video bit rate:"   }, /* Video bitrate */
    { UI_Row_5,  FALSE, "Sound bit rate:"   }, /* Sound bitrate */
    { UI_Row_6,  FALSE, "Time:"             }, /* Time */
    { UI_Row_7,  FALSE, "Demo:"             }, /* Demo name */
    { UI_Row_8,  FALSE, "Display:"          }, /* Display type */
    { UI_Row_9,  FALSE, "Video Codec:"      }, /* Video Codec */
    { UI_Row_10, FALSE, "Resolution:"       }, /* Image resolution */
    { UI_Row_11, FALSE, "Sound Codec:"      }, /* Sound codec */
    { UI_Row_12, FALSE, "Sampling Freq:"    }, /* Sound sampling frequency */
};

/******************************************************************************
 * drawValue
 ******************************************************************************/
static Void drawValue(UI_Handle hUI, UI_Value type, Buffer_Handle hBuf)
{
    Int x, y;

    if (stringAttrs[type].y > UI_Row_6) {
        x = COL_3;
        y = stringAttrs[type].y - 180;
    }
    else {
        x = COL_2;
        y = stringAttrs[type].y;
    }

    if (Text_show(hUI->hFont, stringAttrs[type].col2String,
                  FONT_SIZE, x, yScale(hUI->videoStd, y), hBuf) < 0) {
        ERR("Failed to show text\n");
    }
}

/******************************************************************************
 * cleanupUI
 ******************************************************************************/
static Int cleanupUI(UI_Handle hUI)
{
    Int ret;

    if (hUI->hOsd) {
        ret = Display_delete(hUI->hOsd);
    }

    if (hUI->hFont) {
        ret = Font_delete(hUI->hFont);
    }

    free(hUI);

    UIButtons_deleteButtons();

    return ret;
}

/******************************************************************************
 * setOsdTransparency
 ******************************************************************************/
static Int setOsdTransparency(UI_Handle hUI, Char trans)
{
    Buffer_Handle hBuf;

    if (Display_get(hUI->hAttr, &hBuf) < 0) {
        ERR("Failed to get attribute window buffer\n");
        return FAILURE;
    }

    memset(Buffer_getUserPtr(hBuf), trans, Buffer_getSize(hBuf));

    if (Display_put(hUI->hAttr, hBuf) < 0) {
        ERR("Failed to put display buffer\n");
        return FAILURE;
    }

    return SUCCESS;
}

/******************************************************************************
 * UI_create
 ******************************************************************************/
UI_Handle UI_create(UI_Attrs *attrs)
{
    UIButtons_ButtonCoords ctrlCoords  = {  50, yScale(attrs->videoStd, 330) };
    UIButtons_ButtonCoords navCoords   = { 290, yScale(attrs->videoStd, 310) };
    UIButtons_ButtonCoords infoCoords  = { 570, yScale(attrs->videoStd, 330) };
    UIButtons_ButtonCoords wrongCoords = { 325, yScale(attrs->videoStd, 205) };
    UIButtons_Attrs        btnAttrs    =
        { &ctrlCoords, &navCoords, &infoCoords, &wrongCoords };
    Display_Attrs          oAttrs      = Display_Attrs_DM6446_DM355_OSD_DEFAULT;
    Display_Attrs          aAttrs     = Display_Attrs_DM6446_DM355_ATTR_DEFAULT;
    UI_Handle              hUI;

    if (UIButtons_createButtons(&btnAttrs) < 0) {
        ERR("Failed to create user interface buttons\n");
        return NULL;
    }

    hUI = calloc(1, sizeof(UI_Object));

    if (hUI == NULL) {
        ERR("Failed to allocate space for UI Object\n");
        return NULL;
    }

    hUI->osd = attrs->osd;
    hUI->videoStd = attrs->videoStd;

    aAttrs.videoStd = hUI->videoStd;
    hUI->hAttr = Display_create(NULL, &aAttrs);

    if (hUI->hAttr == NULL) {
        ERR("Failed to create attribute window device\n");
        cleanupUI(hUI);
        return NULL;
    }

    if (attrs->osd) {
        hUI->hFont = Font_create(FONT_NAME);

        if (hUI->hFont == NULL) {
            ERR("Failed to create UI font\n");
            cleanupUI(hUI);
            return NULL;
        }

        oAttrs.videoStd = hUI->videoStd;
        hUI->hOsd = Display_create(NULL, &oAttrs);

        if (hUI->hOsd == NULL) {
            ERR("Failed to create osd window device\n");
            cleanupUI(hUI);
            return NULL;
        }

        hUI->transparency = NORMAL_TRANSPARENCY;
        setOsdTransparency(hUI, hUI->transparency);
    }
    else {
        setOsdTransparency(hUI, 0);
    }

    return hUI;
}

/******************************************************************************
 * UI_init
 ******************************************************************************/
Int UI_init(UI_Handle hUI)
{
    BufferGfx_Dimensions dim;
    Buffer_Handle hBuf;
    Int screenNbr;
    Int cnt, bufIdx;

    if (hUI->osd) {
        for (bufIdx = 0;
             bufIdx < BufTab_getNumBufs(Display_getBufTab(hUI->hOsd));
             bufIdx++) {

            /* Get a buffer from the display device driver */
            if (Display_get(hUI->hOsd, &hBuf) < 0) {
                ERR("Failed to get display buffer\n");
                return FAILURE;
            }

            BufferGfx_getDimensions(hBuf, &dim);
            Screen_clear(hBuf, 0, 0, dim.width, dim.height);

            UIButtons_drawButtons(hBuf);

            for (screenNbr = 0; screenNbr < UI_BufType_NUMTYPES; screenNbr++) {
                for (cnt = 0; cnt < UI_Num_Values; cnt++) {
                    if (stringAttrs[cnt].y >= 0 &&
                        stringAttrs[cnt].y < UI_Row_7) {

                        if (Text_show(hUI->hFont,
                                      stringAttrs[cnt].col1String,
                                      FONT_SIZE,
                                      COL_1,
                                      yScale(hUI->videoStd, stringAttrs[cnt].y),
                                      hBuf) < 0) {
                            ERR("Failed to show text\n");
                            return FAILURE;
                        }
                    }
                }
            }

            UI_show(hUI);

            /* Give a filled buffer back to the display device driver */
            if (Display_put(hUI->hOsd, hBuf) < 0) {
                ERR("Failed to put display buffer\n");
                return FAILURE;
            }
        }
    }

    return SUCCESS;
}

/******************************************************************************
 * UI_pressButton
 ******************************************************************************/
Int UI_pressButton(UI_Handle hUI, UIButtons_State state)
{
    Buffer_Handle hBuf;

    if (!hUI->osd) {
        return SUCCESS;
    }

    if (Display_get(hUI->hOsd, &hBuf) < 0) {
        ERR("Failed to get display buffer\n");
        return FAILURE;
    }

    UIButtons_pressButton(state, hBuf);

    if (Display_put(hUI->hOsd, hBuf) < 0) {
        ERR("Failed to put display buffer\n");
        return FAILURE;
    }

    usleep(HIGHLIGHTEDBUTTONLATENCY);

    if (Display_get(hUI->hOsd, &hBuf) < 0) {
        ERR("Failed to get display buffer\n");
        return FAILURE;
    }

    if (Display_put(hUI->hOsd, hBuf) < 0) {
        ERR("Failed to put display buffer\n");
        return FAILURE;
    }

    if (Display_get(hUI->hOsd, &hBuf) < 0) {
        ERR("Failed to get display buffer\n");
        return FAILURE;
    }

    UIButtons_drawButtons(hBuf);

    if (Display_put(hUI->hOsd, hBuf) < 0) {
        ERR("Failed to put display buffer\n");
        return FAILURE;
    }

    return SUCCESS;
}

/******************************************************************************
 * UI_show
 ******************************************************************************/
Void UI_show(UI_Handle hUI)
{
    hUI->visible = TRUE;
    setOsdTransparency(hUI, hUI->transparency);
}

/******************************************************************************
 * UI_hide
 ******************************************************************************/
Void UI_hide(UI_Handle hUI)
{
    hUI->visible = TRUE;
    setOsdTransparency(hUI, hUI->transparency);
}

/******************************************************************************
 * UI_toggleVisibility
 ******************************************************************************/
Void UI_toggleVisibility(UI_Handle hUI)
{
    if (hUI->visible) {
        hUI->visible = FALSE;
        setOsdTransparency(hUI, 0);
    }
    else {
        hUI->visible = TRUE;
        setOsdTransparency(hUI, hUI->transparency);
    }
}

/******************************************************************************
 * UI_incTransparency
 ******************************************************************************/
Void UI_incTransparency(UI_Handle hUI)
{
    if (hUI->visible && hUI->transparency < MAX_TRANSPARENCY) {
        if (hUI->transparency + INC_TRANSPARENCY > MAX_TRANSPARENCY) {
            hUI->transparency = MAX_TRANSPARENCY;
        }
        else {
            hUI->transparency += INC_TRANSPARENCY;
        }

        setOsdTransparency(hUI, hUI->transparency);
    }
}

/******************************************************************************
 * UI_decTransparency
 ******************************************************************************/
Void UI_decTransparency(UI_Handle hUI)
{
    if (hUI->visible && hUI->transparency > MIN_TRANSPARENCY) {
        if (hUI->transparency - INC_TRANSPARENCY < MIN_TRANSPARENCY) {
            hUI->transparency = MIN_TRANSPARENCY;
        }
        else {
            hUI->transparency -= INC_TRANSPARENCY;
        }

        setOsdTransparency(hUI, hUI->transparency);
    }
}

/******************************************************************************
 * UI_updateValue
 ******************************************************************************/
Void UI_updateValue(UI_Handle hUI, UI_Value type, Char *valString)
{
    if (stringAttrs[type].y >= 0) {
        strncpy(stringAttrs[type].col2String, valString, MAX_STRING_LENGTH);
        stringAttrs[type].valid = TRUE;
    }
}

/******************************************************************************
 * UI_setRow
 ******************************************************************************/
Void UI_setRow(UI_Handle hUI, UI_Value type, UI_Row row)
{
    stringAttrs[type].y = row;
}

/******************************************************************************
 * UI_update
 ******************************************************************************/
Void UI_update(UI_Handle hUI)
{
    BufferGfx_Dimensions dim;
    Buffer_Handle hBuf;
    Int i;

    if (hUI->osd) {
        /* Get a buffer from the display device driver */
        if (Display_get(hUI->hOsd, &hBuf) < 0) {
            ERR("Failed to get display buffer update\n");
            return;
        }

        BufferGfx_getDimensions(hBuf, &dim);

        Screen_clear(hBuf, COL_2, 0,
                     COL_3 - COL_2, yScale(hUI->videoStd, 220));
    }

    for (i = 0; i < UI_Num_Values; i++) {
        if (stringAttrs[i].y >= 0 && stringAttrs[i].valid) {
            if (hUI->osd) {
                drawValue(hUI, i, hBuf);
            }
            else {
                printf("%s %s ", stringAttrs[i].col1String,
                       stringAttrs[i].col2String);
            }
        }
    }

    if (hUI->osd) {
        /* Give a filled buffer back to the display device driver */
        if (Display_put(hUI->hOsd, hBuf) < 0) {
            ERR("Failed to put display buffer\n");
            return;
        }
    }
    else {
        printf("\n\n");
    }
}

/******************************************************************************
 * UI_delete
 ******************************************************************************/
Int UI_delete(UI_Handle hUI)
{
    Int ret = SW_EOK;

    if (hUI) {
        ret = cleanupUI(hUI);
    }

    return ret; 
}
