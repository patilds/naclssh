// Copyright 2010 Google Inc. All Rights Reserved.

/**
 * @fileoverview WebSocket-to-socket proxy selection logic.
 * @author krasin@google.com (Ivan Krasin)
 */

web2socket = new Object();

web2socket.uris = [
  "ws://127.0.0.1:10101/echo",
];

web2socket.preferredIndex = -1;

web2socket.getProxyURI = function() {
  var index = web2socket.preferredIndex;
  if (index == -1) {
    index = 0;
  }
  return web2socket.uris[index];
};

web2socket.tryProxy = function(index) {
  var ws = new WebSocket(web2socket.uris[index]);

  ws.onopen = function() {
    if (web2socket.preferredIndex == -1) {
      web2socket.preferredIndex = index;
    }
    ws.close();
  };
}

web2socket.findNearestProxy = function() {
  for (var i = 0; i < web2socket.uris.length; i++) {
    web2socket.tryProxy(i);
  }
};

web2socket.findNearestProxy();
