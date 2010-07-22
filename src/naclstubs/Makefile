CC=${NACL_SDK_ROOT}/bin/nacl-gcc
AR=${NACL_SDK_ROOT}/bin/nacl-ar

INCLUDE=include/

libstubs.a: ioctl.o socket.o in.o netdb.o poll.o termios.o
	$(AR) r libstubs.a ioctl.o socket.o in.o netdb.o poll.o termios.o

ioctl.o:
	$(CC) ioctl.c  -I$(INCLUDE) -c 

socket.o:
	$(CC) socket.c -I$(INCLUDE) -c

in.o:
	$(CC) in.c -I$(INCLUDE) -c

netdb.o:
	$(CC) netdb.c -I$(INCLUDE) -c

poll.o:
	$(CC) poll.c -I$(INCLUDE) -c

termios.o:
	$(CC) termios.c -I$(INCLUDE) -c

clean:
	rm libstubs.a ioctl.o socket.o in.o netdb.o poll.o termios.o

