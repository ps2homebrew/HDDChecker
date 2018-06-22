
LIBPFS_PATH = ../common/libpfs

IOP_BIN = fsck.irx
PFS_OBJS = $(LIBPFS_PATH)/src/bitmap.o $(LIBPFS_PATH)/src/inode.o $(LIBPFS_PATH)/src/journal.o $(LIBPFS_PATH)/src/misc.o $(LIBPFS_PATH)/src/super.o $(LIBPFS_PATH)/src/cache.o $(LIBPFS_PATH)/src/block.o
IOP_OBJS = fsck.o misc.o bitmap.o $(PFS_OBJS) imports.o

IOP_INCS += -I$(CURDIR) -I$(LIBPFS_PATH)/include
IOP_CFLAGS += -Wall -fno-builtin -DPFS_OSD_VER
IOP_LDFLAGS += -s
IOP_LIBS += -lgcc

all: $(IOP_BIN)

clean:
	rm -f $(IOP_BIN) $(IOP_OBJS)

include $(PS2SDK)/Defs.make
include ../common/Rules.make
