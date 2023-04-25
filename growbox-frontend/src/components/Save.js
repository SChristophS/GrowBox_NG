import React, { useContext, useState } from 'react';
import { Container, Form, Button } from 'react-bootstrap';
import { SettingsContext } from '../contexts/SettingsContext';

const Save = () => {
  const { ledCycles, tempCycles, wateringCycles } = useContext(SettingsContext);
  const [growCycleName, setGrowCycleName] = useState('');
  const [description, setDescription] = useState('');

  const handleSave = () => {
    const growPlan = {
      growCycleName,
      description,
      ledCycles,
      tempCycles,
      wateringCycles,
    };

    fetch('http://localhost:5000/save-grow-plan', {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
      },
      body: JSON.stringify(growPlan),
    })
      .then((response) => response.json())
      .then((data) => console.log(data))
      .catch((error) => console.error('Error:', error));
  };

  return (
    <Container>
      <h2>Speichern</h2>
      <Form>
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
        <Button onClick={handleSave}>Zyklus speichern</Button>
      </Form>
    </Container>
  );
};

export default Save;
