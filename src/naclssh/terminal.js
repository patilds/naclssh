// Author: Kate Volkova <gloom.msk@gmail.com>

// To be used without main.js keyPressHandler and keyDownHandler should be defined

var maxcol = 90;
var maxrow = 40;

var scrollStart = 0;
var scrollEnd = maxrow - 1;


// TODO: make buffer class!

var screenBuffer = new Object();
screenBuffer.chars = new Array(maxrow);
screenBuffer.colors = new Array(maxrow);
screenBuffer.bgrColors = new Array(maxrow);
screenBuffer.tabs = new Array(maxcol);

var alternateScreenBuffer = new Object();
alternateScreenBuffer.chars = new Array(maxrow);
alternateScreenBuffer.colors = new Array(maxrow);
alternateScreenBuffer.bgrColors = new Array(maxrow);
alternateScreenBuffer.tabs = new Array(maxcol);

var activeScreenBuf = screenBuffer;

var DEFAULT_COLOR = 'white';
var DEFAULT_BGRCOLOR = 'black';

var activeColor = DEFAULT_COLOR;
var bgrColor = DEFAULT_BGRCOLOR;

var cursorPos = new Object();

var cursorActive = true;

var savedCursorPos = new Object();
savedCursorPos.col = 0;
savedCursorPos.row = 0;

var reverseVideoOn = false;
var charSet = 'USASCII';

var wrapMode = true;

var lastChar = '&nbsp;';

// replace and insert
var mode = 'r'; // 'i'

var escapeMap = new Object();

var charSetMap = new Object();

var lineGraphicsMap = new Object();

var tabActive = true;

//TODO: escape more symbols
function initEscapeMap() {
  escapeMap[' '] = '&nbsp;';
  escapeMap['<'] = '&lt;';
  escapeMap['>'] = '&gt;';
  escapeMap['&'] = '&amp;';
}

function setWrapMode() {
  wrapMode = true;
}

function setNoWrapMode() {
  wrapMode = false;
}


//TODO: add other charsets
function initCharSetMap() {
  charSetMap['0'] = 'LineGraphics';
  charSetMap['B'] = 'USASCII';
}

function setCharSet(c) {
  var cs = charSetMap[c];
  if (cs == undefined) {
    myAlert('Char set ' + c + ' is not defined');
  } else {
    charSet = cs;
  }
}

function setAttribute(data) {
  for (var i = 0; i < data.length; ++i) {
    switch(data[i]) {
      case 0: // all attributes are off
        activeColor = DEFAULT_COLOR;
        bgrColor = DEFAULT_BGRCOLOR;
        reverseVideoOn = false;
        break;
      case 7:
        reverseVideoOn = true;
        break;
      case 27:
        reverseVideoOn = false;
        break;
      case 30:
        activeColor = 'black';
        break;
      case 31:
        activeColor = 'red';
        break;
      case 32:
        activeColor = 'green';
        break;
      case 33:
        activeColor = 'yellow';
        break;
      case 34:
        activeColor = 'blue';
        break;
      case 35:
        activeColor = 'magenta';
        break;
      case 36:
        activeColor = 'cyan';
        break;
      case 37:
        activeColor = 'white';
        break;
      case 39:
        activeColor = DEFAULT_COLOR;
        break;
      case 40:
        bgrColor = 'black';
        break;
      case 41:
        bgrColor = 'red';
        break;
      case 42:
        bgrColor = 'green';
        break;
      case 43:
        bgrColor = 'yellow';
        break;
      case 44:
        bgrColor = 'blue';
        break;
      case 45:
        bgrColor = 'magenta';
        break;
      case 46:
        bgrColor = 'cyan';
        break;
      case 47:
        bgrColor = 'white';
        break;
      case 49:
        bgrColor = DEFAULT_BGRCOLOR;
        break;
   }
  }
}


function createNewLine(i) {
  activeScreenBuf.chars[i] = new Array(maxcol);
  activeScreenBuf.colors[i] = new Array(maxcol);
  activeScreenBuf.bgrColors[i] = new Array(maxcol);
}

function copyChar(drow, dcol, srow, scol) {
  activeScreenBuf.chars[drow][dcol] = activeScreenBuf.chars[srow][scol];
  activeScreenBuf.colors[drow][dcol] = activeScreenBuf.colors[srow][scol];
  activeScreenBuf.bgrColors[drow][dcol] = activeScreenBuf.bgrColors[srow][scol];
}

function copyLine(dest, source) {
  activeScreenBuf.chars[dest] = activeScreenBuf.chars[source];
  activeScreenBuf.colors[dest] = activeScreenBuf.colors[source];
  activeScreenBuf.bgrColors[dest] = activeScreenBuf.bgrColors[source]; 
}

function initScreenBuf() {
  for (var i = 0; i < maxrow; i++) {
    createNewLine(i);
    for (var j = 0; j < maxcol; j++) {
      clearChar(i,j);
    }
  }

  setDefaultTabs();
}

function initTable() {
  // init chars buffer
  // TODO: all funcs are using active screenbuf - clearChar for example
  // when they use this, then we don't have to switch between bufs here
  initScreenBuf();
  activeScreenBuf = alternateScreenBuffer;
  initScreenBuf();
  activeScreenBuf = screenBuffer;

  var table = document.getElementById('termTable');
  var tblBody = document.createElement("tbody");

  for (var j = 0; j < maxrow; j++) {
    // creates a table row
    var row = document.createElement("tr");
    row.id = j + '_';  
    row.style.color = DEFAULT_COLOR;
    row.style.backgroundColor = DEFAULT_BGRCOLOR;
    row.onclick=createClosure(j);

    var cell = document.createElement("td");
    cell.innerHTML = '&nbsp;';
    row.appendChild(cell);

    // add the row to the end of the table body
    tblBody.appendChild(row);
  }

  // put the <tbody> in the <table>
  table.appendChild(tblBody);
}

function initTerminal(proxyURI, host, user, pwd) {
  initEscapeMap();
  initCharSetMap();
  initLineGraphicsMap();

  initTable();

  cursorPos.col = 0;
  cursorPos.row = 0; 
}

function write(c) {
  writeTo(cursorPos.row, cursorPos.col, c);
}

function writeTo(i, j, c) {
  activeScreenBuf.chars[i][j] = c;
  lastChar = c;


  if (reverseVideoOn) {
    activeScreenBuf.colors[i][j] = bgrColor;
    activeScreenBuf.bgrColors[i][j] = activeColor;
  } else {
    activeScreenBuf.colors[i][j] = activeColor;
    activeScreenBuf.bgrColors[i][j] = bgrColor;
  }

}

function repLast(n) {
  for (var i = 0; i < n; ++i) {
    writeCharToTerminal(lastChar);
  }
}

// Hard terminal reset
function ris() {
  softReset();
  clearAll();
  setCursorPos(1,1); 
  setDefaultTabs();

  //TODO:
  // Sets the SGR state to normal.
  // Sets all character sets to the default.
  // Sets the selective erase attribute write state to "not erasable".
}

function softReset() {
  setAttribute([0]);  
}


function clear(srow, erow, scol, ecol) {
  for (var i = srow; i < erow; ++i) {
    for (var j = scol; j < ecol; ++j) {
      clearChar(i,j);
    }
  }
}

function clearAll() {
  clear(0, maxrow, 0, maxcol);
}

function clearBelow() {
  clear(cursorPos.row, maxrow, cursorPos.col, maxcol);
}

function clearAbove() {
  clear(0, cursorPos.row + 1, 0, cursorPos.col + 1);
}

function eraseInDisplay(arg) {
  switch(arg) {
    case 0:
      clearBelow();
      break;

    case 1:
      clearAbove();
      break;

    case 2:
      clearAll();
      break;

    case 3:
      //TODO: clear saved lines (?)
      break;
  }
}

function useAltScreenBuf() {
  activeScreenBuf = alternateScreenBuffer;
}

function useNormalScreenBuf() {
  activeScreenBuf = screenBuffer;
}

function setPrivateMode(modes) {
  for (var i = 0; i < modes.length; ++i) {
  switch(modes[i]) {
    case 1:
      // application cursor case, TMP ignore
      break;
    case  7:
      setWrapMode();
    break;

    case 25:
      showCursor();
      break;

    case 47:
      useAltScreenBuf();
      break;

    case 1000:
      //Send Mouse X & Y on button press and release. See the section Mouse Tracking. 
      break;

    case 1047:
       useAltScreenBuf();
      break;

    case 1048:
      saveCursor();
      break;

    case 1049:
      saveCursor();
      useAltScreenBuf();
      clearAll();
    break;

    default:
      //myAlert("!! ?h mode " + modes + "should be parsed");
  }
  }
}

function hideCursor() {
  cursorActive = false;
}

function showCursor() {
  cursorActive = true;
} 

function resetPrivateMode(modes) {
  for (var i = 0; i < modes.length; ++i) {
  switch(modes[i]) {
    case  7:
      setNoWrapMode();
    break;

    case 12:
      // TODO
      //stopBlinkingCursor();
      break;

    case 25:
      hideCursor();
      break;

    case 47:
      useNormalScreenBuf();
      break;

    case 1000:
      //Send Mouse X & Y on button press and release. See the section Mouse Tracking. 
      break;

    case 1047:
      //Use Normal Screen Buffer, clearing screen first if in the Alternate Screen (unless disabled by the titeInhibit resource)
      // TODO: check when to clean
       useNormalScreenBuf();
      break;

    case 1048:
      restoreCursor();
      break;
    
    case 1049:
      // TODO: check when to clean
      useNormalScreenBuf();
      restoreCursor();
    break;

    default:
      //myAlert("!! ?l mode " + modes + "should be parsed");
  }
  }
}

function setMode(modes) {
  for (var i = 0; i < modes.length; ++i) {
    switch(modes[i]) {
      case 4:
        // insert mode
        mode = 'i';
        break;
    default:
      //myAlert("!! h mode " + mode + "should be parsed");
    }
  }
}

function resetMode(modes) {
  for (var i = 0; i < modes.length; ++i) {
    switch(modes[i]) {
      case 4:
        // replacement mode
        mode = 'r';
        break;
    default:
      //myAlert("!! h mode " + modes[i] + "should be parsed");
    }
  }
}

function setScrollRegion(r1, r2) {
  if (r1 == r2 && r2 == 0) {
    // default values
    scrollStart = 0;
    scrollEnd = maxrow - 1;
  } else {
    scrollStart = r1 - 1;
    scrollEnd = r2 - 1;
  }
}

function scroll(r1, r2) {
  for (var i = r1; i < r2; ++i) {
    copyLine(i, i + 1);
  }

  createNewLine(r2);

  for (var i = 0; i < maxcol; ++i) {
    clearChar(r2,i);
  }
}

function scrollBack(r1, r2) {
  for (var i = r2; i > r1; --i) {
    copyLine(i, i - 1);
  }

  createNewLine(r1);

  for (var i = 0; i < maxcol; ++i) {
    clearChar(r1, i);
  }
}



function checkMaxRow() {
  if (cursorPos.row == scrollEnd + 1) {
    scroll(scrollStart, scrollEnd);
    cursorPos.row = scrollEnd;
  }
}

function checkMinRow() {
  if (cursorPos.row == scrollStart - 1) {
    scrollBack(scrollStart, scrollEnd);
    cursorPos.row = scrollStart;
  }
}

// TODO: rename insertBlankChars
// and split insert and blank part!
function insertChars(n) {
  for (var i = maxcol - 1; i >= cursorPos.col + n ; --i) {
    copyChar(cursorPos.row, i, cursorPos.row, i - n);
  }
  for (var i = cursorPos.col + n - 1; i >= cursorPos.col; --i) {
    clearChar(cursorPos.row, i);
  }
}

function insertLines(n) {
  for (var i = scrollEnd - n; i >= cursorPos.row; --i) {
    copyLine(i + n, i);
  }
  for (var i = cursorPos.row; i < cursorPos.row + n; ++i) {
    createNewLine(i);
    for (var j = 0; j < maxcol; ++j) {
      clearChar(i, j);
    }
  }
}

function deleteLines(n) {
  for (var i = cursorPos.row; i <= scrollEnd - n; ++i) {
    copyLine(i, i + n);
  }

  for (var i = 1; i <= n; ++i) {
    createNewLine(scrollEnd + 1 - i);
    // TODO: move init to the separate func
    for (var j = 0; j < maxcol; ++j) {
      clearChar(scrollEnd + 1 - i, j);
    }
  }
}

//To draw cursor: swaps background and foreground colors
function swapColors() {
  var c = Math.min(cursorPos.col, maxcol - 1);
  var color = activeScreenBuf.colors[cursorPos.row][c];
  activeScreenBuf.colors[cursorPos.row][c] = activeScreenBuf.bgrColors[cursorPos.row][c];
  activeScreenBuf.bgrColors[cursorPos.row][c] = color;
}

//copy charsbuf to the table
function terminalUpdate() {
  var rows = document.getElementById('termTable').getElementsByTagName('tbody')[0].getElementsByTagName('tr');

  if (cursorActive) {
    swapColors();
  }

  var charsCopy = new Array(maxrow);

  for (var i = 0; i < rows.length; ++i) {
    charsCopy[i] = new Array(maxcol);
    charsCopy[i][0] = '<span style="color:' + activeScreenBuf.colors[i][0] + ';background-color:'+
        activeScreenBuf.bgrColors[i][0] + '">' + activeScreenBuf.chars[i][0];

    var prevColor = activeScreenBuf.colors[i][0];
    var prevBColor = activeScreenBuf.bgrColors[i][0];
    for (var j = 1; j < maxcol; ++j) {
      if (activeScreenBuf.colors[i][j] == prevColor && activeScreenBuf.bgrColors[i][j] == prevBColor) {
          // do nothing with colors
          charsCopy[i][j] = activeScreenBuf.chars[i][j];
        } else {
          // close span, start the new one if not default color
          charsCopy[i][j] = '</span>';

    if (activeScreenBuf.colors[i][j] == DEFAULT_COLOR && activeScreenBuf.bgrColors[i][j] == DEFAULT_BGRCOLOR) {
            // no new span is needed
      charsCopy[i][j] += activeScreenBuf.chars[i][j];
          } else {
            charsCopy[i][j] += '<span style="color:' + activeScreenBuf.colors[i][j] + ';background-color:'+
                activeScreenBuf.bgrColors[i][j] + '">' + activeScreenBuf.chars[i][j];
          }
          prevColor = activeScreenBuf.colors[i][j];
          prevBColor = activeScreenBuf.bgrColors[i][j];
        }
    }

    // Closing span at the end of each line
    rows[i].innerHTML = charsCopy[i].join('') + '</span>';
  }

  if (cursorActive) {
    swapColors();
  }
}

function scrollDown() {
  cursorPos.row--;
  checkMinRow();
}

// data - array
// this func is called for printing wrong escape sequences => checks for empty chars should be added
function writeToTerminal(data) {
  for (var i = 0; i < data.length; i++) {
    if (data[i]) {
      writeCodeToTerminal(data[i]);
    }
  }
}

// code should be already checked if it's not zero e.t.c.
function writeCodeToTerminal(code) {
  writeCharToTerminal(String.fromCharCode(checkCharSet(code)));
}

function initLineGraphicsMap() {
  // `
  lineGraphicsMap[96] = 0x25C6;  // diamond

  // a
  lineGraphicsMap[97] = 0x2592;  // checker board (stipple)

  // f - z
  lineGraphicsMap[102] = 0xB0;    // degree symbol
  lineGraphicsMap[103] = 0xB1;    // plus/minus 
  lineGraphicsMap[104] = 0x2424;  // board of squares
  lineGraphicsMap[105] = 0x240B;  // lantern symbol
  lineGraphicsMap[106] = 0x2518;  // lower right corner
  lineGraphicsMap[107] = 0x2510;  // upper right corner
  lineGraphicsMap[108] = 0x250C;  // upper left corner
  lineGraphicsMap[109] = 0x2514;  // lower left corner
  lineGraphicsMap[110] = 0x253C;  // large plus or crossover
  lineGraphicsMap[111] = 0x23BA;  // scan line 1 
  lineGraphicsMap[112] = 0x23BB;  // scan line 3 
  lineGraphicsMap[113] = 0x2500;  // horizontal line
  lineGraphicsMap[114] = 0x23BC;  // scan line 7 
  lineGraphicsMap[115] = 0x23BD;  // scan line 9 
  lineGraphicsMap[116] = 0x251C;  // tee pointing right 
  lineGraphicsMap[117] = 0x2524;  // tee pointing left
  lineGraphicsMap[118] = 0x2534;  // tee pointing up
  lineGraphicsMap[119] = 0x252C;  // tee pointing down
  lineGraphicsMap[120] = 0x2502;  // vertical line
  lineGraphicsMap[121] = 0x2264;  // less-than-or-equal-to
  lineGraphicsMap[122] = 0x2265;  // greater-than-or-equal-to

  // {
  lineGraphicsMap[123] = 0x3C0;   //  π

  // |
  lineGraphicsMap[124] = 0x2260;    //  not-equal  

  // }
  lineGraphicsMap[125] = 0xA3;     // £
}

function checkCharSet(c) {
  switch (charSet) {
    case 'USASCII':
      return c;

    case 'LineGraphics':
      var res = lineGraphicsMap[c];

      if (res == undefined) {
        myAlert('Line graphics char set, unexpected ' + c);
        res = 0x2218;
      }
      return res;
  }
}

function escapeChar(c) {
  var res = escapeMap[c];
  if (res == undefined) {
    return c;
  }
  return res;
}

function writeCharToTerminal(c) {

  // mode == 'r'
  if (c == '\n') {
    cursorPos.row++;
    checkMaxRow();
  } else {

    if (mode == 'i') {
      // TODO: split into 2 funcs, don't need to put nbsp
      insertChars(1);
    }

    var c1 = escapeChar(c);
    if (cursorPos.col == maxcol) {
      if (wrapMode) {
        cursorPos.col = 0;
        cursorPos.row++;
        checkMaxRow();
      } else {
        cursorPos.col = maxcol - 1;
      }
    }
    write(c1);
    cursorPos.col++;
  }
}


function setFocusToTerm() {
  setFocus('terminal');
}

function saveCursor() {
  savedCursorPos.row = cursorPos.row;
  savedCursorPos.col = cursorPos.col;
}

function restoreCursor() {
  cursorPos.row = savedCursorPos.row;
  cursorPos.col = savedCursorPos.col;
}

function carriageReturn() {
  cursorPos.col = 0;
}

function clearChar(row, column) {
  activeScreenBuf.chars[row][column] = '&nbsp;';

  if (reverseVideoOn) {
    activeScreenBuf.colors[row][column] = bgrColor;
    activeScreenBuf.bgrColors[row][column] = activeColor;
  } else {
    activeScreenBuf.colors[row][column] = activeColor;
    activeScreenBuf.bgrColors[row][column] = bgrColor;
  }
}

// 0 - default - to the right
// 1 - to the left
// 2 - the whole line
function eraseInLine(n) {
//  myAlert("eraseInLine " + cursorPos.row + "  " + cursorPos.col);

  var start = 0;
  var end = maxcol - 1;

  switch(n) {
    case 0:
      start = cursorPos.col;
      //end = maxcol - 1;
      break;

    case 1:
      //start = 0;
      end = cursorPos.col;
      break;

    // TODO: check - don't we have to scroll?
    case 2:
      //start = 0;
      //end = maxcol - 1;
      break;
  }

  for (var i = start; i <= end; ++i) {
    clearChar(cursorPos.row, i);
  }
}

function eraseChar(n) {
  for (var i = cursorPos.col; i < cursorPos.col + n; ++i) {
    clearChar(cursorPos.row, i);
  }
}

// at cursor position
function deleteChar(n) {
//  myAlert("eraseChar " + cursorPos.row + "  " + cursorPos.col);
  for (var i = cursorPos.col; i < maxcol - n; ++i) {
    copyChar(cursorPos.row, i, cursorPos.row, i + n);
  }

  for (var i = maxcol - n; i < maxcol; ++i) {
    clearChar(cursorPos.row, i);
  }
}

function setCursorPos(r,c) {
//  myAlert('setCursorPos' + r + c);
  setCursorRow(r);
  setCursorCol(c);
}
function setCursorCol(c) {
  if (c >= maxcol) {
    cursorPos.col = maxcol - 1;
  } else {
    cursorPos.col = c - 1;
  }
}

function setCursorRow(r) {
  if (r >= maxrow) {
    cursorPos.row = maxrow - 1;
  } else {
    cursorPos.row = r - 1;
  }
}



function cursorLeft(n) {
  cursorPos.col = Math.max(cursorPos.col - n, 0);
}

function cursorRight(n) {
  cursorPos.col = Math.min(cursorPos.col + n, maxcol - 1);
}

function cursorUp(n) {
  cursorPos.row = Math.max(cursorPos.row - n, 0);
}

function cursorDown(n) {
  cursorPos.row = Math.min(cursorPos.row + n, maxrow - 1);
}

function backSpace() {
  cursorLeft(1);
}


function setFocus(id) {
  var elem = document.getElementById(id);
  elem.focus();
}

function tab() {
  if (tabActive) {
    if (cursorPos.col < maxcol - 1) {
      ++cursorPos.col;
    }
    for (; cursorPos.col < maxcol - 1; ++cursorPos.col) {
      if (activeScreenBuf.tabs[cursorPos.col]) {
        break;
      }
    }
  }
}

function tabSet() {
  if (cursorPos.col < maxcol) {
    activeScreenBuf.tabs[cursorPos.col] = true;
  }
  tabActive = true;
}

function tabClear(arg) {
  switch (arg) {
    case 0:
      if (cursorPos.col < maxcol) {
        activeScreenBuf.tabs[cursorPos.col] = false;
      }
      break;

    case 3:
      clearAllTabs();
      break;
  }
}

function setDefaultTabs() {
  for (var i = 0; i < maxcol; ++i) {
    activeScreenBuf.tabs[i] = i % 8 == 0;
  }
  tabActive = true;
}

function clearAllTabs() {
  for (var i = 0; i < maxcol; ++i) {
    activeScreenBuf.tabs[i] = false;
  }
  tabActive = false;
}


function copyArray(src) {
  var dest = new Array(src.length);
  for (var i = 0; i < src.length; ++i) {
    dest[i] = src[i];
  }
  return dest;
}

function copyMatrix(src) {
  var dest = new Array(src.length);
  for (var i = 0; i < src.length; ++i) {
    dest[i] = copyArray(src[i]);
  }
  return dest;
}

// For tests
function makeSnapshot() {
  var snapshot = new Object();
  snapshot.screenBuffer = new Object();

  snapshot.screenBuffer.chars = copyMatrix(activeScreenBuf.chars);
  snapshot.screenBuffer.colors = copyMatrix(activeScreenBuf.colors);
  snapshot.screenBuffer.bgrColors = copyMatrix(activeScreenBuf.bgrColors);

  snapshot.screenBuffer.tabs = copyArray(activeScreenBuf.tabs);

  snapshot.cursorPos = new Object();
  snapshot.cursorPos.col = cursorPos.col;
  snapshot.cursorPos.row = cursorPos.row;

  return snapshot;
}

// TMP DEBUG!
function myAlert(msg) {
  d = document.getElementById('log');
  d.innerHTML += msg + '\n';
}

// TMP DEBUG!
function createClosure(i) {
  return function() { mouseClickHandler(i);}
}

// TMP DEBUG!
function mouseClickHandler(i) {
  var str = '';
  for (var j = 0; j < maxcol; ++j) {
    if (activeScreenBuf.chars[i][j].length == 1) {
      str += activeScreenBuf.chars[i][j].charCodeAt() + '  ';
    } else {
      str += activeScreenBuf.chars[i][j];
    }
  }
  myAlert(str);
}

