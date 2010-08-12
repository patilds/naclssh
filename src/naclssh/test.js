function checkCursorPos(expected, result) {
  return expected.row == result.row && expected.col == result.col;
}

function compareArrays(a1, a2) {
  if (a1.length != a2.length) {
    return false;
  }

  for (var i = 0; i < a1.length; ++i) {
    if (a1[i] != a2[i]) {
      return false;
    }
  }

  return true;
}

var tests = [
  function() {
    initTerminal();

    // "^[]0 gloom@gloom-laptop: ~7gloom@gloom-laptop:~$ "
    printToTerminal([27,93,48,59,103,108,111,111,109,64,103,108,111,111,109,45,
        108,97,112,116,111,112,58,32,126,7,103,108,111,111,109,64,103,108,111,
        111,109,45,108,97,112,116,111,112,58,126,36,32]);

    var snapshot = makeSnapshot();

    // only "gloom@gloom-laptop:~$ " should be printed
    var expectedLine = ['g','l','o','o', 'm', '@', 'g', 'l', 'o', 'o', 'm', '-', 'l', 'a', 'p', 't', 'o', 'p', ':', '~', '$'];
    var expectedCursor = new Object();
    expectedCursor.row = 0;
    expectedCursor.col = 22;

    for (var i = expectedLine.length; i < maxcol; ++i) {
      expectedLine.push('&nbsp;');
    }

    return checkCursorPos(expectedCursor, snapshot.cursorPos) &&
        compareArrays(expectedLine, snapshot.screenBuffer.chars[0]);
  },

  // Moving cursor to the next line
  function() {
    initTerminal();
    var snapshot = makeSnapshot();

    var toPrint = new Array(maxcol + 2);
    for (var i = 0; i < toPrint.length; ++i) {
      toPrint[i] = 49;  // '1'
    }
    
    printToTerminal(toPrint);
    var snapshot = makeSnapshot();

    var expectedCursor = new Object();
    expectedCursor.row = 1;
    expectedCursor.col = 2;

    return checkCursorPos(expectedCursor, snapshot.cursorPos);
  }
]

// TODO: remove initTable from initTerminal
function runTests() {
  for (var i = 0; i < tests.length; ++i) {
    if (tests[i]()) {
      myAlert('Test ' + i + ' passed');
    }
    else {
      myAlert('Test ' + i + ' failed');
    }
  }
}
