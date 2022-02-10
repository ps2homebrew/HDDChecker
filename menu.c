#include <kernel.h>
#include <libhdd.h>
#include <libmc.h>
#include <libpad.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <osd_config.h>
#include <timer.h>
#include <limits.h>
#include <wchar.h>

#include <libgs.h>

#include "main.h"
#include "system.h"
#include "pad.h"
#include "graphics.h"
#include "font.h"
#include "UI.h"
#include "menu.h"
#ifndef FSCK
#include "hdst.h"
#endif

extern struct UIDrawGlobal UIDrawGlobal;
extern GS_IMAGE BackgroundTexture;
extern GS_IMAGE PadLayoutTexture;

#ifndef FSCK
enum MAIN_MENU_ID {
    MAIN_MENU_ID_HDD0_MODEL = 1,
    MAIN_MENU_ID_HDD0_SERIAL,
    MAIN_MENU_ID_HDD0_FIRMWARE,
    MAIN_MENU_ID_HDD0_SMART,
    MAIN_MENU_ID_HDD0_CAPACITY,
    MAIN_MENU_ID_HDD0_CAPACITY_UNIT,
    MAIN_MENU_ID_DESCRIPTION,
    MAIN_MENU_ID_VERSION,
    MAIN_MENU_ID_BTN_SCAN,
    MAIN_MENU_ID_BTN_OPT,
    MAIN_MENU_ID_BTN_SURF_SCAN,
    MAIN_MENU_ID_BTN_ZERO_FILL,
    MAIN_MENU_ID_BTN_EXIT,
};

enum PRG_SCREEN_ID {
    PRG_SCREEN_ID_TITLE = 1,
    PRG_SCREEN_ID_ETA_HOURS,
    PRG_SCREEN_ID_ETA_MINS,
    PRG_SCREEN_ID_ETA_SECS,
    PRG_SCREEN_ID_LBL_RD_ERRS,
    PRG_SCREEN_ID_RD_ERRS,
    PRG_SCREEN_ID_PROGRESS,
    PRG_SCREEN_ID_TOTAL_PROGRESS,
};

static struct UIMenuItem HDDMainMenuItems[] = {
    {MITEM_LABEL, 0, 0, 0, 0, 0, 0, SYS_UI_LBL_ATA_UNIT_0},
    {MITEM_SEPERATOR},

    {MITEM_LABEL, 0, 0, 0, 0, 0, 0, SYS_UI_LBL_MODEL},
    {MITEM_TAB},
    {MITEM_TAB},
    {MITEM_TAB},
    {MITEM_STRING, MAIN_MENU_ID_HDD0_MODEL, MITEM_FLAG_READONLY},
    {MITEM_BREAK},
    {MITEM_LABEL, 0, 0, 0, 0, 0, 0, SYS_UI_LBL_SERIAL},
    {MITEM_TAB},
    {MITEM_TAB},
    {MITEM_TAB},
    {MITEM_STRING, MAIN_MENU_ID_HDD0_SERIAL, MITEM_FLAG_READONLY},
    {MITEM_BREAK},
    {MITEM_LABEL, 0, 0, 0, 0, 0, 0, SYS_UI_LBL_FIRMWARE},
    {MITEM_TAB},
    {MITEM_TAB},
    {MITEM_STRING, MAIN_MENU_ID_HDD0_FIRMWARE, MITEM_FLAG_READONLY},
    {MITEM_BREAK},
    {MITEM_LABEL, 0, 0, 0, 0, 0, 0, SYS_UI_LBL_SMART_STATUS},
    {MITEM_TAB},
    {MITEM_TAB},
    {MITEM_LABEL, MAIN_MENU_ID_HDD0_SMART},
    {MITEM_BREAK},
    {MITEM_LABEL, 0, 0, 0, 0, 0, 0, SYS_UI_LBL_CAPACITY},
    {MITEM_TAB},
    {MITEM_TAB},
    {MITEM_VALUE, MAIN_MENU_ID_HDD0_CAPACITY, MITEM_FLAG_READONLY},
    {MITEM_SPACE},
    {MITEM_LABEL, MAIN_MENU_ID_HDD0_CAPACITY_UNIT},
    {MITEM_BREAK},
    {MITEM_BREAK},

    {MITEM_BUTTON, MAIN_MENU_ID_BTN_SCAN, MITEM_FLAG_POS_MID, 0, 24, 0, 0, SYS_UI_LBL_SCAN_DISK},
    {MITEM_BREAK},
    {MITEM_BREAK},
    {MITEM_BUTTON, MAIN_MENU_ID_BTN_OPT, MITEM_FLAG_POS_MID, 0, 24, 0, 0, SYS_UI_LBL_OPT_DISK},
    {MITEM_BREAK},
    {MITEM_BREAK},
    {MITEM_BUTTON, MAIN_MENU_ID_BTN_SURF_SCAN, MITEM_FLAG_POS_MID, 0, 24, 0, 0, SYS_UI_LBL_SURF_SCAN_DISK},
    {MITEM_BREAK},
    {MITEM_BREAK},
    {MITEM_BUTTON, MAIN_MENU_ID_BTN_ZERO_FILL, MITEM_FLAG_POS_MID, 0, 24, 0, 0, SYS_UI_LBL_ZERO_FILL_DISK},
    {MITEM_BREAK},
    {MITEM_BREAK},
    {MITEM_BUTTON, MAIN_MENU_ID_BTN_EXIT, MITEM_FLAG_POS_MID, 0, 24, 0, 0, SYS_UI_LBL_QUIT},
    {MITEM_BREAK},
    {MITEM_BREAK},

    {MITEM_STRING, MAIN_MENU_ID_DESCRIPTION, MITEM_FLAG_POS_ABS | MITEM_FLAG_READONLY, 0, 0, 32, 370},
    {MITEM_BREAK},
    {MITEM_STRING, MAIN_MENU_ID_VERSION, MITEM_FLAG_POS_ABS | MITEM_FLAG_READONLY, 0, 0, 520, 420},
    {MITEM_BREAK},

    {MITEM_TERMINATOR}};

static struct UIMenuItem ProgressScreenItems[] = {
    {MITEM_LABEL, PRG_SCREEN_ID_TITLE},
    {MITEM_SEPERATOR},
    {MITEM_LABEL, 0, 0, 0, 0, 0, 0, SYS_UI_LBL_ETA},
    {MITEM_TAB},
    {MITEM_VALUE, PRG_SCREEN_ID_ETA_HOURS, MITEM_FLAG_READONLY, MITEM_FORMAT_UDEC, 2},
    {MITEM_COLON},
    {MITEM_VALUE, PRG_SCREEN_ID_ETA_MINS, MITEM_FLAG_READONLY, MITEM_FORMAT_UDEC, 2},
    {MITEM_COLON},
    {MITEM_VALUE, PRG_SCREEN_ID_ETA_SECS, MITEM_FLAG_READONLY, MITEM_FORMAT_UDEC, 2},
    {MITEM_BREAK},
    {MITEM_LABEL, PRG_SCREEN_ID_LBL_RD_ERRS, 0, 0, 0, 0, 0, SYS_UI_LBL_BAD_SECTORS},
    {MITEM_TAB},
    {MITEM_VALUE, PRG_SCREEN_ID_RD_ERRS, MITEM_FORMAT_UDEC | MITEM_FLAG_READONLY},
    {MITEM_BREAK},
    {MITEM_BREAK},
    {MITEM_PROGRESS, PRG_SCREEN_ID_PROGRESS, MITEM_FLAG_POS_ABS, 0, 0, 0, 280},
    {MITEM_BREAK},
    {MITEM_BREAK},
    {MITEM_PROGRESS, PRG_SCREEN_ID_TOTAL_PROGRESS},
    {MITEM_BREAK},

    {MITEM_TERMINATOR}};

static struct UIMenu HDDMainMenu    = {NULL, NULL, HDDMainMenuItems, {{BUTTON_TYPE_SYS_SELECT, SYS_UI_LBL_OK}, {BUTTON_TYPE_SYS_CANCEL, SYS_UI_LBL_QUIT}}};
static struct UIMenu ProgressScreen = {NULL, NULL, ProgressScreenItems, {{BUTTON_TYPE_SYS_CANCEL, SYS_UI_LBL_CANCEL}, {-1, -1}}};
#endif

enum SCAN_RESULTS_SCREEN_ID {
    SCN_SCREEN_ID_TITLE = 1,
    SCN_SCREEN_ID_RESULT,
    SCN_SCREEN_ID_LBL_ERRS_FOUND,
    SCN_SCREEN_ID_ERRS_FOUND,
    SCN_SCREEN_ID_LBL_ERRS_FIXED,
    SCN_SCREEN_ID_ERRS_FIXED,
    SCN_SCREEN_ID_LBL_SOME_ERRS_NOT_FIXED,
    SCN_SCREEN_ID_BTN_OK,
};

static struct UIMenuItem ScanResultsScreenItems[] = {
    {MITEM_LABEL, SCN_SCREEN_ID_TITLE, 0, 0, 0, 0, 0, SYS_UI_LBL_SCAN_RESULTS},
    {MITEM_SEPERATOR},
    {MITEM_BREAK},

    {MITEM_STRING, SCN_SCREEN_ID_RESULT, MITEM_FLAG_READONLY},
    {MITEM_BREAK},
    {MITEM_BREAK},

    {MITEM_LABEL, SCN_SCREEN_ID_LBL_ERRS_FOUND, 0, 0, 0, 0, 0, SYS_UI_LBL_ERRORS_FOUND},
    {MITEM_TAB},
    {MITEM_VALUE, SCN_SCREEN_ID_ERRS_FOUND, MITEM_FLAG_READONLY},
    {MITEM_BREAK},
    {MITEM_LABEL, SCN_SCREEN_ID_LBL_ERRS_FIXED, 0, 0, 0, 0, 0, SYS_UI_LBL_ERRORS_FIXED},
    {MITEM_TAB},
    {MITEM_VALUE, SCN_SCREEN_ID_ERRS_FIXED, MITEM_FLAG_READONLY},
    {MITEM_BREAK},
    {MITEM_BREAK},

    {MITEM_LABEL, SCN_SCREEN_ID_LBL_SOME_ERRS_NOT_FIXED, 0, 0, 0, 0, 0, SYS_UI_LBL_SOME_ERRORS_NOT_FIXED},
    {MITEM_BREAK},

    {MITEM_BREAK},
    {MITEM_BREAK},
    {MITEM_BREAK},
    {MITEM_BREAK},
    {MITEM_BREAK},
    {MITEM_BREAK},
    {MITEM_BREAK},
    {MITEM_BREAK},
    {MITEM_BREAK},

    {MITEM_BUTTON, SCN_SCREEN_ID_BTN_OK, MITEM_FLAG_POS_MID, 0, 16},

    {MITEM_TERMINATOR}};

static struct UIMenu ScanResultsScreen = {NULL, NULL, ScanResultsScreenItems, {{BUTTON_TYPE_SYS_SELECT, SYS_UI_LBL_OK}, {-1, -1}}};

static void DrawMenuEntranceSlideInMenuAnimation(int SelectedOption)
{
    int i;
    GS_RGBAQ rgbaq;

    rgbaq.r = 0;
    rgbaq.g = 0;
    rgbaq.b = 0;
    rgbaq.q = 0;
    for (i = 30; i > 0; i--) {
        rgbaq.a = 0x80 - (i * 4);
        DrawSprite(&UIDrawGlobal, 0, 0,
                   UIDrawGlobal.width, UIDrawGlobal.height,
                   0, rgbaq);
#ifndef FSCK
        UIDrawMenu(&HDDMainMenu, i, UI_OFFSET_X + i * 6, UI_OFFSET_Y, SelectedOption);
#endif
        SyncFlipFB(&UIDrawGlobal);
    }
}

static void DrawMenuExitAnimation(void)
{
    int i;
    GS_RGBAQ rgbaq;

    rgbaq.r = 0;
    rgbaq.g = 0;
    rgbaq.b = 0;
    rgbaq.q = 0;
    for (i = 30; i > 0; i--) {
        rgbaq.a = 0x80 - (i * 4);
        DrawSprite(&UIDrawGlobal, 0, 0,
                   UIDrawGlobal.width, UIDrawGlobal.height,
                   0, rgbaq);
        SyncFlipFB(&UIDrawGlobal);
    }
}

#ifndef FSCK
static int PerformZeroFillDoubleConfirmation(void)
{
    int done, result;
    unsigned int PadStatus, frame;

    done   = 0;
    result = 0;
    frame  = 0;
    while (!done) {
        DrawBackground(&UIDrawGlobal, &BackgroundTexture);

        FontPrintf(&UIDrawGlobal, 20, 64, 1, 1.0f, GS_WHITE_FONT, GetUIString(SYS_UI_MSG_ZERO_FILL_DISK_CFM_2));

        if (frame > 0 && frame % 30 == 0) {
            PadStatus = ReadCombinedPadStatus();

            if (PadStatus != 0) {
                if ((PadStatus & PAD_SELECT) && (PadStatus & PAD_START))
                    result = 1;

                done = 1;
            }

            frame = 0;
        }

        frame++;
        SyncFlipFB(&UIDrawGlobal);
    }

    return result;
}

static int MainMenuUpdateCallback(struct UIMenu *menu, unsigned short int frame, int selection, u32 padstatus)
{
    if (padstatus != 0) {
        if (selection >= 0) {
            switch (menu->items[selection].id) {
                case MAIN_MENU_ID_BTN_SCAN:
                    UISetString(menu, MAIN_MENU_ID_DESCRIPTION, GetUIString(SYS_UI_MSG_DSC_SCAN_DISK));
                    break;
                case MAIN_MENU_ID_BTN_OPT:
                    UISetString(menu, MAIN_MENU_ID_DESCRIPTION, GetUIString(SYS_UI_MSG_DSC_OPT_DISK));
                    break;
                case MAIN_MENU_ID_BTN_SURF_SCAN:
                    UISetString(menu, MAIN_MENU_ID_DESCRIPTION, GetUIString(SYS_UI_MSG_DSC_SURF_SCAN_DISK));
                    break;
                case MAIN_MENU_ID_BTN_ZERO_FILL:
                    UISetString(menu, MAIN_MENU_ID_DESCRIPTION, GetUIString(SYS_UI_MSG_DSC_ZERO_FILL_DISK));
                    break;
                case MAIN_MENU_ID_BTN_EXIT:
                    UISetString(menu, MAIN_MENU_ID_DESCRIPTION, GetUIString(SYS_UI_MSG_DSC_QUIT));
                    break;
                default:
                    UISetString(menu, MAIN_MENU_ID_DESCRIPTION, NULL);
            }
        } else
            UISetString(menu, MAIN_MENU_ID_DESCRIPTION, NULL);
    }

    return 0;
}

static int ProcessSpaceValue(u32 space, u32 *ProcessedSpace)
{
    u32 temp;
    int unit;

    unit = 0;
    temp = space / 2048; //*512 / 1024 / 1024
    // 0 = MB, 1 = GB, 2 = TB
    while (temp >= 1000 && unit <= 2) {
        unit++;
        temp /= 1000;
    }

    *ProcessedSpace = temp;
    return (SYS_UI_LBL_MB + unit);
}
#endif

static int CheckFormat(void)
{
    int status;

    status = HDDCheckStatus();
    switch (status) {
        case 1: // Not formatted
#ifdef FSCK
            DisplayErrorMessage(SYS_UI_MSG_HDD_NOT_FORMATTED);
#else
            if (DisplayPromptMessage(SYS_UI_MSG_FORMAT_HDD, SYS_UI_LBL_CANCEL, SYS_UI_LBL_OK) == 2) {
                status = 0;

                if (hddFormat() != 0)
                    DisplayErrorMessage(SYS_UI_MSG_FORMAT_HDD_FAILED);
            }
#endif
            break;
        case 0: // Formatted
            break;
        case 2:  // Not a usable HDD
        case 3:  // No HDD connected
        default: // Unknown errors
            DisplayErrorMessage(SYS_UI_MSG_NO_HDD);
            break;
    }

    return status;
}

void MainMenu(void)
{
#ifndef FSCK
    int result;
    short int option;
    unsigned char done;
    u32 ProcessedSpace;
    int SpaceUnitLabel;
    struct UIMenu *CurrentMenu;
#endif

    if (CheckFormat() != 0)
        return;

#ifdef FSCK
    // While an indication of whether S.M.A.R.T. has failed or not would be great at this point, the MBR program won't boot FSCK if S.M.A.R.T. has failed.
    if (DisplayPromptMessage(SYS_UI_MSG_AUTO_SCAN_DISK_CFM, SYS_UI_LBL_OK, SYS_UI_LBL_CANCEL) == 1)
        ScanDisk(0);
#else
    if (HDDCheckSMARTStatus())
        DisplayErrorMessage(SYS_UI_MSG_HDD_SMART_FAILED);

    if (HDDCheckSectorErrorStatus() || HDDCheckPartErrorStatus())
        DisplayErrorMessage(SYS_UI_MSG_HDD_CORRUPTED);

    // Setup menu.
    UISetString(&HDDMainMenu, MAIN_MENU_ID_HDD0_MODEL, GetATADeviceModel(0));
    UISetString(&HDDMainMenu, MAIN_MENU_ID_HDD0_SERIAL, GetATADeviceSerial(0));
    UISetString(&HDDMainMenu, MAIN_MENU_ID_HDD0_FIRMWARE, GetATADeviceFWVersion(0));
    UISetLabel(&HDDMainMenu, MAIN_MENU_ID_HDD0_SMART, GetATADeviceSMARTStatus(0) == 0 ? SYS_UI_LBL_PASS : SYS_UI_LBL_FAIL);
    SpaceUnitLabel = ProcessSpaceValue(GetATADeviceCapacity(0), &ProcessedSpace);
    UISetValue(&HDDMainMenu, MAIN_MENU_ID_HDD0_CAPACITY, ProcessedSpace);
    UISetLabel(&HDDMainMenu, MAIN_MENU_ID_HDD0_CAPACITY_UNIT, SpaceUnitLabel);
    UISetString(&HDDMainMenu, MAIN_MENU_ID_VERSION, "v" HDDC_VERSION);
    CurrentMenu = &HDDMainMenu;
    option = 0;

    DrawMenuEntranceSlideInMenuAnimation(0);

    done = 0;
    while (!done) {
        option = UIExecMenu(CurrentMenu, option, &CurrentMenu, &MainMenuUpdateCallback);

        UITransition(&HDDMainMenu, UIMT_LEFT_OUT, option);

        switch (option) {
            case MAIN_MENU_ID_BTN_SCAN:
                if (DisplayPromptMessage(SYS_UI_MSG_SCAN_DISK_CFM, SYS_UI_LBL_CANCEL, SYS_UI_LBL_OK) == 2)
                    ScanDisk(0);
                break;
            case MAIN_MENU_ID_BTN_OPT:
                if (DisplayPromptMessage(SYS_UI_MSG_OPT_DISK_CFM, SYS_UI_LBL_CANCEL, SYS_UI_LBL_OK) == 2)
                    OptimizeDisk(0);
                break;
            case MAIN_MENU_ID_BTN_SURF_SCAN:
                if (DisplayPromptMessage(SYS_UI_MSG_SURF_SCAN_DISK_CFM, SYS_UI_LBL_CANCEL, SYS_UI_LBL_OK) == 2)
                    SurfScanDisk(0);
                break;
            case MAIN_MENU_ID_BTN_ZERO_FILL:
                if (DisplayPromptMessage(SYS_UI_MSG_ZERO_FILL_DISK_CFM, SYS_UI_LBL_CANCEL, SYS_UI_LBL_OK) == 2 && PerformZeroFillDoubleConfirmation() == 1) {
                    ZeroFillDisk(0);
                    if (CheckFormat() != 0)
                        done = 1;
                }
                break;
            case 1: // User cancelled
            case MAIN_MENU_ID_BTN_EXIT:
                if (DisplayPromptMessage(SYS_UI_MSG_QUIT, SYS_UI_LBL_CANCEL, SYS_UI_LBL_OK) == 2)
                    done = 1;
                break;
        }

        if (!done)
            UITransition(&HDDMainMenu, UIMT_LEFT_IN, option);
    }
#endif

    DrawMenuExitAnimation();
}

#ifndef FSCK
static unsigned char ETADisplayed = 1;

void InitProgressScreen(int label)
{
    int ReadErrorDisplay, TotalProgressDisplay;

    switch (label) {
        case SYS_UI_LBL_SURF_SCANNING_DISK:
            ReadErrorDisplay     = 1;
            TotalProgressDisplay = 0;
            break;
        case SYS_UI_LBL_OPTIMIZING_DISK_P1:
        case SYS_UI_LBL_OPTIMIZING_DISK_P2:
            ReadErrorDisplay     = 0;
            TotalProgressDisplay = 1;
            break;
        default:
            ReadErrorDisplay     = 0;
            TotalProgressDisplay = 0;
    }

    UISetLabel(&ProgressScreen, PRG_SCREEN_ID_TITLE, label);
    UISetVisible(&ProgressScreen, PRG_SCREEN_ID_LBL_RD_ERRS, ReadErrorDisplay);
    UISetVisible(&ProgressScreen, PRG_SCREEN_ID_RD_ERRS, ReadErrorDisplay);
    UISetVisible(&ProgressScreen, PRG_SCREEN_ID_TOTAL_PROGRESS, TotalProgressDisplay);
    ETADisplayed = 1;
    UISetType(&ProgressScreen, PRG_SCREEN_ID_ETA_HOURS, MITEM_VALUE);
    UISetType(&ProgressScreen, PRG_SCREEN_ID_ETA_MINS, MITEM_VALUE);
    UISetType(&ProgressScreen, PRG_SCREEN_ID_ETA_SECS, MITEM_VALUE);
}

void DrawDiskSurfScanningScreen(int PercentageComplete, unsigned int SecondsRemaining, unsigned int NumBadSectors)
{
    unsigned int HoursRemaining;
    unsigned char MinutesRemaining;

    UISetValue(&ProgressScreen, PRG_SCREEN_ID_PROGRESS, PercentageComplete);
    UISetValue(&ProgressScreen, PRG_SCREEN_ID_RD_ERRS, NumBadSectors);
    if (SecondsRemaining < UINT_MAX) {
        if (!ETADisplayed) {
            UISetType(&ProgressScreen, PRG_SCREEN_ID_ETA_HOURS, MITEM_VALUE);
            UISetType(&ProgressScreen, PRG_SCREEN_ID_ETA_MINS, MITEM_VALUE);
            UISetType(&ProgressScreen, PRG_SCREEN_ID_ETA_SECS, MITEM_VALUE);
            ETADisplayed = 1;
        }

        HoursRemaining   = SecondsRemaining / 3600;
        MinutesRemaining = (SecondsRemaining - HoursRemaining * 3600) / 60;
        UISetValue(&ProgressScreen, PRG_SCREEN_ID_ETA_HOURS, HoursRemaining);
        UISetValue(&ProgressScreen, PRG_SCREEN_ID_ETA_MINS, MinutesRemaining);
        UISetValue(&ProgressScreen, PRG_SCREEN_ID_ETA_SECS, SecondsRemaining - HoursRemaining * 3600 - MinutesRemaining * 60);
    } else {
        if (ETADisplayed) {
            UISetType(&ProgressScreen, PRG_SCREEN_ID_ETA_HOURS, MITEM_STRING);
            UISetString(&ProgressScreen, PRG_SCREEN_ID_ETA_HOURS, "--");
            UISetType(&ProgressScreen, PRG_SCREEN_ID_ETA_MINS, MITEM_STRING);
            UISetString(&ProgressScreen, PRG_SCREEN_ID_ETA_MINS, "--");
            UISetType(&ProgressScreen, PRG_SCREEN_ID_ETA_SECS, MITEM_STRING);
            UISetString(&ProgressScreen, PRG_SCREEN_ID_ETA_SECS, "--");
            ETADisplayed = 0;
        }
    }

    UIDrawMenu(&ProgressScreen, 0, 0, 0, -1);

    SyncFlipFB(&UIDrawGlobal);
}

void DrawDiskZeroFillingScreen(int PercentageComplete, unsigned int SecondsRemaining)
{
    unsigned int HoursRemaining;
    unsigned char MinutesRemaining;

    UISetValue(&ProgressScreen, PRG_SCREEN_ID_PROGRESS, PercentageComplete);
    if (SecondsRemaining < UINT_MAX) {
        if (!ETADisplayed) {
            UISetType(&ProgressScreen, PRG_SCREEN_ID_ETA_HOURS, MITEM_VALUE);
            UISetType(&ProgressScreen, PRG_SCREEN_ID_ETA_MINS, MITEM_VALUE);
            UISetType(&ProgressScreen, PRG_SCREEN_ID_ETA_SECS, MITEM_VALUE);
            ETADisplayed = 1;
        }

        HoursRemaining   = SecondsRemaining / 3600;
        MinutesRemaining = (SecondsRemaining - HoursRemaining * 3600) / 60;
        UISetValue(&ProgressScreen, PRG_SCREEN_ID_ETA_HOURS, HoursRemaining);
        UISetValue(&ProgressScreen, PRG_SCREEN_ID_ETA_MINS, MinutesRemaining);
        UISetValue(&ProgressScreen, PRG_SCREEN_ID_ETA_SECS, SecondsRemaining - HoursRemaining * 3600 - MinutesRemaining * 60);
    } else {
        if (ETADisplayed) {
            UISetType(&ProgressScreen, PRG_SCREEN_ID_ETA_HOURS, MITEM_STRING);
            UISetString(&ProgressScreen, PRG_SCREEN_ID_ETA_HOURS, "--");
            UISetType(&ProgressScreen, PRG_SCREEN_ID_ETA_MINS, MITEM_STRING);
            UISetString(&ProgressScreen, PRG_SCREEN_ID_ETA_MINS, "--");
            UISetType(&ProgressScreen, PRG_SCREEN_ID_ETA_SECS, MITEM_STRING);
            UISetString(&ProgressScreen, PRG_SCREEN_ID_ETA_SECS, "--");
            ETADisplayed = 0;
        }
    }

    UIDrawMenu(&ProgressScreen, 0, 0, 0, -1);

    SyncFlipFB(&UIDrawGlobal);
}

void DrawDiskScanningScreen(int PercentageComplete, unsigned int SecondsRemaining)
{
    unsigned int HoursRemaining;
    unsigned char MinutesRemaining;

    UISetValue(&ProgressScreen, PRG_SCREEN_ID_PROGRESS, PercentageComplete);
    if (SecondsRemaining < UINT_MAX) {
        if (!ETADisplayed) {
            UISetType(&ProgressScreen, PRG_SCREEN_ID_ETA_HOURS, MITEM_VALUE);
            UISetType(&ProgressScreen, PRG_SCREEN_ID_ETA_MINS, MITEM_VALUE);
            UISetType(&ProgressScreen, PRG_SCREEN_ID_ETA_SECS, MITEM_VALUE);
            ETADisplayed = 1;
        }

        HoursRemaining   = SecondsRemaining / 3600;
        MinutesRemaining = (SecondsRemaining - HoursRemaining * 3600) / 60;
        UISetValue(&ProgressScreen, PRG_SCREEN_ID_ETA_HOURS, HoursRemaining);
        UISetValue(&ProgressScreen, PRG_SCREEN_ID_ETA_MINS, MinutesRemaining);
        UISetValue(&ProgressScreen, PRG_SCREEN_ID_ETA_SECS, SecondsRemaining - HoursRemaining * 3600 - MinutesRemaining * 60);
    } else {
        if (ETADisplayed) {
            UISetType(&ProgressScreen, PRG_SCREEN_ID_ETA_HOURS, MITEM_STRING);
            UISetString(&ProgressScreen, PRG_SCREEN_ID_ETA_HOURS, "--");
            UISetType(&ProgressScreen, PRG_SCREEN_ID_ETA_MINS, MITEM_STRING);
            UISetString(&ProgressScreen, PRG_SCREEN_ID_ETA_MINS, "--");
            UISetType(&ProgressScreen, PRG_SCREEN_ID_ETA_SECS, MITEM_STRING);
            UISetString(&ProgressScreen, PRG_SCREEN_ID_ETA_SECS, "--");
            ETADisplayed = 0;
        }
    }

    UIDrawMenu(&ProgressScreen, 0, 0, 0, -1);

    SyncFlipFB(&UIDrawGlobal);
}

void DrawDiskOptimizationScreen(int PercentageComplete, int TotalPercentageComplete, unsigned int SecondsRemaining)
{
    unsigned int HoursRemaining;
    unsigned char MinutesRemaining;

    UISetValue(&ProgressScreen, PRG_SCREEN_ID_PROGRESS, PercentageComplete);
    UISetValue(&ProgressScreen, PRG_SCREEN_ID_TOTAL_PROGRESS, TotalPercentageComplete);
    if (SecondsRemaining < UINT_MAX) {
        if (!ETADisplayed) {
            UISetType(&ProgressScreen, PRG_SCREEN_ID_ETA_HOURS, MITEM_VALUE);
            UISetType(&ProgressScreen, PRG_SCREEN_ID_ETA_MINS, MITEM_VALUE);
            UISetType(&ProgressScreen, PRG_SCREEN_ID_ETA_SECS, MITEM_VALUE);
            ETADisplayed = 1;
        }

        HoursRemaining   = SecondsRemaining / 3600;
        MinutesRemaining = (SecondsRemaining - HoursRemaining * 3600) / 60;
        UISetValue(&ProgressScreen, PRG_SCREEN_ID_ETA_HOURS, HoursRemaining);
        UISetValue(&ProgressScreen, PRG_SCREEN_ID_ETA_MINS, MinutesRemaining);
        UISetValue(&ProgressScreen, PRG_SCREEN_ID_ETA_SECS, SecondsRemaining - HoursRemaining * 3600 - MinutesRemaining * 60);
    } else {
        if (ETADisplayed) {
            UISetType(&ProgressScreen, PRG_SCREEN_ID_ETA_HOURS, MITEM_STRING);
            UISetString(&ProgressScreen, PRG_SCREEN_ID_ETA_HOURS, "--");
            UISetType(&ProgressScreen, PRG_SCREEN_ID_ETA_MINS, MITEM_STRING);
            UISetString(&ProgressScreen, PRG_SCREEN_ID_ETA_MINS, "--");
            UISetType(&ProgressScreen, PRG_SCREEN_ID_ETA_SECS, MITEM_STRING);
            UISetString(&ProgressScreen, PRG_SCREEN_ID_ETA_SECS, "--");
            ETADisplayed = 0;
        }
    }

    UIDrawMenu(&ProgressScreen, 0, 0, 0, -1);

    SyncFlipFB(&UIDrawGlobal);
}

int GetBadSectorAction(u32 BadSectorLBA)
{
    char CharBuffer[192];

    snprintf(CharBuffer, sizeof(CharBuffer) / sizeof(char), GetUIString(SYS_UI_MSG_BAD_SECTOR_FOUND), BadSectorLBA);
    switch (ShowMessageBox(SYS_UI_LBL_REMAP, SYS_UI_LBL_REMAP_ALL, SYS_UI_LBL_SKIP, SYS_UI_LBL_SKIP_ALL, CharBuffer, SYS_UI_LBL_CONFIRM)) {
        case 4:
            return BAD_SECTOR_HANDLING_MODE_SKIP;
        case 3:
            return BAD_SECTOR_HANDLING_MODE_REMAP_ALL;
        case 2:
            return BAD_SECTOR_HANDLING_MODE_SKIP_ALL;
        default:
            return BAD_SECTOR_HANDLING_MODE_REMAP;
    }
}
#endif

void DisplayScanCompleteResults(unsigned int ErrorsFound, unsigned int ErrorsFixed)
{
    UISetString(&ScanResultsScreen, SCN_SCREEN_ID_RESULT, GetUIString(SYS_UI_MSG_SCAN_DISK_COMPLETED_OK));

    if (ErrorsFound) {
        UISetValue(&ScanResultsScreen, SCN_SCREEN_ID_ERRS_FOUND, ErrorsFound);
        UISetValue(&ScanResultsScreen, SCN_SCREEN_ID_ERRS_FIXED, ErrorsFixed);
        UISetVisible(&ScanResultsScreen, SCN_SCREEN_ID_LBL_ERRS_FOUND, 1);
        UISetVisible(&ScanResultsScreen, SCN_SCREEN_ID_ERRS_FOUND, 1);
        UISetVisible(&ScanResultsScreen, SCN_SCREEN_ID_LBL_ERRS_FIXED, 1);
        UISetVisible(&ScanResultsScreen, SCN_SCREEN_ID_ERRS_FIXED, 1);
    } else {
        UISetVisible(&ScanResultsScreen, SCN_SCREEN_ID_LBL_ERRS_FOUND, 0);
        UISetVisible(&ScanResultsScreen, SCN_SCREEN_ID_ERRS_FOUND, 0);
        UISetVisible(&ScanResultsScreen, SCN_SCREEN_ID_LBL_ERRS_FIXED, 0);
        UISetVisible(&ScanResultsScreen, SCN_SCREEN_ID_ERRS_FIXED, 0);
    }

    UISetVisible(&ScanResultsScreen, SCN_SCREEN_ID_LBL_SOME_ERRS_NOT_FIXED, (ErrorsFound != ErrorsFixed));

    UIExecMenu(&ScanResultsScreen, 0, NULL, NULL);
}

void RedrawLoadingScreen(unsigned int frame)
{
    short int xRel, x, y;
    int NumDots;
    GS_RGBAQ rgbaq;

    SyncFlipFB(&UIDrawGlobal);

    NumDots = frame % 240 / 60;

    DrawBackground(&UIDrawGlobal, &BackgroundTexture);
    FontPrintf(&UIDrawGlobal, 10, 10, 0, 2.5f, GS_WHITE_FONT, "HDDChecker v" HDDC_VERSION);

    x = 420;
    y = 380;
    FontPrintfWithFeedback(&UIDrawGlobal, x, y, 0, 1.8f, GS_WHITE_FONT, "Loading ", &xRel, NULL);
    x += xRel;
    switch (NumDots) {
        case 1:
            FontPrintf(&UIDrawGlobal, x, y, 0, 1.8f, GS_WHITE_FONT, ".");
            break;
        case 2:
            FontPrintf(&UIDrawGlobal, x, y, 0, 1.8f, GS_WHITE_FONT, ". .");
            break;
        case 3:
            FontPrintf(&UIDrawGlobal, x, y, 0, 1.8f, GS_WHITE_FONT, ". . .");
            break;
    }

    if (frame < 60) { // Fade in
        rgbaq.r = 0;
        rgbaq.g = 0;
        rgbaq.b = 0;
        rgbaq.q = 0;
        rgbaq.a = 0x80 - (frame * 2);
        DrawSprite(&UIDrawGlobal, 0, 0,
                   UIDrawGlobal.width, UIDrawGlobal.height,
                   0, rgbaq);
    }
}
