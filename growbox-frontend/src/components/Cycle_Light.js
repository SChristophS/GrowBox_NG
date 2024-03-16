import React, { useContext } from 'react';
import { Container, Button, Row } from 'react-bootstrap';
import CycleInput from './CycleInput';

import { GrowCycleContext } from '../contexts/GrowCycleContext';


const Lightning = () => {
	const { ledSettings, setLedSettings, totalGrowTime } = useContext(GrowCycleContext);

  const handleLightCycleChange = (index, cycleType, name, value) => {
    const newLedCycles = [...ledSettings];
    newLedCycles[index] = { ...newLedCycles[index], [name]: value };
    setLedSettings(newLedCycles);
  };

  const handleAddLightCycle = () => {
    setLedSettings([...ledSettings, { durationOn: 0, durationOff: 0, ledRepetitions: 0 }]);
  };

  const handleDeleteLightCycle = (index) => {
    const newLedCycles = ledSettings.filter((_, i) => i !== index);
    setLedSettings(newLedCycles);
  };

  const totalLedDuration = ledSettings.reduce((acc, cycle) => {
    return acc + (cycle.durationOn + cycle.durationOff) * cycle.ledRepetitions;
  }, 0);

  return (
    <Container>
      <h2>Light Cycles</h2>
      <p>
        Total Grow Time:{' '}
        {totalGrowTime !== undefined ? `${totalGrowTime} Minuten` : 'Noch keine Gesamtdauer festgelegt'}
      </p>
      <p>Aktuelle Gesamtdauer LED Zyklen: {totalLedDuration} Minuten</p>
      {ledSettings.map((lightCycle, index) => (
        <CycleInput
          key={index}
          cycleType="led"
          cycleIndex={index}
          cycle={lightCycle}
          onChange={handleLightCycleChange}
          onDelete={handleDeleteLightCycle}
          showDuration
        />
      ))}
      <Row className="justify-content-center">
        <Button onClick={handleAddLightCycle}>Add Light Cycle</Button>
      </Row>
    </Container>
  );
};

export default Lightning;
