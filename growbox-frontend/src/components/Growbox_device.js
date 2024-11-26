import React from "react";
import { Button } from "react-bootstrap";
import PropTypes from "prop-types";

function Device({ device, onClick, onRunClick, onStopClick }) {
    const { state } = device;

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
            <p>Water Level: {state?.wasserbeckenZustand !== null ? (state.wasserbeckenZustand ? "Full" : "Empty") : "Loading..."}</p>
            <p>Light Intensity: {state?.lightIntensity !== null ? state.lightIntensity : "Loading..."}</p>
            <p>Pumpe Oben: {state?.pumpeOben !== null ? (state.pumpeOben ? "On" : "Off") : "Loading..."}</p>
            <p>Pumpe Unten: {state?.pumpeUnten !== null ? (state.pumpeUnten ? "On" : "Off") : "Loading..."}</p>
            <p>Sensor Oben: {state?.sensorOben !== null ? (state.sensorOben ? "Activated" : "Deactivated") : "Loading..."}</p>
            <p>Sensor Unten: {state?.sensorUnten !== null ? (state.sensorUnten ? "Activated" : "Deactivated") : "Loading..."}</p>
            <p>Automatic Mode: {state?.automaticMode !== null ? (state.automaticMode ? "Enabled" : "Disabled") : "Loading..."}</p>
            <p>Manual Mode: {state?.manualMode !== null ? (state.manualMode ? "Enabled" : "Disabled") : "Loading..."}</p>
            <p>Grow Cycle Config: {state?.growCycleConfig ? "Loaded" : "Not Loaded"}</p>

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
