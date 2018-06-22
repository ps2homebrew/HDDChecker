
IOP_BIN = hdst.irx
IOP_OBJS = main.o imports.o

IOP_CFLAGS += -Wall -fno-builtin
IOP_LDFLAGS += -s

all: $(IOP_BIN)

clean:
	rm -f $(IOP_BIN) $(IOP_OBJS)

include $(PS2SDK)/Defs.make
include ../common/Rules.make
