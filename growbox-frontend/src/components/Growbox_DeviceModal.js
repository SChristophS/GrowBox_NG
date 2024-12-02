import React, { useState, useEffect, forwardRef, useImperativeHandle } from "react";
import { Modal, Form, Button, Alert, Spinner, Row, Col, Card } from "react-bootstrap";
import PropTypes from "prop-types";

const DeviceModal = forwardRef(({ show, onHide, device, onSendSettings }, ref) => {
    const [settings, setSettings] = useState({});
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

        console.log(`[DEBUG] Sending command (${commandKey}):`, command);

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

    // Generische Funktion zum Senden von ControlCommands
    const handleSendControlCommand = (commandKey, target, action, value, additionalFields = {}) => {
        const command = {
            target_UUID: device.device_id,
            device: "frontend",
            message_type: "ControlCommand",
            payload: {
                commands: [
                    {
                        target: target,
                        action: action,
                        value: value,
                        ...additionalFields
                    }
                ]
            }
        };
        sendCommand(commandKey, command);
    };

    // Funktionen für spezifische Befehle
    const handleSendLightIntensity = () => {
        handleSendControlCommand("lightIntensity", "light", "setIntensity", settings.lightIntensity);
    };

    const handleSendAutomaticMode = () => {
        handleSendControlCommand("automaticMode", "system", "setAutomaticMode", settings.automaticMode);
    };

    const handleSendManualMode = () => {
        handleSendControlCommand("manualMode", "system", "setManualMode", settings.manualMode);
    };

    const handleSendPumpCommand = (pumpId, pumpState) => {
        const commandKey = `pump${pumpId}`;
        handleSendControlCommand(commandKey, "pump", "setState", pumpState, { deviceId: pumpId });
    };

    const handleSendWaterState = () => {
        const waterState = settings.waterBeckenZustand ? "full" : "empty";
        handleSendControlCommand("waterBeckenZustand", "water", "setState", waterState);
    };

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
                    {/* Lichtsteuerung */}
                    <Card className="mb-3">
                        <Card.Header>Lichtsteuerung</Card.Header>
                        <Card.Body>
                            <Form.Group as={Row} controlId="formLightIntensity" className="align-items-center">
                                <Form.Label column sm={4}>Lichtintensität</Form.Label>
                                <Col sm={4}>
                                    <Form.Control
                                        type="number"
                                        min="0"
                                        max="100"
                                        value={settings.lightIntensity}
                                        onChange={(e) => setSettings({ ...settings, lightIntensity: parseInt(e.target.value, 10) })}
                                    />
                                </Col>
                                <Col sm={4}>
                                    <Button variant="primary" onClick={handleSendLightIntensity} block>
                                        {loadingStates.lightIntensity ? <Spinner animation="border" size="sm" /> : "Senden"}
                                    </Button>
                                </Col>
                            </Form.Group>
                            {errorStates.lightIntensity && (
                                <Alert variant="warning" className="mt-2">
                                    {errorStates.lightIntensity}
                                </Alert>
                            )}
                        </Card.Body>
                    </Card>

                    {/* Systemsteuerung */}
                    <Card className="mb-3">
                        <Card.Header>Systemsteuerung</Card.Header>
                        <Card.Body>
                            <Form.Group as={Row} controlId="formManualMode" className="align-items-center">
                                <Col sm={8}>
                                    <Form.Check
                                        type="switch"
                                        label="Manueller Modus"
                                        checked={settings.manualMode}
                                        onChange={(e) => setSettings({ ...settings, manualMode: e.target.checked })}
                                    />
                                </Col>
                                <Col sm={4}>
                                    <Button variant="primary" onClick={handleSendManualMode} block>
                                        {loadingStates.manualMode ? <Spinner animation="border" size="sm" /> : "Senden"}
                                    </Button>
                                </Col>
                            </Form.Group>
                            {errorStates.manualMode && (
                                <Alert variant="warning" className="mt-2">
                                    {errorStates.manualMode}
                                </Alert>
                            )}
                        </Card.Body>
                    </Card>

                    {/* Pumpensteuerung */}
                    <Card className="mb-3">
                        <Card.Header>Pumpensteuerung</Card.Header>
                        <Card.Body>
                            <Form.Group as={Row} controlId="formPumpOben" className="align-items-center">
                                <Col sm={8}>
                                    <Form.Check
                                        type="switch"
                                        label="Pumpe Oben"
                                        checked={settings.pumpeOben}
                                        onChange={(e) => setSettings({ ...settings, pumpeOben: e.target.checked })}
                                    />
                                </Col>
                                <Col sm={4}>
                                    <Button variant="primary" onClick={() => handleSendPumpCommand(1, settings.pumpeOben)} block>
                                        {loadingStates.pump1 ? <Spinner animation="border" size="sm" /> : "Senden"}
                                    </Button>
                                </Col>
                            </Form.Group>
                            {errorStates.pump1 && (
                                <Alert variant="warning" className="mt-2">
                                    {errorStates.pump1}
                                </Alert>
                            )}

                            <Form.Group as={Row} controlId="formPumpUnten" className="align-items-center">
                                <Col sm={8}>
                                    <Form.Check
                                        type="switch"
                                        label="Pumpe Unten"
                                        checked={settings.pumpeUnten}
                                        onChange={(e) => setSettings({ ...settings, pumpeUnten: e.target.checked })}
                                    />
                                </Col>
                                <Col sm={4}>
                                    <Button variant="primary" onClick={() => handleSendPumpCommand(2, settings.pumpeUnten)} block>
                                        {loadingStates.pump2 ? <Spinner animation="border" size="sm" /> : "Senden"}
                                    </Button>
                                </Col>
                            </Form.Group>
                            {errorStates.pump2 && (
                                <Alert variant="warning" className="mt-2">
                                    {errorStates.pump2}
                                </Alert>
                            )}
                        </Card.Body>
                    </Card>

                    {/* Wassersteuerung */}
                    <Card className="mb-3">
                        <Card.Header>Wassersteuerung</Card.Header>
                        <Card.Body>
                            <Form.Group as={Row} controlId="formWaterBeckenZustand" className="align-items-center">
                                <Col sm={8}>
                                    <Form.Check
                                        type="switch"
                                        label="Wasserbecken Zustand (Voll)"
                                        checked={settings.waterBeckenZustand}
                                        onChange={(e) => setSettings({ ...settings, waterBeckenZustand: e.target.checked })}
                                    />
                                </Col>
                                <Col sm={4}>
                                    <Button variant="primary" onClick={handleSendWaterState} block>
                                        {loadingStates.waterBeckenZustand ? <Spinner animation="border" size="sm" /> : "Senden"}
                                    </Button>
                                </Col>
                            </Form.Group>
                            {errorStates.waterBeckenZustand && (
                                <Alert variant="warning" className="mt-2">
                                    {errorStates.waterBeckenZustand}
                                </Alert>
                            )}
                        </Card.Body>
                    </Card>

                    {/* Zeitsynchronisation und EEPROM */}
                    <Card className="mb-3">
                        <Card.Header>Systemfunktionen</Card.Header>
                        <Card.Body>
                            <Row className="align-items-center mb-2">
                                <Col sm={8}>
                                    <Form.Label>Zeitsynchronisation</Form.Label>
                                </Col>
                                <Col sm={4}>
                                    <Button variant="primary" onClick={handleSendTimeSync} block>
                                        {loadingStates.timeSync ? <Spinner animation="border" size="sm" /> : "Jetzt synchronisieren"}
                                    </Button>
                                </Col>
                            </Row>
                            {errorStates.timeSync && (
                                <Alert variant="warning" className="mt-2">
                                    {errorStates.timeSync}
                                </Alert>
                            )}

                            <Row className="align-items-center">
                                <Col sm={8}>
                                    <Form.Label>EEPROM löschen</Form.Label>
                                </Col>
                                <Col sm={4}>
                                    <Button variant="danger" onClick={handleEraseEEPROM} block>
                                        {loadingStates.eraseEEPROM ? <Spinner animation="border" size="sm" /> : "Löschen"}
                                    </Button>
                                </Col>
                            </Row>
                            {errorStates.eraseEEPROM && (
                                <Alert variant="warning" className="mt-2">
                                    {errorStates.eraseEEPROM}
                                </Alert>
                            )}
                        </Card.Body>
                    </Card>
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
};

export default DeviceModal;
