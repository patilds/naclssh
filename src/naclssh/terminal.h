// Author: Kate Volkova <gloom.msk@gmail.com>

#ifndef TERMINAL_H_
  #define TERMINAL_H_

#include <sstream>
#include <string>
#include <utility>

void AsyncSend(void* msg);
void AddToRecvBuf(const char* msg, unsigned int length);
void CallJS(const std::ostringstream& os);
std::pair<bool,int> ParseEscapeSequence(char* buffer, int length);
std::string ParseArgs();
void EscapeSequence(std::ostringstream& os);
void PrintToTerminal(char* buffer, int length);

#endif  // SSH_PLUGIN_HELPER_H
