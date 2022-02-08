#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <kernel.h>
#include <limits.h>
#include <libpad.h>
#include <libpwroff.h>
#include <fileXio_rpc.h>
#include <atahw.h>
#include <hdd-ioctl.h>
#include <usbhdfsd-common.h>
#include <sys/fcntl.h>

#include <libgs.h>

#include "main.h"
#include "iop.h"
#include "menu.h"
#include "UI.h"
#include "fsck/fsck-ioctl.h"
#include "hdsk/hdsk-devctl.h"
#include "fssk/fssk-ioctl.h"
#include "hdst.h"
#include "system.h"

extern void *_gp;

extern unsigned short int SelectButton, CancelButton;
extern u8 dev9Loaded;
extern int InstallLockSema;

int GetBootDeviceID(void)
{
	static int BootDevice = -2;
	char path[256];
	int result;

	if(BootDevice < BOOT_DEVICE_UNKNOWN)
	{
		getcwd(path, sizeof(path));

		if(!strncmp(path, "hdd:", 4) || !strncmp(path, "hdd0:", 5)) result=BOOT_DEVICE_HDD;
#ifndef FSCK
		else if(!strncmp(path, "mc0:", 4)) result=BOOT_DEVICE_MC0;
		else if(!strncmp(path, "mc1:", 4)) result=BOOT_DEVICE_MC1;
		else if(!strncmp(path, "mass:", 5) || !strncmp(path, "mass0:", 6)) result=BOOT_DEVICE_MASS;
#endif
		else result=BOOT_DEVICE_UNKNOWN;

		BootDevice = result;
	} else
		result = BootDevice;

	return result;
}

static int SysInitMount(void)
{
	static char BlockDevice[38] = "";
	char command[256];
	const char *MountPath;
	int BlockDeviceNameLen, result;

	if(BlockDevice[0] == '\0')
	{
		//Format: hdd0:partition:pfs:/path_to_file_on_partition
		//However, getcwd will return the path, without the filename (as parsed by libc's init.c).
		getcwd(command, sizeof(command));
		if(strlen(command)>6 && (MountPath=strchr(&command[5], ':'))!=NULL)
		{
			BlockDeviceNameLen = (unsigned int)MountPath-(unsigned int)command;
			strncpy(BlockDevice, command, BlockDeviceNameLen);
			BlockDevice[BlockDeviceNameLen]='\0';

			MountPath++;	//This is the location of the mount path;

			if((result = fileXioMount("pfs0:", BlockDevice, FIO_MT_RDONLY)) >= 0)
				result = chdir(MountPath);

			return result;
		} else
			result = -EINVAL;
	} else {
		if((result = fileXioMount("pfs0:", BlockDevice, FIO_MT_RDONLY)) >= 0)
			result = 0;
	}

	return result;
}

int SysBootDeviceInit(void)
{
	int result;

	switch(GetBootDeviceID())
	{
		case BOOT_DEVICE_HDD:	//This will only work if the modules are loaded (i.e. at boot).
			result = SysInitMount();
			break;
		default:
			result = 0;
	}

	return result;
}

int GetConsoleRegion(void)
{
	static int region = -1;
	FILE *file;

	if(region < 0)
	{
		if((file = fopen("rom0:ROMVER", "r")) != NULL)
		{
			fseek(file, 4, SEEK_SET);
			switch(fgetc(file))
			{
				case 'J':
					region = CONSOLE_REGION_JAPAN;
					break;
				case 'A':
				case 'H':
					region = CONSOLE_REGION_USA;
					break;
				case 'E':
					region = CONSOLE_REGION_EUROPE;
					break;
				case 'C':
					region = CONSOLE_REGION_CHINA;
					break;
			}

			fclose(file);
		}
	}

	return region;
}

int GetConsoleVMode(void)
{
	switch(GetConsoleRegion())
	{
		case CONSOLE_REGION_EUROPE:
			return 1;
		default:
			return 0;
	}
}

int IsPSX(void)
{
	static int psx = -1;
	FILE * file;
	int result;

	if((result = psx) < 0)
	{
		if((file = fopen("rom0:PSXVER", "r")) != NULL)
		{
			psx = 1;
			fclose(file);
		} else
			psx = 0;

		result = psx;
	}

	return result;
}

int SysCreateThread(void *function, void *stack, unsigned int StackSize, void *arg, int priority)
{
	ee_thread_t ThreadData;
	int ThreadID;

	ThreadData.func=function;
	ThreadData.stack=stack;
	ThreadData.stack_size=StackSize;
	ThreadData.gp_reg=&_gp;
	ThreadData.initial_priority=priority;
	ThreadData.attr=ThreadData.option=0;

	if((ThreadID=CreateThread(&ThreadData))>=0)
	{
		if(StartThread(ThreadID, arg)<0)
		{
			DeleteThread(ThreadID);
			ThreadID=-1;
		}
	}

	return ThreadID;
}

#ifndef FSCK
static unsigned char UserAborted;
#endif
static unsigned int ErrorsFound;
static unsigned int ErrorsFixed;

#ifdef FSCK
static int FsckPartition(const char *partition)
{
	char cmd[64];
	struct fsckStatus status;
	int fd, result, InitSemaID;

	DisplayFlashStatusUpdate(SYS_UI_MSG_PLEASE_WAIT);

	InitSemaID = IopInitStart(IOP_MODSET_SA_FSCK);
	result = 0;

	WaitSema(InitSemaID);
	DeleteSema(InitSemaID);

	sprintf(cmd, "fsck:%s", partition);
	if((fd = fileXioOpen(cmd, 0, FSCK_MODE_VERBOSITY(0)|FSCK_MODE_AUTO|FSCK_MODE_WRITE)) >= 0)
	{
		if((result = fileXioIoctl2(fd, FSCK_IOCTL2_CMD_START, NULL, 0, NULL, 0)) == 0)
		{
			result = fileXioIoctl2(fd, FSCK_IOCTL2_CMD_WAIT, NULL, 0, NULL, 0);
		}

		if (result == 0 && (result = fileXioIoctl2(fd, FSCK_IOCTL2_CMD_GET_STATUS, NULL, 0, &status, sizeof(status))) == 0)
			result = 0;

		if(result == 0)
		{
			ErrorsFound += status.errorCount;
			ErrorsFixed += status.fixedErrorCount;
		}

		fileXioClose(fd);
	} else
		result = fd;

	return result;
}

int ScanDisk(int unit)
{
	char ErrorPartName[64] = "hdd0:";

	int result, InitSemaID;

	WaitSema(InstallLockSema);

	DisplayFlashStatusUpdate(SYS_UI_MSG_PLEASE_WAIT);

	if(fileXioDevctl("hdd0:", HDIOC_GETSECTORERROR, NULL, 0, NULL, 0) == 0)
	{
		if(fileXioDevctl("hdd0:", HDIOC_GETERRORPARTNAME, NULL, 0, &ErrorPartName[5], sizeof(ErrorPartName) - 5) != 0)
		{
			ErrorsFound = 0;
			ErrorsFixed = 0;

			if((result = FsckPartition(ErrorPartName)) == 0)
				DisplayScanCompleteResults(ErrorsFound, ErrorsFixed);
			else
				DisplayErrorMessage(SYS_UI_MSG_HDD_FAULT);

			DisplayFlashStatusUpdate(SYS_UI_MSG_PLEASE_WAIT);
			InitSemaID = IopInitStart(IOP_MODSET_SA_MAIN);
			WaitSema(InitSemaID);
			DeleteSema(InitSemaID);
		} else
			DisplayScanCompleteResults(0, 0);	//No fault. Why are we here then?
	} else
		DisplayErrorMessage(SYS_UI_MSG_HDD_FAULT);

	SignalSema(InstallLockSema);

	return result;
}

#else
static int HdckDisk(int unit)
{
	char device[]="hdck0:";
	int InitSemaID, result;

	DisplayFlashStatusUpdate(SYS_UI_MSG_PLEASE_WAIT);

	InitSemaID = IopInitStart(IOP_MODSET_HDCK);
	device[4] = '0' + unit;

	WaitSema(InitSemaID);
	DeleteSema(InitSemaID);

	SysBootDeviceInit();
	ReinitializeUI();

#ifdef LOG_MESSAGES
	IopStartLog("hdck.log");
	printf("# Log generated by HDDChecker v"HDDC_VERSION", built on "__DATE__" "__TIME__"\n");
#endif

	result = fileXioDevctl(device, 0, NULL, 0, NULL, 0);

#ifdef LOG_MESSAGES
	IopStopLog();
#endif

	return result;
}

static int FsckDisk(int unit)
{
	char bdevice[]="hdd0:", cmd[64];
	iox_dirent_t dirent;
	struct fsckStatus status;
	unsigned int PadStatus, CurrentCPUTicks, PreviousCPUTicks, seconds, TimeElasped, rate, partitions, i;
	int PercentageComplete;
	int bfd, fd, result, InitSemaID;

	DisplayFlashStatusUpdate(SYS_UI_MSG_PLEASE_WAIT);

	InitSemaID = IopInitStart(IOP_MODSET_FSCK);
	bdevice[3] = '0' + unit;
	result = 0;

	InitProgressScreen(SYS_UI_LBL_SCANNING_DISK);

	WaitSema(InitSemaID);
	DeleteSema(InitSemaID);

	SysBootDeviceInit();
	ReinitializeUI();

	//Count the number of partitions on the disk.
	partitions = 0;
	if((bfd = fileXioDopen(bdevice)) >= 0)
	{
		while(fileXioDread(bfd, &dirent) > 0)
			partitions++;

		fileXioDclose(bfd);
	}

	if(partitions < 1)	//No partitions?
		return 0;

#ifdef LOG_MESSAGES
	IopStartLog("fsck.log");
	printf("# Log generated by HDDChecker v"HDDC_VERSION", built on "__DATE__" "__TIME__"\n");
#endif

	//Now, scan the disk.
	if((bfd = fileXioDopen(bdevice)) >= 0)
	{
		for(i = 0; fileXioDread(bfd, &dirent) > 0; i++)
		{
			if(!(dirent.stat.attr & APA_FLAG_SUB) && dirent.stat.mode == APA_TYPE_PFS)
			{
				printf("# fsck hdd%d:%s\n", unit, dirent.name);

				sprintf(cmd, "fsck:hdd%d:%s", unit, dirent.name);
				if((fd = fileXioOpen(cmd, 0, FSCK_MODE_VERBOSITY(FSCK_VERBOSITY)|FSCK_MODE_AUTO|FSCK_MODE_WRITE)) >= 0)
				{
					if((result = fileXioIoctl2(fd, FSCK_IOCTL2_CMD_START, NULL, 0, NULL, 0)) == 0)
					{
						result=0;
						TimeElasped=0;
						PreviousCPUTicks=cpu_ticks();
						while(fileXioIoctl2(fd, FSCK_IOCTL2_CMD_POLL, NULL, 0, NULL, 0) == 1)
						{
							CurrentCPUTicks=cpu_ticks();
							if((seconds=(CurrentCPUTicks>PreviousCPUTicks?CurrentCPUTicks-PreviousCPUTicks:UINT_MAX-PreviousCPUTicks+CurrentCPUTicks)/295000000)>0)
							{
								TimeElasped+=seconds;
								PreviousCPUTicks=CurrentCPUTicks;
							}
							PercentageComplete = (int)((u64)i * 100 / partitions);
							rate=(TimeElasped>0)?i/TimeElasped:0;	//In partitions/second

							DrawDiskScanningScreen(PercentageComplete, rate>0?(partitions - i)/rate:UINT_MAX);

							PadStatus=ReadCombinedPadStatus();
							if(PadStatus&CancelButton)
							{
								if(DisplayPromptMessage(SYS_UI_MSG_SCAN_DISK_ABORT_CFM, SYS_UI_LBL_NO, SYS_UI_LBL_YES) == 2)
								{
									fileXioIoctl2(fd, FSCK_IOCTL2_CMD_STOP, NULL, 0, NULL, 0);
									result = 0;
									UserAborted = 1;
									break;
								}
							}
						}
					}

					if(result != 0 || (result = fileXioIoctl2(fd, FSCK_IOCTL2_CMD_GET_STATUS, NULL, 0, &status, sizeof(status))) != 0 || UserAborted)
					{
						fileXioClose(fd);
						break;
					}

					ErrorsFound += status.errorCount;
					ErrorsFixed += status.fixedErrorCount;

					fileXioClose(fd);
				} else {	//Ignore partitions that can't be opened, so that the remainder of the disk can be checked..
					result = fd;
					ErrorsFound++;
				}

				putchar('\n');
			}
		}

		fileXioDclose(bfd);
	} else
		result = bfd;

#ifdef LOG_MESSAGES
	IopStopLog();
#endif

	return result;
}

int ScanDisk(int unit)
{
	int result, InitSemaID;

	WaitSema(InstallLockSema);

	DisplayFlashStatusUpdate(SYS_UI_MSG_PLEASE_WAIT);

	ErrorsFound = 0;
	ErrorsFixed = 0;

	if((result = HdckDisk(unit)) == 0 && !UserAborted)
		result = FsckDisk(unit);

	if(result == 0)
	{
		if(!UserAborted)
			DisplayScanCompleteResults(ErrorsFound, ErrorsFixed);
	} else
		DisplayErrorMessage(SYS_UI_MSG_HDD_FAULT);

	DisplayFlashStatusUpdate(SYS_UI_MSG_PLEASE_WAIT);
	InitSemaID = IopInitStart(IOP_MODSET_MAIN);

	WaitSema(InitSemaID);
	DeleteSema(InitSemaID);

	SysBootDeviceInit();
	ReinitializeUI();

	SignalSema(InstallLockSema);

	return result;
}

static int HdskDisk(int unit)
{
	char bdevice[]="hdsk0:";
	struct hdskStat status;
	unsigned int PadStatus, CurrentCPUTicks, PreviousCPUTicks, seconds, TimeElasped, rate;
	int PercentageComplete;
	int result, InitSemaID;
	u32 progress;

	InitSemaID = IopInitStart(IOP_MODSET_HDSK);
	bdevice[4] = '0' + unit;

	InitProgressScreen(SYS_UI_LBL_OPTIMIZING_DISK_P2);

	WaitSema(InitSemaID);
	DeleteSema(InitSemaID);

	SysBootDeviceInit();
	ReinitializeUI();

#ifdef LOG_MESSAGES
	IopStartLog("hdsk.log");
	printf("# Log generated by HDDChecker v"HDDC_VERSION", built on "__DATE__" "__TIME__"\n");
#endif

	//Now, scan the disk.
	if((result = fileXioDevctl(bdevice, HDSK_DEVCTL_GET_HDD_STAT, NULL, 0, &status, sizeof(struct hdskStat))) >= 0)
	{
		printf("# hdsk: total: %x, free: %x\n", status.total, status.free);

		if((status.total > 0) && (result = fileXioDevctl(bdevice, HDSK_DEVCTL_START, NULL, 0, NULL, 0)) == 0)
		{
			result=0;
			TimeElasped=0;
			PreviousCPUTicks=cpu_ticks();
			while(fileXioDevctl(bdevice, HDSK_DEVCTL_POLL, NULL, 0, NULL, 0) == 1)
			{
				progress = (u32)fileXioDevctl(bdevice, HDSK_DEVCTL_GET_PROGRESS, NULL, 0, NULL, 0);
				PercentageComplete = (int)((u64)progress * 100 / status.total);

				CurrentCPUTicks=cpu_ticks();
				if((seconds=(CurrentCPUTicks>PreviousCPUTicks?CurrentCPUTicks-PreviousCPUTicks:UINT_MAX-PreviousCPUTicks+CurrentCPUTicks)/295000000)>0)
				{
					TimeElasped+=seconds;
					PreviousCPUTicks=CurrentCPUTicks;
				}
				rate=(TimeElasped>0)?progress/TimeElasped:0;	//In sectors/second

				DrawDiskOptimizationScreen(PercentageComplete, 50 + PercentageComplete / 2, rate>0?(status.total - progress)/rate:UINT_MAX);

				PadStatus=ReadCombinedPadStatus();
				if(PadStatus&CancelButton)
				{
					if(DisplayPromptMessage(SYS_UI_MSG_OPT_DISK_ABORT_CFM, SYS_UI_LBL_NO, SYS_UI_LBL_YES) == 2)
					{
						fileXioDevctl(bdevice, HDSK_DEVCTL_STOP, NULL, 0, NULL, 0);
						result = 0;
						UserAborted = 1;
						break;
					}
				}

				if(result != 0 || (result = fileXioDevctl(bdevice, HDSK_DEVCTL_GET_STATUS, NULL, 0, NULL, 0)) != 0 || UserAborted)
					break;
			}
		}
	}

#ifdef LOG_MESSAGES
	IopStopLog();
#endif

	return result;
}

static int FsskDisk(int unit)
{
	char bdevice[]="hdd0:", cmd[64];
	iox_dirent_t dirent;
	struct fsskStatus status;
	unsigned int PadStatus, CurrentCPUTicks, PreviousCPUTicks, seconds, TimeElasped, rate, partitions, i;
	int PercentageComplete;
	int bfd, fd, result, InitSemaID;

	InitSemaID = IopInitStart(IOP_MODSET_FSSK);
	bdevice[3] = '0' + unit;
	partitions = 0;
	result = 0;

	InitProgressScreen(SYS_UI_LBL_OPTIMIZING_DISK_P1);

	WaitSema(InitSemaID);
	DeleteSema(InitSemaID);

	SysBootDeviceInit();
	ReinitializeUI();

	//Count the number of partitions on the disk.
	if((bfd = fileXioDopen(bdevice)) >= 0)
	{
		while(fileXioDread(bfd, &dirent) > 0)
			partitions++;

		fileXioDclose(bfd);
	}

	if(partitions < 1)	//No partitions?
		return 0;

#ifdef LOG_MESSAGES
	IopStartLog("fssk.log");
	printf("# Log generated by HDDChecker v"HDDC_VERSION", built on "__DATE__" "__TIME__"\n");
#endif

	//Now, scan the disk.
	if((bfd = fileXioDopen(bdevice)) >= 0)
	{
		for(i = 0; fileXioDread(bfd, &dirent) > 0; i++)
		{
			if(!(dirent.stat.attr & APA_FLAG_SUB) && dirent.stat.mode == APA_TYPE_PFS)
			{
				printf("# fssk hdd%d:%s\n", unit, dirent.name);

				sprintf(cmd, "fssk:hdd%d:%s", unit, dirent.name);
				if((fd = fileXioOpen(cmd, 0, FSSK_MODE_VERBOSITY(FSSK_VERBOSITY))) >= 0)
				{
					if((result = fileXioIoctl2(fd, FSSK_IOCTL2_CMD_START, NULL, 0, NULL, 0)) == 0)
					{
						result=0;
						TimeElasped=0;
						PreviousCPUTicks=cpu_ticks();
						while(fileXioIoctl2(fd, FSSK_IOCTL2_CMD_POLL, NULL, 0, NULL, 0) == 1)
						{
							CurrentCPUTicks=cpu_ticks();
							if((seconds=(CurrentCPUTicks>PreviousCPUTicks?CurrentCPUTicks-PreviousCPUTicks:UINT_MAX-PreviousCPUTicks+CurrentCPUTicks)/295000000)>0)
							{
								TimeElasped+=seconds;
								PreviousCPUTicks=CurrentCPUTicks;
							}
							PercentageComplete = (int)((u64)i * 100 / partitions);
							rate = (TimeElasped > 0) ? i / TimeElasped : 0;	//In partitions/second

							DrawDiskOptimizationScreen(PercentageComplete, PercentageComplete / 2, rate>0?(partitions - i)/rate:UINT_MAX);

							PadStatus=ReadCombinedPadStatus();
							if(PadStatus&CancelButton)
							{
								if(DisplayPromptMessage(SYS_UI_MSG_OPT_DISK_ABORT_CFM, SYS_UI_LBL_NO, SYS_UI_LBL_YES) == 2)
								{
									fileXioIoctl2(fd, FSSK_IOCTL2_CMD_STOP, NULL, 0, NULL, 0);
									result = 0;
									UserAborted = 1;
									break;
								}
							}
						}
					}

					if(result != 0 || (result = fileXioIoctl2(fd, FSSK_IOCTL2_CMD_GET_STATUS, NULL, 0, &status, sizeof(status))) != 0 || (result = status.hasError) != 0 || UserAborted)
					{
						fileXioClose(fd);
						break;
					}

					fileXioClose(fd);
				} else
					result = fd;

				putchar('\n');
			}
		}

		fileXioDclose(bfd);
	} else
		result = bfd;

#ifdef LOG_MESSAGES
	IopStopLog();
#endif

	return result;
}

int OptimizeDisk(int unit)
{
	int result, InitSemaID;

	WaitSema(InstallLockSema);

	DisplayFlashStatusUpdate(SYS_UI_MSG_PLEASE_WAIT);

	if((result = FsskDisk(unit)) == 0 && !UserAborted)
		result = HdskDisk(unit);

	if(result == 0)
	{
		if(!UserAborted)
			DisplayInfoMessage(SYS_UI_MSG_OPT_DISK_COMPLETED_OK);
	} else
		DisplayErrorMessage(SYS_UI_MSG_HDD_FAULT);

	DisplayFlashStatusUpdate(SYS_UI_MSG_PLEASE_WAIT);
	InitSemaID = IopInitStart(IOP_MODSET_MAIN);

	WaitSema(InitSemaID);
	DeleteSema(InitSemaID);

	SysBootDeviceInit();
	ReinitializeUI();

	SignalSema(InstallLockSema);

	return result;
}

int SurfScanDisk(int unit)
{
	u32 lba, SectorsRemaining, TotalSectors;
	u32 NumSectors, NumBadSectors, PadStatus, CurrentCPUTicks, PreviousCPUTicks, seconds, TimeElasped, rate;
	char DeviceName[8];
	int result, BadSectorHandlingMode, IsRetryCycle, InitSemaID;
	HdstSectorIOParams_t SectorIOParams;
	int PercentageComplete;

	WaitSema(InstallLockSema);

	InitProgressScreen(SYS_UI_LBL_SURF_SCANNING_DISK);

	sprintf(DeviceName, "hdst%u:", unit);
	result=0;
	TotalSectors = GetATADeviceCapacity(unit);
	TimeElasped=0;
	PreviousCPUTicks=cpu_ticks();
	NumBadSectors=0;
	IsRetryCycle=0;
	BadSectorHandlingMode=BAD_SECTOR_HANDLING_MODE_PROMPT;
	for(lba=0,SectorsRemaining=TotalSectors; SectorsRemaining>0; )
	{
		CurrentCPUTicks=cpu_ticks();
		if((seconds=(CurrentCPUTicks>PreviousCPUTicks?CurrentCPUTicks-PreviousCPUTicks:UINT_MAX-PreviousCPUTicks+CurrentCPUTicks)/295000000)>0)
		{
			TimeElasped+=seconds;
			PreviousCPUTicks=CurrentCPUTicks;
		}
		PercentageComplete = (int)((u64)lba * 100 / TotalSectors);
		rate = (TimeElasped > 0) ? (TotalSectors - SectorsRemaining) / TimeElasped : 0;	//In sectors/second

		DrawDiskSurfScanningScreen(PercentageComplete, (rate > 0 ? SectorsRemaining / rate : UINT_MAX), NumBadSectors);
		PadStatus=ReadCombinedPadStatus();
		if(PadStatus&CancelButton)
		{
			if(DisplayPromptMessage(SYS_UI_MSG_SCAN_DISK_ABORT_CFM, SYS_UI_LBL_NO, SYS_UI_LBL_YES) == 2)
			{
				result=1;
				break;
			}
		}

		NumSectors=SectorsRemaining>65536?65536:(u32)SectorsRemaining;
		SectorIOParams.lba=lba;
		SectorIOParams.sectors=NumSectors;
		if((result=fileXioDevctl(DeviceName, HDST_DEVCTL_DEVICE_VERIFY_SECTORS, &SectorIOParams, sizeof(SectorIOParams), NULL, 0))!=0)
		{
			if(!IsRetryCycle)
				NumBadSectors++;

			if(result>0)
			{
				result--;
				lba+=result;
				SectorsRemaining-=result;

				if(BadSectorHandlingMode==BAD_SECTOR_HANDLING_MODE_PROMPT)
				BadSectorHandlingMode=GetBadSectorAction(lba);
				switch(BadSectorHandlingMode)
				{
					case BAD_SECTOR_HANDLING_MODE_REMAP:
						BadSectorHandlingMode=BAD_SECTOR_HANDLING_MODE_PROMPT;
					case BAD_SECTOR_HANDLING_MODE_REMAP_ALL:
						fileXioSetBlockMode(FXIO_WAIT);

						if((result=PatchSector(DeviceName, lba))!=0)
						{
							if(DisplayPromptMessage(SYS_UI_MSG_SECTOR_PATCH_FAIL, SYS_UI_LBL_OK, SYS_UI_LBL_ABORT) == 2)
								goto SurfaceScan_end;

							lba++;
							SectorsRemaining--;
							result=0;
						} else
							IsRetryCycle=1;

						fileXioSetBlockMode(FXIO_NOWAIT);
						break;
					case BAD_SECTOR_HANDLING_MODE_SKIP:
						BadSectorHandlingMode=BAD_SECTOR_HANDLING_MODE_PROMPT;
					case BAD_SECTOR_HANDLING_MODE_SKIP_ALL:
						lba++;
						SectorsRemaining--;
						result=0;
						break;
				}

				PreviousCPUTicks=cpu_ticks();	//Don't include the time spent on patching the disk.
			}
			else{
				DisplayErrorMessage(SYS_UI_MSG_UNEXPECTED_ATA_ERR);
				break;
			}
		}
		else{
			lba+=NumSectors;
			SectorsRemaining-=NumSectors;
			IsRetryCycle=0;
		}
	}

	if(result == 0)
		DisplayInfoMessage(SYS_UI_MSG_SURF_SCAN_DISK_COMPLETED_OK);

SurfaceScan_end:

	//Reboot IOP to load the filesystem modules again (they'll assess the condition of the disk's format).
	DisplayFlashStatusUpdate(SYS_UI_MSG_PLEASE_WAIT);
	InitSemaID = IopInitStart(IOP_MODSET_MAIN);

	WaitSema(InitSemaID);
	DeleteSema(InitSemaID);

	SysBootDeviceInit();
	ReinitializeUI();

	SignalSema(InstallLockSema);

	return result;
}

int ZeroFillDisk(int unit)
{
	u32 lba, SectorsRemaining, TotalSectors;
	u32 NumSectors, PadStatus, CurrentCPUTicks, PreviousCPUTicks, seconds, TimeElasped, rate;
	char DeviceName[8];
	int result, InitSemaID;
	HdstSectorIOParams_t SectorIOParams;
	int PercentageComplete;

	WaitSema(InstallLockSema);

	InitProgressScreen(SYS_UI_LBL_ZERO_FILLING_DISK);

	sprintf(DeviceName, "hdst%u:", unit);
	result=0;
	TotalSectors = GetATADeviceCapacity(unit);
	TimeElasped=0;
	PreviousCPUTicks=cpu_ticks();
	for(lba=0,SectorsRemaining=TotalSectors; SectorsRemaining>0; )
	{
		CurrentCPUTicks=cpu_ticks();
		if((seconds=(CurrentCPUTicks>PreviousCPUTicks?CurrentCPUTicks-PreviousCPUTicks:UINT_MAX-PreviousCPUTicks+CurrentCPUTicks)/295000000)>0)
		{
			TimeElasped+=seconds;
			PreviousCPUTicks=CurrentCPUTicks;
		}
		PercentageComplete = (int)((u64)lba * 100 / TotalSectors);
		rate = (TimeElasped > 0) ? (TotalSectors - SectorsRemaining) / TimeElasped : 0;	//In sectors/second

		DrawDiskZeroFillingScreen(PercentageComplete, (rate > 0 ? SectorsRemaining / rate : UINT_MAX));
		PadStatus=ReadCombinedPadStatus();
		if(PadStatus&CancelButton)
		{
			if(DisplayPromptMessage(SYS_UI_MSG_ZERO_FILL_DISK_ABORT_CFM, SYS_UI_LBL_NO, SYS_UI_LBL_YES) == 2)
			{
				result=1;
				break;
			}
		}

		NumSectors=SectorsRemaining>65536?65536:(u32)SectorsRemaining;
		SectorIOParams.lba=lba;
		SectorIOParams.sectors=NumSectors;
		if((result=fileXioDevctl(DeviceName, HDST_DEVCTL_DEVICE_ERASE_SECTORS, &SectorIOParams, sizeof(SectorIOParams), NULL, 0))!=0)
		{
			DisplayErrorMessage(SYS_UI_MSG_HDD_FAULT);
			break;
		}
		else{
			lba+=NumSectors;
			SectorsRemaining-=NumSectors;
		}
	}

	if(result == 0)
		DisplayInfoMessage(SYS_UI_MSG_ZERO_FILL_DISK_COMPLETED_OK);

	//Reboot IOP to load the filesystem modules again (they'll assess the condition of the disk's format).
	DisplayFlashStatusUpdate(SYS_UI_MSG_PLEASE_WAIT);
	InitSemaID = IopInitStart(IOP_MODSET_MAIN);

	WaitSema(InitSemaID);
	DeleteSema(InitSemaID);

	SysBootDeviceInit();
	ReinitializeUI();

	SignalSema(InstallLockSema);

	return result;
}
#endif

int HDDCheckSMARTStatus(void)
{
	return(fileXioDevctl("hdd0:", HDIOC_SMARTSTAT, NULL, 0, NULL, 0) != 0);
}

int HDDCheckSectorErrorStatus(void)
{
	return(fileXioDevctl("hdd0:", HDIOC_GETSECTORERROR, NULL, 0, NULL, 0) != 0);
}

int HDDCheckPartErrorStatus(void)
{
	return(fileXioDevctl("hdd0:", HDIOC_GETERRORPARTNAME, NULL, 0, NULL, 0) != 0);
}

int HDDCheckStatus(void)
{
	int status;

	status = fileXioDevctl("hdd0:", HDIOC_STATUS, NULL, 0, NULL, 0);

	if(status == 0)
		fileXioRemove("hdd0:_tmp");	//Remove _tmp, if it exists.

	return status;
}

#ifndef FSCK
#ifdef LOG_MESSAGES
static int IsLoggingEnabled(void)
{
	switch(GetBootDeviceID())
	{
		case BOOT_DEVICE_HDD:
			return 0;	//Logging to the HDD (while scanning it) is not supported.
		default:
			return 1;
	}
}

static void WaitLogStart(s32 alarm_id, u16 time, void *common)
{
	iWakeupThread((int)common);
}

void IopStartLog(const char *log)
{
	char blockdev[256];
	int len;

	if(!IsLoggingEnabled())
		return;

	getcwd(blockdev, sizeof(blockdev));
	len = strlen(blockdev);
	if((len > 1) && blockdev[len - 1] != '/')
	{
		blockdev[len] = '/';
		len++;
	}
	strcpy(&blockdev[len], log);

	while(fileXioMount("tty0:", blockdev, O_WRONLY|O_TRUNC|O_CREAT) == -ENODEV)
	{
		SetAlarm(16 * 500, &WaitLogStart, (void*)GetThreadId());
		SleepThread();
	}
}

void IopStopLog(void)
{
	if(!IsLoggingEnabled())
		return;

	fileXioUmount("tty0:");
}
#endif
#endif

void poweroffCallback(void *arg)
{	//Power button was pressed. If no installation is in progress, begin shutdown of the PS2.
	if (PollSema(InstallLockSema) == InstallLockSema)
	{
		//If dev9.irx was loaded successfully, shut down DEV9.
		if(dev9Loaded)
		{
			fileXioDevctl("pfs:", PDIOC_CLOSEALL, NULL, 0, NULL, 0);
			while(fileXioDevctl("dev9x:", DDIOC_OFF, NULL, 0, NULL, 0) < 0){};
		}

#ifndef FSCK
		// As required by some (typically 2.5") HDDs, issue the SCSI STOP UNIT command to avoid causing an emergency park.
		fileXioDevctl("mass:", USBMASS_DEVCTL_STOP_ALL, NULL, 0, NULL, 0);
#endif

		/* Power-off the PlayStation 2 console. */
		poweroffShutdown();
	}
}

