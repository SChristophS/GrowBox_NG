import React, { useContext, useState } from "react";
import { Modal, Container, Button } from "react-bootstrap";
import { AuthContext } from '../contexts/AuthContext';
import { GrowCycleContext } from '../contexts/GrowCycleContext';


const Save = () => {
  const { growCycleName, totalGrowTime, ledSettings, temperatureSettings, wateringSettings, description, sharingStatus} = useContext(
    GrowCycleContext
  );
  const [showConfirmModal, setShowConfirmModal] = useState(false);
  const [overwrite, setOverwrite] = useState(false);
  const { username } = useContext(AuthContext);
  const [saveSuccess, setSaveSuccess] = useState(false);


  const handleSave = (overwriteValue) => {
    const growPlan = {
      username,
      overwrite: overwriteValue,
      growCycleName,
      description,
      sharingStatus,
      growData: {
        totalGrowTime : totalGrowTime,
        ledCycles: ledSettings,
        tempCycles: temperatureSettings,
        wateringCycles: wateringSettings,
      },
    };
  
    console.log("JSON to be sent:", JSON.stringify(growPlan));
  
    fetch("http://localhost:5000/save-cycle-plan", {
      method: "POST",
      headers: {
        "Content-Type": "application/json",
      },
      credentials: "include",
      body: JSON.stringify(growPlan),
    })
      .then((response) => {
        if (response.status === 400) {
          return response.json().then((data) => {
            if (data.message === "Grow plan with this name already exists.") {
              setShowConfirmModal(true);
            }
          });
        } else {
          return response.json();
        }
      })
      .then((data) => {
        if (data) {
          console.log("Response from server:", data);
          if (data.message === "Grow plan saved successfully.") { // Check the success message
            setSaveSuccess(true); // Show success message
          }
        }
      })
      .catch((error) => console.error("Error:", error));
  };
  

  return (
    <Container>
      <h2>Speichern</h2>
      {saveSuccess && <p>Grow Plan wurde erfolgreich gespeichert.</p>}
      <Button
        type="button"
        onClick={() => handleSave(overwrite)} // Pass overwrite as argument
      >
        Zyklus speichern
      </Button>

      <Modal show={showConfirmModal} onHide={() => setShowConfirmModal(false)}>
        <Modal.Header closeButton>
          <Modal.Title>Grow Plan überschreiben</Modal.Title>
        </Modal.Header>
        <Modal.Body>
          Ein Grow Plan mit diesem Namen existiert bereits. Möchten Sie ihn überschreiben?
        </Modal.Body>
        <Modal.Footer>
          <Button variant="secondary" onClick={() => setShowConfirmModal(false)}>
            Abbrechen
          </Button>
          <Button
            type="button"
            onClick={() => {
              setOverwrite(true);
              handleSave(true);
              setShowConfirmModal(false); // Add this line
            }}
          >
            Überschreiben
          </Button>
        </Modal.Footer>
      </Modal>
    </Container>
  );
};

export default Save;
