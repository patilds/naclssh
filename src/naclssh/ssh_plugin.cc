// Author: Kate Volkova <gloom.msk@gmail.com>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <nacl/nacl_npapi.h>

#include "pthread.h"
#include "npp_gate.h"

#include "openssl/rand.h"
#include <libssh2.h>
extern "C" {
  #include "libssh2_nacl.h"
}

#include "terminal.h"

#include <string>
#include <queue>

using std::queue;
using std::string;

extern queue<char> recv_buf;
extern queue<unsigned char> term_buf;

extern struct NppGate *npp_gate;

// These are the method names as JavaScript sees them.
static const char* kSaveDataMethodId = "savedata";
static const char* kSSHConnectMethodId = "sshconnect";
static const char* kSendEscapeSequence = "sendEscapeSequence";
static const char* kSendUnicodeKey = "sendUnicodeKey";

//TODO: add destroying pthread objs
pthread_mutex_t mutexbuf = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t mutexbuf_cvar = PTHREAD_COND_INITIALIZER;
pthread_t libssh_thread;

//TODO: what to do with DEVRANDOM?! : tmp commented in libcrypto
unsigned char random_datasource[] = "La1fk&E8nsjoQ3k!sTg69#d2hoqhz)90yagE3t5d(fSLiygWhaTq4gf-kQu51sHg";


static bool SaveData(NPVariant *result, const NPVariant *args) {
  NPString str = NPVARIANT_TO_STRING(args[0]);
  AddToBuf(str.UTF8Characters, str.UTF8Length);

  if (result) {
    NULL_TO_NPVARIANT(*result);
  }
  return true;
}

struct UserData {
  string username;
  string password;
};

void* StartSSHSession(void* arg) {
  UserData* userdata = static_cast<UserData*>(arg);

  LIBSSH2_SESSION *session;
  LIBSSH2_CHANNEL *channel;

  int rc = libssh2_init (0);
  if (rc != 0) {
    fprintf (stderr, "libssh2 initialization failed (%d)\n", rc);
	  return NULL;
  }

  // TODO: different socket numbers
  int sock = 1;

  /* Create a session instance and start it up. This will trade welcome
   * banners, exchange keys, and setup crypto, compression, and MAC layers
   */
  
  // tmp
  if (RAND_status() == 0) {
    RAND_seed(random_datasource, sizeof(random_datasource));
  }

  session = libssh2_session_init();
    
  // libssh2_trace(session, 0xffffffff);
  libssh2_session_set_blocking(session, 0);

  if (libssh2_session_startup(session, sock)) {
    fprintf(stderr, "Failure establishing SSH session\n");
	  return NULL;
  }


  // TODO: add unknown host adding dialog
  // const char* fingerprint = libssh2_hostkey_hash(session, LIBSSH2_HOSTKEY_HASH_SHA1);

  // Password authentication
  if (libssh2_userauth_password(session, userdata->username.c_str(), userdata->password.c_str())) {
    //fprintf(stderr, "\tAuthentication by password failed!\n");
    goto shutdown;
  }

  /* Request a shell */
  if (!(channel = libssh2_channel_open_session(session))) {
    //fprintf(stderr, "Unable to open a session\n");
    goto shutdown;
  }

  // Request xterm terminal
  if (libssh2_channel_request_pty_ex(channel, "xterm", strlen("xterm"), NULL, 0, 90, 40, 0, 0)) {
    //fprintf(stderr, "Failed requesting pty\n");
    goto skip_shell;
  }

  /* Open a SHELL on that pty */
  if (libssh2_channel_shell(channel)) {
    //fprintf(stderr, "Unable to request shell on allocated pty\n");
    goto shutdown;
  }

  // Non-blocking receive
  nacl_set_block(0);

  //TODO - make constant width and length 
  char buffer[90*40];

  while (true) {
    if (nacl_wait_for_sock()) {
      pthread_mutex_lock (&mutexbuf);

	    if (!term_buf.empty()) {
  	    unsigned i = 0;

        //TODO: read buffer to the end
	      for (; i < sizeof(buffer); ++i) {
	        if (term_buf.empty()) {
	          break;
          }
	        buffer[i] = term_buf.front();
	        term_buf.pop();
	      }

 	      pthread_mutex_unlock (&mutexbuf);

	      libssh2_channel_write(channel, buffer, i);

	    } else {
        pthread_mutex_unlock (&mutexbuf);
	    }

 	    pthread_mutex_lock (&mutexbuf);

      if (!recv_buf.empty()) {
        pthread_mutex_unlock (&mutexbuf);

        int len;
        while((len = libssh2_channel_read(channel, buffer,sizeof(buffer))) > 0) {
	        PrintToTerminal(buffer, len);
	      }
	    } else {
 	      pthread_mutex_unlock (&mutexbuf);
	    }
    }
  }

skip_shell:
  if (channel) {
    libssh2_channel_free(channel);
    channel = NULL;
  }

shutdown:
  delete userdata;

  libssh2_session_disconnect(session, "Normal Shutdown");
  libssh2_session_free(session);

  libssh2_exit();

  return NULL;
}

static bool SSHConnect(NPVariant* result, const NPVariant *args) {
  UserData* userdata = new UserData();

  NPString str = NPVARIANT_TO_STRING(args[0]);
  userdata->username = string(str.UTF8Characters, str.UTF8Length);

  str = NPVARIANT_TO_STRING(args[1]);
  userdata->password = string(str.UTF8Characters, str.UTF8Length);

  if (pthread_create(&libssh_thread, NULL, StartSSHSession, userdata)) {
    fprintf(stderr, "pthread_create error!\n");
  }

  if (result) {
    NULL_TO_NPVARIANT(*result);
  }

  return true;
}


static bool SendUnicodeKey(NPVariant* result, const NPVariant* args) {
  unsigned int code = args[0].value.intValue;

  // fprintf(stderr, "code: %u\n", code);
  
  pthread_mutex_lock (&mutexbuf);
  if (code < 128) {
    term_buf.push((unsigned char)code);

  } else if (code > 127 && code < 2048) {
     term_buf.push((unsigned char)((code >> 6) | 192));
     term_buf.push((unsigned char)((code & 63) | 128));
  } else {
     //TODO: separate 3 and 4 bytes sequences!
     term_buf.push((unsigned char)((code >> 12) | 224));
     term_buf.push((unsigned char)(((code >> 6) & 63) | 128));
     term_buf.push((unsigned char)((code & 63) | 128));
  }

  pthread_cond_signal(&mutexbuf_cvar);
  pthread_mutex_unlock (&mutexbuf);

  if (result) {
    NULL_TO_NPVARIANT(*result);
  }

  return true;
}

static bool SendEscapeSequence(NPVariant *result, const NPVariant *args) {
  NPString str = NPVARIANT_TO_STRING(args[0]);

  pthread_mutex_lock (&mutexbuf);
  term_buf.push('\033');

  for (uint i = 0; i < str.UTF8Length; ++i) {
    term_buf.push(str.UTF8Characters[i]);
  }

  pthread_cond_signal(&mutexbuf_cvar);
  pthread_mutex_unlock (&mutexbuf);

  NULL_TO_NPVARIANT(*result);

  if (result) {
    NULL_TO_NPVARIANT(*result);
  }

  return true;
} 


static NPObject* Allocate(NPP npp, NPClass* npclass) {
  return new NPObject;
}

static void Deallocate(NPObject* object) {
  delete object;
}

// Return |true| if |method_name| is a recognized method.
static bool HasMethod(NPObject* obj, NPIdentifier method_name) {
  char *name = NPN_UTF8FromIdentifier(method_name);
  bool is_method = false;
  if (!strcmp((const char*)name, kSendUnicodeKey)) {
    is_method = true;
  } else if (!strcmp((const char*)name, kSendEscapeSequence)) {
    is_method = true;
  } else if (!strcmp((const char*)name, kSaveDataMethodId)) {
    is_method = true;
  } else if (!strcmp((const char*)name, kSSHConnectMethodId)) {
    is_method = true;
  }
  NPN_MemFree(name);
  return is_method;
}

static bool InvokeDefault(NPObject *obj, const NPVariant *args,
                          uint32_t argCount, NPVariant *result) {
  if (result) {
    NULL_TO_NPVARIANT(*result);
  }
  return true;
}

// Invoke() is called by the browser to invoke a function object whose name
// is |method_name|.
static bool Invoke(NPObject* obj,
                   NPIdentifier method_name,
                   const NPVariant *args,
                   uint32_t arg_count,
                   NPVariant *result) {
  NULL_TO_NPVARIANT(*result);
  char *name = NPN_UTF8FromIdentifier(method_name);
  if (name == NULL)
    return false;
  bool rval = false;

  // Map the method name to a function call.  |result| is filled in by the
  // called function, then gets returned to the browser when Invoke() returns.
  if (!strcmp((const char*)name, kSendUnicodeKey)) {
    rval = SendUnicodeKey(result, args);
  } else if (!strcmp((const char*)name, kSaveDataMethodId)) {
    rval = SaveData(result, args);
  } else if (!strcmp((const char*)name, kSSHConnectMethodId)) {
    rval = SSHConnect(result, args);
  } else if (!strcmp((const char*)name, kSendEscapeSequence)) {
    rval = SendEscapeSequence(result, args);
  }
  // Since name was allocated above by NPN_UTF8FromIdentifier,
  // it needs to be freed here.
  NPN_MemFree(name);
  return rval;
}

// The class structure that gets passed back to the browser.  This structure
// provides funtion pointers that the browser calls.
static NPClass kSSHPluginClass = {
  NP_CLASS_STRUCT_VERSION,
  Allocate,
  Deallocate,
  NULL,  // Invalidate is not implemented
  HasMethod,
  Invoke,
  InvokeDefault,
  NULL,  // HasProperty is not implemented
  NULL,  // GetProperty is not implemented
  NULL,  // SetProperty is not implemented
};

NPClass *GetNPSimpleClass() {
  return &kSSHPluginClass;
}
