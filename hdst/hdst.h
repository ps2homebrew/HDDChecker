typedef struct HdstSectorMiscIOParams
{
    u32 lba;
    u32 sectors;
} HdstSectorIOParams_t;

enum HDST_DEVCTL_CMDS {
    HDST_DEVCTL_DEVICE_PS2_DETECT = 0x00,  // Output = ata_devinfo_t
    HDST_DEVCTL_DEVICE_IDENTIFY,           // Output = 512 byte area.
    HDST_DEVCTL_DEVICE_VERIFY_SECTORS,     // Input = HdckSectorIOParams_t. Output = 0 if no error, >0 for sector errors (sector offset+1), other codes for other errors.
    HDST_DEVCTL_DEVICE_ERASE_SECTORS,      // Input = HdckSectorIOParams_t. Output = 0 if no error, !=0 for errors.
    HDST_DEVCTL_DEVICE_SMART_STATUS,       // Output = 0 if no error, !=0 for errors (1 = SMART threshold exceeded error).
    HDST_DEVCTL_DEVICE_FLUSH_CACHE,        // Output = 0 if no error, !=0 for errors.
    HDST_DEVCTL_SET_IO_BUFFER_SIZE = 0x80, // Input = buffer size in sectors (int).
};
