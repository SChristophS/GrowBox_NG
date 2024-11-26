import React, { useState, useEffect, forwardRef, useImperativeHandle } from "react";
import { Modal, Form, Button, Alert, Spinner, Row, Col } from "react-bootstrap";
import PropTypes from "prop-types";

const DeviceModal = forwardRef(({ show, onHide, device, onSendSettings, onSendSpecificCommand }, ref) => {
    const [settings, setSettings] = useState({
        waterBeckenZustand: device?.state.wasserbeckenZustand || false,
        lightIntensity: device?.state.lightIntensity || 0,
        automaticMode: device?.state.automaticMode || false,
        manualMode: device?.state.manualMode || false,
        pumpeOben: device?.state.pumpeOben || false,
        pumpeUnten: device?.state.pumpeUnten || false,
        // Weitere Einstellungen hier hinzufügen
    });

    const [loadingStates, setLoadingStates] = useState({});
    const [responseReceived, setResponseReceived] = useState({});
    const [errorStates, setErrorStates] = useState({});

    useEffect(() => {
        setSettings({
            waterBeckenZustand: device?.state.wasserbeckenZustand || false,
            lightIntensity: device?.state.lightIntensity || 0,
            automaticMode: device?.state.automaticMode || false,
            manualMode: device?.state.manualMode || false,
            pumpeOben: device?.state.pumpeOben || false,
            pumpeUnten: device?.state.pumpeUnten || false,
            // Weitere Einstellungen hier initialisieren
        });
    }, [device]);

    useImperativeHandle(ref, () => ({
        handleResponse(commandKey) {
            setResponseReceived(prev => ({ ...prev, [commandKey]: true }));
            setLoadingStates(prev => ({ ...prev, [commandKey]: false }));
            setErrorStates(prev => ({ ...prev, [commandKey]: null }));
        }
    }));

    const sendCommand = (commandKey, command) => {
        if (!device) {
            console.error("[ERROR] Device is not defined");
            return;
        }

        setLoadingStates(prev => ({ ...prev, [commandKey]: true }));
        setResponseReceived(prev => ({ ...prev, [commandKey]: false }));
        setErrorStates(prev => ({ ...prev, [commandKey]: null }));

        onSendSettings({ ...command, commandKey });

        // Set up a timeout to check if a response was received
        setTimeout(() => {
            if (!responseReceived[commandKey]) {
                setErrorStates(prev => ({ ...prev, [commandKey]: "Keine Antwort vom Controller erhalten." }));
                setLoadingStates(prev => ({ ...prev, [commandKey]: false }));
            }
        }, 5000); // Warte 5 Sekunden auf eine Antwort
    };

    // Beispiel für das Senden der Lichtintensität
    const handleSendLightIntensity = () => {
        const command = {
            target_UUID: device.device_id,
            device: "frontend",
            message_type: "ControlCommand",
            payload: {
                commands: [
                    {
                        target: "light",
                        action: "setIntensity",
                        value: settings.lightIntensity
                    }
                ]
            }
        };
        sendCommand("lightIntensity", command);
    };

    // Funktion zum Senden des Automatikmodus
    const handleSendAutomaticMode = () => {
        const command = {
            target_UUID: device.device_id,
            device: "frontend",
            message_type: "ControlCommand",
            payload: {
                commands: [
                    {
                        target: "system",
                        action: "setAutomaticMode",
                        value: settings.automaticMode
                    }
                ]
            }
        };
        sendCommand("automaticMode", command);
    };

    // Funktion zum Senden des manuellen Modus
    const handleSendManualMode = () => {
        const command = {
            target_UUID: device.device_id,
            device: "frontend",
            message_type: "ControlCommand",
            payload: {
                commands: [
                    {
                        target: "system",
                        action: "setManualMode",
                        value: settings.manualMode
                    }
                ]
            }
        };
        sendCommand("manualMode", command);
    };

    // Funktion zum Senden der Pumpenbefehle
    const handleSendPumpCommand = (pumpId, pumpState) => {
        const commandKey = `pump${pumpId}`;
        const command = {
            target_UUID: device.device_id,
            device: "frontend",
            message_type: "ControlCommand",
            payload: {
                commands: [
                    {
                        target: "pump",
                        action: "setState",
                        deviceId: pumpId,
                        value: pumpState
                    }
                ]
            }
        };
        sendCommand(commandKey, command);
    };

    // Funktion zum Senden der Wasserzustandsbefehle
    const handleSendWaterState = () => {
        const command = {
            target_UUID: device.device_id,
            device: "frontend",
            message_type: "ControlCommand",
            payload: {
                commands: [
                    {
                        target: "water",
                        action: "setState",
                        value: settings.waterBeckenZustand ? "full" : "empty"
                    }
                ]
            }
        };
        sendCommand("waterBeckenZustand", command);
    };

    // Funktion zum Senden der Zeitsynchronisation
    const handleSendTimeSync = () => {
        const currentTime = new Date().toISOString();
        const command = {
            target_UUID: device.device_id,
            device: "frontend",
            message_type: "TimeSync",
            payload: {
                current_time: currentTime
            }
        };
        sendCommand("timeSync", command);
    };

    // Funktion zum Löschen des EEPROMs
    const handleEraseEEPROM = () => {
        const command = {
            target_UUID: device.device_id,
            device: "frontend",
            message_type: "EraseEEPROM"
        };
        sendCommand("eraseEEPROM", command);
    };

    return (
        <Modal show={show} onHide={onHide} size="lg">
            <Modal.Header closeButton>
                <Modal.Title>Growbox Einstellungen</Modal.Title>
            </Modal.Header>
            <Modal.Body>
                <Form>
                    {/* Lichtintensität */}
                    <Form.Group controlId="formLightIntensity">
                        <Form.Label>Lichtintensität</Form.Label>
                        <Row>
                            <Col xs={8}>
                                <Form.Control
                                    type="number"
                                    min="0"
                                    max="100"
                                    value={settings.lightIntensity}
                                    onChange={(e) => setSettings({ ...settings, lightIntensity: parseInt(e.target.value, 10) })}
                                />
                            </Col>
                            <Col xs={4}>
                                <Button variant="primary" onClick={handleSendLightIntensity}>
                                    {loadingStates.lightIntensity ? <Spinner animation="border" size="sm" /> : "Senden"}
                                </Button>
                            </Col>
                        </Row>
                        {errorStates.lightIntensity && (
                            <Alert variant="warning" className="mt-2">
                                {errorStates.lightIntensity}
                            </Alert>
                        )}
                    </Form.Group>

                    {/* Automatikmodus */}
                    <Form.Group controlId="formAutomaticMode">
                        <Form.Check
                            type="checkbox"
                            label="Automatikmodus"
                            checked={settings.automaticMode}
                            onChange={(e) => setSettings({ ...settings, automaticMode: e.target.checked })}
                        />
                        <Button variant="primary" className="mt-2" onClick={handleSendAutomaticMode}>
                            {loadingStates.automaticMode ? <Spinner animation="border" size="sm" /> : "Senden"}
                        </Button>
                        {errorStates.automaticMode && (
                            <Alert variant="warning" className="mt-2">
                                {errorStates.automaticMode}
                            </Alert>
                        )}
                    </Form.Group>

                    {/* Manueller Modus */}
                    <Form.Group controlId="formManualMode">
                        <Form.Check
                            type="checkbox"
                            label="Manueller Modus"
                            checked={settings.manualMode}
                            onChange={(e) => setSettings({ ...settings, manualMode: e.target.checked })}
                        />
                        <Button variant="primary" className="mt-2" onClick={handleSendManualMode}>
                            {loadingStates.manualMode ? <Spinner animation="border" size="sm" /> : "Senden"}
                        </Button>
                        {errorStates.manualMode && (
                            <Alert variant="warning" className="mt-2">
                                {errorStates.manualMode}
                            </Alert>
                        )}
                    </Form.Group>

                    {/* Pumpensteuerung */}
                    <Form.Group controlId="formPumpControl">
                        <Form.Label>Pumpensteuerung</Form.Label>
                        {/* Pumpe Oben */}
                        <Row className="align-items-center">
                            <Col xs={6}>
                                <Form.Check
                                    type="checkbox"
                                    label="Pumpe Oben"
                                    checked={settings.pumpeOben}
                                    onChange={(e) => setSettings({ ...settings, pumpeOben: e.target.checked })}
                                />
                            </Col>
                            <Col xs={6}>
                                <Button variant="primary" onClick={() => handleSendPumpCommand(1, settings.pumpeOben)}>
                                    {loadingStates.pump1 ? <Spinner animation="border" size="sm" /> : "Senden"}
                                </Button>
                            </Col>
                        </Row>
                        {errorStates.pump1 && (
                            <Alert variant="warning" className="mt-2">
                                {errorStates.pump1}
                            </Alert>
                        )}
                        {/* Pumpe Unten */}
                        <Row className="align-items-center mt-2">
                            <Col xs={6}>
                                <Form.Check
                                    type="checkbox"
                                    label="Pumpe Unten"
                                    checked={settings.pumpeUnten}
                                    onChange={(e) => setSettings({ ...settings, pumpeUnten: e.target.checked })}
                                />
                            </Col>
                            <Col xs={6}>
                                <Button variant="primary" onClick={() => handleSendPumpCommand(2, settings.pumpeUnten)}>
                                    {loadingStates.pump2 ? <Spinner animation="border" size="sm" /> : "Senden"}
                                </Button>
                            </Col>
                        </Row>
                        {errorStates.pump2 && (
                            <Alert variant="warning" className="mt-2">
                                {errorStates.pump2}
                            </Alert>
                        )}
                    </Form.Group>

                    {/* Wasserzustand */}
                    <Form.Group controlId="formWaterBeckenZustand">
                        <Form.Check
                            type="checkbox"
                            label="Wasserbecken Zustand (Voll)"
                            checked={settings.waterBeckenZustand}
                            onChange={(e) => setSettings({ ...settings, waterBeckenZustand: e.target.checked })}
                        />
                        <Button variant="primary" className="mt-2" onClick={handleSendWaterState}>
                            {loadingStates.waterBeckenZustand ? <Spinner animation="border" size="sm" /> : "Senden"}
                        </Button>
                        {errorStates.waterBeckenZustand && (
                            <Alert variant="warning" className="mt-2">
                                {errorStates.waterBeckenZustand}
                            </Alert>
                        )}
                    </Form.Group>

                    {/* Zeitsynchronisation */}
                    <Form.Group controlId="formTimeSync">
                        <Form.Label>Zeitsynchronisation</Form.Label>
                        <Button variant="primary" className="ml-2" onClick={handleSendTimeSync}>
                            {loadingStates.timeSync ? <Spinner animation="border" size="sm" /> : "Zeit synchronisieren"}
                        </Button>
                        {errorStates.timeSync && (
                            <Alert variant="warning" className="mt-2">
                                {errorStates.timeSync}
                            </Alert>
                        )}
                    </Form.Group>

                    {/* EEPROM löschen */}
                    <Button variant="warning" className="mt-3" onClick={handleEraseEEPROM}>
                        {loadingStates.eraseEEPROM ? <Spinner animation="border" size="sm" /> : "EEPROM löschen"}
                    </Button>
                    {errorStates.eraseEEPROM && (
                        <Alert variant="warning" className="mt-2">
                            {errorStates.eraseEEPROM}
                        </Alert>
                    )}
                </Form>
            </Modal.Body>
            <Modal.Footer>
                <Button variant="secondary" onClick={onHide}>Schließen</Button>
            </Modal.Footer>
        </Modal>
    );
});

DeviceModal.propTypes = {
    show: PropTypes.bool.isRequired,
    onHide: PropTypes.func.isRequired,
    device: PropTypes.object,
    onSendSettings: PropTypes.func.isRequired,
    onSendSpecificCommand: PropTypes.func.isRequired,
};

export default DeviceModal;
