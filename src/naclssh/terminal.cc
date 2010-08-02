// Author: Kate Volkova <gloom.msk@gmail.com>

#include <iomanip>
#include <queue>
#include <vector>

#include "string.h"
#include "pthread.h"

#include <nacl/nacl_npapi.h>

extern "C" {
#include "libssh2_nacl.h"
}

#include "terminal.h"
#include "npp_gate.h"

using std::string;
using std::ostringstream;
using std::queue;
using std::vector;
using std::pair;
using std::oct;

// TODO: no global vars (classes working with each buf)
extern queue<char> recv_buf;
extern queue<char> term_buf;

vector<char> es_buf;
vector<unsigned int> utf8_buf;

extern pthread_mutex_t mutexbuf;
extern pthread_cond_t mutexbuf_cvar;
extern struct NppGate *npp_gate;

void AsyncSend(void* msg) {
  NPString npstr;
  npstr.UTF8Characters = (char*)msg;
  npstr.UTF8Length = static_cast<uint>(strlen(npstr.UTF8Characters));

  NPObject* window_object;
  NPN_GetValue(npp_gate->npp, NPNVWindowNPObject, &window_object);

  NPVariant variant;
  NPN_Evaluate(npp_gate->npp, window_object, &npstr, &variant);

  delete (char*) msg;
}


void AddToRecvBuf(const char* msg, unsigned int length) {
  char* data;
  unsigned int datalen;
  base64_to_binary_data(&data, &datalen, msg, length);

  pthread_mutex_lock (&mutexbuf);
  for (unsigned int i = 0; i < datalen; ++i) {
    recv_buf.push(data[i]);
  }

  pthread_cond_signal(&mutexbuf_cvar);
  pthread_mutex_unlock (&mutexbuf);
}

void CallJS(const ostringstream& os) {
  char* msg = new char[os.str().length() + 1];
  strncpy(msg, os.str().c_str(), os.str().length() + 1);
  NPN_PluginThreadAsyncCall(npp_gate->npp, AsyncSend, msg);
}

// bool - if the sequence is finished
// int - index of the last processed character
// es_buf[1] indicates how the sequence is terminated
pair<bool,int> ParseEscapeSequence(char* buffer, int length) {
  static const string chars2_seq_end = "=><78|}~";  // ESC + 8, F,c,l,m,n,o will stop the loop with isalpha check
  bool res = false;
  int i = 0;

  for (; !res && i < length; ++i) {
    es_buf.push_back(buffer[i]);

    if (es_buf.size() > 1) {
      if (es_buf[1] == ']' ) {
        res = buffer[i] == 7;
      } else {
        res = (es_buf.size() == 2 && chars2_seq_end.find(es_buf[1]) != string::npos)
            ||(isalpha(buffer[i]) || buffer[i] == '@')
            ||(es_buf.size() == 3 && es_buf[1] == '(');
      }
    }
  }

  return pair<bool, int>(res, i-1);
}

// 27  [  4  0  ;  2  3  H 
string ParseArgs() {
  static const string default0 = "JKbglmh";
  static const string default1 = "ABCDGMLPXd@";

  ostringstream os;

  vector<char>::size_type i = 2;  // ESC + [

  if (es_buf[i] == '?') {         // ESC + [ + ?
    ++i;
  }

  if (i == es_buf.size() - 1) {
    // no args, default values
    if (es_buf[i] == 'H') {
      os << "1,1";
    } else if (default1.find(es_buf[i]) != string::npos) {
      os << '1';
    } else if (default0.find(es_buf[i]) != string::npos) {
      os << '0';
    } else if (es_buf[i] == 'r') {
      os << "0,0";
    }
  }

  for (; i < es_buf.size() - 1; ++i) {
    if (es_buf[i] == ';') {
      os << ',';
    } else {
      os << es_buf[i];
    }
  }

  return os.str();
}

void PrintSequence() {
  ostringstream os;
  for (unsigned i = 0; i < es_buf.size(); ++i) {
    if (es_buf[i] >= 32) {
      os << es_buf[i] << " ";
    } else {
      os << (int)es_buf[i] << " ";
    }
  }
  //fprintf(stderr, "Escape sequence: %s\n", os.str().c_str());
}


// there is only 1 seq in buf at the time!
void EscapeSequence(ostringstream& os) {
  if (es_buf[1] == ']') {
    // TODO: call set window header func
    // tmp - skip
    es_buf.clear();
    return;
  }

  if (es_buf[1] == '(') {
    os << "setCharSet('" << es_buf[2] << "');";
    es_buf.clear();
    return;
  }

  if (es_buf[1] == 'c') {
    os << "ris();";
    es_buf.clear();
    return;
  }


  switch (es_buf[es_buf.size()-1]) {
    case '7':
      os << "saveCursor();";
      break;
    case '8':
      os << "restoreCursor();";
      break;
    case 'A':
      os << "cursorUp(" << ParseArgs() << ");";
      break;

    case 'B':
      os << "cursorDown(" << ParseArgs() << ");";
      break;

    case 'C':
      os << "cursorRight(" << ParseArgs() << ");";
      break;

    case 'D':
      os << "cursorLeft(" << ParseArgs() << ");";
      break;

    case 'K':
      if (es_buf[1] == '[') {
        os << "eraseInLine(" << ParseArgs() << ");";
      } else {
        //fprintf(stderr, "!!selective erase\n"); //CSI ? P s K
      }
      break;

    case 'P':
      os << "deleteChar(" << ParseArgs() << ");";
      break;

    case 'X':
      if (es_buf.size() == 2) { // ESC + X
        // Start of String ( SOS is 0x98)
        //fprintf(stderr, "ESC + X\n");
      } else {
        os << "eraseChar(" << ParseArgs() << ");";
      }
      break;

    case 'H':
      if (es_buf[1] == '[') {
        os << "setCursorPos(" << ParseArgs() << ");";
      } else {
        os << "tabSet();";
      }
      break;
    case 'J':
        PrintSequence();
      if (es_buf.size() == 2) {
        // ESC J - Erase from the cursor to the end of the screen.
        //fprintf(stderr, "ESC J");
      } else if (es_buf[2] == '?') {
        //fprintf(stderr, "? J - selective erase");
      } else {
        os << "eraseInDisplay(" << ParseArgs() << ");";
      }
      break;
    case 'M':
      if (es_buf[1] == '[') {
        os << "deleteLines(" << ParseArgs() << ");";
      } else {
        os << "scrollDown();";
      }
      break;
    case 'L':
      os << "insertLines(" << ParseArgs() << ");";
     break;
    case 'm':
      if (es_buf[1] == '[') {
        os << "setAttribute([" << ParseArgs() << "]);";
      } else {
        //fprintf(stderr, "Unparsed sequence with m %c\n", es_buf[1]);
      }
     break;
    case 'G':
      if (es_buf[1] == '%') {
        //fprintf(stderr, "ESC procent G\n");
      } else {
        os << "setCursorCol(" << ParseArgs() << ");";
      }
      break;
    case 'd':
      os << "setCursorRow(" << ParseArgs() << ");";
      break;
    case 'b':
      os << "repLast(" << ParseArgs() << ");";
      break;

    case 'g':
      os << "tabClear(" << ParseArgs() << ");";
      break;

    case 'h':
      if (es_buf[2] == '?') {
        os << "setPrivateMode([" << ParseArgs() << "]);";
        //if (es_buf[3] == '7') {
        //  os << "setWrapMode();";
        //} else {
          // TMP - print 1 char
          //fprintf(stderr, "?l: %c\n", es_buf[3]);
        //  PrintSequence();
       // }
      } else {
        os << "setMode([" << ParseArgs() << "]);";
        //fprintf(stderr, "CSI P m h\n");
        //PrintSequence();
      }
      break;

    case 'l':
      if (es_buf[2] == '?') {
        os << "resetPrivateMode([" << ParseArgs() << "]);";
        //if (es_buf[3] == '7') {
        //  os << "setNoWrapMode();";
        //} else {
          // TMP - print 1 char
          //fprintf(stderr, "?l: %c\n", es_buf[3]);
          //PrintSequence();
       // }
      } else {
        os << "resetMode([" << ParseArgs() << "]);";
        //fprintf(stderr, "CSI P m l\n");
        //PrintSequence();
      }

      break;

    case 'p':
      if (es_buf.size() > 3) {
        if (es_buf[2] == '!') {
          os << "softReset();";
        } else if (es_buf[2] == '>') {
          //fprintf(stderr, "CSI > P s p\n");
        }
      }
      break;

    case 'r':
      if (es_buf[es_buf.size() - 2] != '$') {
        if (es_buf[2] == '?') {
          //fprintf(stderr, "CSI ? P m r\n");
        } else {
          os << "setScrollRegion(" << ParseArgs() << ");";
        }
      } else {
         // TODO: add $
        //fprintf(stderr, "CSI P t ; P l; P b ; P r ; P s $ r\n");
      }
      break;

    case 's':
      if (es_buf.size() == 3) {
        os << "saveCursor();";
      } else {
        //fprintf(stderr, "CSI ? P m s\n");
      }
      break;
    case '@':
      if (es_buf[1] == '%') {
        //fprintf(stderr, "ESC percent @\n");
      } else {
        os << "insertChars(" << ParseArgs() << ");";
      }
      break;

    default: //detect unparsed seqs
      //fprintf(stderr, "unparsed sequence: %c\n", es_buf.back());
      break;
  }

  es_buf.clear();
}

// bool - if the sequence is finished
// int - index of the last processed character
pair<bool,int> ParseUTF8(char* buffer, int length) {
  
  int k; // how many bytes do we need to finish the sequence
  utf8_buf.push_back((unsigned int)(unsigned char)buffer[0]);

  if (utf8_buf[0] > 191 && utf8_buf[0] < 224) {
    k = 2;
  } else if (utf8_buf[0] >= 224 && utf8_buf[0] <= 239) {
    k = 3;
  } else {
    k = 4;
    fprintf(stderr, "parseUTF8: k == 4, %02X, len == %d, buffer[0] == %02X, length == %d\n", utf8_buf[0],utf8_buf.size(), buffer[0], length); 
  }
  k -= utf8_buf.size();

  int i = 1;
  for (; k > 0 && i < length; ++i) {
    utf8_buf.push_back((unsigned int)(unsigned char)buffer[i]);
    --k;
  }

  pair<bool, int> result(k==0, i-1);
  return result;
}
 
void UTF8Decode(ostringstream& os) {
  switch (utf8_buf.size()) {
    case 2:
      os << (((utf8_buf[0] & 31) << 6) | (utf8_buf[1] & 63));
      break;

    case 3:
      os << (((utf8_buf[0] & 15) << 12) | ((utf8_buf[1] & 63) << 6) | (utf8_buf[2] & 63));
      break;

    case 4:
      fprintf(stderr, "4-bytes utf8-sequences should be parsed!\n");
      // TODO
      os << "0x3C0";  //TMP
      break;
  }

  utf8_buf.clear();
}

// DEBUG
void PrintBuffer(char* buffer, int length) {
  ostringstream dbgos;
  for (int i = 0; i < length; ++i) {
    if ((unsigned int)buffer[i] >= 32 && (unsigned int)buffer[i] < 128) {
      dbgos << buffer[i];
    } else {
      if ((int)buffer[i] == 27) {
        //dbgos << '\n';
      }
      dbgos << std::hex <<  "\\x" << std::setw(2) <<std::setfill('0') << (unsigned int)(unsigned char)buffer[i];
    }
  }
  fprintf(stderr, "%s\n", dbgos.str().c_str());
}

void FinishWriteToTerm(ostringstream& os, int& state) {
  if (state) {
    os << "]);";
    state = 0;
  }
}

void WriteToTerm(ostringstream& os, int& is_writing) {
  if (is_writing) {
    os << ',';
  } else {
    os << "writeToTerminal([";
    is_writing = 1;
  }
}

void PrintToTerminal(char* buffer, int length) {
  //PrintBuffer(buffer, length);

  ostringstream os;
  pair<bool, int> result;
  int i = 0;
  int state = 0; // 1 - typing writetoTerm, 0 - sequence, e.t.c

  if (!es_buf.empty()) {
    // finish escape sequence parsing
    result = ParseEscapeSequence(buffer, length);
    if (!result.first) {
      return;	// sequence not finished
    }
    EscapeSequence(os);
    i = result.second + 1;
  }

  if (!utf8_buf.empty()) {
    // finish utf8 multibyte sequence
    result = ParseUTF8(buffer, length);
    if (!result.first) {
      return;	// sequence not finished
    }
    WriteToTerm(os, state);
    UTF8Decode(os);
    i = result.second + 1;
  }

  for (; i < length; ++i) {
    unsigned int code = (unsigned int)(unsigned char)buffer[i];

    if (code == 27) {
      // escape sequence
      result = ParseEscapeSequence(buffer + i, length - i);
      if (!result.first) {
        // escape seq will be finished with the next packet
        break;
      } else {
        FinishWriteToTerm(os, state);
        EscapeSequence(os);
	      i += result.second;
      }
    } else if (code == 7) {  // bell
        FinishWriteToTerm(os, state);
    } else if (code == 8) {
        FinishWriteToTerm(os, state);
        os << "backSpace();";
    } else if (code == 9) {
        FinishWriteToTerm(os, state);
        os << "tab();";
    } else if (code == 13) {
        FinishWriteToTerm(os, state);
        os << "carriageReturn();";
    } else {
        if (code < 192) {
          // usual char
          WriteToTerm(os, state);
          os << code;
        } else {
          // UTF8 multibyte sequence
          result = ParseUTF8(buffer + i, length - i);
          if (!result.first) {
            // utf8 seq will be finished with the next packet
            break;
          } else {
            WriteToTerm(os, state);
            UTF8Decode(os);
	          i += result.second;
          }
      }
    }
  }

  FinishWriteToTerm(os, state);
  os <<  "terminalUpdate();";

  //fprintf(stderr, "%s\n", os.str().c_str());

  CallJS(os);
}

