import React, { useState, useEffect, useReducer, useRef, useCallback } from "react";
import GrowPlanServices from '../utility/GrowPlanServices';
import Device from "./Growbox_device";
import DeviceModal from "./Growbox_DeviceModal";
import GrowPlanModal from "./Growbox_GrowPlanModal";
import socketService from "../utility/socketService";
import './Growbox.css';

const API_BASE_URL = process.env.REACT_APP_API_BASE_URL || 'http://localhost:5000';
const WEBSOCKET_HOST = process.env.REACT_APP_WEBSOCKET_HOST || '192.168.178.25';
const WEBSOCKET_PORT = process.env.REACT_APP_WEBSOCKET_PORT || 8085;
const DEFAULT_USERNAME = process.env.REACT_APP_USERNAME || 'christoph';

const initialDeviceState = {
    wasserbeckenZustand: null,
    lightIntensity: null,
    readyForAutoRun: null,
    pumpeOben: null,
    pumpeUnten: null,
    sensorOben: null,
    sensorUnten: null,
    automaticMode: null,
    manualMode: null,
    growCycleConfig: null,
    controllerState: null,
};

const deviceReducer = (state, action) => {
    switch (action.type) {
        case 'SET_DEVICES':
            return action.payload;

        case 'UPDATE_DEVICES_STATUS':
            return state.map(device => {
                if (action.payload.hasOwnProperty(device.device_id)) {
                    return {
                        ...device,
                        isConnected: true,
                        controllerAlive: action.payload[device.device_id].length > 0
                    };
                }
                return device;
            });

        case 'UPDATE_DEVICE_STATE':
            return state.map(device => {
                if (device.device_id === action.payload.UID) {
                    return {
                        ...device,
                        state: {
                            ...device.state,
                            [action.payload.target]: action.payload.value
                        }
                    };
                }
                return device;
            });

        default:
            console.error("Unknown action type:", action.type);
            return state;
    }
};

function Growbox() {
    // Gerätezustand
    const [devices, dispatch] = useReducer(deviceReducer, []);
    const devicesRef = useRef(devices);

    // Modale Zustände
    const [showDeviceModal, setShowDeviceModal] = useState(false);
    const [selectedDevice, setSelectedDevice] = useState(null);

    const [showGrowPlansModal, setShowGrowPlansModal] = useState(false);
    const [growPlans, setGrowPlans] = useState([]);
    const [selectedGrowPlan, setSelectedGrowPlan] = useState(null);

    // Andere Zustände
    const [startFromHere, setStartFromHere] = useState(0);
    const [loadingGrowPlans, setLoadingGrowPlans] = useState(false);
    const [error, setError] = useState("");

    // Referenz auf DeviceModal für Antwortverarbeitung
    const deviceModalRef = useRef(null);

    // Aktualisieren der devicesRef für den aktuellen Zustand
    useEffect(() => {
        devicesRef.current = devices;
    }, [devices]);

    // WebSocket-Verbindung und Geräteabruf
    useEffect(() => {
        fetchDevices();
        const socket = socketService.connect(WEBSOCKET_HOST, WEBSOCKET_PORT, handleWebSocketMessage);

        return () => {
            socketService.disconnect();
        };
    }, []);

    const handleWebSocketMessage = (event) => {
        if (!event?.data) {
            console.error("Received event or event.data is undefined");
            return;
        }

        let data;
        try {
            data = JSON.parse(event.data);
        } catch (error) {
            console.error("Error parsing message data:", error);
            return;
        }

		if (data && typeof data === 'object' && data.message_type) {
			const { message_type, UID, chipID, target, value, controllers, payload, commandKey, changedValue } = data;

            switch (message_type) {
                case "register_confirmed":
                    dispatch({
                        type: 'UPDATE_DEVICES_STATUS',
                        payload: controllers,
                    });
                    // Nach erfolgreicher Registrierung Anfragen an den Controller senden
                    devicesRef.current.forEach(device => {
                        sendControllerRequests(device.device_id);
                    });
                    break;
				
				case "status_update":
					const deviceIdStatus = UID || chipID;
					if (deviceIdStatus && devicesRef.current.length > 0) {
						const device = devicesRef.current.find(device => device.device_id === deviceIdStatus);

						if (!device) {
							console.error(`Device with UID ${deviceIdStatus} not found.`);
							return;
						}

						dispatch({
							type: 'UPDATE_DEVICE_STATE',
							payload: {
								UID: deviceIdStatus,
								target: changedValue,
								value: value
							}
						});
					} else {
						console.error("UID or chipID is missing in status_update message or devices list is empty.");
					}
					break;

                case "controller_update":
                    if (UID && devicesRef.current.length > 0) {
                        const device = devicesRef.current.find(device => device.device_id === UID);

                        if (!device) {
                            console.error(`Device with UID ${UID} not found.`);
                            return;
                        }

                        dispatch({
                            type: 'UPDATE_DEVICE_STATE',
                            payload: {
                                UID,
                                target,
                                value
                            }
                        });
                    } else {
                        console.error("UID is missing in controller_update message or devices list is empty.");
                    }
                    break;

                case "ControllerStateResponse":
                case "AutomaticModeResponse":
                case "GrowCycleConfigResponse":
                    // Verarbeiten der Antworten vom Controller
                    handleControllerResponse(data);
                    break;

                case "ControlCommandResponse":
                    // Benachrichtigen, dass eine Antwort eingegangen ist
                    if (deviceModalRef.current) {
                        deviceModalRef.current.handleResponse(commandKey);
                    }
                    break;

				default:
					console.error("Invalid message format or unsupported message type:", data);
					break;
            }
        } else {
            console.error("Invalid message format:", data);
        }
    };

    const fetchDevices = async () => {
        const username = DEFAULT_USERNAME;
        try {
            const response = await fetch(`${API_BASE_URL}/devices?username=${username}`);
            if (response.ok) {
                const data = await response.json();
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

                const chipIds = newDevices.map(device => device.device_id);
                const registerMessage = {
                    message_type: "register",
                    device: "Frontend",
                    chipIds: chipIds
                };
                socketService.sendMessage(JSON.stringify(registerMessage));
            } else {
                console.error("HTTP Error:", response.status);
            }
        } catch (error) {
            console.error("Error fetching devices:", error);
        }
    };

    const sendControllerRequests = (deviceID) => {
        const requests = [
            { message_type: "ControllerStateRequest" },
            { message_type: "AutomaticModeRequest" },
            { message_type: "GrowCycleConfigRequest" }
        ];

        requests.forEach(request => {
            const message = {
                target_UUID: deviceID,
                device: "frontend",
                ...request
            };
            sendWebSocketMessage(message);
        });
    };

const handleControllerResponse = (data) => {
    const { message_type, UID, payload } = data;

    if (!UID || !payload) {
        console.error("Invalid controller response:", data);
        return;
    }

    switch (message_type) {
        case "ControllerStateResponse":
            const controllerState = payload.ControllerState;
            for (const [key, value] of Object.entries(controllerState)) {
                dispatch({
                    type: 'UPDATE_DEVICE_STATE',
                    payload: {
                        UID,
                        target: key,
                        value: value
                    }
                });
            }
            break;

        case "AutomaticModeResponse":
            dispatch({
                type: 'UPDATE_DEVICE_STATE',
                payload: {
                    UID,
                    target: 'automaticMode',
                    value: payload.automaticMode
                }
            });
            break;

        case "ManualModeResponse":
            dispatch({
                type: 'UPDATE_DEVICE_STATE',
                payload: {
                    UID,
                    target: 'manualMode',
                    value: payload.manualMode
                }
            });
            break;

        case "GrowCycleConfigResponse":
            dispatch({
                type: 'UPDATE_DEVICE_STATE',
                payload: {
                    UID,
                    target: 'growCycleConfig',
                    value: payload.growCycleConfig
                }
            });
            break;

        default:
            console.error("Unhandled controller response type:", message_type);
            break;
    }
};

    const sendCommandToGrowbox = (deviceID, action) => {
        if (socketService.isConnected() && socketService.socket.readyState === WebSocket.OPEN) {
            const message = {
                device: "Frontend",
                chipId: deviceID,
                action: action
            };
            socketService.sendMessage(JSON.stringify(message));
        } else {
            console.error("WebSocket connection is not open.");
        }
    };

    const sendWebSocketMessage = (message) => {
        if (socketService.isConnected() && socketService.socket.readyState === WebSocket.OPEN) {
            socketService.sendMessage(JSON.stringify(message));
        } else {
            console.error("WebSocket connection is not open.");
            socketService.connect(WEBSOCKET_HOST, WEBSOCKET_PORT, handleWebSocketMessage);
            setTimeout(() => {
                if (socketService.isConnected() && socketService.socket.readyState === WebSocket.OPEN) {
                    socketService.sendMessage(JSON.stringify(message));
                } else {
                    console.error("WebSocket connection could not be established.");
                }
            }, 1000);
        }
    };

    const fetchGrowPlans = async () => {
        setLoadingGrowPlans(true);
        setError("");
        const username = DEFAULT_USERNAME;

        try {
            const response = await GrowPlanServices.getGrowPlans(username);
            setGrowPlans(response.data);
        } catch (error) {
            setError("Error fetching grow plans");
            console.error("Error:", error);
        } finally {
            setLoadingGrowPlans(false);
            setShowGrowPlansModal(true);
        }
    };

    const sendGrowPlanToGrowbox = async (selectedGrowPlan) => {
        if (!selectedGrowPlan || !selectedGrowPlan.droppedItems) {
            console.error('Selected grow plan is not set correctly.');
            return;
        }

        try {
            const promises = selectedGrowPlan.droppedItems.map(item => {
                const cycleId = item.id.split('-')[0];
                return GrowPlanServices.getCyclePlanFromID(cycleId)
                    .then(response => JSON.parse(response.data))
                    .catch(error => {
                        console.error("Error loading grow cycle:", error);
                        return null;
                    });
            });

            const results = await Promise.all(promises);

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
                username: DEFAULT_USERNAME,
                growCycleName: selectedGrowPlan.growPlanName,
                description: selectedGrowPlan.description,
                sharingStatus: selectedGrowPlan.sharingStatus,
                growData: growData
            };

            if (socketService.isConnected() && socketService.socket.readyState === WebSocket.OPEN) {
                const message = {
                    target_UUID: selectedDevice?.device_id,
                    device: "frontend",
                    message_type: "ControlCommand",
                    payload: {
                        commands: [
                            {
                                target: "growCycle",
                                action: "setConfig",
                                value: finalData
                            }
                        ]
                    }
                };
                sendWebSocketMessage(message);
                // Nach dem Senden den Zustand aktualisieren
                sendControllerRequests(selectedDevice.device_id);
            } else {
                console.error("WebSocket connection is not open.");
            }
        } catch (error) {
            console.error("Error processing grow plan data:", error);
        }
    };

    const handleDeviceClick = useCallback((deviceId) => {
        const device = devices.find(device => device.device_id === deviceId);

        if (!device) {
            console.error(`Device with ID ${deviceId} not found.`);
            return;
        }

        setSelectedDevice(device);
        setShowDeviceModal(true);
    }, [devices]);

    return (
        <div>
            <h1>Growboxen</h1>

            <div className="devices-container">
                {devices.map(device => (
                    <Device
                        key={device.device_id}
                        device={device}
                        onClick={() => handleDeviceClick(device.device_id)}
                        onRunClick={() => sendCommandToGrowbox(device.device_id, "startGrow")}
                        onStopClick={() => sendCommandToGrowbox(device.device_id, "stopGrow")}
                    />
                ))}
            </div>

            {selectedDevice && (
                <DeviceModal
                    ref={deviceModalRef}
                    show={showDeviceModal}
                    onHide={() => setShowDeviceModal(false)}
                    device={selectedDevice}
                    onSendSettings={(update) => sendWebSocketMessage(update)}
                    onSendSpecificCommand={(message) => {
                        sendWebSocketMessage(message);
                        // Nach dem Senden den Zustand aktualisieren
                        sendControllerRequests(selectedDevice.device_id);
                    }}
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
