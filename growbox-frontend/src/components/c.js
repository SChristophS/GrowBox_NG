import React, { useState } from 'react';
import { Container, Button, Row } from 'react-bootstrap';
import CycleInput from './CycleInput';

const Watering = () => {
  const [waterCycles, setWaterCycles] = useState([]);

  const handleWaterCycleChange = (index, cycleType, name, value) => {
    const newWaterCycles = [...waterCycles];
    newWaterCycles[index] = { ...newWaterCycles[index], [name]: value };
    setWaterCycles(newWaterCycles);
  };

  const handleAddWaterCycle = () => {
    setWaterCycles([...waterCycles, { status1: 'full', duration1: 0, status2: 'empty', duration2: 0, waterRepetitions: 0 }]);
  };

  const handleDeleteWaterCycle = (index) => {
    const newWaterCycles = waterCycles.filter((_, i) => i !== index);
    setWaterCycles(newWaterCycles);
  };

  return (
    <Container>
      <h2>Water Cycles</h2>
      {waterCycles.map((waterCycle, index) => (
        <CycleInput
          key={index}
          cycleType="water"
          cycleIndex={index}
          cycle={waterCycle}
          onChange={handleWaterCycleChange}
          onDelete={handleDeleteWaterCycle}
        />
      ))}
      <Row className="justify-content-center">
        <Button onClick={handleAddWaterCycle}>Add Water Cycle</Button>
      </Row>
    </Container>
  );
};

export default Watering;
