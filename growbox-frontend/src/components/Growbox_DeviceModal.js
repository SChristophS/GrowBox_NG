import React, { useState } from "react";
import { Modal, Form, Button } from "react-bootstrap";
import PropTypes from "prop-types";

function DeviceModal({ show, onHide, device, onSendSettings }) {
    const [waterBeckenZustand, setWaterBeckenZustand] = useState(device ? device.state.wasserbeckenZustand : false);
    const [lightIntensity, setLightIntensity] = useState(device ? device.state.lightIntensity : 0);

    // Funktion zum Senden der Einstellungen
	const handleSendSettings = () => {
		if (!device) {
			console.error("[ERROR] Device is not defined");
			return;
		}

		const updates = [
			{
				uid: device.device_id,
				message_type: "control_command",
				target: "LightIntensity",
				action: "set",
				value: lightIntensity
			},
			{
				uid: device.device_id,
				message_type: "control_command",
				target: "wasserbeckenZustand",
				action: "set",
				value: waterBeckenZustand
			}
		];

		// Funktion zum Senden der Nachrichten mit Verzögerung
		const sendUpdatesWithDelay = (updates, delay) => {
			updates.forEach((update, index) => {
				setTimeout(() => {
					onSendSettings(update);
				}, index * delay); // Die Verzögerung wird mit jedem Update erhöht
			});
		};

		// Sende jede der Einstellungen mit einer Verzögerung von 500 ms
		sendUpdatesWithDelay(updates, 2000);

		// Schließe das Modal nach dem Senden
		onHide();
	};

    return (
        <Modal show={show} onHide={onHide}>
            <Modal.Header closeButton>
                <Modal.Title>Growbox Verbindung</Modal.Title>
            </Modal.Header>
            <Modal.Body>
                <Form>
                    <Form.Group controlId="formWaterBeckenZustand">
                        <Form.Check
                            type="checkbox"
                            label="Wasserbecken Zustand"
                            checked={waterBeckenZustand === 1}
                            onChange={(e) => setWaterBeckenZustand(e.target.checked ? 1 : 0)}
                        />
                    </Form.Group>
                    <Form.Group controlId="formLightIntensity">
                        <Form.Label>Lichtintensität</Form.Label>
                        <Form.Control
                            type="range"
                            min="0"
                            max="100"
                            value={lightIntensity}
                            onChange={(e) => setLightIntensity(parseInt(e.target.value, 10))}
                        />
                    </Form.Group>
                </Form>
            </Modal.Body>
            <Modal.Footer>
                <Button variant="secondary" onClick={onHide}>Abbrechen</Button>
                <Button variant="primary" onClick={handleSendSettings}>Einstellungen senden</Button>
            </Modal.Footer>
        </Modal>
    );
}

DeviceModal.propTypes = {
    show: PropTypes.bool.isRequired,
    onHide: PropTypes.func.isRequired,
    device: PropTypes.object,
    onSendSettings: PropTypes.func.isRequired,
};

export default DeviceModal;
