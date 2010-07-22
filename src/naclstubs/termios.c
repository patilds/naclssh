#include <termios.h>

int tcgetattr (int __fd, struct termios *__termios_p) {
  return 0;
}

int tcsetattr (int __fd, int __optional_actions, const struct termios *__termios_p) {
  return 0;
}

