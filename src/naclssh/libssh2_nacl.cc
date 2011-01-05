// Author: Kate Volkova <gloom.msk@gmail.com>

#include <sstream>
#include <queue>
#include "pthread.h"

#include <nacl/nacl_npapi.h>

extern "C" {
#include "libssh2_nacl.h"
}

#include "js_utilities.h"


using std::ostringstream;
using std::queue;

// Data received from server: to be printed on terminal,
// data is put into recv_buf when SaveData is called
//    (as a reaction to ws.onmessage in javascript)
// data is read from recv_buf when nacl_recv_lock or nacl_recv_non_lock
//    is called from _libssh2_recv
queue<char> recv_buf;

// Data which came from terminal (what user typed): to be send to server,
// data is put into term_buf when SendUnicodeKey or SendEscapeSequence
//    is called (as a reaction to keyPress in javascript)
// data is read from term_buf in StartSSHSession main loop 
//    and is passed to libssh2_channel_write
queue<char> term_buf;

// Defined in ssh_plugin.cc
extern pthread_mutex_t mutexbuf;
extern pthread_cond_t mutexbuf_cvar;

// Switching between locking and non-locking receive modes
//    defined in libssh2-1.2.6/src/misc.c
extern int lock_flag;


// Used in libssh instead of usual send(socket, buffer, length, flags);
int nacl_send(int socket, char* buffer, int length) {
  ostringstream os;
  os << "sendData('";
  os.write(buffer, length);
  os << "');";

  CallJS(os);

  return length;
}


// Used in libssh instead of usual recv(socket, buffer, length, flags)
// in locking receive mode.
// Takes messages from recv_buf, called from _libssh2_recv
int nacl_recv_lock(int socket, char* buffer, int length) {
  int i = 0;
  while(true) {
    if (nacl_wait_for_sock()) {
      pthread_mutex_lock (&mutexbuf);
      if (!recv_buf.empty()) {
        for(; i < length; ++i) {
          if (recv_buf.empty()) {
            break;
          }
          buffer[i] = recv_buf.front();
          recv_buf.pop();
        }
        break;
      } else {
        pthread_mutex_unlock(&mutexbuf);
      }
    }
  }
  pthread_mutex_unlock (&mutexbuf);
  return i;
}


// Used in libssh instead of usual recv(socket, buffer, length, flags)
// in non-locking receive mode.
// Takes messages from recv_buf, called from _libssh2_recv
int nacl_recv_non_lock(int socket, char* buffer, int length) {
  int i = 0;
  pthread_mutex_lock (&mutexbuf);
  for (; i < length; ++i) {
    if (recv_buf.empty()) {
      break;
    }
    buffer[i] = recv_buf.front();
    recv_buf.pop();
  }
  pthread_mutex_unlock (&mutexbuf);

  return i ? i : -1;
}


// For modelling poll or select (waiting for action on the socket),
// waits on conditional variable till there is some data to be read or sent.
// Called in _libssh2_wait_socket
// TODO: return different numbers for reading and writing
int nacl_wait_for_sock() {
  pthread_mutex_lock(&mutexbuf);

  while (recv_buf.empty() && term_buf.empty()) {
    pthread_cond_wait(&mutexbuf_cvar, &mutexbuf);
  }
  pthread_mutex_unlock(&mutexbuf);

  return 1;
}

// Switching between locking and non-locking receive modes
void nacl_set_block(int flag) {
  lock_flag = flag;
}
