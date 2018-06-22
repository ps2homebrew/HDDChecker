
#If desired, set to 1 to build a version of HDSK that supports HDLoader games
HDLFS_SUPPORT = 1

LIBAPA_PATH = ../common/libapa

IOP_BIN = hdsk.irx
APA_OBJS = $(LIBAPA_PATH)/src/misc.o $(LIBAPA_PATH)/src/cache.o $(LIBAPA_PATH)/src/apa.o $(LIBAPA_PATH)/src/journal.o $(LIBAPA_PATH)/src/free.o
IOP_OBJS = hdsk.o misc.o sim.o imports.o $(APA_OBJS)

IOP_INCS += -I$(CURDIR) -I$(LIBAPA_PATH)/include
IOP_CFLAGS += -Wall -fno-builtin -DAPA_OSD_VER
IOP_LDFLAGS += -s

ifeq ($(HDLFS_SUPPORT),1)
	IOP_CFLAGS += -DHDSK_SUPPORT_HDLFS=1
	IOP_OBJS += hdl.o
endif

all: $(IOP_BIN)

clean:
	rm -f $(IOP_BIN) $(IOP_OBJS)

%.o : $(LIBAPA_PATH)/%.c
	$(IOP_CC) $(IOP_CFLAGS) $< -o $@

include $(PS2SDK)/Defs.make
include ../common/Rules.make
