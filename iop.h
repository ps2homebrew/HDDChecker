//Module bits
#define IOP_MOD_HDD	0x01
#define IOP_MOD_PFS	0x02
#define IOP_MOD_HDST	0x04
#define IOP_MOD_HDCK	0x08
#define IOP_MOD_FSCK	0x10
#define IOP_MOD_HDSK	0x20
#define IOP_MOD_FSSK	0x40
#define IOP_REBOOT	0x80
#define IOP_MOD_HDLOG	0x100

//Module set bits
#define IOP_MODSET_INIT		(IOP_MOD_HDD | IOP_MOD_PFS | IOP_MOD_HDST)
#define IOP_MODSET_SA_INIT	(IOP_MOD_HDD | IOP_MOD_PFS)				//For the standalone FSCK tool
#define IOP_MODSET_MAIN		(IOP_REBOOT | IOP_MOD_HDD | IOP_MOD_PFS | IOP_MOD_HDST)
#define IOP_MODSET_SA_MAIN	(IOP_REBOOT | IOP_MOD_HDD | IOP_MOD_PFS)		//For the standalone FSCK tool

#define IOP_MODSET_HDCK		(IOP_REBOOT | IOP_MOD_HDCK | IOP_MOD_HDLOG)
#define IOP_MODSET_FSCK		(IOP_REBOOT | IOP_MOD_FSCK | IOP_MOD_HDD | IOP_MOD_HDLOG)
#define IOP_MODSET_SA_FSCK	(IOP_REBOOT | IOP_MOD_FSCK | IOP_MOD_HDD)		//For the standalone FSCK tool
#define IOP_MODSET_HDSK		(IOP_REBOOT | IOP_MOD_HDSK | IOP_MOD_HDLOG)
#define IOP_MODSET_FSSK		(IOP_REBOOT | IOP_MOD_FSSK | IOP_MOD_HDD | IOP_MOD_HDLOG)

#ifndef LOG_MESSAGES
#define FSCK_VERBOSITY	0
#define FSSK_VERBOSITY	0
#else
#define FSCK_VERBOSITY	2
#define FSSK_VERBOSITY	2
#endif

int IopInitStart(unsigned int flags);
void IopDeinit(void);
void IopStartLog(const char *log);
void IopStopLog(void);
