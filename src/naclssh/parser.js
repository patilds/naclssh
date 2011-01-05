// Author: Kate Volkova <gloom.msk@gmail.com>

var esBuf = new Array();
var utf8Buf = new Array();

function isAlpha(c) {
  return (c >= 97 && c <= 122) || (c >= 65 && c <= 90);
}

function isDigit(c) {
  return c >= 48 && c <= 57;
}

function isInArray(a, c) {
  return a.indexOf(c) != -1;
}

function clearEsBuf() {
  esBuf = new Array();
}

function clearUTFBuf() {
  utf8Buf = new Array();
}

function isEmpty(a) {
  return a.length == 0;
}


// 2 chars sequence (ESC 8, ESC =) terminators
var CHARS2_SEQ_END = [55, 56, 60, 61, 62, 124, 125, 126] 
                  // '7'  '8' '<' '=' '>' '|'  '}'  '~'; F,c,l,m,n,o terminate escape sequence parsing  with isAlpha check

// Returns pair:
//    bool - if the sequence is finished
//    int  - index of the last processed character
// esBuf[1] indicates how the sequence is terminated
function parseEscapeSequence(buffer, start) {
  var res = new Object();
  res.isfinished = false;
  res.index = start;

  for (; !res.isfinished && res.index < buffer.length; ++res.index) {
    esBuf.push(buffer[res.index]);

    if (esBuf.length > 1) {
      if (esBuf[1] == 93) {  // ']' 
        res.isfinished = buffer[res.index] == 7;
      } else {
        res.isfinished = (esBuf.length == 2 && isInArray(CHARS2_SEQ_END, esBuf[1]))
              || isAlpha(buffer[res.index])
              || buffer[res.index] == 64  // '@'
              || (esBuf.length == 3 && esBuf[1] == 40);  // '('
      }
    }
  }

  --res.index;
  return res;
}

// Default parameter for sequences which end with these characters is 0
var DEFAULT0 = [74, 75, 98, 103, 104, 108, 109];
            // 'J' 'K'  'b' 'g'  'h'  'l'  'm'

// Default parameter for sequences which end with these characters is 1
var DEFAULT1 = [64, 65, 66, 67, 68, 71, 76, 77, 80, 88, 100];
            // '@' 'A' 'B' 'C' 'D' 'G' 'L' 'M' 'P' 'X'  'd'

// 27  [  4  0  ;  2  3  H
// Returns array of parameters for escape sequence,
//     returns undefined if escape sequence is not correct
function parseArgs() {
  var i = 2;  // to skip ESC and '['

  if (esBuf[i] == 63) {         // ESC + [ + ?
    ++i;
  }

  if (i == esBuf.length - 1) {
    // no args, default values
    if (esBuf[i] == 72) {  // 'H'
      return [1, 1];
    } else if (isInArray(DEFAULT1, esBuf[i])) {
      return [1];
    } else if (isInArray(DEFAULT0, esBuf[i])) {
      return [0];
    } else if (esBuf[i] == 114) {  // 'r'
      return [0, 0];
    }
  }

  var args = new Array();
  var buf = '';

  var digitExpected = true;
  for (; i < esBuf.length - 1; ++i) {
    if (esBuf[i] == 59) {  // ';'
      if (digitExpected) {
        return undefined;
      }
      args.push(parseInt(buf));
      buf = '';
      digitExpected = true;
    } else if (isDigit(esBuf[i])) {
        buf += String.fromCharCode(esBuf[i]);
        digitExpected = false;
    } else {
      return undefined;
    }
  }

  args.push(parseInt(buf));
  return args;
}


function checkEscSequence(func) {
  var s = parseArgs();
  if (s == undefined) {
    // not an escape sequence, print to terminal
    writeToTerminal(esBuf);
  } else {
    func(s);
  }
}

// there is only 1 seq in buf at a time
function escapeSequence() {
  if (esBuf[1] == 93) {  // ']'
    // TODO: call set window header func
    // tmp - skip
    clearEsBuf();
    return;
  }

  if (esBuf[1] == 40) {  // '('
    setCharSet(String.fromCharCode(esBuf[2]));
    clearEsBuf();
    return;
  }

  if (esBuf[1] == 99) {  // 'c'
    ris();
    clearEsBuf();
    return;
  }


  switch (esBuf[esBuf.length - 1]) {
    case 55:  // '7'
      saveCursor();
      break;

    case 56:  // '8'
      restoreCursor();
      break;

    case 64:  // '@'
      if (esBuf[1] == 37) {  // '%'
        //TODO:
      } else {
        checkEscSequence(wrap1(insertChars));
      }
      break;

    case 65: // 'A'
      checkEscSequence(wrap1(cursorUp));
      break;

    case 66:  // 'B'
      checkEscSequence(wrap1(cursorDown));
      break;

    case 67:  // 'C'
      checkEscSequence(wrap1(cursorRight));
      break;

    case 68:  // 'D'
      checkEscSequence(wrap1(cursorLeft));
      break;

    case 71:  // 'G'
      if (esBuf[1] == 37) {  // '%'
        //TODO: 
      } else {
        checkEscSequence(wrap1(setCursorCol));
      }
      break;

    case 72:  // 'H'
      if (esBuf[1] == 91) {  // '['
        checkEscSequence(wrap2(setCursorPos));
      } else {
        tabSet();
      }
      break;

    case 74:  // 'J'
      if (esBuf.length == 2) {
        //TODO: ESC J - Erase from the cursor to the end of the screen.
      } else if (esBuf[2] == 63) {  // '?'
        //TODO: selective erase
      } else {
        checkEscSequence(wrap1(eraseInDisplay));
      }
      break;

    case 75:  // 'K'
      if (esBuf[1] == 91) {  // '['
        checkEscSequence(wrap1(eraseInLine));
      } else {
        // TODO: selective erase
      }
      break;

    case 76:  // 'L'
      checkEscSequence(wrap1(insertLines));
     break;

    case 77:  // 'M'
      if (esBuf[1] == 91) {  // '['
        checkEscSequence(wrap1(deleteLines));
      } else {
        scrollDown();
      }
      break;

    case 80:  // 'P'
      checkEscSequence(wrap1(deleteChar));
      break;

    case 88:  // 'X'
      if (esBuf.length == 2) { // ESC + X
        // TODO: Start of String ( SOS is 0x98)
      } else {
        checkEscSequence(wrap1(eraseChar));
      }
      break;

    case 98:  // 'b'
      checkEscSequence(wrap1(repLast));
      break;

    case 100: // 'd'
      checkEscSequence(wrap1(setCursorRow));
      break;

    case 103: // 'g'
      checkEscSequence(wrap1(tabClear));
      break;

    case 104: // 'h'
      if (esBuf[2] == 63) {  // '?'
        checkEscSequence(setPrivateMode);
      } else {
        checkEscSequence(setMode);
      }
      break;

    case 108: // 'l'
      if (esBuf[2] == 63) {  // '?'
        checkEscSequence(resetPrivateMode);
      } else {
        checkEscSequence(resetMode);
      }
      break;

    case 109:  // 'm'
      if (esBuf[1] == 91) {  // '['
        checkEscSequence(setAttribute);
      } else {
        //TODO:
      }
      break;

    case 112:  // 'p'
      if (esBuf.length > 3) {
        if (esBuf[2] == 33) {  // '!'
          softReset();
        } else if (esBuf[2] == 62) {  // '>'
          //TODO: "CSI > P s p\n"
        }
      }
      break;

    case 114: // 'r'
      if (esBuf[esBuf.length - 2] != 36) {  // '$'
        if (esBuf[2] == 63) {  // '?'
          // TODO: "CSI ? P m r\n"
        } else {
          checkEscSequence(wrap2(setScrollRegion));
        }
      } else {
         // TODO: "CSI P t ; P l; P b ; P r ; P s $ r"
      }
      break;

    case 115: // 's'
      if (esBuf.length == 3) {
        saveCursor();
      } else {
        //TODO: "CSI ? P m s"
      }
      break;

    default:
      // TODO: check if it's a correct sequence - otherwise print to terminal
      //       the same for parsed and skipped sequences
      // detect unparsed seqs
      break;
  }

  clearEsBuf();
}

// bool - if the sequence is finished
// int - index of the last processed character
// 194 <= utfBuf[0] <= 244 - should be checked before calling
function parseUTF8(buffer, start) {
  
  var k; // how many bytes do we need to finish the sequence
  utf8Buf.push(buffer[start]);

  if (utf8Buf[0] < 224) {
    k = 2;
  } else if (utf8Buf[0] >= 224 && utf8Buf[0] <= 239) {
    k = 3;
  } else {
    k = 4;
  }
  k -= utf8Buf.length;

  var res = new Object();

  for (res.index = start + 1; k > 0 && res.index < buffer.length; ++res.index) {
    utf8Buf.push(buffer[res.index]);
    --k;
  }

  --res.index;
  res.isfinished = k == 0;
  return res;
}

function decodeUTF8() {
  for (var i = 1; i < utf8Buf.length; ++i) {
    // 128-191 Second, third, or fourth byte of a multi-byte sequence
    if (utf8Buf[i] > 191 || utf8Buf[i] < 128) {
      // TODO: print '�' only instead of the wrong bytes!!
      // TODO: check for start of the new unicode sequence or normal chars
      for (var j = 0; j < utf8Buf.length; ++j) {
        writeCodeToTerminal(0xFFFD);
        clearUTFBuf();
        return;
      }
    }
  }

  var code = 0xFFFD;
  switch (utf8Buf.length) {
    case 2:
      code = ((utf8Buf[0] & 31) << 6) | (utf8Buf[1] & 63);
      break;

    case 3:
      code = ((utf8Buf[0] & 15) << 12) | ((utf8Buf[1] & 63) << 6) | (utf8Buf[2] & 63);
      break;

    case 4:
      // TODO:
      //code = 0xFFFD;  //TMP
      break;
  }
  writeCodeToTerminal(code);
  clearUTFBuf();
  return;
}

function writeToTerm(code) {
  // Vertical tab and Form feed are equal to Line feed
  if (code == 11 || code == 12) {
    code = 10;
  }

  if (code >= 32 || code == 10) {
    writeCodeToTerminal(code);
  }
}

function printToTerminal(buffer) {
//  myAlert('printToTerminal: ' + buffer);
  var result;
  var i = 0;

  if (!isEmpty(esBuf)) {
    // finish escape sequence parsing
    result = parseEscapeSequence(buffer, 0);
    if (!result.isfinished) {
      return; // sequence not finished
    }
    escapeSequence();
    i = result.index + 1;
  }

  if (!isEmpty(utf8Buf)) {
    // finish utf8 multibyte sequence
    result = parseUTF8(buffer, 0);
    if (!result.isfinished) {
      return; // sequence not finished
    }
    decodeUTF8();
    i = result.index + 1;
  }

  for (; i < buffer.length; ++i) {
    var code = buffer[i];

    if (code > 193 && code < 245) {
      // UTF8 multibyte sequence
      result = parseUTF8(buffer, i);
      if (!result.isfinished) {
        // utf8 sequence will be finished with the next packet
        break;
      } else {
        decodeUTF8();
        i = result.index;
      }
    } else if (code < 128) {
        if (code == 27) {
          // escape sequence
          result = parseEscapeSequence(buffer, i);
          if (!result.isfinished) {
            // escape seq will be finished with the next packet
            break;
          } else {
            escapeSequence();
            i = result.index;
          }
        } else if (code == 8) {
          backSpace();
        } else if (code == 9) {
          tab();
        } else if (code >= 10 && code <= 12) {
          // Vertical tab (11) and Form feed (12) are equal to Line feed (10)
          writeCodeToTerminal(10);
        } else if (code == 13) {
          carriageReturn();
        } else if (code >= 32) {
          // usual char
          writeCodeToTerminal(code);
        }
    } else {
      // wrong byte, print '�'
      writeCodeToTerminal(0xFFFD);
    }
  }

  terminalUpdate();
}


function wrap1(func) {
  return function(args) { func(args[0]);};
}

function wrap2(func) {
  return function(args) { func(args[0], args[1]);};
}

