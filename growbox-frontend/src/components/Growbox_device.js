import React from "react";
import { Button } from "react-bootstrap";
import PropTypes from "prop-types";

function Device({ device, onClick, onRunClick, onStopClick }) {
    const { state } = device;
	
	console.log(device);

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
                <span className={`status-dot ${state?.isRunning ? 'running' : 'notRunning'}`}></span>
            </p>
            <p>Soll-Wasserlevel: {state?.wasserbeckenZustand !== null ? (state.wasserbeckenZustand ? "Voll" : "Leer") : "Loading..."}</p>
			<p>pumpeZulauf: {state?.pumpeZulauf !== null ? (state.pumpeZulauf ? "On" : "Off") : "Loading..."}</p>
            <p>pumpeAblauf: {state?.pumpeAblauf !== null ? (state.pumpeAblauf ? "On" : "Off") : "Loading..."}</p>
            <p>Sensor Oben: {state?.sensorOben !== null ? (state.sensorOben ? "Activated" : "Deactivated") : "Loading..."}</p>
            <p>Sensor Unten: {state?.sensorUnten !== null ? (state.sensorUnten ? "Activated" : "Deactivated") : "Loading..."}</p>           
			<p>Light Intensity: {state?.lightIntensity !== null ? state.lightIntensity : "Loading..."}</p>
            <p>Manual Mode: {state?.manualMode !== null ? (state.manualMode ? "Enabled" : "Disabled") : "Loading..."}</p>

            <div className="button-container">
                <Button variant="success" onClick={(event) => { event.stopPropagation(); onRunClick(device.device_id); }}>Run</Button>
                <Button variant="danger" onClick={(event) => { event.stopPropagation(); onStopClick(device.device_id); }}>Stop</Button>
            </div>
        </div>
    );
}

Device.propTypes = {
    device: PropTypes.object.isRequired,
    onClick: PropTypes.func.isRequired,
    onRunClick: PropTypes.func.isRequired,
    onStopClick: PropTypes.func.isRequired,
};

export default Device;
