// Author: Kate Volkova <gloom.msk@gmail.com>

#include "string.h"
#include <nacl/nacl_npapi.h>
#include "npp_gate.h"
#include "js_utilities.h"

using std::ostringstream;

extern struct NppGate *npp_gate;

void AsyncSend(void* msg) {
  NPString npstr;
  npstr.UTF8Characters = (char*)msg;
  npstr.UTF8Length = static_cast<uint>(strlen(npstr.UTF8Characters));

  NPObject* window_object;
  NPN_GetValue(npp_gate->npp, NPNVWindowNPObject, &window_object);

  NPVariant variant;
  if (!NPN_Evaluate(npp_gate->npp, window_object, &npstr, &variant)) {
    fprintf(stderr, "\n\nNPN_Evaluate error: %d\n", strlen((char*)msg));
    fprintf(stderr, (char*)msg);
  }

  delete (char*) msg;
}

void CallJS(const ostringstream& os) {
  char* msg = new char[os.str().length() + 1];
  strncpy(msg, os.str().c_str(), os.str().length() + 1);
  NPN_PluginThreadAsyncCall(npp_gate->npp, AsyncSend, msg);
}

