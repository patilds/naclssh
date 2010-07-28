// Copyright 2010 Google Inc. All Rights Reserved.

/**
 * @fileoverview WebSocket-to-socket proxy selection logic.
 * @author krasin@google.com (Ivan Krasin)
 */

web2socket = new Object();

web2socket.getProxyURI = function() {
  return "ws://127.0.0.1:10101/echo";
};

