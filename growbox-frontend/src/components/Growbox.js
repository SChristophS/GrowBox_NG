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

	useEffect(() => {
		// lade die Geräte sobald die Seite geöffnet wird
        fetchDevices();
		
		// Socket-Verbindung herstellen
        socketService.connect('localhost', 8085, handleSocketMessage);
		
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
		console.log("Function call: fetchDevices");
        
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
			
			// WebSocket-Verbindung initiieren
            initiateSocketConnection();
			
        } else {
            console.error("fetchDevices: HTTP-Error: " + response.status);
        }
    };
	
    const initiateWebSocketConnection = () => {
		setShowModal(false);
		
        const ws = new WebSocket(process.env.REACT_APP_WS_URL);
		
        ws.onopen = () => {
            console.log("WebSocket connection established");
			sendRegisterMessage(selectedDeviceId);
        };

        ws.onmessage = handleWebSocketMessage;

        ws.onclose = () => {
            console.log("initiateWebSocketConnection: WebSocket connection closed");
            // Setze isConnected für alle Geräte auf false, wenn die WebSocket-Verbindung geschlossen wird
            // setDevices(devices => devices.map(device => ({ ...device, isConnected: false })));
        };

        //setWebsocket(ws);
		websocketRef.current = ws; // Setze den WebSocket in useRef
    };

	const sendRegisterMessage = (deviceId) => {
		const ws = websocketRef.current;
		
         if (ws && ws.readyState === WebSocket.OPEN) {
            const registerMessage = JSON.stringify({
                device: "Frontend",
                chipId: deviceId,
                message: "register",
                action: "register"
            });
            ws.send(registerMessage);
            console.log("Register message sent for device", deviceId);
        }
    };
	
	const handleWebSocketMessage = (message) => {
		console.log("Message received:", message.data);
		const data = JSON.parse(message.data);
		
		// Behandeln der Nachricht, die die Geräteliste enthält
		if (data.type === "device_list") {
			console.log("Geräteliste erhalten", data.devices);
			
			// Aktualisieren Sie den Zustand basierend auf der Geräteliste
			setDevices(devices => devices.map(device => {
				// Prüfen, ob das Gerät in der Liste ist
				const isDeviceListed = data.devices.some(listedDevice => listedDevice.startsWith(device.device_id));
				
				// Zusätzlich prüfen, ob es sich um einen Controller handelt
				const isControllerAlive = data.devices.includes(`${device.device_id}-controller`);

				return {
					...device,
					isConnected: isDeviceListed, // Für Frontend-Geräte
					controllerAlive: isControllerAlive // Spezifische Prüfung für Controller-Geräte
				};
			}));
		}

		setMessages(prevMessages => [...prevMessages, message.data]);
	};
	
    const updateDeviceConnectionStatus = (deviceId, isConnected) => {
        setDevices(devices => devices.map(device =>
            device.device_id === deviceId ? { ...device, isConnected } : device
        ));
    };

    const handleDeviceClick = (deviceId) => {
        setSelectedDeviceId(deviceId);
        setShowModal(true);
    };
	

	
	const AskControllerToConnect = (event) => {
		// Stoppen des Event-Bubblings
		
		event.stopPropagation();

		const requestData = { device_id: selectedDeviceId };
		console.log('RequestedData: ', requestData);
		console.log("RequestedData:");
		fetch(`${process.env.REACT_APP_API_URL}/ask-growbox-to-socket-connect`, {
		  method: "POST",
		  headers: {
			'Content-Type': 'application/json',
		  },
		  body: JSON.stringify(requestData),
		})
		.then(response => {
		  if (!response.ok) {
			throw new Error('Netzwerkantwort war nicht ok');
		  }
		  return response.json();
		})
		.then(data => {
		  console.log('Erfolg:', data);
		  // Hier könnten Sie weitere Aktionen durchführen, basierend auf der Antwort
		})
		.catch(error => {
		  console.error('Fehler:', error);
		});
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

	const downloadJson = (data, filename) => {
		const blob = new Blob([JSON.stringify(data, null, 2)], { type: 'application/json' });
		const url = URL.createObjectURL(blob);
		const link = document.createElement('a');
		link.href = url;
		link.download = filename;
		document.body.appendChild(link);
		link.click();
		document.body.removeChild(link);
	};

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
						<p>Device Chipd-ID: {device.device_id}</p>
						<p><Button
							variant="primary"
							onClick={(event) => AskControllerToConnect(event)} // Übergeben des Event-Objekts und Aufrufen von stopPropagation
						>
							Request Controller Socket Connection
						</Button></p>
						
						<p>Frontend to Socket <span className={`status-dot ${device.isConnected ? 'connected' : 'disconnected'}`}></span></p>
						<p>Controller to Socket <span className={`status-dot ${device.controllerAlive ? 'connected' : 'disconnected'}`}></span></p>
						<p>Growprogram is running <span className={`status-dot ${device.controllerRunning ? 'running' : 'notRunning'}`}></span></p>
						
						
						<Button
							variant="success"
							onClick={(event) => {
								event.stopPropagation(); // Verhindert das Event-Bubbling
								setShowGrowPlansModal(true);
								fetchGrowPlans();
							}}
							style={{ marginTop: "20px" }}
						>
							Growplan auswählen
						</Button>
						
						<p></p>
						<Button variant="success" onClick={(event) => {handleRunClick(device.device_id, event);}}>Run</Button>
						<Button variant="success" onClick={(event) => {handleStopClick(device.device_id, event);}}>Stop</Button>
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
					Wollen Sie eine Verbindung zur Growbox {selectedDeviceId} aufbauen?
				</Modal.Body>
				<Modal.Footer>
					<Button variant="secondary" onClick={() => setShowModal(false)}>Abbrechen</Button>
					<Button variant="primary" onClick={initiateWebSocketConnection}>Verbinden</Button>
				</Modal.Footer>
			</Modal>
	</div>
    );
}

export default Growbox;
