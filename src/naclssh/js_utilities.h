// Author: Kate Volkova <gloom.msk@gmail.com>

#ifndef JS_UTILITIES_H_
  #define JS_UTILITIES_H_

#include <sstream>

// Calls javascript functions from the thread in which libssh works,
// this can be done only by passing this call to the thread which works
// in browser with NPN_PluginThreadAsyncCall
void CallJS(const std::ostringstream& os);

#endif  // JS_UTILITIES_H_
