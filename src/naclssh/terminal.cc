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

void PrintBuffer(char*, int);

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
// returns empty string if it's not correct escape sequence
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

  bool digit_expected = true;
  for (; i < es_buf.size() - 1; ++i) {
    if (es_buf[i] == ';') {
      if (digit_expected) {
        return "";
      }
      os << ',';
      digit_expected = true;
    } else if (isdigit(es_buf[i])) {
        os << es_buf[i];
        digit_expected = false;
    } else {
      return "";
    }
  }

  return os.str();
}

void PrintSequence() {
  fprintf(stderr, "\nUnparsed escape sequence: ");
  PrintBuffer(&es_buf[0], es_buf.size());
}

void CheckEscSequence(ostringstream& os, const string& func_start, const string& func_end) {
  string s;
  if ((s = ParseArgs()) == "") {
    //not an escape sequence
    os << "writeToTerminal([";
    for (unsigned i = 0; i < es_buf.size() - 1; ++i) {
      os << (unsigned int)(unsigned char)es_buf[i] << ',';
    }
    os << (unsigned int)(unsigned char)es_buf[es_buf.size() - 1] << "]);";
  } else {
    os << func_start << s << func_end;
  }
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
      CheckEscSequence(os, "cursorUp(", ");");
      break;

    case 'B':
      CheckEscSequence(os, "cursorDown(", ");");
      break;

    case 'C':
      CheckEscSequence(os, "cursorRight(",");");
      break;

    case 'D':
      CheckEscSequence(os, "cursorLeft(", ");");
      break;

    case 'K':
      if (es_buf[1] == '[') {
        CheckEscSequence(os, "eraseInLine(", ");");
      } else {
        //fprintf(stderr, "!!selective erase\n"); //CSI ? P s K
      }
      break;

    case 'P':
      CheckEscSequence(os, "deleteChar(", ");");
      break;

    case 'X':
      if (es_buf.size() == 2) { // ESC + X
        // Start of String ( SOS is 0x98)
        //fprintf(stderr, "ESC + X\n");
      } else {
        CheckEscSequence(os, "eraseChar(", ");");
      }
      break;

    case 'H':
      if (es_buf[1] == '[') {
        CheckEscSequence(os, "setCursorPos(", ");");
      } else {
        os << "tabSet();";
      }
      break;
    case 'J':
      if (es_buf.size() == 2) {
        //TODO: ESC J - Erase from the cursor to the end of the screen.
        //fprintf(stderr, "ESC J");
      } else if (es_buf[2] == '?') {
        //fprintf(stderr, "? J - selective erase");
      } else {
        CheckEscSequence(os, "eraseInDisplay(", ");");
      }
      break;
    case 'M':
      if (es_buf[1] == '[') {
        CheckEscSequence(os, "deleteLines(", ");");
      } else {
        os << "scrollDown();";
      }
      break;
    case 'L':
      CheckEscSequence(os, "insertLines(", ");");
     break;
    case 'm':
      if (es_buf[1] == '[') {
        CheckEscSequence(os, "setAttribute([", "]);");
      } else {
        //fprintf(stderr, "Unparsed sequence with m %c\n", es_buf[1]);
      }
     break;
    case 'G':
      if (es_buf[1] == '%') {
        //fprintf(stderr, "ESC procent G\n");
      } else {
        CheckEscSequence(os, "setCursorCol(", ");");
      }
      break;
    case 'd':
      CheckEscSequence(os, "setCursorRow(", ");");
      break;
    case 'b':
      CheckEscSequence(os, "repLast(", ");");
      break;

    case 'g':
      CheckEscSequence(os, "tabClear(", ");");
      break;

    case 'h':
      if (es_buf[2] == '?') {
        CheckEscSequence(os, "setPrivateMode([", "]);");
      } else {
        CheckEscSequence(os, "setMode([", "]);");
      }
      break;

    case 'l':
      if (es_buf[2] == '?') {
        CheckEscSequence(os, "resetPrivateMode([", "]);");
      } else {
        CheckEscSequence(os, "resetMode([", "]);");
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
          CheckEscSequence(os, "setScrollRegion(", ");");
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
        CheckEscSequence(os, "insertChars(", ");");
      }
      break;

    default: //detect unparsed seqs
      //fprintf(stderr, "unparsed sequence: %c\n", es_buf.back());
      //PrintSequence();
      // TODO: check if it's a correct sequence - otherwise print to terminal
      //       the same for parsed and skipped sequences
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
      //fprintf(stderr, "4-bytes utf8-sequences should be parsed!\n");
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
          // unprintable char
          if (code < 32 && code != 10) {
            // Vertical tab and Form feed are equal to Line feed
            if (code == 11 || code == 12) {
              code = 10;
            } else {
              code = 0;
            }
          }

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
