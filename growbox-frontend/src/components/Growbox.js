import React, { useState, useEffect, useRef } from "react";
import { Button, Modal, Form } from "react-bootstrap";
import GrowPlanServices from '../utility/GrowPlanServices';
import socketService from '../utility/socketService';
import './Growbox.css';

function Growbox() {
    const [devices, setDevices] = useState([]);
    const [showModal, setShowModal] = useState(false);
    const [selectedDeviceId, setSelectedDeviceId] = useState("");
    const [websocket, setWebsocket] = useState(null);
    const [messages, setMessages] = useState([]);
	const websocketRef = useRef(null); // Verwendung von useRef für WebSocket
	const [loadingGrowPlans, setLoadingGrowPlans] = useState(false);
	const [error, setError] = useState("");
	const [growPlans, setGrowPlans] = useState([]);
	const [showGrowPlansModal, setShowGrowPlansModal] = useState(false);
	const [selectedGrowPlan, setSelectedGrowPlan] = useState(null);
	const [startFromHere, setStartFromHere] = useState(0);
	
	const [waterBeckenZustand, setWaterBeckenZustand] = useState(false);
	const [lightIntensity, setLightIntensity] = useState(0);
	

	
	
	

    useEffect(() => {
        // Lade die Geräte sobald die Seite geöffnet wird
        fetchDevices();
        
        // Socket-Verbindung herstellen und Nachricht senden
        socketService.connect("192.168.178.25", 8085, handleWebSocketMessage);

       // Nachricht senden, sobald die Verbindung hergestellt ist
		// const intervalId = setInterval(() => {
			// if (socketService.isConnected) {
				// console.log("SocketServer ist verbunden - sende");

				// //Angepasste Nachricht senden
				// socketService.sendMessage(JSON.stringify({
					// message_type: "register_message",
					// payload: {
						// device: "SocketServer",
						// chipId: null,
						// action: "register"
					// }
				// }));
				// clearInterval(intervalId);
			// }
		// }, 100);
        
        return () => {
            // Diese Funktion wird aufgerufen, wenn die Komponente demontiert wird
            socketService.disconnect(); // Schließe die Socket-Verbindung
		};
    }, []);
	
	const fetchGrowPlans = async () => {
		setLoadingGrowPlans(true);
		setError(""); // Setze vorherige Fehlermeldungen zurück
		const username = "christoph";
		
		try {
			GrowPlanServices.getGrowPlans(username)
			.then(response => {
				setGrowPlans(response.data);
				console.log("fetchGrowPlans: response.data: ", response.data);
			  })
			.catch(error => {
				setError("Fehler beim Abrufen der Growpläne");
				console.error("Fehler:", error);
			});
		} catch (error) {
			setError("Fehler beim Abrufen der Growpläne");
			console.error("Fehler:", error);
		} finally {
			setLoadingGrowPlans(false);
			setShowGrowPlansModal(true);
		}  
	};



const fetchDevices = async () => {
    console.log("fetchDevices: Starting fetchDevices");

    const username = "christoph";
    const response = await fetch(`http://localhost:5000/devices?username=${username}`);

    console.log("fetchDevices: response from server:", response);

    if (response.ok) {
        console.log("fetchDevices: response ok");
        const data = await response.json();

        console.log("fetchDevices: data.devices :");
        console.log(data.devices);

        // Initialisiere alle Geräte als nicht verbunden
        setDevices(data.devices.map(device => ({ ...device, isConnected: false, controllerAlive: false })));

        // Sammle alle chipId-Werte
        const chipIds = data.devices.map(device => device.device_id);

        // Sende die Registrierungsnachricht mit den gesammelten chipId-Werten
        const registerMessage = JSON.stringify({
            message_type: "register",
			device : "Frontend",
			chipIds: chipIds
        });

        console.log("fetchDevices: sende register message:");
        console.log(registerMessage);
        socketService.sendMessage(registerMessage);

    } else {
        console.error("fetchDevices: HTTP-Error: " + response.status);
    }
};

	
	
	
	
	const sendControlCommand = (chipId) => {
		const controlMessage = JSON.stringify({
			uid: chipId,
			message_type: "control_command",
			payload: {
				target: "ControllerState",
				action: "update",
				value: true
			}
		});
		
		socketService.sendMessage(controlMessage);
		console.log(`[DEBUG] Sent control command: ${controlMessage}`);
	};

	
	
		const handleWebSocketMessage = (event) => {
        if (!event || !event.data) {
            console.error("[ERROR] Received event or event.data is undefined");
            return;
        }

        let data;
        try {
            if (typeof event.data === 'string') {
                data = JSON.parse(event.data);
            } else {
                data = event.data;
            }
        } catch (error) {
            console.error("[ERROR] Error parsing message data:", error);
            return; // Überspringe die Verarbeitung
        }

        console.log("[DEBUG] Received data:", data);

        if (data && typeof data === 'object' && data.message_type) {
            const { message_type, controllers } = data;

            if (message_type === "register_confirmed") {
                console.log("[DEBUG] Registration confirmed", controllers);

                setDevices(devices => devices.map(device => {
                    const updatedDevice = { ...device };

                    console.log("[DEBUG] Checking device:", device.device_id);

                    if (controllers.hasOwnProperty(device.device_id)) {
                        console.log("Found connected device: ", device.device_id);
                        updatedDevice.isConnected = true;
                        updatedDevice.controllerAlive = controllers[device.device_id].length > 0;
                        console.log(`Device ${device.device_id} is connected: ${updatedDevice.isConnected}, controllerAlive: ${updatedDevice.controllerAlive}`);
                    }

                    return updatedDevice;
                }));
            }
			else if (message_type === "controller_update") {
                const { chipID, status } = data;
                console.log("[DEBUG] Controller update received", data);
                setDevices(devices => devices.map(device => {
                    if (device.device_id === chipID) {
                        return { ...device, controllerAlive: status === "connected" };
                    }
                    return device;
                }));
            }

            setMessages(prevMessages => [...prevMessages, event.data]);
        } else {
            console.error("[ERROR] Invalid message format:", data);
        }
    };




const sendWebSocketMessage = (message) => {
    if (socketService.isConnected() && socketService.socket.readyState === WebSocket.OPEN) {
        socketService.sendMessage(JSON.stringify(message));
    } else {
        console.error("WebSocket-Verbindung ist nicht offen.");
        // Versuchen, die Verbindung erneut herzustellen
        socketService.connect("192.168.178.25", 8085, handleWebSocketMessage);
        // Verzögertes Senden der Nachricht nach erneuter Verbindung
        setTimeout(() => {
            if (socketService.isConnected() && socketService.socket.readyState === WebSocket.OPEN) {
                socketService.sendMessage(JSON.stringify(message));
            } else {
                console.error("WebSocket-Verbindung konnte nicht hergestellt werden.");
            }
        }, 1000);
    }
};




const handleWaterBeckenZustandChange = (event) => {
    const value = event.target.checked ? 1 : 0; // Verwenden von 1 und 0 statt true und false
    setWaterBeckenZustand(value);
    const message = {
        uid: selectedDeviceId,
        message_type: "control_command",
        target: "wasserbeckenZustand",
        action: "set",
        value: value
    };
    sendWebSocketMessage(message);
};

const handleLightIntensityChange = (event) => {
    const value = parseInt(event.target.value, 10);
    setLightIntensity(value);
    const message = {
        uid: selectedDeviceId,
        message_type: "control_command",
        target: "LightIntensity",
        action: "set",
        value: value
    };
    sendWebSocketMessage(message);
};


	
    const updateDeviceConnectionStatus = (deviceId, isConnected) => {
        setDevices(devices => devices.map(device =>
            device.device_id === deviceId ? { ...device, isConnected } : device
        ));
    };

const handleDeviceClick = (deviceId) => {
    setSelectedDeviceId(deviceId);
    setShowModal(true);
    // Hier können Sie initiale Werte für das Modal festlegen, falls gewünscht
    setWaterBeckenZustand(false); // Beispiel initialer Wert
    setLightIntensity(0); // Beispiel initialer Wert
};
	


	async function getCompleteGrowPlans() {
	  // Iteriere über alle growPlans und transformiere jedes Element in ein Promise, 
	  // das die detaillierten Daten der droppedItems holt
	  const plansWithDetailsPromises = growPlans.map(async (growPlan) => {
		// Hole die detaillierten Daten für jedes droppedItem
		const detailedItemsPromises = growPlan.droppedItems.map(async (item) => {
			// Direkter Zugriff auf die Antwort, da sie bereits ein Objekt ist
			const response = await GrowPlanServices.getCyclePlanFromID(item.id);
			return response.data; // Angenommen, die Daten befinden sich im `data`-Attribut der Antwort
		});

		// Warte auf alle detaillierten Daten der droppedItems
		const detailedItems = await Promise.all(detailedItemsPromises);

		// Erstelle ein neues Objekt, das den ursprünglichen growPlan mit den detaillierten droppedItems kombiniert
		return {
		  ...growPlan, // Kopiere alle Eigenschaften des ursprünglichen growPlans
		  droppedItems: detailedItems // Ersetze droppedItems durch die detaillierten Daten
		};
	  });

	  // Warte auf die Auflösung aller Promises, um die vollständigen Growpläne zu erhalten
	  const completeGrowPlans = await Promise.all(plansWithDetailsPromises);

	  // completeGrowPlans enthält jetzt alle growPlans mit den detaillierten Informationen der droppedItems
	  return completeGrowPlans;
	}


	const sendCommandToGrowbox = async (deviceID, action) => {
		console.log("sendCommandToGrowbox: function call");
		console.log("action: " + action);

		// Überprüfen, ob die WebSocket-Verbindung offen ist
			
		if (websocketRef.current && websocketRef.current.readyState === WebSocket.OPEN) {
			// Nachricht zum Senden der Growplan-Daten vorbereiten
			const message = JSON.stringify({
				device: "Frontend",
				chipId: deviceID, // Stellen Sie sicher, dass selectedDeviceId korrekt gesetzt ist
				action: action
			});
			
			// Senden der Nachricht
			websocketRef.current.send(message);
			console.log("sendMessageToGrowbox: Nachricht gesendet:", message);
		} else {
			console.error("WebSocket-Verbindung ist nicht offen.");
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
			// Vorläufige Datenstruktur, um die gewünschte Struktur aufzubauen
			const growData = {
				totalGrowTime: 0, // Dies muss basierend auf den Daten berechnet werden
				startFromHere: parseInt(startFromHere, 10),
				ledCycles: [],
				tempCycles: [],
				wateringCycles: []
			};

			// Integriere die Ergebnisse in die growData-Struktur
			results.forEach(result => {
				if (result && result.growData) {
					growData.ledCycles.push(...result.growData.ledCycles);
					growData.tempCycles.push(...result.growData.tempCycles);
					growData.wateringCycles.push(...result.growData.wateringCycles);
				}
			});

			// Berechne totalGrowTime basierend auf den gesammelten Daten, falls erforderlich
			// Zum Beispiel: growData.totalGrowTime = Berechnung...

			// Endgültige Datenstruktur zusammenbauen
			const finalData = {
				_id: selectedGrowPlan._id,
				username: "christoph", // Beispiel, setze entsprechend
				growCycleName: selectedGrowPlan.growPlanName,
				description: selectedGrowPlan.description,
				sharingStatus: selectedGrowPlan.sharingStatus,
				growData: growData
			};
			
			
			
			
			
			
			
			// Überprüfen, ob die WebSocket-Verbindung offen ist
			if (websocketRef.current && websocketRef.current.readyState === WebSocket.OPEN) {
				// Nachricht zum Senden der Growplan-Daten vorbereiten
				const message = JSON.stringify({
					device: "Frontend",
					chipId: selectedDeviceId, // Stellen Sie sicher, dass selectedDeviceId korrekt gesetzt ist
					action: "send_growplan",
					message: finalData // Hier übergeben Sie die finalData direkt
				});
				
				// Senden der Nachricht
				websocketRef.current.send(message);
				console.log("Growplan-Daten gesendet:", message);
			} else {
				console.error("WebSocket-Verbindung ist nicht offen.");
			}
	
	
	
	
	
	
	
	

			// Download der kombinierten JSON
			//downloadJson(finalData, 'finalGrowData.json');
		})
		.catch(error => console.error("Fehler bei der Verarbeitung der GrowCycle-Daten", error));
	};


    // Event-Handler für den "Run"-Button
    const handleRunClick = (deviceId, event) => {
        event.stopPropagation(); // Verhindern, dass das Klick-Event zum Geräte-Container-Element durchdringt
        sendCommandToGrowbox(deviceId, "startGrow");
    };

    // Event-Handler für den "Stop"-Button
    const handleStopClick = (deviceId, event) => {
        event.stopPropagation(); // Verhindern, dass das Klick-Event zum Geräte-Container-Element durchdringt
        sendCommandToGrowbox(deviceId, "stopGrow");
    };
	
        return (
        <div>
            <h1>Growboxen</h1>
			
			
			
<div className="devices-container">
    {devices.map((device, index) => (
        <div key={index} 
			className={`device ${device.status} ${device.isConnected ? 'connected' : 'disconnected'}`}
            onClick={() => handleDeviceClick(device.device_id)}>
            <p>Device Chip-ID: {device.device_id}</p>
            <p>

            </p>
            
            <p>Frontend to Socket 
				<span className={`status-dot ${device.isConnected ? 'connected' : 'disconnected'}`}></span>
            </p>
            <p>Controller to Socket 
				<span className={`status-dot ${device.controllerAlive ? 'connected' : 'disconnected'}`}></span>
            </p>
            <p>Growprogram is running 
				<span className={`status-dot ${device.controllerRunning ? 'running' : 'notRunning'}`}></span>
            </p>
            
            <Button
                variant="success"
                onClick={(event) => {
                    event.stopPropagation();
                    setShowGrowPlansModal(true);
                    fetchGrowPlans();
                }}
                style={{ marginTop: "20px" }}
            >
                Growplan auswählen
            </Button>
            
            <p></p>
            <Button variant="success" onClick={(event) => { handleRunClick(device.device_id, event); }}>Run</Button>
            <Button variant="success" onClick={(event) => { handleStopClick(device.device_id, event); }}>Stop</Button>
        </div>
    ))}
</div>


			
			
			
			
			
			
			


			<Modal show={showGrowPlansModal} onHide={() => setShowGrowPlansModal(false)} size="lg">
				<Modal.Header closeButton>
					<Modal.Title>Growpläne auswählen</Modal.Title>
				</Modal.Header>
				<Modal.Body>
				<Form>
					{growPlans.map((plan, index) => {
						console.log("Hier kommt der Plan");
						console.log(plan); // Gibt das komplette Objekt in der Konsole aus
						return (
							<Form.Check
								type="radio"
								label={plan.growPlanName} // Überprüfen Sie nach dem Anzeigen des Objekts, ob diese Eigenschaft existiert
								name="growPlanGroup"
								id={`growPlan-${index}`}
								key={index}
								onChange={(e) => {
									console.log("Selected grow plan:", plan);
									setSelectedGrowPlan(plan); // Setze das gesamte Plan-Objekt
								}}
							/>
						);
					})}
				</Form>
				</Modal.Body>
				<Modal.Footer>
					<Button variant="secondary" onClick={() => setShowGrowPlansModal(false)}>
						Abbrechen
					</Button>
					<Button
						variant="primary"
						onClick={() => sendGrowPlanToGrowbox(selectedGrowPlan)} // Stelle sicher, dass die richtige Funktion aufgerufen wird
						disabled={!selectedGrowPlan}
					>
						An Growbox senden
					</Button>
				</Modal.Footer>
			</Modal>



<Modal show={showModal} onHide={() => setShowModal(false)}>
    <Modal.Header closeButton>
        <Modal.Title>Growbox Verbindung</Modal.Title>
    </Modal.Header>
    <Modal.Body>
        <Form>
            <Form.Group controlId="formWaterBeckenZustand">
                <Form.Check 
                    type="checkbox"
                    label="Wasserbecken Zustand"
                    checked={waterBeckenZustand === 1} // Anpassung für 1 und 0
                    onChange={handleWaterBeckenZustandChange}
                />
            </Form.Group>
            <Form.Group controlId="formLightIntensity">
                <Form.Label>Lichtintensität</Form.Label>
                <Form.Control 
                    type="range"
                    min="0"
                    max="100"
                    value={lightIntensity}
                    onChange={handleLightIntensityChange}
                />
            </Form.Group>
        </Form>
    </Modal.Body>
    <Modal.Footer>
        <Button variant="secondary" onClick={() => setShowModal(false)}>Abbrechen</Button>
    </Modal.Footer>
</Modal>



	</div>
    );

}

export default Growbox;
