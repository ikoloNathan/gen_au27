let socket;
let id = 0;


function wsURL() {
  const proto = (location.protocol === 'https:') ? 'wss' : 'ws';
  return `${proto}://${location.host}/ws`;
}

// shared-worker.js
/* eslint-disable no-restricted-globals */

// === Configuration ===
const HEARTBEAT_INTERVAL_MS = 25_000;        // ping interval
const RECONNECT_MIN = 1000;
const RECONNECT_MAX = 15000;

// === State ===
let websocket = null;
let heartbeatTimer = null;
let reconnectTimer = null;
let reconnectDelay = RECONNECT_MIN;
const ports = new Set(); // connected tabs
let lastAuthToken = null; // optionally supplied by pages

// Util: Broadcast to all connected ports
function broadcast(msg) {
  for (const port of ports) {
    try {
      port.postMessage(msg);
    } catch (e) {
      // Drop failed ports
      ports.delete(port);
    }
  }
}

// Util: Exponential backoff (jitter)
function nextBackoff() {
  const jitter = Math.random() * 0.4 + 0.8; // 80%-120%
  const delay = Math.min(RECONNECT_MAX, reconnectDelay * 2) * jitter;
  reconnectDelay = Math.max(RECONNECT_MIN, Math.min(RECONNECT_MAX, delay));
  return Math.floor(reconnectDelay);
}

function resetBackoff() {
  reconnectDelay = RECONNECT_MIN;
}

// Heartbeat (app/server dependent: ping/pong or app-level ping)
function startHeartbeat() {
  stopHeartbeat();
  heartbeatTimer = setInterval(() => {
    try {
      if (websocket && websocket.readyState === WebSocket.OPEN) {
        websocket.send(JSON.stringify({ id: id, addr:0,type: "ping", t: Date.now() }));
      }
    } catch {}
  }, HEARTBEAT_INTERVAL_MS);
}

function stopHeartbeat() {
  if (heartbeatTimer) {
    clearInterval(heartbeatTimer);
    heartbeatTimer = null;
  }
}

// Connect WebSocket (optionally using token)
function connect() {
  clearTimeout(reconnectTimer);
  reconnectTimer = null;

  try {
    // const url = new URL(WS_URL);     

    // Example: attach auth via query or header-equivalent (if server supports subprotocols)
    if (lastAuthToken) {
      // Prefer headers on server using HTTP upgrade; in browsers you can’t set headers here.
      // Common workaround: put token in query string or use a short-lived signed URL.
      url.searchParams.set("token", lastAuthToken);
    }

    websocket = new WebSocket(wsURL(), /*protocols*/ []);

    websocket.addEventListener("open", () => {
      resetBackoff();
      startHeartbeat();
      broadcast({ type: "ws-status", status: "open" });
    });

    websocket.addEventListener("message", (event) => {
      let data = event.data;
      // If server sends text JSON:
      if (typeof data === "string") {
        try {
          data = JSON.parse(data);
          const {type} = data;
          switch(type){
            case 'init':{
              id = data?.id;
            }break;
            default:
              broadcast({ type: "server-message", payload: data });
              break;
          }
        } catch {
          // keep as string if not JSON
        }
      }
      
    });

    websocket.addEventListener("error", (err) => {
      broadcast({ type: "ws-status", status: "error", error: String(err?.message || "error"), data: "broadcasted error"});
    });

    websocket.addEventListener("close", (ev) => {
      stopHeartbeat();
      broadcast({ type: "ws-status", status: "closed", code: ev.code, reason: ev.reason });

      // Attempt reconnect if there are still listeners
      if (ports.size > 0) {
        const delay = nextBackoff();
        reconnectTimer = setTimeout(connect, delay);
        broadcast({ type: "ws-reconnect", inMs: delay });
      }
    });
  } catch (e) {
    // Schedule retry
    const delay = nextBackoff();
    reconnectTimer = setTimeout(connect, delay);
    broadcast({ type: "ws-status", status: "error", error: String(e) ,data: "in try-catch"});
  }
}

// Relay client message to server
function sendToServer(payload) {
  if (!websocket || websocket.readyState !== WebSocket.OPEN) {
    return false;
  }
  // Support sending objects or strings/ArrayBuffer
  if (payload instanceof ArrayBuffer || ArrayBuffer.isView(payload)) {
    websocket.send(payload);
  } else if (typeof payload === "string") {
    websocket.send(payload);
  } else {
    payload.id = id;
    websocket.send(JSON.stringify(payload));
  }
  return true;
}

// SharedWorker entry
onconnect = (e) => {
  const port = e.ports[0];
  ports.add(port);

  // Inform the new client about current status
  port.postMessage({
    type: "ws-status",
    status: websocket ? ["connecting","open","closing","closed"][websocket.readyState] : "disconnected",
    data: "connected"
  });

  // Open socket if none
  if (!websocket || websocket.readyState === WebSocket.CLOSED) {
    connect();
  }

  port.onmessage = (event) => {
    const msg = event.data;

    // Expected message formats:
    // { type: "auth", token: "..." }
    // { type: "client-message", payload: {...} }
    // { type: "broadcast", payload: {...} }  // local cross-tab broadcast only
    // { type: "control", action: "disconnect" | "connect" }

    switch (msg?.type) {
      case "auth":
        lastAuthToken = msg.token || null;
        // Optional: reconnect to apply new token
        if (websocket && websocket.readyState === WebSocket.OPEN) {
          websocket.close(4001, "refresh-auth");
        } else if (!websocket || websocket.readyState === WebSocket.CLOSED) {
          connect();
        }
        break;

      case "client-message":
        {
          sendToServer(msg.payload);
        }
        break;
      case "message-main":
        port.postMessage({type:"message-main",payload:msg.payload});
        break;
      case "broadcast":
        // Cross-tab only (doesn’t go to server)
        broadcast({ type: "tab-broadcast", from: msg.from || null, payload: msg.payload });
        break;

      case "control":
        if (msg.action === "disconnect" && websocket) {
          websocket.close(1000, "client-request");
        } else if (msg.action === "connect") {
          if (!websocket || websocket.readyState === WebSocket.CLOSED) connect();
        }
        break;

      default:
        // Ignore or log
        break;
    }
  };

  port.start();

  // Clean up when port is closed/gc’d
  port.onmessageerror = () => {
    ports.delete(port);
  };
};