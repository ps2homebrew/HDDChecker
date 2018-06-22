#By default, the full HDDChecker tool will be compiled when "make" is issued without arguments.
#Enter "make fsck-tool" to build the standalone FSCK tool.
FSCK ?= 0

LOG_MESSAGES ?= 1

EE_HDDCHECKER_BIN = HDDChecker.elf
EE_FSCK_BIN = fsck.elf
EE_IOP_OBJS = SIO2MAN_irx.o PADMAN_irx.o DEV9_irx.o ATAD_irx.o IOMANX_irx.o FILEXIO_irx.o HDD_irx.o PFS_irx.o FSCK_irx.o
EE_RES_OBJS = background.o buttons.o
EE_OBJS = main.o iop.o pad.o UI.o menu.o system.o graphics.o font.o $(EE_IOP_OBJS) $(EE_RES_OBJS)

EE_INCS := -I$(PS2SDK)/ports/include -I$(PS2SDK)/ee/include -I$(PS2SDK)/common/include -I.
EE_LDFLAGS := -L$(PS2SDK)/ports/lib -L$(PS2SDK)/ee/lib -s
EE_LIBS := -lpng -lz -lm -lfreetype -lhdd -lpatches -lfileXio -lpadx -lgs -lc -lkernel
EE_GPVAL = -G8192
EE_CFLAGS += -mgpopt $(EE_GPVAL)

ifeq ($(FSCK),1)
	LOG_MESSAGES = 0
	EE_CFLAGS += -DFSCK=1
	EE_BIN = $(EE_FSCK_BIN)
else
	EE_IOP_OBJS += MCMAN_irx.o USBD_irx.o USBHDFSD_irx.o HDST_irx.o HDCK_irx.o HDSK_irx.o FSSK_irx.o
	EE_OBJS += hdst.o
	EE_BIN = $(EE_HDDCHECKER_BIN)
endif

ifeq ($(LOG_MESSAGES),1)
	EE_CFLAGS += -DLOG_MESSAGES=1
	EE_IOP_OBJS += HDLOG_irx.o
endif

all:
	$(MAKE) $(EE_BIN)

fsck-tool:
	$(MAKE) FSCK=1 all

%.o : %.c
	$(EE_CC) $(EE_CFLAGS) $(EE_INCS) -c $< -o $@

%.o : %.s
	$(EE_AS) $(EE_ASFLAGS) $< -o $@

%.o : %.S
	$(EE_CC) $(EE_CFLAGS) $(EE_INCS) -c $< -o $@

$(EE_BIN) : $(EE_OBJS) $(PS2SDK)/ee/startup/crt0.o
	$(EE_CC) -nostartfiles -Tlinkfile $(EE_LDFLAGS) \
		-o $(EE_BIN) $(PS2SDK)/ee/startup/crt0.o $(EE_OBJS) $(EE_LIBS)

clean:
	rm -f $(EE_BIN) $(EE_OBJS) $(EE_HDDCHECKER_BIN) $(EE_FSCK_BIN) *_irx.c background.c buttons.c hdst.o
	make -C hdst clean
	make -C hdck clean
	make -C fsck clean
	make -C hdsk clean
	make -C fssk clean
	make -C hdlog clean

background.c:
	bin2c resources/background.png background.c background

buttons.c:
	bin2c resources/buttons.png buttons.c buttons

SIO2MAN_irx.c: $(PS2SDK)/iop/irx/freesio2.irx
	bin2c $(PS2SDK)/iop/irx/freesio2.irx SIO2MAN_irx.c SIO2MAN_irx

MCMAN_irx.c: $(PS2SDK)/iop/irx/mcman.irx
	bin2c $(PS2SDK)/iop/irx/mcman.irx MCMAN_irx.c MCMAN_irx

PADMAN_irx.c: $(PS2SDK)/iop/irx/freepad.irx
	bin2c $(PS2SDK)/iop/irx/freepad.irx PADMAN_irx.c PADMAN_irx

USBD_irx.c: $(PS2SDK)/iop/irx/usbd.irx
	bin2c $(PS2SDK)/iop/irx/usbd.irx USBD_irx.c USBD_irx

USBHDFSD_irx.c: $(PS2SDK)/iop/irx/usbhdfsd.irx
	bin2c $(PS2SDK)/iop/irx/usbhdfsd.irx USBHDFSD_irx.c USBHDFSD_irx

IOMANX_irx.c: $(PS2SDK)/iop/irx/iomanX.irx
	bin2c $(PS2SDK)/iop/irx/iomanX.irx IOMANX_irx.c IOMANX_irx

FILEXIO_irx.c: $(PS2SDK)/iop/irx/fileXio.irx
	bin2c $(PS2SDK)/iop/irx/fileXio.irx FILEXIO_irx.c FILEXIO_irx

DEV9_irx.c: $(PS2SDK)/iop/irx/ps2dev9.irx
	bin2c $(PS2SDK)/iop/irx/ps2dev9.irx DEV9_irx.c DEV9_irx

ATAD_irx.c: $(PS2SDK)/iop/irx/ps2atad.irx
	bin2c $(PS2SDK)/iop/irx/ps2atad.irx ATAD_irx.c ATAD_irx

HDD_irx.c: $(PS2SDK)/iop/irx/ps2hdd-osd.irx
	bin2c $(PS2SDK)/iop/irx/ps2hdd-osd.irx HDD_irx.c HDD_irx

PFS_irx.c: $(PS2SDK)/iop/irx/ps2fs.irx
	bin2c $(PS2SDK)/iop/irx/ps2fs.irx PFS_irx.c PFS_irx

HDST_irx.c:
	make -C hdst
	bin2c hdst/hdst.irx HDST_irx.c HDST_irx

HDCK_irx.c:
	make -C hdck
	bin2c hdck/hdck.irx HDCK_irx.c HDCK_irx

FSCK_irx.c:
	make -C fsck
	bin2c fsck/fsck.irx FSCK_irx.c FSCK_irx

HDSK_irx.c:
	make -C hdsk
	bin2c hdsk/hdsk.irx HDSK_irx.c HDSK_irx

FSSK_irx.c:
	make -C fssk
	bin2c fssk/fssk.irx FSSK_irx.c FSSK_irx

HDLOG_irx.c:
	make -C hdlog
	bin2c hdlog/hdlog.irx HDLOG_irx.c HDLOG_irx

include $(PS2SDK)/Defs.make
