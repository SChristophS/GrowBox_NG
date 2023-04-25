import React, { useState } from "react";
import { Container, Button, Form, Row, Col } from "react-bootstrap";
import CycleInput from "./CycleInput";
import "./styles.css";

const GrowPlanner = () => {
  const [cycles, setCycles] = useState([
    {
      type: "led",
      ledOff: "",
      ledOn: "",
    },
    {
      type: "water",
      tankEmpty: "",
      tankFull: "",
    },
    {
      type: "temperature",
      temperature: "",
    },
  ]);

  const onChange = (index, type, name, value) => {
    const newCycle = { ...cycles[index], [name]: value };
    const newCycles = [...cycles];
    newCycles[index] = newCycle;
    setCycles(newCycles);
  };

  const addCycle = (type) => {
    const newCycle = { type };
    setCycles([...cycles, newCycle]);
  };

  const removeCycle = (index) => {
    const newCycles = cycles.filter((_, i) => i !== index);
    setCycles(newCycles);
  };

  const [totalDuration, setTotalDuration] = useState(0);

  const handleTotalDurationChange = (e) => {
    const value = parseInt(e.target.value);
    const adjustedValue = value < 0 ? 0 : value;
    setTotalDuration(adjustedValue);
  };

  return (
    <Container>
      <h2>Growbox Zyklus Planer</h2>
      <Form.Group className="mb-3">
        <Row>
          <Col>
            <p>Hier bitte die Gesamtdauer des Growvorgangs eingeben:</p>
            <Form.Label>Gesamt Zyklus Dauer</Form.Label>
            <Form.Control
              type="number"
              name="totalDuration"
              value={totalDuration}
              onChange={handleTotalDurationChange}
              style={{ width: "8em" }}
            />
          </Col>
        </Row>
      </Form.Group>
      {cycles.map((cycle, index) => (
        <CycleInput
          key={index}
          cycleType={cycle.type}
          cycleIndex={index}
          cycle={cycle}
          onChange={onChange}
          onDelete={() => removeCycle(index)}
        />
      ))}
      <Button onClick={() => addCycle("led")}>LED-Zyklus hinzufügen</Button>
      <Button className="ml-2" onClick={() => addCycle("water")}>
        Wasser-Zyklus hinzufügen
      </Button>
      <Button className="ml-2" onClick={() => addCycle("temperature")}>
        Temperatur-Zyklus hinzufügen
      </Button>
    </Container>
  );
};

export default GrowPlanner;
