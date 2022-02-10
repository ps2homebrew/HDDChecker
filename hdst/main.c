#include <errno.h>
#include <intrman.h>
#include <iomanX.h>
#include <irx.h>
#include <loadcore.h>
#include <thsemap.h>
#include <stdio.h>
#include <sysclib.h>
#include <sysmem.h>
#include <types.h>

#include <sys/stat.h>

#include <atad.h>
#include <atahw.h>
#include "hdst.h"

#define MODNAME "HDD_status"
IRX_ID(MODNAME, 1, 2);

// #define DEBUG

#ifdef DEBUG
#define DEBUG_PRINTF(args...) printf(args)
#else
#define DEBUG_PRINTF(args...)
#endif

#define MAX_SUPPORTED_UNITS    2
#define HDD_SECTOR_SIZE        512
#define DEFAULT_IO_BUFFER_SIZE 16

static ata_devinfo_t *AtadDevInfo[MAX_SUPPORTED_UNITS];

static void *IOBuffer = NULL;
static unsigned int IOBufferSize; // In sectors.

static int SetIOBufferSize(int size)
{
    int OldState, result;

    CpuSuspendIntr(&OldState);
    if (IOBuffer != NULL)
        FreeSysMemory(IOBuffer);
    IOBuffer = AllocSysMemory(ALLOC_FIRST, size * HDD_SECTOR_SIZE, NULL);
    CpuResumeIntr(OldState);

    if (IOBuffer != NULL) {
        result       = 0;
        IOBufferSize = size;
    } else {
        result       = -ENOMEM;
        IOBufferSize = 0;
    }

    return result;
}

int sceCdRI(unsigned char *id, int *stat);

static int hdst_init(iop_device_t *fd)
{
    unsigned int i;
    int result;
    unsigned char iLinkID[32];

    sceCdRI(iLinkID, &result);

    for (i = 0; i < MAX_SUPPORTED_UNITS; i++) {
        AtadDevInfo[i] = ata_get_devinfo(i);
        if (AtadDevInfo[i]->exists)
            ata_device_sce_sec_unlock(i, iLinkID);
    }

    SetIOBufferSize(DEFAULT_IO_BUFFER_SIZE);

    return 0;
}

static int hdst_deinit(iop_device_t *fd)
{
    return 0;
}

static int hdst_PS2DetectDevice(int device, ata_devinfo_t *info)
{
    ata_devinfo_t *data;
    int result;

    if ((data = ata_get_devinfo(device)) != NULL) {
        memcpy(info, data, sizeof(ata_devinfo_t));
        result = 0;
    } else
        result = -ENODEV;

    return result;
}

static int hdst_EraseSectors(int device, u32 lba, u32 sectors)
{
    u32 nsectors;
    int result;

    result = 0;
    memset(IOBuffer, 0, (sectors > IOBufferSize ? IOBufferSize : sectors) * HDD_SECTOR_SIZE);
    while (sectors > 0) {
        nsectors = sectors > IOBufferSize ? IOBufferSize : sectors;

        if ((result = ata_device_sector_io(device, IOBuffer, lba, nsectors, ATA_DIR_WRITE)) != 0)
            break;

        lba += nsectors;
        sectors -= nsectors;
    }

    return result;
}

static int ata_device_identify(int device, void *info)
{
    int res;

    if (!(res = ata_io_start(info, 1, 0, 0, 0, 0, 0, (device << 4) & 0xffff, ATA_C_IDENTIFY_DEVICE)))
        res = ata_io_finish();

    return res;
}

static int ata_device_read_verify(int device, u32 lba, u32 sectors)
{
    USE_ATA_REGS;
    int res = 0;
    u16 sector, lcyl, hcyl, command, select;
    u32 StartLBA, len;

    StartLBA = lba;
    while (sectors > 0) {
        /* Variable lba is only 32 bits so no change for lcyl and hcyl.  */
        lcyl = (lba >> 8) & 0xff;
        hcyl = (lba >> 16) & 0xff;

        if (AtadDevInfo[device]->lba48) {
            /* Setup for 48-bit LBA.  */
            len = (sectors > 65536) ? 65536 : sectors;

            /* Combine bits 24-31 and bits 0-7 of lba into sector.  */
            sector = ((lba >> 16) & 0xff00) | (lba & 0xff);
            /* 0x40 enables LBA.  */
            select  = ((device << 4) | 0x40) & 0xffff;
            command = ATA_C_READ_VERIFY_SECTOR_EXT;
        } else {
            /* Setup for 28-bit LBA.  */
            len    = (sectors > 256) ? 256 : sectors;
            sector = lba & 0xff;
            /* 0x40 enables LBA.  */
            select  = ((device << 4) | ((lba >> 24) & 0xf) | 0x40) & 0xffff;
            command = ATA_C_READ_VERIFY_SECTOR;
        }

        if ((res = ata_io_start(NULL, 0, 0, (u16)len, sector, lcyl, hcyl, select, command)) == 0) {
            if ((res = ata_io_finish()) != 0) {
                if (res == ATA_RES_ERR_IO) {
                    if (AtadDevInfo[device]->lba48) {
                        lba                   = (ata_hwport->r_sector & 0xFF) | ((u32)ata_hwport->r_lcyl & 0xFF) << 8 | ((u32)ata_hwport->r_hcyl & 0xFF) << 16;
                        ata_hwport->r_control = 0x80; // Toggle the HOB bit.
                        lba |= ((u32)ata_hwport->r_sector & 0xFF) << 24;
                        ata_hwport->r_control = 0x00;
                    } else
                        lba = (ata_hwport->r_sector & 0xFF) | ((u32)ata_hwport->r_lcyl & 0xFF) << 8 | ((u32)ata_hwport->r_hcyl & 0xFF) << 16 | ((u32)ata_hwport->r_select & 0xF) << 24;

                    printf("atad: READ VERIFY LBA 0x%lx, ERR: 0x%02x\n", lba, ata_get_error());

                    // Check hardware status and obtain the bad sector number.
                    if (ata_get_error() & ATA_ERR_ECC)
                        res = lba - StartLBA + 1;
                }

                break;
            }
        } else
            break;

        lba += len;
        sectors -= len;
    }

    return res;
}

static int hdst_devctl(iop_file_t *fd, const char *path, int cmd, void *arg, unsigned int arglen, void *buf, unsigned int buflen)
{
    int result;

    if (fd->unit < MAX_SUPPORTED_UNITS && AtadDevInfo[fd->unit]->exists) {
        switch (cmd) {
            case HDST_DEVCTL_DEVICE_PS2_DETECT:
                result = hdst_PS2DetectDevice(fd->unit, buf);
                break;
            case HDST_DEVCTL_DEVICE_IDENTIFY:
                result = ata_device_identify(fd->unit, buf);
                break;
            case HDST_DEVCTL_DEVICE_VERIFY_SECTORS:
                result = ata_device_read_verify(fd->unit, ((HdstSectorIOParams_t *)arg)->lba, ((HdstSectorIOParams_t *)arg)->sectors);
                break;
            case HDST_DEVCTL_DEVICE_ERASE_SECTORS:
                result = hdst_EraseSectors(fd->unit, ((HdstSectorIOParams_t *)arg)->lba, ((HdstSectorIOParams_t *)arg)->sectors);
                break;
            case HDST_DEVCTL_DEVICE_SMART_STATUS:
                result = ata_device_smart_get_status(fd->unit);
                break;
            case HDST_DEVCTL_DEVICE_FLUSH_CACHE:
                result = ata_device_flush_cache(fd->unit);
            case HDST_DEVCTL_SET_IO_BUFFER_SIZE:
                result = SetIOBufferSize(*(int *)arg);
                break;
            default:
                result = -EINVAL;
        }
    } else
        result = -ENODEV;

    return result;
}

static int hdst_NulldevFunction(void)
{
    return -EIO;
}

/* Device driver I/O functions */
static iop_device_ops_t hdst_functarray = {
    &hdst_init,                    /* INIT */
    &hdst_deinit,                  /* DEINIT */
    (void *)&hdst_NulldevFunction, /* FORMAT */
    (void *)&hdst_NulldevFunction, /* OPEN */
    (void *)&hdst_NulldevFunction, /* CLOSE */
    (void *)&hdst_NulldevFunction, /* READ */
    (void *)&hdst_NulldevFunction, /* WRITE */
    (void *)&hdst_NulldevFunction, /* LSEEK */
    (void *)&hdst_NulldevFunction, /* IOCTL */
    (void *)&hdst_NulldevFunction, /* REMOVE */
    (void *)&hdst_NulldevFunction, /* MKDIR */
    (void *)&hdst_NulldevFunction, /* RMDIR */
    (void *)&hdst_NulldevFunction, /* DOPEN */
    (void *)&hdst_NulldevFunction, /* DCLOSE */
    (void *)&hdst_NulldevFunction, /* DREAD */
    (void *)&hdst_NulldevFunction, /* GETSTAT */
    (void *)&hdst_NulldevFunction, /* CHSTAT */
    (void *)&hdst_NulldevFunction, /* RENAME */
    (void *)&hdst_NulldevFunction, /* CHDIR */
    (void *)&hdst_NulldevFunction, /* SYNC */
    (void *)&hdst_NulldevFunction, /* MOUNT */
    (void *)&hdst_NulldevFunction, /* UMOUNT */
    (void *)&hdst_NulldevFunction, /* LSEEK64 */
    &hdst_devctl,                  /* DEVCTL */
    (void *)&hdst_NulldevFunction, /* SYMLINK */
    (void *)&hdst_NulldevFunction, /* READLINK */
    (void *)&hdst_NulldevFunction  /* IOCTL2 */
};

static const char hdst_dev_name[] = "hdst";

static iop_device_t hdst_dev = {
    hdst_dev_name,            /* Device name */
    IOP_DT_FS | IOP_DT_FSEXT, /* Device type flag */
    1,                        /* Version */
    "HDD status driver",      /* Description. */
    &hdst_functarray          /* Device driver function pointer array */
};

/* Entry point */
int _start(int argc, char **argv)
{
    DelDrv(hdst_dev_name);
    AddDrv(&hdst_dev);

    return MODULE_RESIDENT_END;
}
