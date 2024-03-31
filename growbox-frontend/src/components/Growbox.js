import React, { useState, useEffect } from "react";
import { Button, Modal } from "react-bootstrap";
import './Growbox.css';

function Growbox() {
    const [devices, setDevices] = useState([]);
    const [showModal, setShowModal] = useState(false);
    const [selectedDeviceId, setSelectedDeviceId] = useState("");
    const [websocket, setWebsocket] = useState(null);
    const [messages, setMessages] = useState([]);
	
	

    const fetchDevices = async () => {
		console.log("fetchDevices");
        const username = "christoph"; // Setze den aktuellen Benutzernamen
        const response = await fetch(`http://localhost:5000/devices?username=${username}`);

		console.log("response:", response);
        if (response.ok) {
			console.log("response ok");
            const data = await response.json();
			console.log("data.devices:");
			console.log(data.devices);
            setDevices(data.devices);
        } else {
            console.error("HTTP-Error: " + response.status);
        }
    };
	
const sendMessageOverWebSocket = (actionContent, messageContent) => {
    if (websocket && websocket.readyState === WebSocket.OPEN) {
        const message = JSON.stringify({
            device: "Frontend",
            chipId: selectedDeviceId,
            action: actionContent, // Parameter verwendet
            message: messageContent    // Parameter verwendet
        });
        websocket.send(message);
        console.log("Nachricht gesendet:", message);
    }
};

	
	const sendRegisterMsgToWebSocket = () => {
		console.log("sendRegisterMsgToWebSocket:");
		  if (websocket && websocket.readyState === WebSocket.OPEN) {
				const register_message = JSON.stringify({
					device: "Frontend",
					chipId: selectedDeviceId,
					message: "register",
				});
				websocket.send(register_message);
				console.log("Nachricht gesendet:", register_message);
		  }
	};


const initiateWebSocketConnection = (wsUrl) => {
    const ws = new WebSocket('ws://localhost:8085');
    ws.onopen = () => {
		console.log("WebSocket connection established");
		sendRegisterMsgToWebSocket();
	}
	
	ws.onmessage = (message) => {
		console.log("Empfangene Nachricht: ", message.data);
		setMessages(prevMessages => {
			const updatedMessages = [...prevMessages, message.data];
			console.log("Aktualisierte Nachrichten: ", updatedMessages);
			return updatedMessages;
		});
	};
    setWebsocket(ws);
	
	
	
};

	useEffect(() => {
		fetchDevices();

		// Erstelle ein Intervall, um eine Alive-Nachricht zu senden
		const intervalId = setInterval(() => {
			if (websocket && websocket.readyState === WebSocket.OPEN) {
				const aliveMessage = JSON.stringify({
					device: "Frontend",
					chipId:  selectedDeviceId,
					message: "alive",
				});
				websocket.send(aliveMessage);
				console.log("Alive-Nachricht gesendet:", aliveMessage);
			}
		}, 10000); // Sende alle 10 Sekunden

		// Bereinige das Intervall, wenn die Komponente demontiert wird
		return () => clearInterval(intervalId);
	}, [websocket]); // Führe diesen Effekt aus, wenn sich `websocket` ändert

    const handleDeviceClick = (deviceId) => {
        setSelectedDeviceId(deviceId);
        setShowModal(true);
    };

    const connectToDevice = () => {
        const requestData = { device_id: selectedDeviceId };
        fetch(`http://localhost:5000/ask-growbox-to-socket-connect`, {
            method: "POST",
            headers: {
                'Content-Type': 'application/json',
            },
            body: JSON.stringify(requestData),
        })
        .then(response => response.json())
		.then(data => {
			console.log(data.message);
			const wsUrl = 'ws://localhost:8085';
			initiateWebSocketConnection(wsUrl);

			setShowModal(false); // Schließt das Modal nach dem Senden
		})
        .catch(error => {
            console.error('Error:', error);
        });
    };


    return (
        <div>
            <h1>Verbundene Growboxen</h1>
            <div style={{ display: "flex", flexWrap: "wrap" }}>
                {devices.map((device, index) => (
                    <div key={index} 
                         className="device" 
                         onClick={() => { setSelectedDeviceId(device.device_id); setShowModal(true); }}>
                        <p>Device ID: {device.device_id}</p>
                        <p>Status: {device.status}</p>
                    </div>
                ))}
            </div>

            <Modal show={showModal} onHide={() => setShowModal(false)}>
                <Modal.Header closeButton>
                    <Modal.Title>Growbox Verbindung</Modal.Title>
                </Modal.Header>
                <Modal.Body>Möchten Sie eine Socketverbindung mit der Growbox {selectedDeviceId} aufbauen?</Modal.Body>
                <Modal.Footer>
                    <Button variant="secondary" onClick={() => setShowModal(false)}>Abbrechen</Button>
                    <Button variant="primary" onClick={() => connectToDevice(selectedDeviceId)}>Verbinden</Button>
                </Modal.Footer>
            </Modal>
			
			<Button variant="primary" onClick={() => sendMessageOverWebSocket("live", "activate")}>
				Live-Data Aktivieren
			</Button>
			
			<Button variant="primary" onClick={() => sendMessageOverWebSocket("live", "deactivate")}>
				Live-Data deaktivieren
			</Button>			
			
			<Button variant="primary" onClick={() => sendMessageOverWebSocket("new_growplan", "HierderNeueGrowplandannalsjson")}>
				New Growplan
			</Button>
			
			<Button variant="primary" onClick={() => sendMessageOverWebSocket("control", "becken_voll")}>
				Becken voll
			</Button>
			
			<Button variant="primary" onClick={() => sendMessageOverWebSocket("control", "becken_leer")}>
				Becken leer
			</Button>
			
			
			{/* Nachrichtenanzeige */}
			<div className="messages-display">
				<h2>Empfangene Nachrichten</h2>
				<div className="messages-list">
					{messages.map((msg, index) => {
						console.log("Nachricht rendern:: ", msg);
						return (<div key={index} className="message">{msg}</div>);
					})}
				</div>
			</div>
			


			
        </div>
    );
}

export default Growbox;
