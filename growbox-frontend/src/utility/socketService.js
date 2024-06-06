const net = require('net');

class SocketService {
  constructor() {
    this.client = null;
    this.isConnected = false;
  }

  connect(host, port, onMessage) {
    this.client = new net.Socket();

    this.client.connect(port, host, () => {
      console.log(`[DEBUG] Connected to server at ${host}:${port}`);
      this.isConnected = true;
    });

    this.client.on('data', (data) => {
      console.log(`[DEBUG] Data received from server: ${data}`);
      if (onMessage) {
        onMessage(data.toString());
      }
    });

    this.client.on('close', () => {
      console.log('[DEBUG] Connection to server closed');
      this.isConnected = false;
    });

    this.client.on('error', (err) => {
      console.error('[DEBUG] Connection error: ', err);
      this.isConnected = false;
    });
  }

  sendMessage(message) {
    if (this.isConnected && this.client) {
      console.log(`[DEBUG] Sending message to server: ${message}`);
      this.client.write(message);
    } else {
      console.error('[DEBUG] Not connected to server, cannot send message');
    }
  }

  disconnect() {
    if (this.client) {
      console.log('[DEBUG] Disconnecting from server');
      this.client.destroy();
      this.isConnected = false;
    }
  }
}

module.exports = new SocketService();
