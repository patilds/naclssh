#include <sys/socket.h>

int socket (int __domain, int __type, int __protocol) {
  return 0;
}

int shutdown(int s, int how) {
  return 0;
}

int recv(int s, void *buf, size_t len, int flags) {
  return 0;
}

int send(int s, const void *msg, size_t len, int flags) {
  return 0;
}

int getsockname(int socket, struct sockaddr *address, socklen_t *address_len) {
  return 0;
}

int setsockopt(int socket, int level, int option_name, const void *option_value, socklen_t option_len) {
  return 0;
}

int connect(int socket, const struct sockaddr *address, socklen_t address_len) {
  return 0;
}

int listen (int __fd, int __n) {
  return 0;
}

int accept (int __fd, struct sockaddr* __addr, socklen_t *__restrict __addr_len) {
  return 0;
}

int getsockopt (int __fd, int __level, int __optname,
           void *__restrict __optval,
           socklen_t *__restrict __optlen) {
  return 0;
}


