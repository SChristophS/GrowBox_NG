import React, { useState, useEffect, useReducer, useRef } from "react";
import GrowPlanServices from '../utility/GrowPlanServices';
import Device from "./Growbox_device";
import DeviceModal from "./Growbox_DeviceModal";
import GrowPlanModal from "./Growbox_GrowPlanModal";
import socketService from "../utility/socketService";
import './Growbox.css';

const initialDeviceState = {
    wasserbeckenZustand: false,
    lightIntensity: 0,
    readyForAutoRun: false,
    pumpeZulauf: false,
    pumpeAblauf: false,
    sensorVoll: false,
    sensorLeer: false,
};

const deviceReducer = (state, action) => {
    switch (action.type) {
        case 'SET_DEVICES':
            console.log("[DEBUG] SET_DEVICES action received. Payload:", action.payload);
            return action.payload;

        case 'UPDATE_DEVICES_STATUS':
            console.log("[DEBUG] UPDATE_DEVICES_STATUS action received. Payload:", action.payload);
            return state.map(device => {
                if (action.payload.hasOwnProperty(device.device_id)) {
                    console.log(`[DEBUG] Updating device ${device.device_id} status. Connected: true, Controller Alive: ${action.payload[device.device_id].length > 0}`);
                    return {
                        ...device,
                        isConnected: true,
                        controllerAlive: action.payload[device.device_id].length > 0
                    };
                }
                console.log(`[DEBUG] No status update for device ${device.device_id}`);
                return device;
            });

        case 'UPDATE_DEVICE_STATE':
            console.log("[DEBUG] UPDATE_DEVICE_STATE action received. UID:", action.payload.UID, "Target:", action.payload.target, "Value:", action.payload.value);
            return state.map(device => {
                if (device.device_id === action.payload.UID) {
                    console.log(`[DEBUG] Updating device ${device.device_id}. Current state:`, device.state);
                    const updatedDevice = {
                        ...device,
                        state: {
                            ...device.state,
                            [action.payload.target]: action.payload.value
                        }
                    };
                    console.log(`[DEBUG] Updated device ${device.device_id}. New state:`, updatedDevice.state);
                    return updatedDevice;
                }
                return device;
            });

        default:
            console.log("[DEBUG] Unknown action type:", action.type);
            return state;
    }
};

function Growbox() {
    const [devices, dispatch] = useReducer(deviceReducer, []);
    const devicesRef = useRef(devices);
    const [showModal, setShowModal] = useState(false);
    const [selectedDeviceId, setSelectedDeviceId] = useState("");
    const [showGrowPlansModal, setShowGrowPlansModal] = useState(false);
    const [selectedGrowPlan, setSelectedGrowPlan] = useState(null);
    const [startFromHere, setStartFromHere] = useState(0);
    const [loadingGrowPlans, setLoadingGrowPlans] = useState(false);
    const [error, setError] = useState("");
    const [growPlans, setGrowPlans] = useState([]);
    const [selectedDevice, setSelectedDevice] = useState(null);

    useEffect(() => {
        devicesRef.current = devices;
    }, [devices]);

    useEffect(() => {
        fetchDevices();
        const socket = socketService.connect("192.168.178.25", 8085, handleWebSocketMessage);

        return () => {
            socketService.disconnect();
        };
    }, []);

    const handleWebSocketMessage = (event) => {
        if (!event || !event.data) {
            console.error("[ERROR] Received event or event.data is undefined");
            return;
        }

        let data;
        try {
            data = JSON.parse(event.data);
        } catch (error) {
            console.error("[ERROR] Error parsing message data:", error);
            return;
        }

        console.log("[DEBUG] Received data:", data);

        if (data && typeof data === 'object' && data.message_type) {
            const { message_type, UID, target, action, value, controllers } = data;

            switch (message_type) {
                case "register_confirmed":
                    console.log("[DEBUG] Registration confirmed", controllers);

                    dispatch({
                        type: 'UPDATE_DEVICES_STATUS',
                        payload: controllers,
                    });
                    break;

                case "controller_update":
                    if (UID && devicesRef.current.length > 0) {
                        console.log("[DEBUG] Processing controller update for UID:", UID);

                        const device = devicesRef.current.find(device => device.device_id === UID);

                        if (!device) {
                            console.error(`[ERROR] Device with UID ${UID} not found.`);
                            console.log("[DEBUG] Full devices list:", devicesRef.current);
                            return;
                        }

                        dispatch({
                            type: 'UPDATE_DEVICE_STATE',
                            payload: {
                                UID: UID,
                                target: target,
                                value: value
                            }
                        });
                        console.log(`[DEBUG] Updated device ${UID} with new state.`);
                    } else {
                        console.error("[ERROR] UID is missing in controller_update message or devices list is empty.");
                    }
                    break;

                default:
                    console.error("[ERROR] Invalid message format or unsupported message type:", data);
                    break;
            }
        } else {
            console.error("[ERROR] Invalid message format:", data);
        }
    };

    const fetchDevices = async () => {
        console.log("fetchDevices: Starting fetchDevices");

        const username = "christoph";
        try {
            const response = await fetch(`http://localhost:5000/devices?username=${username}`);
            if (response.ok) {
                const data = await response.json();
                console.log("fetchDevices: data.devices :", data.devices);

                const newDevices = data.devices.map(device => ({
                    ...device,
                    isConnected: false,
                    controllerAlive: false,
                    state: { ...initialDeviceState }
                }));

                dispatch({
                    type: 'SET_DEVICES',
                    payload: newDevices
                });

                console.log("[DEBUG] Devices dispatched to state:", newDevices);

                const chipIds = newDevices.map(device => device.device_id);
                const registerMessage = JSON.stringify({
                    message_type: "register",
                    device: "Frontend",
                    chipIds: chipIds
                });
                console.log("fetchDevices: sende register message:", registerMessage);
                socketService.sendMessage(registerMessage);
            } else {
                console.error("fetchDevices: HTTP-Error: " + response.status);
            }
        } catch (error) {
            console.error("fetchDevices: Fehler beim Abrufen der Geräte", error);
        }
    };

    const sendCommandToGrowbox = async (deviceID, action) => {
        console.log("sendCommandToGrowbox: function call");
        console.log("action: " + action);

        if (socketService.isConnected() && socketService.socket.readyState === WebSocket.OPEN) {
            const message = JSON.stringify({
                device: "Frontend",
                chipId: deviceID,
                action: action
            });

            socketService.sendMessage(message);
            console.log("sendCommandToGrowbox: Nachricht gesendet:", message);
        } else {
            console.error("WebSocket-Verbindung ist nicht offen.");
        }
    };

    const sendWebSocketMessage = (message) => {
        if (socketService.isConnected() && socketService.socket.readyState === WebSocket.OPEN) {
            socketService.sendMessage(JSON.stringify(message));
        } else {
            console.error("WebSocket-Verbindung ist nicht offen.");
            socketService.connect("192.168.178.25", 8085, handleWebSocketMessage);
            setTimeout(() => {
                if (socketService.isConnected() && socketService.socket.readyState === WebSocket.OPEN) {
                    socketService.sendMessage(JSON.stringify(message));
                } else {
                    console.error("WebSocket-Verbindung konnte nicht hergestellt werden.");
                }
            }, 1000);
        }
    };

    const fetchGrowPlans = async () => {
        setLoadingGrowPlans(true);
        setError("");
        const username = "christoph";

        try {
            const response = await GrowPlanServices.getGrowPlans(username);
            setGrowPlans(response.data);
            console.log("fetchGrowPlans: response.data: ", response.data);
        } catch (error) {
            setError("Fehler beim Abrufen der Growpläne");
            console.error("Fehler:", error);
        } finally {
            setLoadingGrowPlans(false);
            setShowGrowPlansModal(true);
        }
    };

    const sendGrowPlanToGrowbox = async (selectedGrowPlan) => {
        console.log("sendGrowPlanToGrowbox: function call");
        console.log(selectedGrowPlan);
        if (!selectedGrowPlan || !selectedGrowPlan.droppedItems) {
            console.error('Selected grow plan is not set correctly.');
            return;
        }

        let promises = selectedGrowPlan.droppedItems.map(item =>
            GrowPlanServices.getCyclePlanFromID(item.id.split('-')[0])
                .then(response => JSON.parse(response.data))
                .catch(error => console.error("Fehler beim Laden des GrowCycles", error))
        );

        Promise.all(promises).then(results => {
            const growData = {
                totalGrowTime: 0,
                startFromHere: parseInt(startFromHere, 10),
                ledCycles: [],
                tempCycles: [],
                wateringCycles: []
            };

            results.forEach(result => {
                if (result && result.growData) {
                    growData.ledCycles.push(...result.growData.ledCycles);
                    growData.tempCycles.push(...result.growData.tempCycles);
                    growData.wateringCycles.push(...result.growData.wateringCycles);
                }
            });

            const finalData = {
                _id: selectedGrowPlan._id,
                username: "christoph",
                growCycleName: selectedGrowPlan.growPlanName,
                description: selectedGrowPlan.description,
                sharingStatus: selectedGrowPlan.sharingStatus,
                growData: growData
            };

            if (socketService.isConnected() && socketService.socket.readyState === WebSocket.OPEN) {
                const message = JSON.stringify({
                    device: "Frontend",
                    chipId: selectedDeviceId,
                    action: "send_growplan",
                    message: finalData
                });

                socketService.sendMessage(message);
                console.log("Growplan-Daten gesendet:", message);
            } else {
                console.error("WebSocket-Verbindung ist nicht offen.");
            }
        })
            .catch(error => console.error("Fehler bei der Verarbeitung der GrowCycle-Daten", error));
    };

    const handleDeviceClick = (deviceId) => {
        const selectedDevice = devices.find(device => device.device_id === deviceId);

        if (!selectedDevice) {
            console.error(`[ERROR] Device with ID ${deviceId} not found.`);
            return;
        }

        setSelectedDevice(selectedDevice);
        setShowModal(true);
    };

    return (
        <div>
            <h1>Growboxen</h1>

            <div className="devices-container">
                {devices.map((device, index) => (
                    <Device
                        key={index}
                        device={device}
                        onClick={() => handleDeviceClick(device.device_id)} // Handle the click and pass the device_id
                        onRunClick={() => sendCommandToGrowbox(device.device_id, "startGrow")} // Specify action for the run button
                        onStopClick={() => sendCommandToGrowbox(device.device_id, "stopGrow")} // Specify action for the stop button
                    />
                ))}
            </div>

            {selectedDevice && ( // Conditionally render the DeviceModal if a device is selected
                <DeviceModal
                    show={showModal}
                    onHide={() => setShowModal(false)}
                    device={selectedDevice}
                    onSendSettings={(update) => sendWebSocketMessage(update)}
                />
            )}

            <GrowPlanModal
                show={showGrowPlansModal}
                onHide={() => setShowGrowPlansModal(false)}
                growPlans={growPlans}
                selectedGrowPlan={selectedGrowPlan}
                setSelectedGrowPlan={setSelectedGrowPlan}
                fetchGrowPlans={fetchGrowPlans}
                loadingGrowPlans={loadingGrowPlans}
                error={error}
                onSendGrowPlan={() => sendGrowPlanToGrowbox(selectedGrowPlan)}
            />
        </div>
    );
}

export default Growbox;
