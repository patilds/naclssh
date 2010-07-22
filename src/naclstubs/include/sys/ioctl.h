#ifndef NACL_SYS_IOCTL_STUB_H_
  #define NACL_SYS_IOCTL_STUB_H_

//#include <asm/termbits.h>
#define TCGETA		0x5405
#define TCSETA		0x5406

/* tcsetattr uses these */
#define	TCSANOW		0
#define	TCSADRAIN	1
#define	TCSAFLUSH	2

#define ECHO	0000010

int ioctl (int __fd, unsigned long int __request, ...);

#endif
