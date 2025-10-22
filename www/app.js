const statusEl = document.getElementById('status');

const worker = new SharedWorker("worker.js");
worker.port.start();

worker.port.onmessage = function(event) {
  const { type,status, data } = event.data;
  if (type === "message") {
    statusEl.textContent = data;
  } else if (type === "error") {
    console.error("WebSocket error:", data);
  }else if(type === "ws-status"){
    if(status === "open"){
      statusEl.textContent = "connected";
      console.log("opening now");
    }else if(status === "disconnected"){
      statusEl.textContent = "disconnected";
      console.log("socket disconnected");
    }else{
      statusEl.textContent = "closed";
    }
    
  }else if(type === "ws-reconnect"){
    console.log("attempting to reconnect");
  }
};

function sendMessage() {
  const input = document.getElementById("messageInput");
  const message = input.value;
  worker.port.postMessage({ type: "send", data: message });
  input.value = "";
}