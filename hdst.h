#include "hdst/hdst.h"

typedef struct _ata_devinfo
{
    s32 exists;          /* Was successfully probed.  */
    s32 has_packet;      /* Supports the PACKET command set.  */
    u32 total_sectors;   /* Total number of user sectors.  */
    u32 security_status; /* Word 0x100 of the identify info.  */
} ata_devinfo_t;

#define NUM_SUPPORTED_DEVICES 2

#ifndef BSWAP16
#define BSWAP16(x) ((x << 8 | x >> 8))
#endif

int SetHDSTIOBufferSize(int sectors);
int InitATADevice(int unit);
const char *GetATADeviceModel(int unit);
const char *GetATADeviceSerial(int unit);
const char *GetATADeviceFWVersion(int unit);
u32 GetATADeviceCapacity(int unit);
int GetATADeviceSMARTStatus(int unit);
int PatchSector(const char *device, u32 lba);
int IsATADeviceInstalled(int unit);

// void ShowHDDInfo(int unit);
