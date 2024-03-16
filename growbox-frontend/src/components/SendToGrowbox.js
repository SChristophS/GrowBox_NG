import React, { useContext, useState, useEffect } from "react";
import { Modal, Button, Container, Table, Row, Col } from "react-bootstrap";
import { AuthContext } from "../contexts/AuthContext";
import { SettingsContext } from "../contexts/SettingsContext";
import GrowplanChecks from './GrowplanChecks';
import GrowPlanServices from '../utility/GrowPlanServices';

const SendToGrowbox = () => {
	const { username } = useContext(AuthContext);
    const [growPlans, setGrowPlans] = useState([]);
    const [statusMessage, setStatusMessage] = useState("");
    const [loadStatus, setLoadStatus] = useState("");
	const [showModal, setShowModal] = useState(false);
	const [growboxIDs, setGrowboxIDs] = useState([]);
	const [combinedData, setcombinedData] = useState([]);
	

	const downloadJson = (jsonData, fileName) => {
		// Konvertieren des JSON-Objekts in einen String
		const jsonString = JSON.stringify(jsonData, null, 2); // Der zweite Parameter sorgt für eine schön formatierte Ausgabe

		// Erstellen eines neuen Blobs mit dem JSON-String
		const blob = new Blob([jsonString], { type: 'application/json' });

		// Erstellen eines temporären Download-Links
		const href = URL.createObjectURL(blob);

		// Erstellen eines temporären HTML-Elementes <a> für den Download
		const link = document.createElement('a');
		link.href = href;
		link.download = fileName;

		// Hinzufügen des Links zum DOM und Auslösen des Downloads
		document.body.appendChild(link);
		link.click();

		// Entfernen des Links nach dem Download
		document.body.removeChild(link);
		URL.revokeObjectURL(href); // Diese Zeile verhindert Speicherlecks
	};
	
	const transmitJson = (device_id) => {
		console.log("transmitJson mit: ", combinedData);
		console.log("transmitJson mit: ", device_id);

		const additionalData = {
			"device_id": device_id,
		};
	
		const growplan_with_targetID = Object.assign({}, combinedData, additionalData);

		console.log("growplan_with_targetID");
		console.log(growplan_with_targetID);
	
	
		GrowPlanServices.transmitGrowPlanToTarget(growplan_with_targetID)
		.then(response => {
			console.log("response from transmitGrowPlanToTarget:", response)
		})
		.catch(error => {
			console.log("Fehler beim Laden der GrowCycles");
		});
		
	};
	
	
	const getGrowPlans = () => {
		GrowPlanServices.getGrowPlans(username)
    .then(response => {
		console.log("response from loadGrowPlans:", response)
        setGrowPlans(response.data);
		setStatusMessage("Grow-Pläne erfolgreich geladen.");
      })
      .catch(error => {
        setStatusMessage("Fehler beim Laden der GrowCycles");
      });
	};

	const sendPlanToGrowbox = (plan) => {
		//console.log(plan);
				
		let promises = [];

		plan.droppedItems.forEach(item => {
			// Fügen Sie jeden Promise zum Array hinzu
			promises.push(
				GrowPlanServices.getCyclePlanFromID(item.id.split('-')[0])
				.then(response => {
					return JSON.parse(response.data); // Die geparsten Daten zurückgeben
				})
				.catch(error => {
					setStatusMessage("Fehler beim Laden der GrowCycles");
				})
			);
		});

		// Warten, bis alle Promises aufgelöst sind
		Promise.all(promises).then(results => {
			
			// Extrahieren Sie ledCycles aus jedem Ergebnis und fügen Sie sie in einem einzigen Array zusammen
			let allLedCycles = results.reduce((acc, result) => {
				if (result && result.growData && result.growData.ledCycles) {
					return acc.concat(result.growData.ledCycles);
				}
				return acc;
			}, []);
			
			let allTemperatureCycles = results.reduce((acc, result) => {
				if (result && result.growData && result.growData.tempCycles) {
					return acc.concat(result.growData.tempCycles);
				}
				return acc;
			}, []);
			
			let allWateringCycles = results.reduce((acc, result) => {
				if (result && result.growData && result.growData.wateringCycles) {
					return acc.concat(result.growData.wateringCycles);
				}
				return acc;
			}, []);			

			//console.log(allLedCycles); // Hier ist Ihr Array von ledCycles
			//console.log(allTemperatureCycles); // Hier ist Ihr Array von ledCycles
			//console.log(allWateringCycles); // Hier ist Ihr Array von ledCycles
			
			// Erstellen des großen JSON-Objekts
			let combinedData = {
				ledCycles: allLedCycles,
				temperatureCycles: allTemperatureCycles,
				wateringCycles: allWateringCycles
			};
			
			// hier kann die json downgeloaded werden wenn aktiv
			//downloadJson(combinedData, 'meineDaten.json');
			
			// wir zeigen eine Auswahl aller möglichen verbundene Growboxen an
			// damit der User auswählen kann auf welche er die neuen Daten
			// senden will
			console.log("SendToGrowbox: ask for connected GrowboxIDs for username: ", username);

			GrowPlanServices.getGrowboxIDsFromUsername(username)
			.then(response => {
				console.log("response from getGrowboxIDsFromUsername:", response)
				setGrowboxIDs(response.data); // Speichert die Growbox-IDs im State
				setcombinedData(combinedData);
				setShowModal(true); // Öffnet das Modal
			})
     		
			
			


			
			
		}).catch(error => {
			// Fehlerbehandlung, falls einer der Promises fehlschlägt
			console.error("Fehler bei der Verarbeitung der Promises", error);
		});
		

	}
	
	
	
  
    useEffect(() => {
        getGrowPlans();
    }, []);

    return (
        <Container>
            <h2>An Growbox senden</h2>
			
			<Modal show={showModal} onHide={() => setShowModal(false)}>
			<Modal.Header closeButton>
				<Modal.Title>Wähle eine Growbox</Modal.Title>
			</Modal.Header>
			<Modal.Body>
				{growboxIDs.map((box, index) => (
					<div key={index} className="growbox-selection">
						<p>Device ID: {box.device_id} - Status: {box.status}</p>
						<Button onClick={() => {
							// Logik, um Daten an die ausgewählte Growbox zu senden
							console.log("Senden an", box.device_id);
							transmitJson(box.device_id)
							setShowModal(false); // Schließt das Modal
						}}>Senden</Button>
					</div>
				))}
			</Modal.Body>
			</Modal>

            <Row>
                <Col>
                    <p>{statusMessage}</p>
                    <p>{loadStatus}</p>
                </Col>
            </Row>
            <Table striped bordered hover size="sm">
                <thead>
                    <tr>
                        <th>Benutzername</th>
                        <th>Growname</th>
                        <th>Beschreibung</th>
                        <th>Send</th>
                    </tr>
                </thead>
                <tbody>
                    {(growPlans || []).map((plan) => (
                        <tr key={`${plan.username}-${plan.growCycleName}`}>
                            <td>{plan.username}</td>
                            <td>{plan.growPlanName}</td>
                            <td>{plan.growPlanDescription}</td>
                            <td>
								<button onClick={() => sendPlanToGrowbox(plan)}>Send</button>
							</td>
                        </tr>
                    ))}
                </tbody>
            </Table>
        </Container>
    );
};

export default SendToGrowbox;
