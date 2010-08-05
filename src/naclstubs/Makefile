CC=${NACL_SDK_ROOT}/bin/nacl-gcc
AR=${NACL_SDK_ROOT}/bin/nacl-ar

# -m32 flag is used as a default value for nacl-gcc
# to build 64-bit version "export CFLAGS=-m64" should be done before running make

INCLUDE=include/

all: clean check_variables libstubs.a

libstubs.a: ioctl.o socket.o in.o netdb.o poll.o termios.o
	$(AR) r libstubs.a ioctl.o socket.o in.o netdb.o poll.o termios.o

ioctl.o:
	$(CC) ioctl.c ${CFLAGS} -I$(INCLUDE) -c 

socket.o:
	$(CC) socket.c ${CFLAGS} -I$(INCLUDE) -c

in.o:
	$(CC) in.c ${CFLAGS} -I$(INCLUDE) -c

netdb.o:
	$(CC) netdb.c ${CFLAGS} -I$(INCLUDE) -c

poll.o:
	$(CC) poll.c ${CFLAGS} -I$(INCLUDE) -c

termios.o:
	$(CC) termios.c ${CFLAGS} -I$(INCLUDE) -c

clean:
	rm -f libstubs.a ioctl.o socket.o in.o netdb.o poll.o termios.o

check_variables:
ifeq (${NACL_SDK_ROOT}, undefined)
	@echo "Error: NACL_SDK_ROOT is undefined" ; exit 42
endif
ifeq (${CFLAGS}, undefined)
	@echo "No CFLAGS is specified: by default -m32 is used";
endif

