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


queue<char> recv_buf;
queue<char> term_buf;


extern pthread_mutex_t mutexbuf;
extern pthread_cond_t mutexbuf_cvar;
extern int lock_flag;


int nacl_send(int socket, char* buffer, int length) {
  ostringstream os;
  os << "sendData('";
  os.write(buffer, length);
  os << "');";

  CallJS(os);

  return length;
}

//takes messages from buf, they are put in the buf when ws.onmessage
int nacl_recv_lock(int socket, char* buffer, int length) {
  int i = 0;
  while(true) {
    if (nacl_wait_for_sock()) {
      pthread_mutex_lock (&mutexbuf);
	    if (!recv_buf.empty()) {
	      for(; i< length; ++i) {
	        if (recv_buf.empty()) { break; }
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


//TODO: return different numbers for reading and writing
int nacl_wait_for_sock() {
  pthread_mutex_lock(&mutexbuf);

  while (recv_buf.empty() && term_buf.empty()) {
    pthread_cond_wait(&mutexbuf_cvar, &mutexbuf);
  }
  pthread_mutex_unlock(&mutexbuf);

  return 1;
}


void nacl_set_block(int flag) {
  lock_flag = flag;
}
