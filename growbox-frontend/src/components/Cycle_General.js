import React, { useContext, useEffect } from 'react';
import { Container, Form } from "react-bootstrap";
import { SettingsContext } from "../contexts/SettingsContext";
import { AuthContext } from '../contexts/AuthContext';
import { GrowCycleContext } from '../contexts/GrowCycleContext';



const General_zycle = () => {
	
  const {
    growCycleName,
    setGrowCycleName,
    description,
    setDescription,
    sharingStatus,
    setSharingStatus,
    totalGrowTime,
    setTotalGrowTime,
  } = useContext(GrowCycleContext);

  const { username } = useContext(AuthContext);

	// Debugging: Loggen der Werte aus dem Context
	  useEffect(() => {
		console.log("growCycleName:", growCycleName);
		console.log("description:", description);
		console.log("sharingStatus:", sharingStatus);
		console.log("totalGrowTime:", totalGrowTime);
	  }, [growCycleName, description, sharingStatus, totalGrowTime]); // Abhängigkeiten hinzufügen, um Änderungen zu verfolgen
	  
  const handleTotalGrowTimeChange = (value) => {
	setTotalGrowTime(value * 1440); // Umrechnung von Tagen in Minuten
  };
  
  

  return (
    <Container>
      <h2>Allgemein</h2>
      <Form>
      <p>Benutzername: {username}</p>

        <Form.Group>
          <Form.Label>Growzyklus Name</Form.Label>
          <Form.Control
            type="text"
            value={growCycleName}
            onChange={(e) => setGrowCycleName(e.target.value)}
          />
        </Form.Group>
        <Form.Group>
          <Form.Label>Beschreibung</Form.Label>
          <Form.Control
            type="text"
            value={description}
            onChange={(e) => setDescription(e.target.value)}
          />
        </Form.Group>
        <Form.Group>
          <Form.Label>Freigabestatus</Form.Label>
          <Form.Control
            as="select"
            value={sharingStatus}
            onChange={(e) => setSharingStatus(e.target.value)}
          >
            <option value="public">public</option>
            <option value="private">private</option>
          </Form.Control>
        </Form.Group>
<Form.Group>
  <Form.Label>TotalGrowTime (in Tagen)</Form.Label>
  <Form.Control
    type="number"
    value={totalGrowTime ? totalGrowTime / 1440 : ''} // Umrechnung von Minuten in Tage
    onChange={(e) => handleTotalGrowTimeChange(parseFloat(e.target.value))}
  />
</Form.Group>
      </Form>
    </Container>
  );
};

export default General_zycle;
