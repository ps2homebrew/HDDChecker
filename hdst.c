#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <kernel.h>
#include <limits.h>
#include <fileXio_rpc.h>
#include <atahw.h>
#include <hdd-ioctl.h>
#include <sys/fcntl.h>

#include "hdst.h"

int SetHDSTIOBufferSize(int sectors)
{
    return fileXioDevctl("hdst:", HDST_DEVCTL_SET_IO_BUFFER_SIZE, &sectors, sizeof(sectors), NULL, 0);
}

struct AtaDeviceData
{
    u16 IdentificationData[256];
    ata_devinfo_t PS2AtadData;
    int SMARTStatus;
    u32 NumSectors;
    char model[42], serial[22], FWVersion[10];
};
static struct AtaDeviceData AtadDeviceData[NUM_SUPPORTED_DEVICES];

static void TrimWhitespacing(char *string, int maxlen)
{
    int i, CharsStart;
    // Locate the start of the string;
    for (i = 0; i < maxlen; i++) {
        if (string[i] != ' ') {
            CharsStart = i;
            for (i = 0; i < maxlen; i++) {
                if ((string[i] = string[CharsStart + i]) == '\0')
                    break;
            }
            break;
        }
    }
}

int InitATADevice(int unit)
{
    int result, i;
    char DeviceName[8];

    memset(&AtadDeviceData[unit], 0, sizeof(AtadDeviceData[unit]));
    sprintf(DeviceName, "hdst%u:", unit);
    if ((result = fileXioDevctl(DeviceName, HDST_DEVCTL_DEVICE_PS2_DETECT, NULL, 0, &AtadDeviceData[unit].PS2AtadData, sizeof(AtadDeviceData[unit].PS2AtadData))) == 0) {
        if (AtadDeviceData[unit].PS2AtadData.exists) {
            if ((result = fileXioDevctl(DeviceName, HDST_DEVCTL_DEVICE_IDENTIFY, NULL, 0, AtadDeviceData[unit].IdentificationData, sizeof(AtadDeviceData[unit].IdentificationData))) == 0) {
                AtadDeviceData[unit].SMARTStatus = fileXioDevctl(DeviceName, HDST_DEVCTL_DEVICE_SMART_STATUS, NULL, 0, NULL, 0);

                if (!IsPSX() && (AtadDeviceData[unit].IdentificationData[ATA_ID_COMMAND_SETS_SUPPORTED] & 0x400)) {
                    if (AtadDeviceData[unit].IdentificationData[ATA_ID_48BIT_SECTOTAL_HI] == 0)
                        AtadDeviceData[unit].NumSectors = AtadDeviceData[unit].IdentificationData[ATA_ID_48BIT_SECTOTAL_LO] | ((u32)AtadDeviceData[unit].IdentificationData[ATA_ID_48BIT_SECTOTAL_MI] << 16);
                    else // If the disk is larger than 2TB, limit the capacity to 2TB.
                        AtadDeviceData[unit].NumSectors = 0xFFFFFFFF;
                } else {
                    AtadDeviceData[unit].NumSectors = AtadDeviceData[unit].IdentificationData[ATA_ID_SECTOTAL_LO] | ((u32)AtadDeviceData[unit].IdentificationData[ATA_ID_SECTOTAL_HI] << 16);
                }

                for (i = 0; i < 20; i++)
                    ((u16 *)AtadDeviceData[unit].model)[i] = BSWAP16(AtadDeviceData[unit].IdentificationData[27 + i]);
                TrimWhitespacing(AtadDeviceData[unit].model, sizeof(AtadDeviceData[unit].model) - 1);
                for (i = 0; i < 10; i++)
                    ((u16 *)AtadDeviceData[unit].serial)[i] = BSWAP16(AtadDeviceData[unit].IdentificationData[10 + i]);
                TrimWhitespacing(AtadDeviceData[unit].serial, sizeof(AtadDeviceData[unit].serial) - 1);
                for (i = 0; i < 4; i++)
                    ((u16 *)AtadDeviceData[unit].FWVersion)[i] = BSWAP16(AtadDeviceData[unit].IdentificationData[23 + i]);
                TrimWhitespacing(AtadDeviceData[unit].FWVersion, sizeof(AtadDeviceData[unit].FWVersion) - 1);
            }
        } else
            result = -ENODEV;
    }

    return result;
}

int PatchSector(const char *device, u32 lba)
{
    HdstSectorIOParams_t SectorIOParams;

    SectorIOParams.lba     = lba;
    SectorIOParams.sectors = 1;
    return fileXioDevctl(device, HDST_DEVCTL_DEVICE_ERASE_SECTORS, &SectorIOParams, sizeof(SectorIOParams), NULL, 0);
}

int IsATADeviceInstalled(int unit)
{
    return AtadDeviceData[unit].PS2AtadData.exists;
}

const char *GetATADeviceModel(int unit)
{
    return AtadDeviceData[unit].model;
}

const char *GetATADeviceSerial(int unit)
{
    return AtadDeviceData[unit].serial;
}

const char *GetATADeviceFWVersion(int unit)
{
    return AtadDeviceData[unit].FWVersion;
}

u32 GetATADeviceCapacity(int unit)
{
    return AtadDeviceData[unit].NumSectors;
}

int GetATADeviceSMARTStatus(int unit)
{
    return AtadDeviceData[unit].SMARTStatus;
}

#if 0
static u16 DeviceIdentificationInfo[NUM_SUPPORTED_DEVICES][256] ALIGNED(16);

struct AtaDeviceData
{
    int exists;
    u64 NumSectors;
};
static ata_devinfo_t PS2AtaDeviceData[NUM_SUPPORTED_DEVICES];
static struct AtaDeviceData AtadDeviceData[NUM_SUPPORTED_DEVICES];

static const char *xfer_types[] = {
    "PIO",
    "MDMA",
    "UDMA"};

void ShowHDDInfo(int unit)
{
    unsigned int i;
    const char *selected_xfer_type;
    unsigned char selected_xfer_mode;
    char name[41], DeviceName[8];

    printf("HDD unit %u:\n", unit);
    sprintf(DeviceName, "hdst%u:", unit);
    if (unit != 0 || fileXioDevctl(DeviceName, HDST_DEVCTL_DEVICE_IDENTIFY, NULL, 0, DeviceIdentificationInfo[unit], sizeof(DeviceIdentificationInfo[unit])) != 0) {
        printf("\tNot installed.\n");
        AtadDeviceData[unit].exists = 0;
        return;
    }
    AtadDeviceData[unit].exists = 1;

    name[40] = '\0';
    for (i = 0; i < 20; i++)
        ((u16 *)name)[i] = BSWAP16(DeviceIdentificationInfo[unit][27 + i]);
    printf("\tModel:\t%s\n", name);
    name[20] = '\0';
    for (i = 0; i < 10; i++)
        ((u16 *)name)[i] = BSWAP16(DeviceIdentificationInfo[unit][10 + i]);
    printf("\tSerial:\t%s\n", name);
    name[8] = '\0';
    for (i = 0; i < 4; i++)
        ((u16 *)name)[i] = BSWAP16(DeviceIdentificationInfo[unit][23 + i]);
    printf("\tFW Ver:\t%s\n", name);

    printf("\tThis drive supports ");
    if (DeviceIdentificationInfo[unit][ATA_ID_COMMAND_SETS_SUPPORTED] & 0x400) {
        printf("48-bit");
        AtadDeviceData[unit].NumSectors = DeviceIdentificationInfo[unit][ATA_ID_48BIT_SECTOTAL_LO] | ((u32)DeviceIdentificationInfo[unit][ATA_ID_48BIT_SECTOTAL_MI] << 16) | ((u64)DeviceIdentificationInfo[unit][ATA_ID_48BIT_SECTOTAL_HI] << 32);
    } else {
        printf("28-bit");
        AtadDeviceData[unit].NumSectors = DeviceIdentificationInfo[unit][ATA_ID_SECTOTAL_LO] | ((u32)DeviceIdentificationInfo[unit][ATA_ID_SECTOTAL_HI] << 16);
    }
    printf(" LBA addressing.\n");
    printf("\tNumber of sectors: %lu\n", AtadDeviceData[unit].NumSectors);

    /* The default transfer mode is PIO. */
    selected_xfer_type = xfer_types[0];
    selected_xfer_mode = 0;

    printf("\tSupported DMA tranfer modes:\n");
    printf("\t\tMDMA: ");
    if ((DeviceIdentificationInfo[unit][63] & 0x7) & 1)
        printf("0 ");
    if ((DeviceIdentificationInfo[unit][63] & 0x7) & 2)
        printf("1 ");
    if ((DeviceIdentificationInfo[unit][63] & 0x7) & 4)
        printf("2 ");

    /* If any of the MDMA modes were selected: */
    if ((DeviceIdentificationInfo[unit][63] >> 8) & 0x7) {
        selected_xfer_type = xfer_types[1];

        if (((DeviceIdentificationInfo[unit][63] >> 8) & 0x7) & 1)
            selected_xfer_mode = 0;
        else if (((DeviceIdentificationInfo[unit][63] >> 8) & 0x7) & 2)
            selected_xfer_mode = 1;
        else if (((DeviceIdentificationInfo[unit][63] >> 8) & 0x7) & 4)
            selected_xfer_mode = 2;
    }
    printf("\n");

    printf("\t\tUDMA: ");
    if ((DeviceIdentificationInfo[unit][53] & 2) != 0) {
        if ((DeviceIdentificationInfo[unit][88] & 0x3F) & 0x01)
            printf("0 ");
        if ((DeviceIdentificationInfo[unit][88] & 0x3F) & 0x02)
            printf("1 ");
        if ((DeviceIdentificationInfo[unit][88] & 0x3F) & 0x04)
            printf("2 ");
        if ((DeviceIdentificationInfo[unit][88] & 0x3F) & 0x08)
            printf("3 ");
        if ((DeviceIdentificationInfo[unit][88] & 0x3F) & 0x10)
            printf("4 ");
        if ((DeviceIdentificationInfo[unit][88] & 0x3F) & 0x20)
            printf("5 ");
        printf("\n");

        /* If any of the UDMA modes were selected: */
        if ((DeviceIdentificationInfo[unit][88] >> 8) & 0x3F) {
            selected_xfer_type = xfer_types[2];

            if (((DeviceIdentificationInfo[unit][88] >> 8) & 0x3F) & 0x01)
                selected_xfer_mode = 0;
            else if (((DeviceIdentificationInfo[unit][88] >> 8) & 0x3F) & 0x02)
                selected_xfer_mode = 1;
            else if (((DeviceIdentificationInfo[unit][88] >> 8) & 0x3F) & 0x04)
                selected_xfer_mode = 2;
            else if (((DeviceIdentificationInfo[unit][88] >> 8) & 0x3F) & 0x08)
                selected_xfer_mode = 3;
            else if (((DeviceIdentificationInfo[unit][88] >> 8) & 0x3F) & 0x10)
                selected_xfer_mode = 4;
            else if (((DeviceIdentificationInfo[unit][88] >> 8) & 0x3F) & 0x20)
                selected_xfer_mode = 5;
        }
    } else
        printf("Not available.\n");

    printf("\tCurrent transfer mode: %s mode %u\n", selected_xfer_type, selected_xfer_mode);

    if (DeviceIdentificationInfo[unit][82] == DeviceIdentificationInfo[unit][83] == 0x0000 || DeviceIdentificationInfo[unit][82] == DeviceIdentificationInfo[unit][83] == 0xFFFF) {
        printf("\tCommand set notification is NOT enabled or NOT supported.\n");
    } else {
        printf("\tSMART is ");
        if (!(DeviceIdentificationInfo[unit][82] & 1)) {
            printf("NOT ");
        } else
            printf("supported.\n");
    }

    if (DeviceIdentificationInfo[unit][85] == DeviceIdentificationInfo[unit][86] == DeviceIdentificationInfo[unit][87] == 0x0000 || DeviceIdentificationInfo[unit][85] == DeviceIdentificationInfo[unit][86] == DeviceIdentificationInfo[unit][87] == 0xFFFF) {
        printf("\tCommand set enabled notification is NOT enabled or NOT supported.\n");
    } else {
        printf("\tSMART is ");
        if (DeviceIdentificationInfo[unit][85] & 1) {
            printf("ENABLED.\n");
        } else
            printf("DISABLED.\n");
    }


    printf("\tThis drive ");
    if (DeviceIdentificationInfo[unit][ATA_ID_SECURITY_STATUS] & 1) {
        printf("supports");
    } else
        printf("does NOT support");
    printf(" security features.\n");

    printf("\tSecurity features are ");
    if (DeviceIdentificationInfo[unit][ATA_ID_SECURITY_STATUS] & ATA_F_SEC_ENABLED) {
        printf("enabled\n");
    } else
        printf("disabled\n");

    printf("\tDrive is ");
    if (!(DeviceIdentificationInfo[unit][ATA_ID_SECURITY_STATUS] & ATA_F_SEC_LOCKED)) {
        printf("not ");
    }
    printf("locked\n");

    /* printf("\tDrive is ");
    if (!ata_device_is_sce(unit)) {
        printf("not ");
    }
    printf("a Sony Playstation 2 HDD unit.\n"); */
}
#endif
