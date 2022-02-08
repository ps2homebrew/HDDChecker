#include <stdio.h>
#include <string.h>
#include <kernel.h>
#include <hdd-ioctl.h>
#include <sys/fcntl.h>

#include <libgs.h>

#include "main.h"
#include "iop.h"
#include "hdst.h"
#include "system.h"
#include "UI.h"
#include "menu.h"
#include "pad.h"

int VBlankStartSema;
int InstallLockSema;

static int VBlankStartHandler(int cause)
{
	ee_sema_t sema;
	iReferSemaStatus(VBlankStartSema, &sema);
	if(sema.count<sema.max_count) iSignalSema(VBlankStartSema);
	return 0;
}

static void DeinitServices(void)
{
	DisableIntc(kINTC_VBLANK_START);
	RemoveIntcHandler(kINTC_VBLANK_START, 0);
	DeleteSema(VBlankStartSema);
	DeleteSema(InstallLockSema);

	IopDeinit();
}

int main(int argc, char *argv[])
{
	unsigned char PadStatus, done;
	unsigned int FrameNum;
	ee_sema_t ThreadSema;
	int result, InitSemaID, BootDevice;

	chdir("mass:/HDDChecker/");
	//chdir("hdd0:__system:pfs:/HDDChecker/");
	//chdir("hdd0:__system:pfs:/fsck/");	//For testing the standalone FSCK tool.
	if((BootDevice = GetBootDeviceID()) == BOOT_DEVICE_UNKNOWN)
		Exit(ENODEV);

#ifdef FSCK
	InitSemaID = IopInitStart(IOP_MODSET_SA_INIT);
#else
	InitSemaID = IopInitStart(IOP_MODSET_INIT);
#endif

	ThreadSema.init_count=0;
	ThreadSema.max_count=1;
	ThreadSema.attr=ThreadSema.option=0;
	VBlankStartSema=CreateSema(&ThreadSema);

	ThreadSema.init_count=1;
	ThreadSema.max_count=1;
	ThreadSema.attr=ThreadSema.option=0;
	InstallLockSema=CreateSema(&ThreadSema);

	AddIntcHandler(kINTC_VBLANK_START, &VBlankStartHandler, 0);
	EnableIntc(kINTC_VBLANK_START);

	if(BootDevice != BOOT_DEVICE_HDD)
	{
		if(SysBootDeviceInit() != 0)
		{
			WaitSema(InitSemaID);
			DeinitServices();
			Exit(-1);
		}

		if(InitializeUI(0) != 0)
		{
			DeinitializeUI();

			WaitSema(InitSemaID);
			DeinitServices();
			Exit(-1);
		}

		FrameNum=0;
		/* Draw something nice here while waiting... */
		do{
			RedrawLoadingScreen(FrameNum);
			FrameNum++;
		}while(PollSema(InitSemaID)!=InitSemaID);
	} else
		WaitSema(InitSemaID);
	DeleteSema(InitSemaID);

	if(BootDevice == BOOT_DEVICE_HDD)
	{
		if(SysBootDeviceInit() != 0)
		{
			DeinitServices();
			Exit(-1);
		}

		if(InitializeUI(1) != 0)
		{
			DeinitializeUI();

			fileXioUmount("pfs0:");
			DeinitServices();
			Exit(-1);
		}
	}

	MainMenu();

	DeinitializeUI();
	if(BootDevice == BOOT_DEVICE_HDD)
		fileXioUmount("pfs0:");
	DeinitServices();

	return 0;
}
