// Author: Kate Volkova <gloom.msk@gmail.com>

#include <sstream>
#include "js_utilities.h"
#include "terminal.h"

// Calls javascript to display data from buffer on the terminal
void PrintToTerminal(char* buffer, int length) {
  std::ostringstream os;

  os << "printToTerminal([";
  for (int i = 0; i < length - 1; ++i) {
    os << (unsigned int)(unsigned char)buffer[i] << ',';
  }
  os << (unsigned int)(unsigned char)buffer[length-1] << "]);";

  //fprintf(stderr, os.str().c_str());
  //fprintf(stderr, "\n\n\n");

  CallJS(os);
}
