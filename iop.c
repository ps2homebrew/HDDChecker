#include <iopcontrol.h>
#include <iopheap.h>
#include <kernel.h>
#include <loadfile.h>
#include <libpad.h>
#include <libpwroff.h>
#include <sbv_patches.h>
#include <sifrpc.h>
#include <stdio.h>
#include <string.h>

#include <fileXio_rpc.h>

#include "main.h"
#include "iop.h"
#include "system.h"
#ifndef FSCK
#include "hdst.h"
#endif

extern unsigned char SIO2MAN_irx[];
extern unsigned int size_SIO2MAN_irx;

extern unsigned char PADMAN_irx[];
extern unsigned int size_PADMAN_irx;

#ifndef FSCK
extern unsigned char MCMAN_irx[];
extern unsigned int size_MCMAN_irx;

extern unsigned char USBD_irx[];
extern unsigned int size_USBD_irx;

extern unsigned char USBHDFSD_irx[];
extern unsigned int size_USBHDFSD_irx;
#endif

extern unsigned char POWEROFF_irx[];
extern unsigned int size_POWEROFF_irx;

extern unsigned char DEV9_irx[];
extern unsigned int size_DEV9_irx;

extern unsigned char ATAD_irx[];
extern unsigned int size_ATAD_irx;

extern unsigned char HDD_irx[];
extern unsigned int size_HDD_irx;

extern unsigned char PFS_irx[];
extern unsigned int size_PFS_irx;

#ifndef FSCK
extern unsigned char HDLOG_irx[];
extern unsigned int size_HDLOG_irx;

extern unsigned char HDST_irx[];
extern unsigned int size_HDST_irx;

extern unsigned char HDCK_irx[];
extern unsigned int size_HDCK_irx;

extern unsigned char HDSK_irx[];
extern unsigned int size_HDSK_irx;

extern unsigned char FSSK_irx[];
extern unsigned int size_FSSK_irx;
#endif

extern unsigned char FSCK_irx[];
extern unsigned int size_FSCK_irx;

extern unsigned char IOMANX_irx[];
extern unsigned int size_IOMANX_irx;

extern unsigned char FILEXIO_irx[];
extern unsigned int size_FILEXIO_irx;

u8 dev9Loaded;

#define SYSTEM_INIT_THREAD_STACK_SIZE 0x1000

struct SystemInitParams
{
    int InitCompleteSema;
    unsigned int flags;
};

static void SystemInitThread(struct SystemInitParams *SystemInitParams)
{
    static const char HDD_args[] = "-o\0"
                                   "3\0"
                                   "-n\0"
                                   "16";
    static const char PFS_args[] = "-o\0"
                                   "2\0"
                                   "-n\0"
                                   "12";
    int i;

    SifExecModuleBuffer(ATAD_irx, size_ATAD_irx, 0, NULL, NULL);
    if (SystemInitParams->flags & IOP_MOD_HDD)
        SifExecModuleBuffer(HDD_irx, size_HDD_irx, sizeof(HDD_args), HDD_args, NULL);
    if (SystemInitParams->flags & IOP_MOD_PFS)
        SifExecModuleBuffer(PFS_irx, size_PFS_irx, sizeof(PFS_args), PFS_args, NULL);
#ifndef FSCK
#ifdef LOG_MESSAGES
    if (SystemInitParams->flags & IOP_MOD_HDLOG)
        SifExecModuleBuffer(HDLOG_irx, size_HDLOG_irx, 0, NULL, NULL);
#endif

    if (SystemInitParams->flags & IOP_MOD_HDST) {
        SifExecModuleBuffer(HDST_irx, size_HDST_irx, 0, NULL, NULL);
        for (i = 0; i < NUM_SUPPORTED_DEVICES; i++)
            InitATADevice(i);
        SetHDSTIOBufferSize(256);
    }

    if (SystemInitParams->flags & IOP_MOD_HDCK)
        SifExecModuleBuffer(HDCK_irx, size_HDSK_irx, 0, NULL, NULL);
    if (SystemInitParams->flags & IOP_MOD_HDSK)
        SifExecModuleBuffer(HDSK_irx, size_HDSK_irx, 0, NULL, NULL);
    if (SystemInitParams->flags & IOP_MOD_FSSK)
        SifExecModuleBuffer(FSSK_irx, size_FSSK_irx, 0, NULL, NULL);
#endif
    if (SystemInitParams->flags & IOP_MOD_FSCK)
        SifExecModuleBuffer(FSCK_irx, size_FSCK_irx, 0, NULL, NULL);

    SifExitIopHeap();
    SifLoadFileExit();

    SignalSema(SystemInitParams->InitCompleteSema);
    ExitDeleteThread();
}

int IopInitStart(unsigned int flags)
{
    int ret, stat;
    ee_sema_t sema;
    static struct SystemInitParams InitThreadParams;
    static u8 SysInitThreadStack[SYSTEM_INIT_THREAD_STACK_SIZE] __attribute__((aligned(16)));

    if (!(flags & IOP_REBOOT)) {
        SifInitRpc(0);
    } else {
        PadDeinitPads();
    }

    while (!SifIopReset("", 0)) {};

    // Do something useful while the IOP resets.
    sema.init_count = 0;
    sema.max_count  = 1;
    sema.attr = sema.option           = 0;
    InitThreadParams.InitCompleteSema = CreateSema(&sema);
    InitThreadParams.flags            = flags;

    while (!SifIopSync()) {};

    SifInitRpc(0);
    SifInitIopHeap();
    SifLoadFileInit();

    sbv_patch_enable_lmb();

    SifExecModuleBuffer(IOMANX_irx, size_IOMANX_irx, 0, NULL, NULL);
    SifExecModuleBuffer(FILEXIO_irx, size_FILEXIO_irx, 0, NULL, NULL);

    fileXioInit();

    SifExecModuleBuffer(POWEROFF_irx, size_POWEROFF_irx, 0, NULL, NULL);
    ret        = SifExecModuleBuffer(DEV9_irx, size_DEV9_irx, 0, NULL, &stat);
    dev9Loaded = (ret >= 0 && stat == 0); // dev9.irx must have loaded successfully and returned RESIDENT END.

    SifExecModuleBuffer(SIO2MAN_irx, size_SIO2MAN_irx, 0, NULL, NULL);
    SifExecModuleBuffer(PADMAN_irx, size_PADMAN_irx, 0, NULL, NULL);

#ifndef FSCK
    SifExecModuleBuffer(MCMAN_irx, size_MCMAN_irx, 0, NULL, NULL);
    SifExecModuleBuffer(USBD_irx, size_USBD_irx, 0, NULL, NULL);
    SifExecModuleBuffer(USBHDFSD_irx, size_USBHDFSD_irx, 0, NULL, NULL);
#endif

    SysCreateThread(SystemInitThread, SysInitThreadStack, SYSTEM_INIT_THREAD_STACK_SIZE, &InitThreadParams, 0x2);

    poweroffInit();
    poweroffSetCallback(&poweroffCallback, NULL);
    PadInitPads();

    return InitThreadParams.InitCompleteSema;
}

void IopDeinit(void)
{
    PadDeinitPads();

    fileXioExit();
    SifExitRpc();
}
