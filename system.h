enum BootDeviceIDs {
    BOOT_DEVICE_UNKNOWN = -1,
    BOOT_DEVICE_MC0     = 0,
    BOOT_DEVICE_MC1,
    BOOT_DEVICE_MASS,
    BOOT_DEVICE_HDD,
    BOOT_DEVICE_HOST,

    BOOT_DEVICE_COUNT,
};

enum CONSOLE_REGION {
    CONSOLE_REGION_JAPAN = 0,
    CONSOLE_REGION_USA, // USA and Asia
    CONSOLE_REGION_EUROPE,
    CONSOLE_REGION_CHINA,

    CONSOLE_REGION_COUNT
};

#ifndef FSCK
enum BAD_SECTOR_HANDLING_MODES {
    BAD_SECTOR_HANDLING_MODE_PROMPT = 0,
    BAD_SECTOR_HANDLING_MODE_REMAP,
    BAD_SECTOR_HANDLING_MODE_SKIP,
    BAD_SECTOR_HANDLING_MODE_REMAP_ALL,
    BAD_SECTOR_HANDLING_MODE_SKIP_ALL,

    BAD_SECTOR_HANDLING_MODE_COUNT
};
#endif

int GetBootDeviceID(void);
int GetConsoleRegion(void);
int GetConsoleVMode(void);
int IsPSX(void);
int ScanDisk(int unit);
#ifndef FSCK
int OptimizeDisk(int unit);
int SurfScanDisk(int unit);
int ZeroFillDisk(int unit);
#endif

int HDDCheckSMARTStatus(void);
int HDDCheckSectorErrorStatus(void);
int HDDCheckPartErrorStatus(void);
int HDDCheckStatus(void);

int SysBootDeviceInit(void);
int SysCreateThread(void *function, void *stack, unsigned int StackSize, void *arg, int priority);

void poweroffCallback(void *arg);
