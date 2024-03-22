import React, { useState, useEffect } from "react";
import { Button, Modal } from "react-bootstrap";

function Growbox() {
    const [devices, setDevices] = useState([]);
    const [showModal, setShowModal] = useState(false);
    const [selectedDeviceId, setSelectedDeviceId] = useState("");

    const fetchDevices = async () => {
        const username = "christoph"; // Setze den aktuellen Benutzernamen
        const response = await fetch(`http://localhost:5000/devices?username=${username}`);

        if (response.ok) {
            const data = await response.json();
            setDevices(data.devices);
        } else {
            console.error("HTTP-Error: " + response.status);
        }
    };

    useEffect(() => {
        fetchDevices();
    }, []);

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
                    <div key={index} onClick={() => handleDeviceClick(device.device_id)} style={{ width: "150px", margin: "10px", padding: "10px", border: "1px solid black", cursor: "pointer" }}>
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
                    <Button variant="primary" onClick={connectToDevice}>Verbinden</Button>
                </Modal.Footer>
            </Modal>
        </div>
    );
}

export default Growbox;
