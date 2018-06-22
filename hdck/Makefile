
LIBAPA_PATH = ../common/libapa

IOP_BIN = hdck.irx
APA_OBJS = $(LIBAPA_PATH)/src/misc.o $(LIBAPA_PATH)/src/cache.o $(LIBAPA_PATH)/src/apa.o $(LIBAPA_PATH)/src/journal.o
IOP_OBJS = hdck.o misc.o imports.o $(APA_OBJS)

IOP_INCS += -I$(CURDIR) -I$(LIBAPA_PATH)/include
IOP_CFLAGS += -Wall -fno-builtin -DAPA_OSD_VER
IOP_LDFLAGS += -s

IOP_CFLAGS += -DHDCK_CHECK_CROSSLINK=1	#If desired, uncomment to build a version of HDCK that checks for crosslinked partitions (unofficial).

all: $(IOP_BIN)

clean:
	rm -f $(IOP_BIN) $(IOP_OBJS)

include $(PS2SDK)/Defs.make
include ../common/Rules.make
