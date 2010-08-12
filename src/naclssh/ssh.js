// Author: Kate Volkova <gloom.msk@gmail.com>

// keep synchronized with kNaClChunkSize in ssh_plugin.cc
var NACL_CHUNK_SIZE = 60 * 1024;

var ws;

// Called from libssh NaCl port
function sendData(c) {
  ws.send(c);
}

function sendEscSeq(key) {
  document.getElementById('ssh_plugin').sendEscapeSequence(key);
}

function sendKey(key) {
  document.getElementById('ssh_plugin').sendUnicodeKey(key);
}

// proxyURI is like ws://hostname:port/echo
// and points to websocket-to-socket proxy
// like http://web2socket.googlecode.com
function loadData(proxyURI, host, user, pwd) {

  ws = new WebSocket(proxyURI);

  ws.onopen = function() {
    document.getElementById('ssh_plugin').sshconnect(user, pwd);
    ws.send(host);
  };

  ws.onmessage = function(msg) {
    for (var i = 0; i < msg.data.length; i += NACL_CHUNK_SIZE) {
      document.getElementById('ssh_plugin').savedata(
          msg.data.length <= NACL_CHUNK_SIZE ? msg.data : msg.data.substring(i, i + NACL_CHUNK_SIZE), i, msg.data.length);
    }
  }

  ws.onclose = function() {
    myAlert('socket closed');
  }

  ws.onerror = function() {
    myAlert('error');
  }
}

function loadNaClSSHClient(contentDiv, naclElementId, nexes) {
  // Load the published .nexe.  This includes the 'nexes' attribute which
  // shows how to load multi-architecture modules.  Each entry in the
  // table is a key-value pair: the key is the runtime ('x86-32',
  // 'x86-64', etc.); the value is a URL for the desired NaCl module.
  contentDiv.innerHTML = '<embed id="' + naclElementId + '" '
      + 'style="width:0;height:0" '
      + 'type="application/x-nacl-srpc" '
      + 'onload=moduleDidLoad() />';
  // Note: this code is here to work around a bug in Chromium build
  // #47357.  See also
  // http://code.google.com/p/nativeclient/issues/detail?id=500
  document.getElementById(naclElementId).nexes = nexes;
}


function moduleDidLoad() {
  // TODO: status message
}

