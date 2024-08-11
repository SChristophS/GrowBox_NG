import React from "react";
import { Button } from "react-bootstrap";
import PropTypes from "prop-types";


function Device({ device, onClick, onRunClick, onStopClick }) {
    console.log("[DEBUG] Rendering device:", device.device_id, "State:", device.state, "Connected:", device.isConnected, "Controller Alive:", device.controllerAlive);

    return (
        <div
            className={`device ${device.isConnected ? 'connected' : 'disconnected'}`}
            onClick={onClick}
        >
            <p>Device Chip-ID: {device.device_id}</p>
            <p>Frontend to Socket
                <span className={`status-dot ${device.isConnected ? 'connected' : 'disconnected'}`}></span>
            </p>
            <p>Controller to Socket
                <span className={`status-dot ${device.controllerAlive ? 'connected' : 'disconnected'}`}></span>
            </p>
            <p>Growprogram is running
                <span className={`status-dot ${device.controllerRunning ? 'running' : 'notRunning'}`}></span>
            </p>
            <p>Water Level: {device.state.wasserbeckenZustand ? "Full" : "Empty"}</p>
            <p>Light Intensity: {device.state.lightIntensity}</p>
            <p>Ready for AutoRun: {device.state.readyForAutoRun ? "Yes" : "No"}</p>
            <p>Pumpe Zulauf: {device.state.pumpeZulauf ? "On" : "Off"}</p>
            <p>Pumpe Ablauf: {device.state.pumpeAblauf ? "On" : "Off"}</p>
            <p>Sensor Voll: {device.state.sensorVoll ? "Activated" : "Deactivated"}</p>
            <p>Sensor Leer: {device.state.sensorLeer ? "Activated" : "Deactivated"}</p>

            <div className="button-container">
                <Button variant="success" onClick={(event) => { event.stopPropagation(); onRunClick(device.device_id, "startGrow"); }}>Run</Button>
                <Button variant="danger" onClick={(event) => { event.stopPropagation(); onStopClick(device.device_id, "stopGrow"); }}>Stop</Button>
            </div>
        </div>
    );
}


export default Device;
