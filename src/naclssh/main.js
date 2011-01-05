// Author: Kate Volkova <gloom.msk@gmail.com>

// To be used without ssh.js sendEscSeq and sendKey should be defined

// printable chars handler
function keyPressHandler(key) {
//  myAlert(key);
  sendKey(key);
}

function stopEvent() {
  try {
    event.stopPropagation();
    event.preventDefault();
  } catch (e) {}
  try {
    event.cancelBubble = true;
    event.returnValue  = false;
  } catch (e) {}

   return false;
}

// chars that are not handled with keypress event 
function keyDownHandler(key) {
  if (window.event.ctrlKey) {
    //TODO: add @, ^ _  - 2 + shift, 6 +shift
    if (key >= 65 && key <= 93) {
      sendKey(key - 64);
    }
  }

  else {
    switch (key) {
      case 32:  // space
        sendKey(32);
        break;
      case 27:
        sendKey(27); //esc
      break;
      case 8: // backspace
        sendKey(127);
        break;
      case 9: // tab
        sendKey(9);
        break;
      case 13:  // enter
        sendKey(10); // line feed
        break;
      case 33:  // pg up
        sendEscSeq('[5~');
        break;
      case 34:  // pg down
        sendEscSeq('[6~');
        break;
      case 35:  // end
        sendEscSeq('OF');
        break;
      case 36:  // home
        sendEscSeq('OH');
        break;
      case 37:  // left
        sendEscSeq('OD');
        break;
      case 38:  // up
        sendEscSeq('OA');
        break;
      case 39:  // right
        sendEscSeq('OC');
        break;
      case 40:  // down
        sendEscSeq('OB');
        break;
      case 46:  // del
        sendEscSeq('[3~');
        break;

      case 112: // F1
        sendEscSeq('OP');
        break;

      case 113: // F2
        sendEscSeq('OQ');
        break;

      case 114: // F3
        sendEscSeq('OR');
        break;

     case 115: // F4
        sendEscSeq('OS');
        break;

      case 116: // F5
        sendEscSeq('[15~');
        break;

      case 117: // F6
        sendEscSeq('[17~');
        break;

      case 118: // F7
        sendEscSeq('[18~');
        break;

      case 119: // F8
        sendEscSeq('[19~');
        break;

      case 120: // F9
        sendEscSeq('[20~');
        break;

      case 121: // F10
        sendEscSeq('[21~');
        break;

      case 122: // F11
        sendEscSeq('[23~');
        break;

      case 123: // F12
        sendEscSeq('[24~');
        break;

      default:  // printable character - don't have to stop event
        return;
    }

  }

  return stopEvent();
}

