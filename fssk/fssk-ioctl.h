struct fsskStatus{
	u32 zoneUsed;		//0x00
	u32 inodeBlockCount;	//0x04
	u32 files;		//0x08
	u32 directories;	//0x0C
	u32 PWDLevel;		//0x10
	u32 hasError;		//0x14
	u32 partsDeleted;	//0x18
};

//IOCTL2 codes - none of these commands have any inputs or outputs, unless otherwise specified.
enum FSSK_IOCTL2_CMD{
	FSSK_IOCTL2_CMD_GET_ESTIMATE	= 0,	//Output = u32 time
	FSSK_IOCTL2_CMD_START,
	FSSK_IOCTL2_CMD_WAIT,
	FSSK_IOCTL2_CMD_POLL,
	FSSK_IOCTL2_CMD_GET_STATUS,	//Output = struct fsskStatus
	FSSK_IOCTL2_CMD_STOP,
	FSSK_IOCTL2_CMD_SET_MINFREE,
	FSSK_IOCTL2_CMD_SIM
};

#define FSSK_MODE_VERBOSITY(x)	(((x)&0xF)<<4)
