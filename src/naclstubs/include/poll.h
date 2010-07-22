#ifndef NACL_POLL_STUB_H_
  #define NACL_POLL_STUB_H_


/* Event types that can be polled for.  These bits may be set in `events'
   to indicate the interesting event types; they will appear in `revents'
   to indicate the status of the file descriptor.  */
#define POLLIN		0x001		/* There is data to read.  */
#define POLLPRI		0x002		/* There is urgent data to read.  */
#define POLLOUT		0x004		/* Writing now will not block.  */

typedef unsigned long int nfds_t;

struct pollfd {
    int fd;        /* The descriptor. */
    short events;  /* The event(s) is/are specified here. */
    short revents; /* Events found are returned here. */
};

int poll(struct pollfd* fds, nfds_t n_fds_t_, int t);

#endif
