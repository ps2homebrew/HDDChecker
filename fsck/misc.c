#include <cdvdman.h>
#include <errno.h>
#include <intrman.h>
#include <thevent.h>
#include <thbase.h>
#include <sysmem.h>
#include <sysclib.h>
#include <stdio.h>

#include "pfs-opt.h"
#include "libpfs.h"
#include "misc.h"

//0x00002c00
int fsckCreateEventFlag(void)
{
	iop_event_t EventFlagData;

	EventFlagData.attr = EA_MULTI;
	EventFlagData.bits = 0;
	return CreateEventFlag(&EventFlagData);
}

//0x00002c2c
int fsckCreateThread(void (*function)(void *arg), int StackSize)
{
	iop_thread_t ThreadData;

	ThreadData.attr = TH_C;
	ThreadData.thread = function;
	ThreadData.priority = 0x7b;
	ThreadData.stacksize = StackSize;
	return CreateThread(&ThreadData);
}
