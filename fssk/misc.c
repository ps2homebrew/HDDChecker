#include <errno.h>
#include <stdio.h>
#include <sysclib.h>
#include <thbase.h>
#include <thevent.h>
#include <hdd-ioctl.h>

#include "pfs-opt.h"
#include "libpfs.h"
#include "misc.h"

int fsskCreateEventFlag(void)
{
    iop_event_t EventFlagData;

    EventFlagData.attr = EA_MULTI;
    EventFlagData.bits = 0;
    return CreateEventFlag(&EventFlagData);
}

int fsskCreateThread(void (*function)(void *arg), int StackSize)
{
    iop_thread_t ThreadData;

    ThreadData.attr      = TH_C;
    ThreadData.thread    = function;
    ThreadData.priority  = 0x7b;
    ThreadData.stacksize = StackSize;
    return CreateThread(&ThreadData);
}
