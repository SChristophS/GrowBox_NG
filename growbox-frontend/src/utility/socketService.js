class SocketService {
    constructor() {
        this.socket = null;
        this.connected = false;
    }

    connect(host, port, onMessage) {
        const url = `ws://${host}:${port}`;
        this.socket = new WebSocket(url);

        this.socket.onopen = () => {
            console.log(`[DEBUG] Connected to server at ${host}:${port}`);
            this.connected = true;
        };

        this.socket.onmessage = (event) => {
            console.log(`[DEBUG] Data received from server: ${event.data}`);
            if (onMessage) {
                onMessage(event); // Pass the event object instead of event.data
            }
        };

        this.socket.onclose = () => {
            console.log('[DEBUG] Connection to server closed');
            this.connected = false;
        };

        this.socket.onerror = (err) => {
            console.error('[DEBUG] Connection error: ', err);
            this.connected = false;
        };
    }

    sendMessage(message) {
        if (this.connected && this.socket) {
            console.log(`[DEBUG] Sending message to server: ${message}`);
            this.socket.send(message);
        } else {
            console.error('[DEBUG] Not connected to server, cannot send message');
        }
    }

    disconnect() {
        if (this.socket) {
            console.log('[DEBUG] Disconnecting from server');
            this.socket.close();
            this.connected = false;
        }
    }

    isConnected() {
        return this.connected;
    }
}

const socketService = new SocketService();
export default socketService;
